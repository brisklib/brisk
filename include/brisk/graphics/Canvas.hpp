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
 */                                                                                                          \
#pragma once

#include "Renderer.hpp"
#include "Fonts.hpp"
#include "Path.hpp"
#include <brisk/core/internal/Span.hpp>
#include "Gradients.hpp"
#include <brisk/core/internal/InlineVector.hpp>
#include "Image.hpp"

namespace Brisk {

float& pixelRatio() noexcept;

template <typename T>
inline float dp(T value) noexcept {
    return static_cast<float>(value) * pixelRatio();
}

template <typename T>
inline int idp(T value) noexcept {
    return static_cast<int>(std::round(static_cast<float>(value) * pixelRatio()));
}

template <typename T>
inline float invertdp(T value) noexcept {
    return static_cast<float>(value) / pixelRatio();
}

template <typename T>
inline int invertidp(T value) noexcept {
    return static_cast<int>(std::round(static_cast<float>(value) / pixelRatio()));
}

inline float operator""_dp(long double value) noexcept {
    return dp(value);
}

inline int operator""_idp(long double value) noexcept {
    return idp(value);
}

inline float operator""_dp(unsigned long long value) noexcept {
    return dp(value);
}

inline int operator""_idp(unsigned long long value) noexcept {
    return idp(value);
}

inline float scalePixels(float x) noexcept {
    return dp(x);
}

inline int scalePixels(int x) noexcept {
    return idp(x);
}

inline PointF scalePixels(PointF x) noexcept {
    return { dp(x.x), dp(x.y) };
}

inline Point scalePixels(Point x) noexcept {
    return { idp(x.x), idp(x.y) };
}

inline SizeF scalePixels(SizeF x) noexcept {
    return { dp(x.x), dp(x.y) };
}

inline Size scalePixels(Size x) noexcept {
    return { idp(x.x), idp(x.y) };
}

inline EdgesF scalePixels(const EdgesF& x) noexcept {
    return { dp(x.x1), dp(x.y1), dp(x.x2), dp(x.y2) };
}

inline Edges scalePixels(const Edges& x) noexcept {
    return { idp(x.x1), idp(x.y1), idp(x.x2), idp(x.y2) };
}

inline Font scalePixels(Font x) noexcept {
    Font result          = std::move(x);
    result.fontSize      = dp(result.fontSize);
    result.letterSpacing = dp(result.letterSpacing);
    result.wordSpacing   = dp(result.wordSpacing);
    return result;
}

inline float unscalePixels(float x) noexcept {
    return invertdp(x);
}

inline int unscalePixels(int x) noexcept {
    return invertidp(x);
}

inline PointF unscalePixels(PointF x) noexcept {
    return { invertdp(x.x), invertdp(x.y) };
}

inline Point unscalePixels(Point x) noexcept {
    return { invertidp(x.x), invertidp(x.y) };
}

inline SizeF unscalePixels(SizeF x) noexcept {
    return { invertdp(x.x), invertdp(x.y) };
}

inline Size unscalePixels(Size x) noexcept {
    return { invertidp(x.x), invertidp(x.y) };
}

inline EdgesF unscalePixels(const EdgesF& x) noexcept {
    return { invertdp(x.x1), invertdp(x.y1), invertdp(x.x2), invertdp(x.y2) };
}

inline Edges unscalePixels(const Edges& x) noexcept {
    return { invertidp(x.x1), invertidp(x.y1), invertidp(x.x2), invertidp(x.y2) };
}

inline Font unscalePixels(Font x) noexcept {
    Font result          = std::move(x);
    result.fontSize      = invertdp(result.fontSize);
    result.letterSpacing = invertdp(result.letterSpacing);
    result.wordSpacing   = invertdp(result.wordSpacing);
    return result;
}

using GeometryGlyphs = std::vector<GeometryGlyph>;

namespace Internal {
GeometryGlyphs pathLayout(SpriteResources& sprites, const RasterizedPath& path);
}

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
    Rc<Image> image;                      ///< The image used as the texture.
    Matrix matrix;                        ///< The transformation matrix applied to the texture.
    SamplerMode mode = SamplerMode::Wrap; ///< The sampler mode (Clamp or Wrap).
    float blurRadius = 0.f;               ///< The radius of the blur applied to the image.
};

/**
 * @typedef Paint
 * @brief A type representing various fill and stroke styles.
 *
 * Paint can hold one of several types representing
 * different kinds of paints: a solid color (ColorW), a gradient (Gradient),
 * or a texture (Texture).
 */
using Paint = std::variant<ColorW, Gradient, Texture>;

namespace Internal {
struct PaintAndTransform;
} // namespace Internal

/**
 * @brief Enum class defining flags for canvas rendering options.
 */
enum class CanvasFlags {
    None    = 0,  ///< No special rendering flags.
    Sdf     = 1,  ///< Use Signed Distance Field for compatible shapes.
    Default = Sdf ///< Default rendering flag, set to Sdf.
};

template <>
constexpr inline bool isBitFlags<CanvasFlags> = true;

/**
 * @class Canvas
 * @brief A high-level class for rendering graphical elements on a canvas.
 *
 * The Canvas class provides an interface for drawing various shapes, text, and images
 * onto a canvas. It extends the functionality of BasicCanvas by adding state management
 * and more sophisticated drawing operations.
 */
class Canvas {
public:
    /**
     * @brief Constructs a Canvas object using a RenderContext.
     *
     * @param context The rendering context used for drawing operations.
     */
    explicit Canvas(RenderContext& context, CanvasFlags flags = CanvasFlags::Default);

    /**
     * @brief Retrieves the render context for the canvas.
     * @return Reference to the RenderContext object.
     */
    RenderContext& renderContext() const {
        return m_context;
    }

    /**
     * @brief Retrieves the canvas rendering flags.
     * @return The current CanvasFlags value.
     */
    CanvasFlags flags() const noexcept {
        return m_flags;
    }

    /**
     * @brief Draws a stroked path on the canvas with specified parameters.
     *
     * This stateless function renders a path outline using the provided stroke paint, parameters,
     * transformation matrix, clipping rectangle, and opacity. It does not modify or access the canvas state.
     *
     * @param path The path to be stroked.
     * @param strokePaint The paint used for stroking the path.
     * @param params The stroke parameters (e.g., width, cap, join).
     * @param matrix The transformation matrix to apply to the path.
     * @param clipRect The clipping rectangle in canvas coordinates.
     * @param opacity The opacity value (0.0 to 1.0) for rendering.
     */
    void strokePath(Path path, const Paint& strokePaint, const StrokeParams& params, const Matrix& matrix,
                    RectangleF clipRect, float opacity);

    /**
     * @brief Fills a path on the canvas with specified parameters.
     *
     * This stateless function fills a path using the provided fill paint, parameters, transformation
     * matrix, clipping rectangle, and opacity. It does not modify or access the canvas state.
     *
     * @param path The path to be filled.
     * @param fillPaint The paint used for filling the path.
     * @param fillParams The fill parameters (e.g., fill rule).
     * @param matrix The transformation matrix to apply to the path.
     * @param clipRect The clipping rectangle in canvas coordinates.
     * @param opacity The opacity value (0.0 to 1.0) for rendering.
     */
    void fillPath(Path path, const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                  RectangleF clipRect, float opacity);

    /**
     * @brief Draws a path on the canvas with both stroke and fill using specified parameters.
     *
     * This stateless function renders a path with both stroking and filling, using the provided stroke
     * and fill paints, parameters, transformation matrix, clipping rectangle, and opacity. It does not
     * modify or access the canvas state.
     *
     * @param path The path to be drawn.
     * @param strokePaint The paint used for stroking the path.
     * @param strokeParams The stroke parameters (e.g., width, cap, join).
     * @param fillPaint The paint used for filling the path.
     * @param fillParams The fill parameters (e.g., fill rule).
     * @param matrix The transformation matrix to apply to the path.
     * @param clipRect The clipping rectangle in canvas coordinates.
     * @param opacity The opacity value (0.0 to 1.0) for rendering.
     */
    void drawPath(Path path, const Paint& strokePaint, const StrokeParams& strokeParams,
                  const Paint& fillPaint, const FillParams& fillParams, const Matrix& matrix,
                  RectangleF clipRect, float opacity);

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
     * This function renders the outline of the specified path using the canvas's current stroke
     * settings (e.g., stroke paint, stroke parameters).
     *
     * @param path The Path struct to stroke.
     */
    void strokePath(Path path);

    /**
     * @brief Fills a given path with the current fill settings.
     *
     * This function fills the specified path using the canvas's current fill settings
     * (e.g., fill paint, fill parameters).
     *
     * @param path The Path struct to fill.
     */
    void fillPath(Path path);

    /**
     * @brief Draws a given path with the current stroke and fill settings.
     *
     * This function renders the specified path by applying both the canvas's current stroke
     * and fill settings (e.g., stroke paint, fill paint, stroke and fill parameters).
     *
     * @param path The Path struct to draw.
     */
    void drawPath(Path path);

    /**
     * @brief Strokes a rectangle with the current stroke settings.
     *
     * This function renders the outline of the specified rectangle using the canvas's current stroke
     * settings. The rectangle can have rounded corners defined by borderRadius, and if squircle is true,
     * the rounded corners will use a squircle shape.
     *
     * @param rect The RectangleF struct defining the rectangle to stroke.
     * @param borderRadius The CornersF struct specifying the border radius for each corner (default: 0.f).
     * @param squircle If true, rounded corners will have a squircle shape (default: false).
     */
    void strokeRect(RectangleF rect, CornersF borderRadius = 0.f, bool squircle = false);

    /**
     * @brief Fills a rectangle with the current fill settings.
     *
     * This function fills the specified rectangle using the canvas's current fill settings.
     * The rectangle can have rounded corners defined by borderRadius, and if squircle is true,
     * the rounded corners will use a squircle shape.
     *
     * @param rect The RectangleF struct defining the rectangle to fill.
     * @param borderRadius The CornersF struct specifying the border radius for each corner (default: 0.f).
     * @param squircle If true, rounded corners will have a squircle shape (default: false).
     */
    void fillRect(RectangleF rect, CornersF borderRadius = 0.f, bool squircle = false);

    /**
     * @brief Draws a rectangle with the current stroke and fill settings.
     *
     * This function renders the specified rectangle by applying both the canvas's current stroke
     * and fill settings. The rectangle can have rounded corners defined by borderRadius, and if
     * squircle is true, the rounded corners will use a squircle shape.
     *
     * @param rect The RectangleF struct defining the rectangle to draw.
     * @param borderRadius The CornersF struct specifying the border radius for each corner (default: 0.f).
     * @param squircle If true, rounded corners will have a squircle shape (default: false).
     */
    void drawRect(RectangleF rect, CornersF borderRadius = 0.f, bool squircle = false);

    /**
     * @brief Draws a blurred rectangle, typically used for rendering shadows.
     *
     * This function renders a blurred rectangle with the specified blur radius. The rectangle can
     * have rounded corners defined by borderRadius, and if squircle is true, the rounded corners
     * will use a squircle shape. This is often used to create shadow effects.
     *
     * @param rect The RectangleF struct defining the rectangle to blur.
     * @param blurRadius The radius of the blur effect.
     * @param borderRadius The CornersF struct specifying the border radius for each corner (default: 0.f).
     * @param squircle If true, rounded corners will have a squircle shape (default: false).
     */
    void blurRect(RectangleF rect, float blurRadius, CornersF borderRadius = 0.f, bool squircle = false);

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

    void drawEllipse(RectangleF rect);

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
    void fillText(TextWithOptions text, PointF position, PointF alignment = PointF{ 0.f, 0.f });

    /**
     * @brief Fills text within a specified rectangular area with alignment.
     *
     * @param text The text to render.
     * @param position The RectangleF struct representing the area to fill the text within.
     * @param alignment The alignment of the text relative to the rectangle. Defaults to {0.f, 0.f}.
     */
    void fillText(TextWithOptions text, RectangleF position, PointF alignment = PointF{ 0.f, 0.f });

    /**
     * @brief Fills pre-rendered text at the specified position.
     *
     * This function renders the provided PreparedText at the given position using the canvas's
     * current fill settings.
     *
     * @param position The PointF specifying the top-left position of the text.
     * @param text The PreparedText struct containing the pre-rendered text to fill.
     */
    void fillText(PointF position, const PreparedText& text);

    /**
     * @brief Fills pre-rendered text with specified alignment.
     *
     * This function renders the provided PreparedText at the given position, aligned according
     * to the specified alignment point, using the canvas's current fill settings.
     *
     * @param position The PointF specifying the reference position for the text.
     * @param alignment The PointF specifying the alignment offset relative to the position.
     * @param text The PreparedText struct containing the pre-rendered text to fill.
     */
    void fillText(PointF position, PointF alignment, const PreparedText& text);

    /**
     * @brief Fills the selection rectangle(s) for pre-rendered text.
     *
     * This function renders the selection rectangle(s) for the specified range within the
     * PreparedText at the given position, using the canvas's current fill settings. It does
     * not render the text itself, only the selection background.
     *
     * @param position The PointF specifying the top-left position of the text.
     * @param text The PreparedText struct containing the pre-rendered text.
     * @param selection The Range<uint32_t> specifying the start and end indices of the selection.
     */
    void fillTextSelection(PointF position, const PreparedText& text, Range<uint32_t> selection);

    /**
     * @brief Fills the selection rectangle(s) for pre-rendered text with specified alignment.
     *
     * This function renders the selection rectangle(s) for the specified range within the
     * PreparedText at the given position, aligned according to the specified alignment point,
     * using the canvas's current fill settings. It does not render the text itself, only the
     * selection background.
     *
     * @param position The PointF specifying the reference position for the text.
     * @param alignment The PointF specifying the alignment offset relative to the position.
     * @param text The PreparedText struct containing the pre-rendered text.
     * @param selection The Range<uint32_t> specifying the start and end indices of the selection.
     */
    void fillTextSelection(PointF position, PointF alignment, const PreparedText& text,
                           Range<uint32_t> selection);

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
    void drawImage(RectangleF rect, Rc<Image> image, Matrix matrix = {},
                   SamplerMode samplerMode = SamplerMode::Clamp, float blurRadius = 0.f);

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
    std::optional<RectangleF> getClipRect() const;

    /**
     * @brief Sets the clipping rectangle.
     *
     * @param rect The Rectangle object defining the new clipping area.
     */
    void setClipRect(RectangleF rect);

    /**
     * @brief Resets the clipping rectangle to cover the entire canvas.
     */
    void resetClipRect();

    /**
     * @brief Resets the Canvas state to its default values.
     *
     * This function clears all current state settings and restores the canvas to its initial
     * default state as defined by defaultState.
     */
    void reset();

    /**
     * @brief Saves the current state of the Canvas.
     *
     * The saved state, including clip rectangle, transform, paints, and other parameters, is
     * pushed onto an internal stack and can be restored later using the restore() function.
     */
    void save();

    /**
     * @brief Restores the most recently saved Canvas state.
     *
     * This function pops the most recently saved state from the internal stack and applies it
     * to the canvas, discarding the previous state.
     */
    void restore();

    /**
     * @brief Restores the most recently saved Canvas state without removing it from the stack.
     *
     * This function applies the most recently saved state to the canvas but keeps the state
     * on the internal stack for future restores.
     */
    void restoreNoPop();

    /**
     * @brief Structure holding the complete state of the Canvas.
     *
     * This struct contains all configurable properties of the canvas, such as clipping,
     * transformation, paints, and drawing parameters.
     */
    struct State {
        RectangleF clipRect;       ///< The current clipping rectangle.
        Matrix transform;          ///< The current transformation matrix.
        Paint strokePaint;         ///< The current stroke paint settings.
        Paint fillPaint;           ///< The current fill paint settings.
        StrokeParams strokeParams; ///< The current stroke parameters (e.g., width, cap, join).
        float opacity;             ///< The current opacity value (0.0 to 1.0).
        FillParams fillParams;     ///< The current fill parameters (e.g., fill rule).
        Font font;                 ///< The current font settings.
    };

    /**
     * @brief The default state of the Canvas.
     *
     * This static constant defines the initial values for all state properties when the canvas
     * is reset or initialized.
     */
    static const State defaultState;

    /**
     * @brief Retrieves the current state of the Canvas.
     *
     * @return A const reference to the current State object.
     */
    const State& state() const noexcept;

    /**
     * @brief Sets the Canvas state to the specified state.
     *
     * This function applies the provided State object to the canvas, updating all relevant
     * properties such as clip rectangle, transform, paints, and drawing parameters.
     *
     * @param state The State object to apply to the canvas.
     */
    void setState(State state);

    /**
     * @brief RAII utility class for saving and restoring the Canvas state.
     *
     * This class saves the current canvas state upon construction and restores it upon
     * destruction. It provides access to the current state via operator* and operator->,
     * allowing direct modification of state fields for drawing operations. To minimize
     * assignments and copies, assign the result to an auto&& variable, e.g.,
     * `auto&& state = canvas.saveState();`.
     */
    struct StateSaver {
        Canvas& canvas; ///< Reference to the associated Canvas.
        State saved;    ///< The saved state to restore on destruction.

        /**
         * @brief Constructs a StateSaver, saving the current canvas state.
         *
         * @param canvas The Canvas whose state is to be saved.
         */
        StateSaver(Canvas& canvas);

        /**
         * @brief Provides access to the current canvas state for modification.
         *
         * @return Reference to the current State object.
         */
        State& operator*();

        /**
         * @brief Provides pointer-like access to the current canvas state for modification.
         *
         * @return Pointer to the current State object.
         */
        State* operator->();

        /**
         * @brief Destructor that restores the saved canvas state.
         */
        ~StateSaver();
    };

    /**
     * @brief Saves the current canvas state and returns a StateSaver for RAII management.
     *
     * The returned StateSaver saves the current state and restores it upon destruction.
     * Assign the result to an auto&& variable to avoid unnecessary copies, e.g.,
     * `auto&& state = canvas.saveState();`.
     *
     * @return A StateSaver object managing the saved state.
     */
    StateSaver saveState() &;

    /**
     * @brief RAII utility class for saving and restoring the Canvas clip rectangle.
     *
     * This class saves the current clip rectangle upon construction and restores it upon
     * destruction. It provides access to the current clip rectangle via operator* and
     * operator->, allowing direct modification for drawing operations. To minimize
     * assignments and copies, assign the result to an auto&& variable, e.g.,
     * `auto&& clip = canvas.saveClipRect();`.
     */
    struct ClipRectSaver {
        Canvas& canvas;   ///< Reference to the associated Canvas.
        RectangleF saved; ///< The saved clip rectangle to restore on destruction.

        /**
         * @brief Constructs a ClipRectSaver, saving the current clip rectangle.
         *
         * @param canvas The Canvas whose clip rectangle is to be saved.
         */
        ClipRectSaver(Canvas& canvas);

        /**
         * @brief Provides access to the current clip rectangle for modification.
         *
         * @return Reference to the current RectangleF object.
         */
        RectangleF& operator*();

        /**
         * @brief Provides pointer-like access to the current clip rectangle for modification.
         *
         * @return Pointer to the current RectangleF object.
         */
        RectangleF* operator->();

        /**
         * @brief Destructor that restores the saved clip rectangle.
         */
        ~ClipRectSaver();
    };

    /**
     * @brief Saves the current clip rectangle and returns a ClipRectSaver for RAII management.
     *
     * The returned ClipRectSaver saves the current clip rectangle and restores it upon
     * destruction. Assign the result to an auto&& variable to avoid unnecessary copies, e.g.,
     * `auto&& clip = canvas.saveClipRect();`.
     *
     * @return A ClipRectSaver object managing the saved clip rectangle.
     */
    ClipRectSaver saveClipRect() &;

    int rasterizedPaths() const noexcept;

private:
    RenderContext& m_context;
    CanvasFlags m_flags = CanvasFlags::Default;
    State m_state;              ///< The current state of the Canvas.
    std::vector<State> m_stack; ///< The stack of saved Canvas states.

    int m_rasterizedPaths = 0;
    void drawRasterizedPath(const RasterizedPath& path, const Internal::PaintAndTransform& paint,
                            Quad3 scissors);
};

} // namespace Brisk
