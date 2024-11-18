/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
#include <brisk/graphics/RawCanvas.hpp>
#include <brisk/core/Encoding.hpp>
#include <vpath.h>
#include <vrle.h>
#include <vraster.h>

namespace Brisk {

float& pixelRatio() noexcept {
    thread_local float ratio = 1;
    return ratio;
}

static float uintToFloatSafe(uint32_t x) noexcept {
    return std::bit_cast<float>((x & 0x0FFF'FFFFu) | 0x3000'0000u);
}

static int32_t findOrAdd(SpriteResources& container, RC<SpriteResource> value) {
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

static GeometryGlyphs glyphLayout(SpriteResources& sprites, const PrerenderedText& prerendered) {
    GeometryGlyphs result;
    for (const GlyphRun& run : prerendered.runs) {
        for (const Internal::Glyph& g : run.glyphs) {
            optional<Internal::GlyphData> data = g.load(run);
            if (data && data->sprite) {
                GeometryGlyph glyphDesc;
                PointF pos = g.pos + run.position;
                glyphDesc.rect.p1 =
                    PointF(pos.x, std::lround(pos.y)) + PointF(data->offset_x, -data->offset_y);
                glyphDesc.rect.p2 =
                    glyphDesc.rect.p1 + PointF(float(data->size.width) / fonts->hscale(), data->size.height);
                glyphDesc.sprite = static_cast<float>(findOrAdd(sprites, data->sprite));
                glyphDesc.stride = data->size.width;
                glyphDesc.size   = data->size;

                result.push_back(std::move(glyphDesc));
            }
        }
    }
    return result;
}

GeometryGlyphs pathLayout(SpriteResources& sprites, const RasterizedPath& path) {
    GeometryGlyphs result;
    if (path.sprite) {
        result.push_back(GeometryGlyph{
            path.bounds,
            path.bounds.size(),
            static_cast<float>(findOrAdd(sprites, path.sprite)),
            float(path.sprite->size.width),
        });
    }
    return result;
}

RawCanvas& RawCanvas::drawText(PointF pos, const TextWithOptions& text, const Font& font,
                               const ColorF& textColor) {
    PrerenderedText run = fonts->prerender(font, text);
    run.applyOffset(pos);
    drawText(run, fillColor = textColor);
    return *this;
}

RawCanvas& RawCanvas::drawText(PointF pos, float x_alignment, float y_alignment, const TextWithOptions& text,
                               const Font& font, const ColorF& textColor) {
    PrerenderedText run = fonts->prerender(font, text);
    run.alignLines(pos, x_alignment, y_alignment);
    drawText(run, fillColor = textColor);
    return *this;
}

RawCanvas& RawCanvas::drawText(RectangleF rect, float x_alignment, float y_alignment,
                               const TextWithOptions& text, const Font& font, const ColorF& textColor) {
    PrerenderedText run = fonts->prerender(font, text);
    run.alignLines(rect, x_alignment, y_alignment);
    drawText(run, fillColor = textColor);
    return *this;
}

RawCanvas::RawCanvas(RenderContext& context) : m_context(context) {}

RectangleF RawCanvas::align(RectangleF rect) const {
    return RectangleF{ PointF(round(rect.p1.v)), SizeF(max(SIMD{ 1.f, 1.f }, round(rect.size().v))) };
}

PointF RawCanvas::align(PointF v) const {
    return PointF(round(v.v));
}

RenderStateEx RawCanvas::prepareState(RenderStateEx&& state) {
    prepareStateInplace(state);
    return state;
}

void RawCanvas::prepareStateInplace(RenderStateEx& state) {
    state.scissor     = m_state.scissors;
    state.coordMatrix = state.coordMatrix.translate(m_state.offset);
    state.premultiply();
}

RawCanvas& RawCanvas::drawLine(PointF p1, PointF p2, float thickness, const ColorF& color, LineEnd end) {
    return drawLine(p1, p2, thickness, end, fillColor = color, strokeWidth = 0.f);
}

RawCanvas& RawCanvas::drawLine(PointF p1, PointF p2, float thickness, LineEnd end, RenderStateExArgs args) {
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
        end == LineEnd::Round ? thickness * 0.5f : 0.f, angle, args);
}

RawCanvas& RawCanvas::drawRectangle(RectangleF rect, float borderRadius, float angle,
                                    RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Rectangles, args)),
                      one(GeometryRectangle{ rect, angle, borderRadius, 255.f, 0.f }));
    return *this;
}

RawCanvas& RawCanvas::drawText(const PrerenderedText& run, RenderStateExArgs args) {
    SpriteResources sprites;
    GeometryGlyphs g = glyphLayout(sprites, run);
    drawText(std::move(sprites), g, args);
    for (const GlyphRun& run : run.runs) {
        if (run.decoration != TextDecoration::None) {
            run.updateRanges();
            PointF p1{ run.textHRange.min + run.position.x, run.position.y };
            PointF p2{ run.textHRange.max + run.position.x, run.position.y };

            if (run.decoration && TextDecoration::Underline)
                drawLine(p1 + PointF{ 0.f, run.metrics.underlineOffset() },
                         p2 + PointF{ 0.f, run.metrics.underlineOffset() }, run.metrics.lineThickness,
                         LineEnd::Butt, strokeWidth = 0.f, args);
            if (run.decoration && TextDecoration::Overline)
                drawLine(p1 + PointF{ 0.f, run.metrics.overlineOffset() },
                         p2 + PointF{ 0.f, run.metrics.overlineOffset() }, run.metrics.lineThickness,
                         LineEnd::Butt, strokeWidth = 0.f, args);
            if (run.decoration && TextDecoration::LineThrough)
                drawLine(p1 + PointF{ 0.f, run.metrics.lineThroughOffset() },
                         p2 + PointF{ 0.f, run.metrics.lineThroughOffset() }, run.metrics.lineThickness,
                         LineEnd::Butt, strokeWidth = 0.f, args);
        }
    }
    return *this;
}

RawCanvas& RawCanvas::drawRectangle(const GeometryRectangle& rect, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Rectangles, args)), one(rect));
    return *this;
}

RawCanvas& RawCanvas::drawShadow(RectangleF rect, float borderRadius, float angle, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Shadow, args)),
                      one(GeometryRectangle{ rect, angle, borderRadius, 255.f, 0.f }));
    return *this;
}

RawCanvas& RawCanvas::drawEllipse(RectangleF rect, float angle, RenderStateExArgs args) {
    m_context.command(
        prepareState(RenderStateEx(ShaderType::Rectangles, args)),
        one(GeometryRectangle{ rect, angle, std::min(rect.width(), rect.height()) * 0.5f, 255.f, 0.f }));
    return *this;
}

RawCanvas& RawCanvas::drawTexture(RectangleF rect, const ImageHandle& tex, const Matrix2D& matrix,
                                  RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Rectangles, args);
    prepareStateInplace(style);
    style.imageHandle    = tex;
    style.texture_matrix = (Matrix2D::scaling(rect.width() / tex->width(), rect.height() / tex->height()) *
                            matrix * Matrix2D::translation(rect.x1, rect.y1))
                               .invert()
                               .value_or(Matrix2D{});
    m_context.command(std::move(style), one(GeometryRectangle{ rect, 0.f, 0.f, 0.f, 0.f }));
    return *this;
}

RawCanvas& RawCanvas::drawArc(PointF center, float outerRadius, float innerRadius, float startAngle,
                              float endEngle, RenderStateExArgs args) {
    m_context.command(prepareState(RenderStateEx(ShaderType::Arcs, args)),
                      one(GeometryArc{ center, outerRadius, innerRadius, startAngle, endEngle, 0.f, 0.f }));
    return *this;
}

RawCanvas& RawCanvas::drawMask(SpriteResources sprites, std::span<GeometryGlyph> glyphs,
                               RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Mask, glyphs.size(), args);
    style.subpixel_mode       = SubpixelMode::Off;
    style.sprite_oversampling = 1;
    style.sprites             = std::move(sprites);
    prepareStateInplace(style);
    m_context.command(std::move(style), glyphs);
    return *this;
}

RawCanvas& RawCanvas::drawText(SpriteResources sprites, std::span<GeometryGlyph> glyphs,
                               RenderStateExArgs args) {
    RenderStateEx style(ShaderType::Text, glyphs.size(), args);
    style.subpixel_mode       = SubpixelMode::RGB;
    style.sprite_oversampling = fonts->hscale();
    style.sprites             = std::move(sprites);
    prepareStateInplace(style);
    m_context.command(std::move(style), glyphs);
    return *this;
}
} // namespace Brisk
