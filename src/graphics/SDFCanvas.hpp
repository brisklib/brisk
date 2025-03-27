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
#pragma once
#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/graphics/Canvas.hpp>
#include "RenderStateArgs.hpp"

namespace Brisk {

class SDFCanvas {
public:
    explicit SDFCanvas(RenderContext& context, CanvasFlags flags);

    enum class LineEnd {
        Butt,
        Square,
        Round,
    };

    void drawLine(PointF p1, PointF p2, float thickness, LineEnd end, RenderStateExArgs args);

    void drawTextSelection(PointF pos, const PreparedText& prepared, Range<uint32_t> selection,
                           RenderStateExArgs args);
    void drawText(PointF pos, const PreparedText& run, RenderStateExArgs args);
    void drawText(SpriteResources sprites, std::span<const GeometryGlyph> glyphs, RenderStateExArgs args);

    void drawRectangle(RectangleF rect, CornersF borderRadius, RenderStateExArgs args);
    void drawRectangle(RectangleF rect, CornersF borderRadius, bool squircle, RenderStateExArgs args);
    void drawRectangle(const GeometryRectangle& rect, RenderStateExArgs args);

    void drawShadow(RectangleF rect, CornersF borderRadius, RenderStateExArgs args);

    void drawEllipse(RectangleF rect, RenderStateExArgs args);

    void drawArc(PointF center, float outerRadius, float innerRadius, float startAngle, float endEngle,
                 RenderStateExArgs args);

    void drawTexture(RectangleF rect, RC<Image> tex, const Matrix& matrix, RenderStateExArgs args);

    void drawMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs, RenderStateExArgs args);
    void drawColorMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                       RenderStateExArgs args);

    void drawLine(PointF p1, PointF p2, float thickness, ColorW color, LineEnd end = LineEnd::Butt);

    /// Draw text at the given point
    void drawText(PointF pos, const TextWithOptions& text, const Font& f, ColorW textColor);

    /// Draw text aligned inside the given rectangle
    void drawText(RectangleF rect, float x_alignment, float y_alignment, const TextWithOptions& text,
                  const Font& f, ColorW textColor);

    /// Draw text aligned around the given point
    void drawText(PointF pos, float x_alignment, float y_alignment, const TextWithOptions& text,
                  const Font& f, ColorW textColor);

    int rasterizedPaths() const noexcept;

    static void prepareStateInplace(RenderStateEx& state);
    static RenderStateEx prepareState(RenderStateEx&& state);

protected:
    RenderContext& m_context;
    CanvasFlags m_flags = CanvasFlags::Default;
};
} // namespace Brisk
