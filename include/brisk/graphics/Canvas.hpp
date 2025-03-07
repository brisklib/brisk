#pragma once

#include "RawCanvas.hpp"
#include "Gradients.hpp"
#include <brisk/core/internal/InlineVector.hpp>
#include "Image.hpp"

namespace Brisk {

class Canvas;

/**
 * @struct Texture
 * @brief Represents a textured fill pattern for drawing operations.
 *
 * The Texture structure holds an image and a transformation matrix. The image is
 * used as a texture, and the matrix defines how the texture is transformed when
 * applied to a surface.
 */
struct Texture {
    RC<Image> image;                      ///< The image used as the texture.
    Matrix matrix;                        ///< The transformation matrix applied to the texture.
    SamplerMode mode = SamplerMode::Wrap; ///< The sampler mode.
};

/**
 * @typedef Paint
 * @brief A versatile type representing various fill and stroke styles.
 *
 * Paint can hold one of several types representing
 * different kinds of paints: a solid color (ColorW), a gradient (RC<Gradient>),
 * or a texture (Texture).
 */
using Paint = std::variant<ColorW, RC<Gradient>, Texture>;

void applier(RenderStateEx*, const std::pair<Canvas*, Paint*>);

/**
 * @class Canvas
 * @brief A high-level class for rendering graphical elements on a canvas.
 *
 * The Canvas class provides an interface for drawing various shapes, text, and images
 * onto a canvas. It extends the functionality of RawCanvas by adding state management
 * and more sophisticated drawing operations.
 */
class Canvas : protected RawCanvas {
public:
    /**
     * @brief Constructs a Canvas object using a RenderContext.
     *
     * @param context The rendering context used for drawing operations.
     */
    explicit Canvas(RenderContext& context);

    using RawCanvas::renderContext;

    /**
     * @brief Constructs a Canvas object using an existing RawCanvas.
     *
     * @param canvas The RawCanvas instance to wrap.
     */
    explicit Canvas(RawCanvas& canvas);

    /**
     * @brief Provides access to the underlying RawCanvas object.
     *
     * @return A reference to the underlying RawCanvas.
     */
    BRISK_INLINE RawCanvas& raw() noexcept {
        return static_cast<RawCanvas&>(*this);
    }

    /**
     * @brief Retrieves the current stroke paint configuration.
     *
     * @return The current stroke Paint struct.
     */
    [[nodiscard]] const Paint& getStrokePaint() const;

    /**
     * @brief Sets the stroke paint configuration.
     *
     * @param paint The Paint struct to use for strokes.
     */
    void setStrokePaint(Paint paint);

    /**
     * @brief Retrieves the current fill paint configuration.
     *
     * @return The current fill Paint struct.
     */
    [[nodiscard]] const Paint& getFillPaint() const;

    /**
     * @brief Sets the fill paint configuration.
     *
     * @param paint The Paint struct to use for fills.
     */
    void setFillPaint(Paint paint);

    /**
     * @brief Retrieves the current stroke width.
     *
     * @return The stroke width in pixels.
     */
    [[nodiscard]] float getStrokeWidth() const;

    /**
     * @brief Sets the stroke width.
     *
     * @param width The width of the stroke in pixels.
     */
    void setStrokeWidth(float width);

    /**
     * @brief Retrieves the current opacity level.
     *
     * @return The opacity level as a float, where 1.0 is fully opaque and 0.0 is fully transparent.
     */
    [[nodiscard]] float getOpacity() const;

    /**
     * @brief Sets the opacity level.
     *
     * @param opacity The opacity level, where 1.0 is fully opaque and 0.0 is fully transparent.
     */
    void setOpacity(float opacity);

    /**
     * @brief Retrieves the current stroke color.
     *
     * @return The stroke color as a ColorW struct.
     */
    [[nodiscard]] ColorW getStrokeColor() const;

    /**
     * @brief Sets the stroke color.
     *
     * @param color The ColorW struct representing the stroke color.
     */
    void setStrokeColor(ColorW color);

    /**
     * @brief Retrieves the current fill color.
     *
     * @return The fill color as a ColorW struct.
     */
    [[nodiscard]] ColorW getFillColor() const;

    /**
     * @brief Sets the fill color.
     *
     * @param color The ColorW struct representing the fill color.
     */
    void setFillColor(ColorW color);

    /**
     * @brief Retrieves the current miter limit for strokes.
     *
     * @return The miter limit as a float.
     */
    [[nodiscard]] float getMiterLimit() const;

    /**
     * @brief Sets the miter limit for strokes.
     *
     * The miter limit controls the maximum length of the miter join between
     * stroke segments. When the miter limit is exceeded, a bevel join is used instead.
     *
     * @param limit The miter limit as a float.
     */
    void setMiterLimit(float limit);

    /**
     * @brief Retrieves the current fill rule used for determining the interior of shapes.
     *
     * @return The FillRule enumeration value.
     */
    [[nodiscard]] FillRule getFillRule() const;

    /**
     * @brief Sets the fill rule used for determining the interior of shapes.
     *
     * @param fillRule The FillRule enumeration value.
     */
    void setFillRule(FillRule fillRule);

    /**
     * @brief Retrieves the current join style for stroke paths.
     *
     * @return The JoinStyle enumeration value.
     */
    [[nodiscard]] JoinStyle getJoinStyle() const;

    /**
     * @brief Sets the join style for stroke paths.
     *
     * @param joinStyle The JoinStyle enumeration value.
     */
    void setJoinStyle(JoinStyle joinStyle);

    /**
     * @brief Retrieves the current cap style for stroke endpoints.
     *
     * @return The CapStyle enumeration value.
     */
    [[nodiscard]] CapStyle getCapStyle() const;

    /**
     * @brief Sets the cap style for stroke endpoints.
     *
     * @param capStyle The CapStyle enumeration value.
     */
    void setCapStyle(CapStyle capStyle);

    /**
     * @brief Retrieves the current dash offset for dashed lines.
     *
     * @return The dash offset as a float.
     */
    [[nodiscard]] float getDashOffset() const;

    /**
     * @brief Sets the dash offset for dashed lines.
     *
     * @param offset The dash offset as a float.
     */
    void setDashOffset(float offset);

    /**
     * @brief Retrieves the current dash pattern for dashed lines.
     *
     * @return A reference to the DashArray representing the dash pattern.
     */
    [[nodiscard]] const DashArray& getDashArray() const;

    /**
     * @brief Sets the dash pattern for dashed lines.
     *
     * @param array A DashArray representing the dash pattern.
     */
    void setDashArray(const DashArray& array);

    /**
     * @brief Strokes a given path with the current stroke settings.
     *
     * @param path The Path struct to stroke.
     */
    void strokePath(Path path);

    /**
     * @brief Fills a given path with the current fill settings.
     *
     * @param path The Path struct to fill.
     */
    void fillPath(Path path);

    void strokePath(Path path, const Paint& strokePaint, const StrokeParams& params, const Matrix& matrix,
                    RectangleF clipRect);
    void fillPath(Path path, const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                  RectangleF clipRect);
    void drawPath(Path path, const Paint& strokePaint, const StrokeParams& strokeParams,
                  const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                  RectangleF clipRect);

    /**
     * @brief Strokes a rectangle with the current stroke settings.
     *
     * @param rect The RectangleF struct defining the rectangle to stroke.
     */
    void strokeRect(RectangleF rect);

    /**
     * @brief Fills a rectangle with the current fill settings.
     *
     * @param rect The RectangleF struct defining the rectangle to fill.
     */
    void fillRect(RectangleF rect);

    /**
     * @brief Strokes an ellipse defined by the bounding rectangle.
     *
     * @param rect The RectangleF struct defining the bounding box of the ellipse.
     */
    void strokeEllipse(RectangleF rect);

    /**
     * @brief Fills an ellipse defined by the bounding rectangle.
     *
     * @param rect The RectangleF struct defining the bounding box of the ellipse.
     */
    void fillEllipse(RectangleF rect);

    /**
     * @brief Strokes a polygon defined by a series of points.
     *
     * @param points A span of PointF structs defining the polygon vertices.
     * @param close Whether to close the polygon by connecting the last point to the first. Defaults to true.
     */
    void strokePolygon(std::span<const PointF> points, bool close = true);

    /**
     * @brief Fills a polygon defined by a series of points.
     *
     * @param points A span of PointF structs defining the polygon vertices.
     * @param close Whether to close the polygon by connecting the last point to the first. Defaults to true.
     */
    void fillPolygon(std::span<const PointF> points, bool close = true);

    /**
     * @brief Retrieves the current font used for text rendering.
     *
     * @return The current Font.
     */
    Font getFont() const;

    /**
     * @brief Sets the font used for text rendering.
     *
     * @param font The Font to use.
     */
    void setFont(const Font& font);

    /**
     * @brief Fills text at a specified position with alignment.
     *
     * @param text The text to render.
     * @param position The PointF struct representing the text position.
     * @param alignment The alignment of the text relative to the position. Defaults to {0.f, 0.f}.
     */
    void fillText(std::string_view text, PointF position, PointF alignment = PointF{ 0.f, 0.f });

    /**
     * @brief Fills text within a specified rectangular area with alignment.
     *
     * @param text The text to render.
     * @param position The RectangleF struct representing the area to fill the text within.
     * @param alignment The alignment of the text relative to the rectangle. Defaults to {0.f, 0.f}.
     */
    void fillText(std::string_view text, RectangleF position, PointF alignment = PointF{ 0.f, 0.f });

    /**
     * @brief Fills pre-rendered text.
     *
     * @param text The PreparedText struct to render.
     */
    void fillText(PointF position, const PreparedText& text);

    void fillText(PointF position, PointF alignment, const PreparedText& text);

    /**
     * @brief Strokes a line between two points.
     *
     * @param pt1 The starting point of the line.
     * @param pt2 The ending point of the line.
     */
    void strokeLine(PointF pt1, PointF pt2);

    /**
     * @brief Draws an image within a specified rectangular area.
     *
     * @param rect The RectangleF struct defining the area to draw the image.
     * @param image The Image to draw.
     * @param matrix The transformation matrix to apply to the image. Defaults to the identity matrix.
     */
    void drawImage(RectangleF rect, RC<Image> image, Matrix matrix = {},
                   SamplerMode samplerMode = SamplerMode::Clamp);

    /**
     * @brief Retrieves the current transformation matrix.
     *
     * @return The current transformation Matrix struct.
     */
    Matrix getTransform() const;

    /**
     * @brief Sets the transformation matrix.
     *
     * @param matrix The Matrix struct representing the transformation.
     */
    void setTransform(const Matrix& matrix);

    /**
     * @brief Applies an additional transformation to the current matrix.
     *
     * @param matrix The Matrix struct representing the transformation to apply.
     */
    void transform(const Matrix& matrix);

    /**
     * @brief Retrieves the current clipping rectangle.
     *
     * @return An optional Rectangle object representing the clipping area. If no clipping is applied, the
     * optional is empty.
     */
    std::optional<Rectangle> getClipRect() const;

    /**
     * @brief Sets the clipping rectangle.
     *
     * @param rect The Rectangle object defining the new clipping area.
     */
    void setClipRect(Rectangle rect);

    /**
     * @brief Resets the clipping rectangle to cover the entire canvas.
     */
    void resetClipRect();

    /**
     * @brief Resets the Canvas state to its default values.
     */
    void reset();

    /**
     * @brief Saves the current state of the Canvas.
     *
     * The saved state can be restored later using the restore() function.
     */
    void save();

    /**
     * @brief Restores the most recently saved Canvas state.
     *
     * This operation also removes the state from the stack.
     */
    void restore();

    /**
     * @brief Restores the most recently saved Canvas state without removing it from the stack.
     */
    void restoreNoPop();

private:
    struct State {
        Rectangle clipRect;
        Matrix transform;
        Paint strokePaint;
        Paint fillPaint;
        StrokeParams strokeParams;
        float opacity;
        FillParams fillParams;
        Font font;
    };

    static const State defaultState;
    State m_state;              ///< The current state of the Canvas.
    std::vector<State> m_stack; ///< The stack of saved Canvas states.
    void drawPath(const RasterizedPath& path, const Paint& paint);

    static Rectangle transformedClipRect(const Matrix& matrix, RectangleF clipRect);
    void setPaint(RenderStateEx& renderState, const Paint& paint);
    friend void applier(RenderStateEx*, const std::pair<Canvas*, Paint*>);
};

} // namespace Brisk
