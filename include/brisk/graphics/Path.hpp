#pragma once

#include <unordered_map>
#include "Matrix.hpp"
#include <brisk/core/RC.hpp>
#include <mutex>
#include <variant>
#include <brisk/core/internal/InlineVector.hpp>
#include "internal/Sprites.hpp"

namespace Brisk {

/**
 * @brief Represents a rasterized path with a sprite and bounding rectangle.
 */
struct RasterizedPath {
    RC<SpriteResource> sprite; ///< The sprite resource associated with the rasterized path.
    Rectangle bounds;          ///< The bounding rectangle of the rasterized path.
};

/**
 * @brief Enum representing the fill rules for paths.
 */
enum class FillRule : uint8_t {
    EvenOdd, ///< Even-Odd fill rule.
    Winding, ///< Winding fill rule.
};

/**
 * @brief Enum representing different join styles for strokes.
 */
enum class JoinStyle : uint8_t {
    Miter, ///< Miter join style.
    Bevel, ///< Bevel join style.
    Round, ///< Round join style.
};

/**
 * @brief Enum representing different cap styles for strokes.
 */
enum class CapStyle : uint8_t {
    Flat,   ///< Flat cap style.
    Square, ///< Square cap style.
    Round,  ///< Round cap style.
};

/**
 * @typedef DashArray
 * @brief A container for storing dash patterns used in stroking paths.
 *
 * DashArray holds a sequence of floats,
 * representing the lengths of dashes and gaps in a dashed line pattern.
 */
using DashArray = inline_vector<float, 7>;

/**
 * @brief Structure representing stroke parameters.
 */
struct StrokeParams {
    JoinStyle joinStyle = JoinStyle::Miter; ///< The join style of the stroke.
    CapStyle capStyle   = CapStyle::Flat;   ///< The cap style of the stroke.
    float strokeWidth   = 1.f;              ///< The width of the stroke.
    float miterLimit    = 10.f;             ///< The limit for miter joins.
    DashArray dashArray{};
    float dashOffset = 0.f;

    StrokeParams scale(float value) const noexcept {
        StrokeParams copy = *this;
        copy.strokeWidth *= value;
        copy.miterLimit *= value;
        copy.dashOffset *= value;
        for (float& v : copy.dashArray)
            v *= value;
        return copy;
    }
};

/**
 * @brief Structure representing fill parameters.
 */
struct FillParams {
    FillRule fillRule = FillRule::Winding; ///< The fill rule to be used.
};

/**
 * @brief A type alias for fill or stroke parameters, using std::variant.
 */
using FillOrStrokeParams = std::variant<FillParams, StrokeParams>;

struct Path;

namespace Internal {
/**
 * @brief Rasterizes the given path with specified parameters and clipping rectangle.
 *
 * @param path The path to rasterize.
 * @param params The fill or stroke parameters.
 * @param clipRect The clipping rectangle. Use noClipRect to disable clipping.
 * @return RasterizedPath The resulting rasterized path.
 */
RasterizedPath rasterizePath(Path path, const FillOrStrokeParams& params, Rectangle clipRect);
} // namespace Internal

/**
 * @brief Represents a geometric path that can be rasterized for rendering.
 */
struct Path {
    Path();                       ///< Default constructor.
    ~Path();                      ///< Destructor.
    Path(Path&&);                 ///< Move constructor.
    Path(const Path&);            ///< Copy constructor.
    Path& operator=(Path&&);      ///< Move assignment operator.
    Path& operator=(const Path&); ///< Copy assignment operator.

    enum class Direction : uint8_t { CCW, CW };                      ///< Enum for the direction of the path.
    enum class Element : uint8_t { MoveTo, LineTo, CubicTo, Close }; ///< Enum for the elements of the path.

    /**
     * @brief Checks if the path is empty.
     * @return true if the path is empty, false otherwise.
     */
    bool empty() const;

    /**
     * @brief Moves the current point to a specified point.
     * @param p The point to move to.
     */
    void moveTo(PointF p);

    /**
     * @brief Moves the current point to specified coordinates.
     * @param x The x-coordinate to move to.
     * @param y The y-coordinate to move to.
     */
    void moveTo(float x, float y);

    /**
     * @brief Draws a line to a specified point.
     * @param p The point to draw to.
     */
    void lineTo(PointF p);

    /**
     * @brief Draws a line to specified coordinates.
     * @param x The x-coordinate to draw to.
     * @param y The y-coordinate to draw to.
     */
    void lineTo(float x, float y);

    /**
     * @brief Draws a quadratic Bézier curve to a specified endpoint using a control point.
     * @param c1 The control point.
     * @param e The endpoint.
     */
    void quadraticTo(PointF c1, PointF e);

    /**
     * @brief Draws a quadratic Bézier curve to specified coordinates using control point coordinates.
     * @param c1x The x-coordinate of the control point.
     * @param c1y The y-coordinate of the control point.
     * @param ex The x-coordinate of the endpoint.
     * @param ey The y-coordinate of the endpoint.
     */
    void quadraticTo(float c1x, float c1y, float ex, float ey);

    /**
     * @brief Draws a cubic Bézier curve to a specified endpoint using two control points.
     * @param c1 The first control point.
     * @param c2 The second control point.
     * @param e The endpoint.
     */
    void cubicTo(PointF c1, PointF c2, PointF e);

    /**
     * @brief Draws a cubic Bézier curve to specified coordinates using two control point coordinates.
     * @param c1x The x-coordinate of the first control point.
     * @param c1y The y-coordinate of the first control point.
     * @param c2x The x-coordinate of the second control point.
     * @param c2y The y-coordinate of the second control point.
     * @param ex The x-coordinate of the endpoint.
     * @param ey The y-coordinate of the endpoint.
     */
    void cubicTo(float c1x, float c1y, float c2x, float c2y, float ex, float ey);

    /**
     * @brief Draws an arc to a specified rectangle defined by its start angle and sweep length.
     * @param rect The rectangle defining the arc.
     * @param startAngle The starting angle of the arc.
     * @param sweepLength The angle of the arc's sweep.
     * @param forceMoveTo If true, moves to the endpoint of the arc.
     */
    void arcTo(RectangleF rect, float startAngle, float sweepLength, bool forceMoveTo);

    /**
     * @brief Closes the current sub-path by drawing a line back to the starting point.
     */
    void close();

    bool isClosed() const;

    /**
     * @brief Resets the path to an empty state.
     */
    void reset();

    /**
     * @brief Adds a circle to the path.
     * @param cx The x-coordinate of the center of the circle.
     * @param cy The y-coordinate of the center of the circle.
     * @param radius The radius of the circle.
     * @param dir The direction in which the circle is added (default is clockwise).
     */
    void addCircle(float cx, float cy, float radius, Direction dir = Direction::CW);

    /**
     * @brief Adds an ellipse to the path.
     * @param rect The rectangle that bounds the ellipse.
     * @param dir The direction in which the ellipse is added (default is clockwise).
     */
    void addEllipse(RectangleF rect, Direction dir = Direction::CW);

    /**
     * @brief Adds a rounded rectangle to the path.
     * @param rect The rectangle that defines the bounds of the rounded rectangle.
     * @param rx The radius of the horizontal corners.
     * @param ry The radius of the vertical corners.
     * @param dir The direction in which the rectangle is added (default is clockwise).
     */
    void addRoundRect(RectangleF rect, float rx, float ry, Direction dir = Direction::CW);

    /**
     * @brief Adds a rounded rectangle to the path with uniform corner rounding.
     * @param rect The rectangle that defines the bounds of the rounded rectangle.
     * @param roundness The uniform rounding radius for all corners.
     * @param dir The direction in which the rectangle is added (default is clockwise).
     */
    void addRoundRect(RectangleF rect, float roundness, Direction dir = Direction::CW);

    /**
     * @brief Adds a rectangle to the path.
     * @param rect The rectangle to add.
     * @param dir The direction in which the rectangle is added (default is clockwise).
     */
    void addRect(RectangleF rect, Direction dir = Direction::CW);

    /**
     * @brief Adds a polystar shape to the path.
     * @param points The number of points in the star.
     * @param innerRadius The inner radius of the star.
     * @param outerRadius The outer radius of the star.
     * @param innerRoundness The roundness of the inner points.
     * @param outerRoundness The roundness of the outer points.
     * @param startAngle The starting angle for the star.
     * @param cx The x-coordinate of the center of the star.
     * @param cy The y-coordinate of the center of the star.
     * @param dir The direction in which the polystar is added (default is clockwise).
     */
    void addPolystar(float points, float innerRadius, float outerRadius, float innerRoundness,
                     float outerRoundness, float startAngle, float cx, float cy,
                     Direction dir = Direction::CW);

    /**
     * @brief Adds a polygon to the path.
     * @param points The number of points in the polygon.
     * @param radius The radius of the polygon.
     * @param roundness The roundness of the corners.
     * @param startAngle The starting angle for the polygon.
     * @param cx The x-coordinate of the center of the polygon.
     * @param cy The y-coordinate of the center of the polygon.
     * @param dir The direction in which the polygon is added (default is clockwise).
     */
    void addPolygon(float points, float radius, float roundness, float startAngle, float cx, float cy,
                    Direction dir = Direction::CW);

    /**
     * @brief Adds another path to this path.
     * @param path The path to add.
     */
    void addPath(const Path& path);

    /**
     * @brief Adds another path to this path with a transformation matrix.
     * @param path The path to add.
     * @param m The transformation matrix to apply.
     */
    void addPath(const Path& path, const Matrix& m);

    /**
     * @brief Transforms the path using a transformation matrix.
     * @param m The transformation matrix to apply to the path.
     */
    void transform(const Matrix& m);

    /**
     * @brief Returns a new path that is a transformed version of this path.
     * @param m The transformation matrix to apply.
     * @return Path The transformed path.
     */
    Path transformed(const Matrix& m) const&;

    Path transformed(const Matrix& m) &&;

    /**
     * @brief Calculates the length of the path.
     * @return float The length of the path.
     */
    float length() const;

    /**
     * @brief Creates a dashed version of the path based on a pattern.
     * @param pattern A span of floats defining the dash pattern.
     * @param offset The starting offset into the pattern.
     * @return Path The dashed path.
     */
    Path dashed(std::span<const float> pattern, float offset) const;

    /**
     * @brief Creates a copy of this path.
     * @return Path A clone of the current path.
     */
    Path clone() const;

    /**
     * @brief Calculates an approximate bounding box of the path.
     * @return RectangleF The approximate bounding box.
     */
    RectangleF boundingBoxApprox() const;

    std::optional<RectangleF> asRectangle() const;
    std::optional<std::array<PointF, 2>> asLine() const;

    /// Rasterizes the path for filling.
    /// @param fill Fill parameters.
    /// @param clipRect Clipping rectangle. Pass noClipRect to disable clipping.
    RasterizedPath rasterize(const FillParams& fill, Rectangle clipRect = noClipRect) {
        return Internal::rasterizePath(*this, fill, clipRect);
    }

    /// Rasterizes the path for stroking.
    /// @param stroke Stroke parameters.
    /// @param clipRect Clipping rectangle. Pass noClipRect to disable clipping.
    RasterizedPath rasterize(const StrokeParams& stroke, Rectangle clipRect = noClipRect) {
        return Internal::rasterizePath(*this, stroke, clipRect);
    }

private:
    [[maybe_unused]] void* impl; ///< Pointer to implementation details.
};

} // namespace Brisk
