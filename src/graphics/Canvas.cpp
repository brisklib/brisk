/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <brisk/graphics/Canvas.hpp>
#include <optional>
#include <array>
#include <brisk/core/Log.hpp>
#include <brisk/graphics/Color.hpp>
#include "RenderStateArgs.hpp"
#include <brisk/graphics/Renderer.hpp>
#include "Mask.hpp"

namespace Brisk {

const Canvas::State Canvas::defaultState{
    noClipRect,             /* clipRect */
    Matrix{},               /* transform */
    ColorW(Palette::black), /* strokePaint */
    ColorW(Palette::white), /* fillPaint */
    StrokeParams{},         /* dashArray */
    1.f,                    /* opacity */
    FillParams{},           /* fillRule */
    Font{},                 /* font*/
    true,                   /* subpixelText */
    Path{},                 /* clipPath */
    Composition{},          /* composition */
};

static int32_t findOrAdd(SpriteResources& container, Rc<SpriteResource> value) {
    if (!value)
        return -1;
    auto it = std::find(container.begin(), container.end(), value);
    if (it != container.end()) {
        return static_cast<int32_t>(it - container.begin());
    }
    size_t n = container.size();
    container.push_back(std::move(value));
    return static_cast<int32_t>(n);
}

static PointF quantize(PointF pt, unsigned value) {
    return PointF{
        std::round(pt.x * value) / value,
        std::round(pt.y),
    };
}

GeometryGlyphs Internal::pathLayout(SpriteResources& sprites, const RasterizedPath& path) {
    GeometryGlyphs result;
    if (path.sprite) {
        result.push_back(GeometryGlyph{
            Rectangle(quantize(path.bounds.p1, 1), quantize(path.bounds.p2, 1)),
            path.bounds.size(),
            static_cast<float>(findOrAdd(sprites, path.sprite)),
            float(path.sprite->size.width),
        });
    }
    return result;
}

static void setRenderComposition(RenderStateEx& state, const Composition& composition) {
    if (composition) {
        state.mode = toBlendingCompositionMode(composition.blend, composition.compose);
        if (composition.backdrop) {
            state.backImageHandle = composition.backdrop;
            state.hasBackTexture  = true;
        }
    } else {
        state.mode = BlendingCompositionMode::Normal;
        state.backImageHandle.reset();
        state.hasBackTexture = false;
    }
}

void Canvas::drawColorSprites(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                              RenderStateExArgs args) {
    RenderStateEx style(ShaderType::ColorMask, glyphs.size(), args);
    style.subpixelMode       = SubpixelMode::Off;
    style.spriteOversampling = 1;
    style.sprites            = std::move(sprites);
    style.scissor            = m_state.scissor;
    style.premultiply();
    setRenderComposition(style, m_state.composition);
    m_context->command(std::move(style), glyphs);
}

void Canvas::drawTextSprites(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                             RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Text, glyphs.size(), args);
    if (style.subpixelMode != SubpixelMode::Off) {
        Simd<float, 4> abcd{ style.coordMatrix.a, style.coordMatrix.b, style.coordMatrix.c,
                             style.coordMatrix.d };
        if (horizontalAll(eq(abcd, Simd<float, 4>{ 1.f, 0.f, 0.f, 1.f }))) {
            style.subpixelMode = SubpixelMode::RGB;
        } else if (horizontalAll(eq(abcd, Simd<float, 4>{ -1.f, -0.f, -0.f, -1.f }))) {
            style.subpixelMode = SubpixelMode::BGR;
        } else {
            style.subpixelMode = SubpixelMode::Off;
        }
    }
    style.spriteOversampling = fonts->hscale();
    style.sprites            = std::move(sprites);
    style.scissor            = m_state.scissor;
    style.premultiply();
    setRenderComposition(style, m_state.composition);
    m_context->command(std::move(style), glyphs);
}

float& pixelRatio() noexcept {
    thread_local float ratio = 1;
    return ratio;
}

namespace Internal {
struct PaintAndTransform {
    const Paint& paint;
    const Matrix& transform;
    float opacity;
};
} // namespace Internal

void applier(RenderStateEx* renderState, const Internal::PaintAndTransform& paint) {
    switch (paint.paint.index()) {
    case 0: { // Color
        renderState->fillColor1 = renderState->fillColor2 = ColorF(get<ColorW>(paint.paint));
        renderState->opacity                              = paint.opacity;
        break;
    }
    case 1: { // Gradient
        const Gradient& gradient = get<Gradient>(paint.paint);
        if (gradient.colorStops().empty()) {
            break;
        }
        renderState->gradientPoint1 = paint.transform.transform(gradient.getStartPoint());
        renderState->gradientPoint2 = paint.transform.transform(gradient.getEndPoint());
        renderState->gradient       = gradient.getType();
        renderState->opacity        = paint.opacity;
        if (gradient.colorStops().size() == 1) {
            renderState->fillColor1 = renderState->fillColor2 = ColorF(gradient.colorStops().front().color);
        } else if (gradient.colorStops().size() == 2) {
            renderState->fillColor1 = ColorF(gradient.colorStops().front().color);
            renderState->fillColor2 = ColorF(gradient.colorStops().back().color);
        } else {
            renderState->gradientHandle = gradient.rasterize();
        }
        break;
    }
    case 2: { // Texture
        const Texture& texture         = std::get<Texture>(paint.paint);
        Matrix mat                     = texture.matrix * paint.transform;
        renderState->textureMatrix     = mat.invert().value_or(Matrix{});
        renderState->sourceImageHandle = texture.image;
        renderState->samplerMode       = texture.mode;
        renderState->opacity           = paint.opacity;
        renderState->blurDirections    = 0u;
        if (texture.blurRadius.vertical > 0.f)
            renderState->blurDirections |= 2u;
        if (texture.blurRadius.horizontal > 0.f)
            renderState->blurDirections |= 1u;
        renderState->blurRadius =
            texture.blurRadius.vertical > 0.f ? texture.blurRadius.vertical : texture.blurRadius.horizontal;

        float aaRadius = 0.5f / mat.estimateScale() - 0.5f;
        if (renderState->blurRadius == 0 && aaRadius > 0.f) {
            renderState->blurDirections |= 4u;
            renderState->blurRadius = aaRadius;
        }
        break;
    }
    }
}

using Internal::PaintAndTransform;

void Canvas::drawPreparedPath(const PreparedPath& path, const PaintAndTransform& paint, Rectangle scissor) {
    if (path.empty() || scissor.empty())
        return;
#ifdef BRISK_1PASS_BLUR
    return drawPreparedPathCmd(path, paint, scissor);
#else
    if (paint.paint.index() != 2) {
        return drawPreparedPathCmd(path, paint, scissor);
    }
    const Texture& texture = std::get<Texture>(paint.paint);

    if (!texture.blurRadius.bidirectional() || texture.blurRadius.max() < 3.f) {
        return drawPreparedPathCmd(path, paint, scissor);
    }

    int blurPad            = static_cast<int>(std::ceil(texture.blurRadius.vertical * 3));
    Rectangle bounds       = path.mask().pixelBounds();
    Rectangle paddedBounds = bounds.withMargin(0, blurPad);

    // First step, sample horizontally, draw rectangle covering the path bounds
    // expanded by the blur radius.
    beginLayer(paddedBounds.size());
    drawPreparedPathCmd(RectangleF(paddedBounds).withOffset(-paddedBounds.p1),
                        PaintAndTransform{ Texture{
                                               texture.image,
                                               texture.matrix.translate(-paddedBounds.p1),
                                               texture.mode,
                                               BlurRadius{ texture.blurRadius.horizontal, 0.f },
                                           },
                                           {},
                                           1.f },
                        scissor == noClipRect ? noClipRect
                                              : scissor.withMargin(0, blurPad).withOffset(-paddedBounds.p1));

    // Second step, draw the path, sampling from the horizontally blurred layer.
    Rc<Image> layerImage = finishLayer();
    drawPreparedPathCmd(path,
                        PaintAndTransform{ Texture{
                                               std::move(layerImage),
                                               Matrix{}.translate(paddedBounds.p1),
                                               texture.mode,
                                               BlurRadius{ 0.f, texture.blurRadius.vertical },
                                           },
                                           {},
                                           paint.opacity },
                        scissor);
#endif
}

void Canvas::drawPreparedPathCmd(const PreparedPath& path, const PaintAndTransform& paint,
                                 Rectangle scissor) {
    if (path.isRectangle()) {
        RenderStateEx renderState(ShaderType::Rectangle, 1, nullptr);
        applier(&renderState, paint);
        renderState.scissor = scissor;
        renderState.premultiply();
        setRenderComposition(renderState, m_state.composition);
        m_context->command(std::move(renderState), one(path.m_mask.rectangle));
    } else if (path.isSparse()) {
        RenderStateEx renderState(ShaderType::Mask, path.m_mask.patches.size(), nullptr);
        renderState.subpixelMode = SubpixelMode::Off;
        applier(&renderState, paint);
        renderState.scissor = scissor;
        renderState.premultiply();

        std::vector<uint32_t> data;
        size_t patchesSize = path.m_mask.patches.size() * (sizeof(Internal::Patch) / sizeof(uint32_t));
        size_t patchDataSize =
            path.m_mask.patchData.size() * (sizeof(Internal::PatchData) / sizeof(uint32_t));
        data.resize(alignUp(patchesSize, 4u) + patchDataSize);
        memcpy(data.data(), path.m_mask.patches.data(), patchesSize * sizeof(uint32_t));
        memcpy(data.data() + alignUp(patchesSize, 4u), path.m_mask.patchData.data(),
               patchDataSize * sizeof(uint32_t));

        setRenderComposition(renderState, m_state.composition);
        m_context->command(std::move(renderState), std::span{ data });
    }
}

static float roundRadius(JoinStyle joinStyle, float radius) {
    return std::max(radius, joinStyle == JoinStyle::Miter ? 0.f : 0.5f);
}

static Rectangle transformedClipRect(const Matrix& matrix, RectangleF clipRect) {
    return horizontalMax(clipRect.p1.v) <= float(INT32_MIN) &&
                   horizontalMin(clipRect.p2.v) >= float(INT32_MAX)
               ? noClipRect
               : Rectangle(matrix.transform(clipRect).roundOutward());
}

template <typename T>
struct CopyOrRef {
    CopyOrRef(T&& copy) : copy(std::move(copy)) {}

    CopyOrRef(T& ref)                      = delete;
    CopyOrRef(const CopyOrRef&)            = delete;
    CopyOrRef(CopyOrRef&&)                 = delete;
    CopyOrRef& operator=(const CopyOrRef&) = delete;
    CopyOrRef& operator=(CopyOrRef&&)      = delete;

    CopyOrRef(const T& ref) noexcept : ref(&ref) {}

    const T& operator*() const noexcept {
        return ref ? *ref : *copy;
    }

    template <typename Fn>
    void apply(Fn&& fn) & {
        if (ref) {
            copy = fn(*ref);
            ref  = nullptr;
        } else {
            copy = fn(std::move(*copy));
        }
    }

    std::optional<T> copy;
    const T* ref = nullptr;
};

static bool isTransparent(const Paint& paint) {
    return paint.index() == 0 && (std::get<0>(paint).a == 0);
}

void Canvas::fillPreparedPath(const PreparedPath& path, const Paint& fillPaint, Rectangle scissor,
                              float opacity) {
    if (opacity < 0.04f || scissor.empty() || isTransparent(fillPaint))
        return;
    drawPreparedPath(path, Internal::PaintAndTransform{ fillPaint, {}, opacity }, scissor);
}

void Canvas::fillPath(const Path& path, const Paint& fillPaint, const FillParams& fillParams,
                      const Matrix& matrix, const PreparedPath& clipPath, Rectangle scissor, float opacity) {
    if (opacity < 0.04f || scissor.empty() || isTransparent(fillPaint))
        return;
    CopyOrRef transformedPath(path);
    if (!matrix.isIdentity()) {
        transformedPath.apply([&]<typename T>(T&& x) {
            return std::forward<T>(x).transformed(matrix);
        });
    }
    PreparedPath preparedPath((*transformedPath), fillParams, scissor);
    if (!clipPath.empty()) {
        preparedPath = PreparedPath::intersection(preparedPath, clipPath);
    }
    drawPreparedPath(preparedPath, Internal::PaintAndTransform{ fillPaint, matrix, opacity }, scissor);
}

void Canvas::strokePath(const Path& path, const Paint& strokePaint, const StrokeParams& strokeParams,
                        const Matrix& matrix, const PreparedPath& clipPath, Rectangle scissor,
                        float opacity) {
    if (opacity < 0.04f || scissor.empty() || strokeParams.strokeWidth < 1e-6 || isTransparent(strokePaint))
        return;

    CopyOrRef transformedPath(path);

    if (!strokeParams.dashArray.empty()) {
        transformedPath.apply([&]<typename T>(T&& x) {
            return std::forward<T>(x).dashed(strokeParams.dashArray, strokeParams.dashOffset);
        });
    }
    if (!matrix.isIdentity()) {
        transformedPath.apply([&]<typename T>(T&& x) {
            return std::forward<T>(x).transformed(matrix);
        });
    }
    float scale = matrix.estimateScale();
    PreparedPath preparedPath((*transformedPath), strokeParams.scale(scale), scissor);
    if (!clipPath.empty()) {
        preparedPath = PreparedPath::intersection(preparedPath, clipPath);
    }
    drawPreparedPath(preparedPath, Internal::PaintAndTransform{ strokePaint, matrix, opacity }, scissor);
}

Canvas::Canvas(RenderContext& context, CanvasFlags flags)
    : m_context(&context), m_flags(flags), m_state(defaultState) {}

Canvas::~Canvas() {}

const Paint& Canvas::getStrokePaint() const {
    return m_state.strokePaint;
}

void Canvas::setStrokePaint(Paint paint) {
    m_state.strokePaint = std::move(paint);
}

const Paint& Canvas::getFillPaint() const {
    return m_state.fillPaint;
}

void Canvas::setFillPaint(Paint paint) {
    m_state.fillPaint = std::move(paint);
}

float Canvas::getStrokeWidth() const {
    return m_state.strokeParams.strokeWidth;
}

void Canvas::setStrokeWidth(float width) {
    m_state.strokeParams.strokeWidth = width;
}

float Canvas::getOpacity() const {
    return m_state.opacity;
}

void Canvas::setOpacity(float opacity) {
    m_state.opacity = opacity;
}

ColorW Canvas::getStrokeColor() const {
    if (auto* c = get_if<ColorW>(&m_state.strokePaint))
        return *c;
    return Palette::transparent;
}

void Canvas::setStrokeColor(ColorW color) {
    m_state.strokePaint = color;
}

ColorW Canvas::getFillColor() const {
    if (auto* c = get_if<ColorW>(&m_state.fillPaint))
        return *c;
    return Palette::transparent;
}

void Canvas::setFillColor(ColorW color) {
    m_state.fillPaint = color;
}

float Canvas::getMiterLimit() const {
    return m_state.strokeParams.miterLimit;
}

void Canvas::setMiterLimit(float limit) {
    m_state.strokeParams.miterLimit = limit;
}

FillRule Canvas::getFillRule() const {
    return m_state.fillParams.fillRule;
}

void Canvas::setFillRule(FillRule fillRule) {
    m_state.fillParams.fillRule = fillRule;
}

JoinStyle Canvas::getJoinStyle() const {
    return m_state.strokeParams.joinStyle;
}

void Canvas::setJoinStyle(JoinStyle joinStyle) {
    m_state.strokeParams.joinStyle = joinStyle;
}

CapStyle Canvas::getCapStyle() const {
    return m_state.strokeParams.capStyle;
}

void Canvas::setCapStyle(CapStyle capStyle) {
    m_state.strokeParams.capStyle = capStyle;
}

float Canvas::getDashOffset() const {
    return m_state.strokeParams.dashOffset;
}

void Canvas::setDashOffset(float offset) {
    m_state.strokeParams.dashOffset = offset;
}

const DashArray& Canvas::getDashArray() const {
    return m_state.strokeParams.dashArray;
}

void Canvas::setDashArray(const DashArray& array) {
    m_state.strokeParams.dashArray = array;
}

void Canvas::strokeRect(RectangleF rect, CornersF borderRadius, bool squircle) {
    Path path;
    if (borderRadius.max() == 0.f)
        path.addRect(rect);
    else
        path.addRoundRect(rect, borderRadius, squircle);
    strokePath(path);
}

void Canvas::fillRect(RectangleF rect, CornersF borderRadius, bool squircle) {
    Path path;
    if (borderRadius.max() == 0.f)
        path.addRect(rect);
    else
        path.addRoundRect(rect, borderRadius, squircle);
    fillPath(path);
}

void Canvas::drawRect(RectangleF rect, CornersF borderRadius, bool squircle) {
    Path path;
    if (borderRadius.max() == 0.f)
        path.addRect(rect);
    else
        path.addRoundRect(rect, borderRadius, squircle);
    drawPath(path);
}

struct GeometryRectangle {
    RectangleF rectangle;
    CornersF borderRadii;
};

void Canvas::blurRect(RectangleF rect, float blurRadius, CornersF borderRadius, bool squircle) {
    RenderStateEx style(ShaderType::Shadow, std::tuple{ Arg::blurRadius = blurRadius * 0.36f,
                                                        Internal::PaintAndTransform{
                                                            m_state.fillPaint, Matrix{}, m_state.opacity },
                                                        coordMatrix = m_state.transform });
    style.premultiply();
    style.scissor = m_state.scissor;
    setRenderComposition(style, m_state.composition);
    m_context->command(std::move(style), one(GeometryRectangle{ rect, borderRadius }));
}

void Canvas::strokeEllipse(RectangleF rect) {
    Path path;
    path.addEllipse(rect);
    strokePath(path);
}

void Canvas::fillEllipse(RectangleF rect) {
    Path path;
    path.addEllipse(rect);
    fillPath(path);
}

void Canvas::drawEllipse(RectangleF rect) {
    Path path;
    path.addEllipse(rect);
    drawPath(path);
}

void Canvas::strokeLine(PointF pt1, PointF pt2) {
    Path path;
    path.moveTo(pt1);
    path.lineTo(pt2);
    strokePath(path);
}

void Canvas::strokePolygon(std::span<const PointF> points, bool close) {
    if (points.empty())
        return;
    Path path;
    path.moveTo(points.front());
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    if (close)
        path.close();
    strokePath(path);
}

void Canvas::fillPolygon(std::span<const PointF> points, bool close) {
    if (points.empty())
        return;
    Path path;
    path.moveTo(points.front());
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    if (close)
        path.close();
    fillPath(path);
}

Font Canvas::getFont() const {
    return m_state.font;
}

void Canvas::setFont(const Font& font) {
    m_state.font = font;
}

bool Canvas::getSubpixelTextRendering() const {
    return m_state.subpixelText;
}

void Canvas::setSubpixelTextRendering(bool value) {
    m_state.subpixelText = value;
}

void Canvas::reset() {
    m_state = defaultState;
    m_preparedClipPath.reset();
}

void Canvas::save() {
    m_stack.push_back(m_state);
}

void Canvas::restore() {
    if (m_stack.empty())
        return;
    m_state = std::move(m_stack.back());
    m_stack.pop_back();
    m_preparedClipPath.reset();
}

void Canvas::restoreNoPop() {
    if (m_stack.empty())
        return;
    m_state = m_stack.back();
    m_preparedClipPath.reset();
}

Matrix Canvas::getTransform() const {
    return Matrix(m_state.transform);
}

void Canvas::setTransform(const Matrix& matrix) {
    m_state.transform = matrix;
    m_preparedClipPath.reset();
}

void Canvas::transform(const Matrix& matrix) {
    m_state.transform = Matrix(m_state.transform) * matrix;
    m_preparedClipPath.reset();
}

const Path& Canvas::getClipPath() const {
    return m_state.clipPath;
}

void Canvas::setClipPath(Path path) {
    m_state.clipPath = std::move(path);
    m_preparedClipPath.reset();
}

void Canvas::resetClipPath() {
    m_state.clipPath = Path{};
    m_preparedClipPath.reset();
}

void Canvas::strokePath(const Path& path) {
    strokePath(path, m_state.strokePaint, m_state.strokeParams, m_state.transform, preparedClipPath(),
               m_state.scissor, m_state.opacity);
}

void Canvas::fillPath(const Path& path) {
    fillPath(path, m_state.fillPaint, m_state.fillParams, m_state.transform, preparedClipPath(),
             m_state.scissor, m_state.opacity);
}

void Canvas::drawPath(const Path& path) {
    fillPath(path, m_state.fillPaint, m_state.fillParams, m_state.transform, preparedClipPath(),
             m_state.scissor, m_state.opacity);
    strokePath(path, m_state.strokePaint, m_state.strokeParams, m_state.transform, preparedClipPath(),
               m_state.scissor, m_state.opacity);
}

void Canvas::drawImage(RectangleF rect, Rc<Image> image, Matrix matrix, SamplerMode samplerMode,
                       BlurRadius blurRadius) {
    Path path;
    path.addRect(rect);
    Size size = image->size();
    fillPath(path,
             Texture{ std::move(image), matrix * Matrix::mapRect(RectangleF{ {}, size }, rect), samplerMode,
                      blurRadius },
             FillParams{}, m_state.transform, preparedClipPath(), m_state.scissor, m_state.opacity);
}

void Canvas::fillText(PointF position, const DocumentLayout& text) {
    fillText(position, { 0, 0 }, text);
}

void Canvas::fillText(PointF position, PointF alignment, const DocumentLayout& text) {
    if (alignment != PointF{}) {
        position -= PointF(text.bounds().size()) * alignment;
    }

    const Paint textPaint = m_state.fillPaint;
    SpriteResources sprites;
    GeometryGlyphs glyphs;
    std::optional<Color> runColor;
    bool multicolor = false;

    auto flush      = [&] {
        if (glyphs.empty()) {
            sprites.clear();
            return;
        }
        if (multicolor) {
            drawColorSprites(std::move(sprites), glyphs,
                             std::tuple{ Arg::coordMatrix  = m_state.transform,
                                         Arg::subpixelMode = SubpixelMode::Off,
                                         Internal::PaintAndTransform{ Palette::white, m_state.transform,
                                                                      m_state.opacity } });
        } else {
            Paint paint = textPaint;
            if (runColor) {
                paint = ColorW(*runColor);
            }
            drawTextSprites(
                std::move(sprites), glyphs,
                std::tuple{ Arg::coordMatrix  = m_state.transform,
                            Arg::subpixelMode = m_state.subpixelText ? SubpixelMode::RGB : SubpixelMode::Off,
                            Internal::PaintAndTransform{ paint, m_state.transform, m_state.opacity } });
        }
        glyphs.clear();
        sprites.clear();
    };

    Internal::forEachTextLayoutGlyph(text, position, [&](const Internal::TextLayoutGlyph& glyph) {
        const bool glyphMulticolor = glyph.bitmap.color;
        if (!glyphs.empty() && (glyphMulticolor != multicolor || glyph.color != runColor)) {
            flush();
        }
        if (glyphs.empty()) {
            multicolor = glyphMulticolor;
            runColor   = glyph.color;
        }
        if (!glyph.bitmap.sprite) {
            return;
        }
        GeometryGlyph desc;
        desc.rect.p1 = quantize(glyph.position + PointF{ glyph.bitmap.offsetX, -float(glyph.bitmap.offsetY) },
                                glyph.bitmap.horizontalScale);
        desc.rect.p2 = desc.rect.p1 + PointF{ float(glyph.bitmap.logicalSize.width),
                                              float(glyph.bitmap.logicalSize.height) };
        desc.sprite  = static_cast<float>(findOrAdd(sprites, glyph.bitmap.sprite));
        desc.stride  = glyph.bitmap.size.width;
        desc.size    = glyph.bitmap.size;
        glyphs.push_back(std::move(desc));
    });
    flush();

    Internal::forEachTextLayoutDecoration(
        text, position, [&](const Internal::TextLayoutDecoration& decoration) {
            Path path;
            const Paint paint = decoration.color ? Paint{ ColorW(*decoration.color) } : textPaint;
            const StrokeParams params{ .capStyle = CapStyle::Flat, .strokeWidth = decoration.thickness };
            if (decoration.decoration && TextDecoration::Underline) {
                const std::array<PointF, 2> points{
                    decoration.start + PointF{ 0.f, decoration.underlineOffset },
                    decoration.end + PointF{ 0.f, decoration.underlineOffset }
                };
                path.addPolyline(points);
            }
            if (decoration.decoration && TextDecoration::Overline) {
                const std::array<PointF, 2> points{
                    decoration.start + PointF{ 0.f, decoration.overlineOffset },
                    decoration.end + PointF{ 0.f, decoration.overlineOffset }
                };
                path.addPolyline(points);
            }
            if (decoration.decoration && TextDecoration::LineThrough) {
                const std::array<PointF, 2> points{
                    decoration.start + PointF{ 0.f, decoration.lineThroughOffset },
                    decoration.end + PointF{ 0.f, decoration.lineThroughOffset }
                };
                path.addPolyline(points);
            }
            if (!path.empty()) {
                strokePath(path, paint, params, m_state.transform, preparedClipPath(), m_state.scissor,
                           m_state.opacity);
            }
        });
}

void Canvas::fillText(TextWithOptions text, PointF position, PointF alignment) {
    PreparedDocument prepared = fonts->prepareDocument(m_state.font, text);
    TextLayoutOptions options;
    options.maxLineWidth        = 0.f;
    options.alignment           = alignment.x <= 0.f   ? TextLayoutAlignment::Left
                                  : alignment.x >= 1.f ? TextLayoutAlignment::Right
                                                       : TextLayoutAlignment::Center;
    const DocumentLayout layout = prepared.layout(options);
    const RectangleF bounds     = layout.bounds();
    const PointF origin{ position.x, position.y - bounds.p1.y - bounds.height() * alignment.y };
    return fillText(origin, layout);
}

void Canvas::fillText(TextWithOptions text, RectangleF position, PointF alignment) {
    PreparedDocument prepared = fonts->prepareDocument(m_state.font, text);
    TextLayoutOptions options;
    options.maxLineWidth        = std::max(0.f, position.width());
    options.alignment           = alignment.x <= 0.f   ? TextLayoutAlignment::Left
                                  : alignment.x >= 1.f ? TextLayoutAlignment::Right
                                                       : TextLayoutAlignment::Center;
    const DocumentLayout layout = prepared.layout(options);
    const RectangleF bounds     = layout.bounds();
    const PointF anchor         = position.at(alignment);
    const PointF origin{ position.p1.x, anchor.y - bounds.p1.y - bounds.height() * alignment.y };
    return fillText(origin, layout);
}

Canvas::StateSaver Canvas::saveState() & {
    return { *this };
}

void Canvas::setState(State state) {
    m_state = std::move(state);
    m_preparedClipPath.reset();
}

Canvas::StateSaver::StateSaver(Canvas& canvas) : canvas(canvas) {
    saved = canvas.m_state;
}

Canvas::State& Canvas::StateSaver::operator*() {
    return canvas.m_state;
}

Canvas::State* Canvas::StateSaver::operator->() {
    return &canvas.m_state;
}

Canvas::StateSaver::~StateSaver() {
    canvas.m_state = std::move(saved);
    canvas.m_preparedClipPath.reset();
}

const Canvas::State& Canvas::state() const noexcept {
    return m_state;
}

Canvas::ScissorSaver::ScissorSaver(Canvas& canvas) : canvas(canvas) {
    saved = canvas.m_state.scissor;
}

Rectangle& Canvas::ScissorSaver::operator*() {
    return canvas.m_state.scissor;
}

Rectangle* Canvas::ScissorSaver::operator->() {
    return &canvas.m_state.scissor;
}

Canvas::ScissorSaver::~ScissorSaver() {
    canvas.m_state.scissor = std::move(saved);
}

Canvas::ScissorSaver Canvas::saveScissor() & {
    return { *this };
}

void Canvas::fillTextSelection(PointF position, const DocumentLayout& text, Range<uint32_t> selection) {
    Internal::textLayoutSelectionRects(text, selection, [&](const TextSelectionRect& rect) {
        if (rect.line >= text.lineCount()) {
            return;
        }
        const DocumentLine line = text.line(rect.line);
        const float x0          = std::round(position.x + rect.x0);
        const float x1          = std::round(position.x + rect.x1);
        // Distribute leading equally around the line's ink metrics so that
        // adjacent selection rectangles remain contiguous.
        const float halfLeading = line.leading * 0.5f;
        const float y0          = std::round(position.y + line.baseline - line.ascender - halfLeading);
        const float y1          = std::round(position.y + line.baseline + line.descender + halfLeading);
        fillRect(RectangleF{ PointF{ x0, y0 }, PointF{ x1, y1 } });
    });
}

void Canvas::fillTextSelection(PointF position, PointF alignment, const DocumentLayout& text,
                               Range<uint32_t> selection) {
    if (alignment != PointF{}) {
        position -= PointF(text.bounds().size()) * alignment;
    }
    fillTextSelection(position, text, selection);
}

Canvas::Layer::Layer(RenderContext* context, Size layerSize) {
    parentPipeline = dynamicCast<RenderPipeline*>(context);
    if (!parentPipeline) {
        BRISK_LOG_ERROR("RenderContext doesn't implement RenderPipeline");
        return;
    }
    parentPipeline->flush();
    Rc<RenderEncoder> encoder = parentPipeline->encoder();
    if (!encoder) {
        return;
    }
    parentTarget = encoder->currentTarget();
    if (!parentTarget) {
        return;
    }
    RenderDevice* device = encoder->device();
    if (!device) {
        return;
    }
    layerTarget = device->createImageTarget(layerSize);
    if (!layerTarget) {
        return;
    }
    encoder->end(); // End the current encoder to switch to the new target.
    layerPipeline = std::make_shared<RenderPipeline>(
        encoder, layerTarget,
        Palette::transparent); // begin will be called in Pipeline constructor.
}

Rc<Image> Canvas::contentsAsImage() {
    RenderPipeline* pipeline = dynamicCast<RenderPipeline*>(m_context);
    if (!pipeline) {
        BRISK_LOG_ERROR("RenderContext doesn't implement RenderPipeline");
        return nullptr;
    }
    Rc<RenderEncoder> encoder = pipeline->encoder();
    if (!encoder) {
        return nullptr;
    }
    Rc<RenderTarget> target = encoder->currentTarget();
    if (!target) {
        return nullptr;
    }
    if (target->type() != RenderTargetType::Image) {
        BRISK_LOG_ERROR("Current render target is not an image target");
        return nullptr;
    }
    pipeline->flush(); // Ensure all drawing commands are executed before capturing.
    encoder->end();

    ImageRenderTarget* imageTarget = static_cast<ImageRenderTarget*>(target.get());
    Rc<Image> image                = imageTarget->image(true);
    encoder->begin(target, Palette::transparent); // Reopen the encoder for further drawing.
    return image;
}

Rc<Image> Canvas::finishLayer() {
    if (m_layers.empty()) {
        return nullptr; // No layers to finish.
    }
    Layer layer = std::move(m_layers.back());
    m_layers.pop_back();
    // Ensure all drawing commands are executed.
    layer.layerPipeline->flush();
    Rc<RenderEncoder> encoder = layer.layerPipeline->encoder();
    // Switch back to the parent context.
    layer.layerPipeline       = nullptr; // This flushes commands and ends encoding.
    encoder->begin(layer.parentTarget, std::nullopt);
    // Restore the previous state of the Canvas.
    restore();
    m_context = layer.parentPipeline;
    // Return the rendered image from the layer.
    return layer.layerTarget->image();
}

void Canvas::beginLayer(Size layerSize) {
    Layer layer(m_context, layerSize); // Create a new layer with the specified size.
    if (!layer.ok()) {
        BRISK_LOG_ERROR("Failed to create layer with size: {}", layerSize);
        return;
    }
    m_context = layer.layerPipeline.get(); // Set the current context to the new layer's pipeline.
    m_layers.push_back(std::move(layer));  // Push the new layer onto the stack.
    save();  // Save the current state of the Canvas before starting the new layer.
    reset(); // Reset the Canvas state to default for the new layer.
}

size_t Canvas::layers() const noexcept {
    return m_layers.size();
}

const PreparedPath& Canvas::preparedClipPath() {
    if (!m_preparedClipPath.has_value()) {
        if (m_state.transform.isIdentity())
            m_preparedClipPath = PreparedPath(m_state.clipPath);
        else
            m_preparedClipPath = PreparedPath(m_state.clipPath.transformed(m_state.transform));
    }
    return *m_preparedClipPath;
}

void Canvas::setScissor(Rectangle scissor) {
    m_state.scissor = scissor;
}

void Canvas::intersectScissor(Rectangle scissor) {
    m_state.scissor = m_state.scissor.intersection(scissor);
}

void Canvas::resetScissor() {
    m_state.scissor = noClipRect;
}

void Canvas::setComposition(Composition composition) {
    m_state.composition = std::move(composition);
}

void Canvas::resetComposition() {
    m_state.composition = {};
}
} // namespace Brisk
