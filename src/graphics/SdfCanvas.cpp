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
#include "SdfCanvas.hpp"
#include <brisk/core/Encoding.hpp>

namespace Brisk {

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

static GeometryGlyphs glyphLayout(uint32_t& runIndex, bool& multicolor, std::optional<Color>& color,
                                  SpriteResources& sprites, const PreparedText& prepared,
                                  PointF offset = { 0, 0 }) {
    GeometryGlyphs result;
    bool first = true;
    for (; runIndex < prepared.runs.size(); ++runIndex) {
        const GlyphRun& run = prepared.runVisual(runIndex);
        if (first) {
            color      = run.color;
            multicolor = run.hasColor();
            first      = false;
        } else {
            if (run.color != color || run.hasColor() != multicolor)
                return result;
        }

        for (const Internal::Glyph& g : run.glyphs) {
            std::optional<Internal::GlyphData> data = g.load(run);
            if (data && data->sprite) {
                GeometryGlyph glyphDesc;
                PointF pos        = g.pos + run.position + offset;
                glyphDesc.rect.p1 = quantize(pos + PointF(data->offset_x, -data->offset_y), run.hscale());
                glyphDesc.rect.p2 =
                    glyphDesc.rect.p1 + PointF(float(data->size.width) / run.hscale(), data->size.height);
                glyphDesc.sprite = static_cast<float>(findOrAdd(sprites, data->sprite));
                glyphDesc.stride = data->size.width;
                if (run.hasColor())
                    glyphDesc.stride *= 4;
                glyphDesc.size = data->size;

                result.push_back(std::move(glyphDesc));
            }
        }
    }
    return result;
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

void SdfCanvas::drawText(PointF pos, const TextWithOptions& text, const Font& font, ColorW textColor) {
    PreparedText run = fonts->prepare(font, text);
    drawText(pos, run, std::tuple{ fillColor = textColor });
}

void SdfCanvas::drawText(PointF pos, float x_alignment, float y_alignment, const TextWithOptions& text,
                         const Font& font, ColorW textColor) {
    PreparedText run = fonts->prepare(font, text);
    PointF offset    = run.alignLines(x_alignment, y_alignment);
    drawText(pos + offset, run, std::tuple{ fillColor = textColor });
}

void SdfCanvas::drawText(RectangleF rect, float x_alignment, float y_alignment, const TextWithOptions& text,
                         const Font& font, ColorW textColor) {
    PreparedText run = fonts->prepare(font, text);
    PointF offset    = run.alignLines(x_alignment, y_alignment);
    drawText(rect.at(x_alignment, y_alignment) + offset, run, std::tuple{ fillColor = textColor });
}

SdfCanvas::SdfCanvas(RenderContext& context, CanvasFlags flags) : m_context(context), m_flags(flags) {}

RenderStateEx SdfCanvas::prepareState(RenderStateEx&& state) {
    prepareStateInplace(state);
    return state;
}

void SdfCanvas::prepareStateInplace(RenderStateEx& state) {
    state.premultiply();
}

void SdfCanvas::drawLine(PointF p1, PointF p2, float thickness, ColorW color, LineEnd end) {
    return drawLine(p1, p2, thickness, end, std::tuple{ fillColor = color, strokeWidth = 0.f });
}

void SdfCanvas::drawLine(PointF p1, PointF p2, float thickness, LineEnd end, RenderStateExArgs args) {
    const PointF center       = PointF((p1.v + p2.v) * 0.5f);
    const float halfThickness = thickness * 0.5f;
    const float length        = p1.distance(p2);
    const float angle         = std::atan2(p1.y - p2.y, p1.x - p2.x);
    const float extension     = end == LineEnd::Butt ? 0.f : halfThickness;
    return drawRectangle(
        RectangleF{
            center.x - length * 0.5f - extension,
            center.y - thickness * 0.5f,
            center.x + length * 0.5f + extension,
            center.y + thickness * 0.5f,
        },
        end == LineEnd::Round ? thickness * 0.5f : 0.f,
        std::tuple{ coordMatrix = Matrix{}.rotate(rad2deg<float> * angle, center), args });
}

void SdfCanvas::drawRectangle(RectangleF rect, CornersF borderRadius, RenderStateExArgs args) {
    return drawRectangle(rect, borderRadius, false, args);
}

void SdfCanvas::drawRectangle(RectangleF rect, CornersF borderRadius, bool squircle, RenderStateExArgs args) {
    RenderStateEx state(ShaderType::Rectangles, args);
    borderRadius.v = abs(borderRadius.v);
    if (squircle) {
        borderRadius.v = -borderRadius.v;
    }
    m_context.command(prepareState(std::move(state)), one(GeometryRectangle{ rect, borderRadius }));
}

void SdfCanvas::drawTextSelection(PointF pos, const PreparedText& prepared, Range<uint32_t> selection,
                                  RenderStateExArgs args) {
    if (selection.distance() != 0) {
        selection.min = prepared.characterToGrapheme(selection.min);
        selection.max = prepared.characterToGrapheme(selection.max);
        for (uint32_t gr : selection) {
            uint32_t lineIndex = prepared.graphemeToLine(gr);
            if (lineIndex == UINT32_MAX)
                continue;
            auto range       = prepared.ranges[gr];
            const auto& line = prepared.lines[lineIndex];
            drawRectangle(Rectangle(pos + PointF(range.min, line.baseline - line.ascDesc.ascender),
                                    pos + PointF(range.max, line.baseline + line.ascDesc.descender)),
                          0.f, args);
        }
    }
}

void SdfCanvas::drawText(PointF pos, const PreparedText& prepared, RenderStateExArgs args) {
    SpriteResources sprites;
    uint32_t runIndex = 0;
    while (runIndex < prepared.runs.size()) {
        RenderStateExArgs runArgs = args;
        std::optional<Color> color;
        uint32_t oldRunIndex = runIndex;
        bool multicolor      = false;
        GeometryGlyphs g     = glyphLayout(runIndex, multicolor, color, sprites, prepared, pos);
        std::tuple argsColor{ args, fillColor = color.value_or(Color{}) };
        if (color) {
            runArgs = argsColor;
        } else {
            runArgs = args;
        }
        if (multicolor)
            drawColorMask(std::move(sprites), g, std::tuple{ args, fillColor = Palette::white });
        else
            drawText(std::move(sprites), g, runArgs);
        for (uint32_t ri = oldRunIndex; ri < runIndex; ++ri) {
            const GlyphRun& run = prepared.runVisual(ri);
            if (run.decoration != TextDecoration::None) {
                run.updateRanges();
                PointF p1{ run.textHRange.min + run.position.x, run.position.y };
                PointF p2{ run.textHRange.max + run.position.x, run.position.y };
                p1 += pos;
                p2 += pos;

                if (run.decoration && TextDecoration::Underline)
                    drawLine(p1 + PointF{ 0.f, run.metrics.underlineOffset() },
                             p2 + PointF{ 0.f, run.metrics.underlineOffset() }, run.metrics.lineThickness,
                             LineEnd::Butt, std::tuple{ strokeWidth = 0.f, runArgs });
                if (run.decoration && TextDecoration::Overline)
                    drawLine(p1 + PointF{ 0.f, run.metrics.overlineOffset() },
                             p2 + PointF{ 0.f, run.metrics.overlineOffset() }, run.metrics.lineThickness,
                             LineEnd::Butt, std::tuple{ strokeWidth = 0.f, runArgs });
                if (run.decoration && TextDecoration::LineThrough)
                    drawLine(p1 + PointF{ 0.f, run.metrics.lineThroughOffset() },
                             p2 + PointF{ 0.f, run.metrics.lineThroughOffset() }, run.metrics.lineThickness,
                             LineEnd::Butt, std::tuple{ strokeWidth = 0.f, runArgs });
            }
        }
    }
}

void SdfCanvas::drawRectangle(const GeometryRectangle& rect, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Rectangles, args)), one(rect));
}

void SdfCanvas::drawShadow(RectangleF rect, CornersF borderRadius, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Shadow, args)),
                      one(GeometryRectangle{ rect, borderRadius }));
}

void SdfCanvas::drawEllipse(RectangleF rect, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Rectangles, args)),
                      one(GeometryRectangle{ rect, std::min(rect.width(), rect.height()) * 0.5f }));
}

void SdfCanvas::drawTexture(RectangleF rect, Rc<Image> tex, const Matrix& matrix, RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Rectangles, args);
    prepareStateInplace(style);
    style.textureMatrix = (Matrix::scaling(rect.width() / tex->width(), rect.height() / tex->height()) *
                           matrix * Matrix::translation(rect.x1, rect.y1))
                              .invert()
                              .value_or(Matrix{});
    style.imageHandle = std::move(tex);
    style.strokeWidth = 0.f;
    m_context.command(std::move(style), one(GeometryRectangle{ rect, 0.f }));
}

void SdfCanvas::drawArc(PointF center, float outerRadius, float innerRadius, float startAngle, float endEngle,
                        RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Arcs, args)),
                      one(GeometryArc{ center, outerRadius, innerRadius, startAngle, endEngle, 0.f, 0.f }));
}

void SdfCanvas::drawMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                         RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Mask, glyphs.size(), args);
    style.subpixelMode       = SubpixelMode::Off;
    style.spriteOversampling = 1;
    style.sprites            = std::move(sprites);
    prepareStateInplace(style);
    m_context.command(std::move(style), glyphs);
}

void SdfCanvas::drawColorMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                              RenderStateExArgs args) {
    RenderStateEx style(ShaderType::ColorMask, glyphs.size(), args);
    style.subpixelMode       = SubpixelMode::Off;
    style.spriteOversampling = 1;
    style.sprites            = std::move(sprites);
    prepareStateInplace(style);
    m_context.command(std::move(style), glyphs);
}

void SdfCanvas::drawText(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                         RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Text, glyphs.size(), args);
    Simd<float, 4> abcd{ style.coordMatrix.a, style.coordMatrix.b, style.coordMatrix.c, style.coordMatrix.d };
    if (horizontalAll(eq(abcd, Simd<float, 4>{ 1.f, 0.f, 0.f, 1.f }))) {
        style.subpixelMode = SubpixelMode::RGB;
    } else if (horizontalAll(eq(abcd, Simd<float, 4>{ -1.f, -0.f, -0.f, -1.f }))) {
        style.subpixelMode = SubpixelMode::BGR;
    } else {
        style.subpixelMode = SubpixelMode::Off;
    }
    style.spriteOversampling = fonts->hscale();
    style.sprites            = std::move(sprites);
    prepareStateInplace(style);
    m_context.command(std::move(style), glyphs);
}

} // namespace Brisk
