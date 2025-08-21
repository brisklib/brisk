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

#include <unordered_map>
#include "Matrix.hpp"
#include <brisk/core/Rc.hpp>
#include <mutex>
#include <variant>
#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/core/Time.hpp>
#include "internal/Sprites.hpp"

namespace Brisk {

/**
 * @brief Represents a rasterized path with a sprite and bounding rectangle.
 */
struct RasterizedPath {
    Rc<SpriteResource> sprite; ///< The sprite resource associated with the rasterized path.
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
using DashArray = SmallVector<float, 2>;

/**
 * @brief Structure representing stroke parameters.
 */
struct StrokeParams {
    JoinStyle joinStyle = JoinStyle::Miter; ///< The join style of the stroke.
    CapStyle capStyle   = CapStyle::Flat;   ///< The cap style of the stroke.
    float strokeWidth   = 1.f;              ///< The width of the stroke.
    float miterLimit    = 10.f;             ///< The limit for miter joins.
    float dashOffset    = 0.f;
    DashArray dashArray{};

    StrokeParams scale(float value) const noexcept {
        StrokeParams copy = *this;
        copy.strokeWidth *= value;
        copy.miterLimit *= value;
        copy.dashOffset *= value;
        for (float& v : copy.dashArray)
            v *= value;
        return copy;
    }

    bool operator==(const StrokeParams& other) const noexcept = default;
};

/**
 * @brief Structure representing fill parameters.
 */
struct FillParams {
    FillRule fillRule = FillRule::Winding; ///< The fill rule to be used.

    bool operator==(const FillParams& other) const noexcept = default;
};

/**
 * @brief A type alias for fill or stroke parameters, using std::variant.
 */
using FillOrStrokeParams = std::variant<FillParams, StrokeParams>;

struct Path;

namespace Internal {
extern PerformanceDuration performancePathScanline;
extern PerformanceDuration performancePathRasterization;

union alignas(8) PatchData {
    uint64_t data_u64[2];
    uint32_t data_u32[4];
    uint16_t data_u16[8];
    uint8_t data_u8[16];

    static const PatchData filled;

    static PatchData fromBits(uint16_t bits) {
        PatchData result;
        for (size_t i = 0; i < 16; ++i) {
            result.data_u8[i] = bits & (1 << (15 - i)) ? 255 : 0;
        }
        return result;
    }

    bool operator==(const PatchData& other) const noexcept {
        return data_u64[0] == other.data_u64[0] && data_u64[1] == other.data_u64[1];
    }

    bool operator<(const PatchData& other) const noexcept {
        return data_u64[0] < other.data_u64[0] ||
               (data_u64[0] == other.data_u64[0] && data_u64[1] < other.data_u64[1]);
    }

    bool empty() const noexcept {
        return data_u64[0] == 0 && data_u64[1] == 0;
    }
};

inline const PatchData PatchData::filled = { .data_u64 = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF } };

struct Patch {
    uint16_t x, y;   // screen-aligned
    uint32_t offset; // index in patchData

    bool operator==(const Patch& other) const noexcept {
        return x == other.x && y == other.y;
    }

    bool operator<(const Patch& other) const noexcept {
        uint32_t xy       = uint32_t(x) | (uint32_t(y) << 16);
        uint32_t other_xy = uint32_t(other.x) | (uint32_t(other.y) << 16);
        return xy < other_xy;
    }
};

struct PatchDataHash {
    size_t operator()(const PatchData& data) const noexcept {
        if constexpr (sizeof(size_t) == 8) {
            return std::hash<uint64_t>()(data.data_u64[0]) ^ std::hash<uint64_t>()(data.data_u64[1]);
        } else {
            return std::hash<uint32_t>()(data.data_u32[0]) ^ std::hash<uint32_t>()(data.data_u32[1]) ^
                   std::hash<uint32_t>()(data.data_u32[2]) ^ std::hash<uint32_t>()(data.data_u32[3]);
        }
    }
};

static_assert(sizeof(Patch) == 8, "Patch size must be 8 bytes");
} // namespace Internal

struct Path;

class Rle;

struct PreparedPath {
public:
    PreparedPath()                    = default;
    PreparedPath(const PreparedPath&) = default;
    PreparedPath(PreparedPath&&)      = default;
    PreparedPath(const Path& path, const FillParams& params = {}, Rectangle clipRect = noClipRect);
    PreparedPath(const Path& path, const StrokeParams& params, Rectangle clipRect = noClipRect);
    PreparedPath(Rectangle rectangle);

    static PreparedPath union_(const PreparedPath& a, const PreparedPath& b);
    static PreparedPath intersection(const PreparedPath& a, const PreparedPath& b);
    static PreparedPath difference(const PreparedPath& a, const PreparedPath& b);
    static PreparedPath symmetricDifference(const PreparedPath& a, const PreparedPath& b);

    static PreparedPath booleanOp(MaskOp op, const PreparedPath& a, const PreparedPath& b);

    bool empty() const noexcept {
        return m_patches.empty();
    }

protected:
    const std::vector<Internal::Patch>& patches() const noexcept {
        return m_patches;
    }

    const std::vector<Internal::PatchData>& patchData() const noexcept {
        return m_patchData;
    }

    Rectangle patchBounds() const;

private:
    std::vector<Internal::Patch> m_patches;
    std::vector<Internal::PatchData> m_patchData;
    mutable std::optional<Rectangle> m_patchBounds;

    friend class Canvas;
    void init(Rle&& rle);
    static PreparedPath merge(const PreparedPath& a, const PreparedPath& b);
};

class Dasher;

/**
 * @brief Represents a geometric path that can be rasterized for rendering.
 */
struct Path {
    Path()                       = default; ///< Default constructor.
    ~Path()                      = default; ///< Destructor.
    Path(Path&&)                 = default; ///< Move constructor.
    Path(const Path&)            = default; ///< Copy constructor.
    Path& operator=(Path&&)      = default; ///< Move constructor.
    Path& operator=(const Path&) = default; ///< Copy constructor.

    friend class Dasher;

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
    void moveTo(float x, float y) {
        moveTo({ x, y });
    }

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
    void lineTo(float x, float y) {
        lineTo({ x, y });
    }

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
    void quadraticTo(float c1x, float c1y, float ex, float ey) {
        quadraticTo({ c1x, c1y }, { ex, ey });
    }

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
    void cubicTo(float c1x, float c1y, float c2x, float c2y, float ex, float ey) {
        cubicTo({ c1x, c1y }, { c2x, c2y }, { ex, ey });
    }

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
    void addRoundRect(RectangleF rect, float rx, float ry, bool squircle = false,
                      Direction dir = Direction::CW);

    void addRoundRect(RectangleF rect, CornersF r, bool squircle = false, Direction dir = Direction::CW) {
        addRoundRect(rect, r, r, squircle, dir);
    }

    void addRoundRect(RectangleF rect, CornersF rx, CornersF ry, bool squircle = false,
                      Direction dir = Direction::CW);

    /**
     * @brief Adds a rounded rectangle to the path with uniform corner rounding.
     * @param rect The rectangle that defines the bounds of the rounded rectangle.
     * @param roundness The uniform rounding radius for all corners.
     * @param dir The direction in which the rectangle is added (default is clockwise).
     */
    void addRoundRect(RectangleF rect, float roundness, bool squircle = false,
                      Direction dir = Direction::CW) {
        addRoundRect(rect, roundness, roundness, squircle, dir);
    }

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

    void addPolyline(std::span<const PointF> points);

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
     * @brief Calculates an approximate bounding box of the path.
     * @return RectangleF The approximate bounding box.
     */
    RectangleF boundingBoxApprox() const;

    const std::vector<Path::Element>& elements() const {
        return m_elements;
    }

    const std::vector<PointF>& points() const {
        return m_points;
    }

    size_t segments() const {
        return m_segments;
    }

private:
    std::vector<PointF> m_points;
    std::vector<Element> m_elements;
    size_t m_segments{ 0 };
    PointF mStartPoint{ 0.f, 0.f };
    mutable float mLength{ 0 };
    mutable bool mLengthDirty{ true };
    bool mNewSegment{ false };

    void checkNewSegment();
    void reserve(size_t pts, size_t elms);
};

} // namespace Brisk
