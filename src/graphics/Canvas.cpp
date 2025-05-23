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
#include "SdfCanvas.hpp"

namespace Brisk {

float& pixelRatio() noexcept {
    thread_local float ratio = 1;
    return ratio;
}

namespace Internal {
struct PaintAndTransform {
    const Paint& paint;
    const Matrix& transform;
    float opacity;
    bool stroke = false;
};
} // namespace Internal

inline SdfCanvas sdf(Canvas* self) {
    return SdfCanvas{ self->renderContext(), self->flags() };
}

void applier(RenderStateEx* renderState, const Internal::PaintAndTransform& paint) {
    switch (paint.paint.index()) {
    case 0: { // Color
        if (paint.stroke)
            renderState->strokeColor1 = renderState->strokeColor2 = ColorF(get<ColorW>(paint.paint));
        else
            renderState->fillColor1 = renderState->fillColor2 = ColorF(get<ColorW>(paint.paint));
        renderState->opacity = paint.opacity;
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
            if (paint.stroke)
                renderState->strokeColor1 = renderState->strokeColor2 =
                    ColorF(gradient.colorStops().front().color);
            else
                renderState->fillColor1 = renderState->fillColor2 =
                    ColorF(gradient.colorStops().front().color);
        } else if (gradient.colorStops().size() == 2) {
            if (paint.stroke) {
                renderState->strokeColor1 = ColorF(gradient.colorStops().front().color);
                renderState->strokeColor2 = ColorF(gradient.colorStops().back().color);
            } else {
                renderState->fillColor1 = ColorF(gradient.colorStops().front().color);
                renderState->fillColor2 = ColorF(gradient.colorStops().back().color);
            }
        } else {
            renderState->gradientHandle = gradient.rasterize();
        }
        break;
    }
    case 2: { // Texture
        const Texture& texture     = std::get<Texture>(paint.paint);
        renderState->textureMatrix = texture.matrix.invert().value_or(Matrix{});
        renderState->imageHandle   = texture.image;
        renderState->samplerMode   = texture.mode;
        renderState->opacity       = paint.opacity;
        renderState->blurRadius    = texture.blurRadius;
        break;
    }
    }
}

void Canvas::drawRasterizedPath(const RasterizedPath& path, const Internal::PaintAndTransform& paint,
                                Quad3 scissors) {
    ++m_rasterizedPaths;
    RenderStateEx renderState(ShaderType::Mask, 1, nullptr);
    renderState.subpixelMode = SubpixelMode::Off;
    applier(&renderState, paint);
    renderState.scissorQuad = scissors;
    SdfCanvas::prepareStateInplace(renderState);
    GeometryGlyphs data = Internal::pathLayout(renderState.sprites, path);
    if (!data.empty()) {
        m_context.command(std::move(renderState), std::span{ data });
    }
}

static bool sdfCompat(const FillParams& fillParams) {
    return fillParams.fillRule == FillRule::Winding;
}

static bool sdfCompat(const StrokeParams& strokeParams, bool closed) {
    return strokeParams.dashArray.empty() &&
           (strokeParams.joinStyle == JoinStyle::Round || strokeParams.joinStyle == JoinStyle::Miter);
}

static bool isTransparent(const Paint& paint) {
    return paint.index() == 0 && (std::get<0>(paint).a == 0);
}

static bool colorOrSimpleGradient(const Paint& paint) {
    return paint.index() == 0 || paint.index() == 1 && (std::get<1>(paint).colorStops().size() <= 2);
}

static bool sdfCompat(const Paint& fillPaint, const FillParams& fillParams, const Paint& strokePaint,
                      const StrokeParams& strokeParams, bool closed) {
    if (!sdfCompat(fillParams) || !sdfCompat(strokeParams, closed))
        return false;
    return colorOrSimpleGradient(fillPaint) && colorOrSimpleGradient(strokePaint);
}

static float roundRadius(JoinStyle joinStyle, float radius) {
    return std::max(radius, joinStyle == JoinStyle::Miter ? 0.f : 0.5f);
}

void Canvas::drawPath(Path path, const Paint& strokePaint, const StrokeParams& strokeParams,
                      const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                      RectangleF clipRect, float opacity) {
    if (opacity == 0 || clipRect.empty())
        return;
    if ((m_flags && CanvasFlags::Sdf) && matrix.isUniformScale() &&
        sdfCompat(fillPaint, fillParams, strokePaint, strokeParams, path.isClosed())) {
        if (auto rrect = path.asRoundRectangle()) {
            float scale = matrix.estimateScale();
            sdf(this).drawRectangle(
                std::get<0>(*rrect), roundRadius(strokeParams.joinStyle, std::get<1>(*rrect)) / scale,
                std::get<2>(*rrect),
                std::tuple{ strokeWidth = strokeParams.strokeWidth, scissors = clipRect,
                            Internal::PaintAndTransform{ strokePaint, Matrix{}, opacity, true },
                            Internal::PaintAndTransform{ fillPaint, Matrix{}, opacity },
                            coordMatrix = matrix });
            return;
        }
    }
    if (!isTransparent(fillPaint))
        fillPath(path, fillPaint, fillParams, matrix, clipRect, opacity);
    if (!isTransparent(strokePaint) && strokeParams.strokeWidth > 0)
        strokePath(path, strokePaint, strokeParams, matrix, clipRect, opacity);
}

static Rectangle transformedClipRect(const Matrix& matrix, RectangleF clipRect) {
    return horizontalMax(clipRect.p1.v) <= float(INT32_MIN) &&
                   horizontalMin(clipRect.p2.v) >= float(INT32_MAX)
               ? noClipRect
               : Rectangle(matrix.transform(clipRect).roundOutward());
}

void Canvas::fillPath(Path path, const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                      RectangleF clipRect, float opacity) {
    if (opacity == 0 || clipRect.empty())
        return;
    if ((m_flags && CanvasFlags::Sdf) && matrix.isUniformScale() && sdfCompat(fillParams)) {
        if (auto rrect = path.asRoundRectangle()) {
            float scale = matrix.estimateScale();
            sdf(this).drawRectangle(std::get<0>(*rrect),
                                    roundRadius(JoinStyle::Round, std::get<1>(*rrect)) / scale,
                                    std::get<2>(*rrect),
                                    std::tuple{ strokeWidth = 0.f, scissors = clipRect,
                                                Internal::PaintAndTransform{ fillPaint, Matrix{}, opacity },
                                                coordMatrix = matrix });
            return;
        }
    }
    path = std::move(path).transformed(matrix);
    drawRasterizedPath(path.rasterize(fillParams, transformedClipRect(matrix, clipRect)),
                       Internal::PaintAndTransform{ fillPaint, matrix, opacity }, clipRect);
}

void Canvas::strokePath(Path path, const Paint& strokePaint, const StrokeParams& strokeParams,
                        const Matrix& matrix, RectangleF clipRect, float opacity) {
    if (opacity == 0 || clipRect.empty())
        return;
    if ((m_flags && CanvasFlags::Sdf) && matrix.isUniformScale() &&
        sdfCompat(strokeParams, path.isClosed())) {
        if (auto rrect = path.asRoundRectangle()) {
            float scale = matrix.estimateScale();
            sdf(this).drawRectangle(
                std::get<0>(*rrect), roundRadius(strokeParams.joinStyle, std::get<1>(*rrect)) / scale,
                std::get<2>(*rrect),
                std::tuple{ strokeWidth = strokeParams.strokeWidth, fillColor = Palette::transparent,
                            scissors = clipRect,
                            Internal::PaintAndTransform{ strokePaint, matrix, opacity, true },
                            coordMatrix = matrix });
            return;
        }
        if (auto line = path.asLine()) {
            sdf(this).drawLine((*line)[0], (*line)[1], strokeParams.strokeWidth,
                               staticMap(strokeParams.capStyle, CapStyle::Flat, SdfCanvas::LineEnd::Butt,
                                         CapStyle::Square, SdfCanvas::LineEnd::Square,
                                         SdfCanvas::LineEnd::Round),
                               std::tuple{ strokeWidth = 0.f, scissors = clipRect,
                                           Internal::PaintAndTransform{ strokePaint, matrix, opacity },
                                           coordMatrix = matrix });
            return;
        }
    }

    if (!strokeParams.dashArray.empty()) {
        path = path.dashed(strokeParams.dashArray, strokeParams.dashOffset);
    }
    path        = std::move(path).transformed(matrix);
    float scale = matrix.estimateScale();
    drawRasterizedPath(path.rasterize(strokeParams.scale(scale), transformedClipRect(matrix, clipRect)),
                       Internal::PaintAndTransform{ strokePaint, matrix, opacity }, clipRect);
}

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
};

Canvas::Canvas(RenderContext& context, CanvasFlags flags)
    : m_context(context), m_flags(flags), m_state(defaultState) {}

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

void Canvas::blurRect(RectangleF rect, float blurRadius, CornersF borderRadius, bool squircle) {
    sdf(this).drawShadow(
        rect, borderRadius,
        std::tuple{ scissors = m_state.clipRect, Arg::blurRadius = blurRadius * 0.36f, strokeWidth = 0.f,
                    Internal::PaintAndTransform{ m_state.fillPaint, Matrix{}, m_state.opacity },
                    coordMatrix = m_state.transform });
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
}

void Canvas::save() {
    m_stack.push_back(m_state);
}

void Canvas::restore() {
    if (m_stack.empty())
        return;
    m_state = std::move(m_stack.back());
    m_stack.pop_back();
}

void Canvas::restoreNoPop() {
    if (m_stack.empty())
        return;
    m_state = m_stack.back();
}

Matrix Canvas::getTransform() const {
    return Matrix(m_state.transform);
}

void Canvas::setTransform(const Matrix& matrix) {
    m_state.transform = matrix;
}

void Canvas::transform(const Matrix& matrix) {
    m_state.transform = Matrix(m_state.transform) * matrix;
}

std::optional<RectangleF> Canvas::getClipRect() const {
    if (Rectangle(m_state.clipRect) == noClipRect)
        return std::nullopt;
    return m_state.clipRect;
}

void Canvas::setClipRect(RectangleF rect) {
    m_state.clipRect = rect;
}

void Canvas::resetClipRect() {
    m_state.clipRect = noClipRect;
}

void Canvas::strokePath(Path path) {
    strokePath(std::move(path), m_state.strokePaint, m_state.strokeParams, m_state.transform,
               m_state.clipRect, m_state.opacity);
}

void Canvas::fillPath(Path path) {
    fillPath(std::move(path), m_state.fillPaint, m_state.fillParams, m_state.transform, m_state.clipRect,
             m_state.opacity);
}

void Canvas::drawPath(Path path) {
    drawPath(std::move(path), m_state.strokePaint, m_state.strokeParams, m_state.fillPaint,
             m_state.fillParams, m_state.transform, m_state.clipRect, m_state.opacity);
}

void Canvas::drawImage(RectangleF rect, Rc<Image> image, Matrix matrix, SamplerMode samplerMode,
                       float blurRadius) {
    sdf(this).drawTexture(rect, image, matrix,
                          std::tuple{ scissors = m_state.clipRect, coordMatrix = m_state.transform,
                                      Arg::samplerMode = samplerMode, Arg::blurRadius = blurRadius });
}

void Canvas::fillText(PointF position, const PreparedText& text) {
    fillText(position, { 0, 0 }, text);
}

void Canvas::fillText(PointF position, PointF alignment, const PreparedText& text) {
    if (alignment != PointF{}) {
        position -= text.bounds().size() * alignment;
    }
    sdf(this).drawText(
        position, text,
        std::tuple{
            scissors = m_state.clipRect,
            Internal::PaintAndTransform{ m_state.fillPaint, m_state.transform, m_state.opacity },
            coordMatrix  = m_state.transform,
            subpixelMode = m_state.subpixelText ? SubpixelMode::RGB : SubpixelMode::Off,
        });
}

void Canvas::fillText(TextWithOptions text, PointF position, PointF alignment) {
    PreparedText prepared = fonts->prepare(m_state.font, text);
    PointF offset         = prepared.alignLines(alignment.x, alignment.y);
    return fillText(position + offset, prepared);
}

void Canvas::fillText(TextWithOptions text, RectangleF position, PointF alignment) {
    PreparedText prepared = fonts->prepare(m_state.font, text);
    PointF offset         = prepared.alignLines(alignment.x, alignment.y);
    return fillText(position.at(alignment) + offset, prepared);
}

int Canvas::rasterizedPaths() const noexcept {
    return m_rasterizedPaths;
}

Canvas::StateSaver Canvas::saveState() & {
    return { *this };
}

void Canvas::setState(State state) {
    m_state = std::move(state);
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
}

const Canvas::State& Canvas::state() const noexcept {
    return m_state;
}

Canvas::ClipRectSaver::ClipRectSaver(Canvas& canvas) : canvas(canvas) {
    saved = canvas.m_state.clipRect;
}

RectangleF& Canvas::ClipRectSaver::operator*() {
    return canvas.m_state.clipRect;
}

RectangleF* Canvas::ClipRectSaver::operator->() {
    return &canvas.m_state.clipRect;
}

Canvas::ClipRectSaver::~ClipRectSaver() {
    canvas.m_state.clipRect = std::move(saved);
}

Canvas::ClipRectSaver Canvas::saveClipRect() & {
    return { *this };
}

void Canvas::fillTextSelection(PointF position, const PreparedText& text, Range<uint32_t> selection) {
    sdf(this).drawTextSelection(
        position, text, selection,
        std::tuple{ scissors = m_state.clipRect, strokeWidth = 0.f,
                    Internal::PaintAndTransform{ m_state.fillPaint, Matrix{}, m_state.opacity },
                    coordMatrix = m_state.transform });
}

void Canvas::fillTextSelection(PointF position, PointF alignment, const PreparedText& text,
                               Range<uint32_t> selection) {
    if (alignment != PointF{}) {
        position -= text.bounds().size() * alignment;
    }
    fillTextSelection(position, text, selection);
}
} // namespace Brisk
