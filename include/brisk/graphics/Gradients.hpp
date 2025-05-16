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

#include <brisk/core/Utilities.hpp>
#include <brisk/core/Rc.hpp>
#include <brisk/graphics/Color.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/internal/Function.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/core/internal/Generation.hpp>
#include <brisk/core/internal/FunctionRef.hpp>

namespace Brisk {

/**
 * @brief Represents a color stop in a gradient.
 */
struct ColorStop {
    float position; ///< The position of the color stop within the gradient, ranging from 0.0 to 1.0.
    ColorW color;   ///< The color associated with this color stop.
};

/**
 * @brief Enumeration for different types of gradients.
 */
enum class GradientType : int {
    Linear,    ///< A linear gradient.
    Radial,    ///< A radial gradient.
    Angle,     ///< An angular (conic) gradient.
    Reflected, ///< A reflected gradient.
};

/**
 * @brief A small vector type for storing an array of color stops.
 */
using ColorStopArray                       = SmallVector<ColorStop, 2>;

/**
 * @brief The resolution for the gradient, used in shader calculations.
 *
 * @note Must match the value in Shader
 */
constexpr inline size_t gradientResolution = 1024;

struct Gradient;

/**
 * @brief Struct for storing gradient data.
 */
struct GradientData {
    std::array<ColorF, gradientResolution> data;

    GradientData() noexcept                               = default; ///< Default constructor.
    GradientData(const GradientData&) noexcept            = default; ///< Copy constructor.
    GradientData(GradientData&&) noexcept                 = default; ///< Move constructor.
    GradientData& operator=(const GradientData&) noexcept = default; ///< Copy assignment operator.
    GradientData& operator=(GradientData&&) noexcept      = default; ///< Move assignment operator.

    /**
     * @brief Constructs GradientData from a Gradient object.
     * @param gradient The gradient from which to construct the data.
     */
    explicit GradientData(const Gradient& gradient);

    /**
     * @brief Constructs GradientData from a function mapping float to ColorW.
     * @param func The function to map positions to colors.
     */
    explicit GradientData(function_ref<ColorW(float)> func);

    /**
     * @brief Constructs GradientData from a vector of colors and a gamma correction factor.
     * @param list A vector of colors to use in the gradient.
     * @param gamma The gamma correction factor to apply.
     */
    explicit GradientData(const std::vector<ColorW>& list, float gamma);

    /**
     * @brief Gets the color at a specified position.
     * @param x The position (between 0.0 and 1.0) to query.
     * @return The color at the specified position in the gradient with premultiplied alpha.
     */
    ColorF operator()(float x) const;
};

/**
 * @brief Represents a resource associated with a gradient.
 */
struct GradientResource {
    uint64_t id;       ///< Unique identifier for the gradient resource.
    GradientData data; ///< The gradient data.
};

/**
 * @brief Creates a new gradient resource.
 * @param data The gradient data to associate with the resource.
 * @return A reference-counted pointer to the newly created GradientResource.
 */
inline Rc<GradientResource> makeGradient(const GradientData& data) {
    return rcnew GradientResource{ autoincremented<GradientResource, uint64_t>(), std::move(data) };
}

/**
 * @brief Represents a gradient for rendering.
 */
struct Gradient {
public:
    /**
     * @brief Constructs a gradient of a specified type.
     * @param type The type of the gradient.
     */
    explicit Gradient(GradientType type);

    /**
     * @brief Constructs a gradient of a specified type between two points.
     * @param type The type of the gradient.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     */
    explicit Gradient(GradientType type, PointF startPoint, PointF endPoint);

    /**
     * @brief Constructs a gradient with color stops.
     * @param type The type of the gradient.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     * @param colorStops Array of color stops.
     */
    explicit Gradient(GradientType type, PointF startPoint, PointF endPoint, ColorStopArray colorStops);

    /**
     * @brief Constructs a gradient with start and end colors.
     * @param type The type of the gradient.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     * @param startColor The starting color.
     * @param endColor The ending color.
     */
    explicit Gradient(GradientType type, PointF startPoint, PointF endPoint, ColorW startColor,
                      ColorW endColor);

    /**
     * @brief Gets the type of the gradient.
     * @return The gradient type.
     */
    GradientType getType() const noexcept;

    /**
     * @brief Gets the starting point of the gradient.
     * @return The starting point of the gradient.
     */
    PointF getStartPoint() const;

    /**
     * @brief Sets the starting point of the gradient.
     * @param pt The new starting point.
     */
    void setStartPoint(PointF pt);

    /**
     * @brief Gets the ending point of the gradient.
     * @return The ending point of the gradient.
     */
    PointF getEndPoint() const;

    /**
     * @brief Sets the ending point of the gradient.
     * @param pt The new ending point.
     */
    void setEndPoint(PointF pt);

    /**
     * @brief Adds a color stop to the gradient.
     * @param position The position of the color stop (between 0.0 and 1.0).
     * @param color The color of the stop.
     */
    void addStop(float position, ColorW color);

    /**
     * @brief Adds a color stop to the gradient.
     * @param colorStop The color stop to add.
     */
    void addStop(ColorStop colorStop);

    /**
     * @brief Gets the array of color stops defined in the gradient.
     * @return A reference to the array of color stops.
     */
    const ColorStopArray& colorStops() const;

    /**
     * @brief Rasterizes the gradient into a GradientResource.
     * @return A reference-counted pointer to the rasterized gradient resource.
     */
    Rc<GradientResource> rasterize() const {
        return makeGradient(GradientData{ *this });
    }

private:
    GradientType m_type;         ///< The type of the gradient.
    PointF m_startPoint;         ///< The starting point of the gradient.
    PointF m_endPoint;           ///< The ending point of the gradient.
    ColorStopArray m_colorStops; ///< The color stops for the gradient.
};

struct LinearGradient : public Gradient {
    /**
     * @brief Constructs an empty linear gradient.
     */
    explicit LinearGradient();

    /**
     * @brief Constructs a linear gradient between two points.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     */
    explicit LinearGradient(PointF startPoint, PointF endPoint);

    /**
     * @brief Constructs a linear gradient with color stops.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     * @param colorStops Array of color stops.
     */
    explicit LinearGradient(PointF startPoint, PointF endPoint, ColorStopArray colorStops);

    /**
     * @brief Constructs a linear gradient with start and end colors.
     * @param startPoint The starting point of the gradient.
     * @param endPoint The ending point of the gradient.
     * @param startColor The starting color.
     * @param endColor The ending color.
     */
    explicit LinearGradient(PointF startPoint, PointF endPoint, ColorW startColor, ColorW endColor);
};

struct RadialGradient : public Gradient {
    /**
     * @brief Constructs an empty radial gradient.
     */
    explicit RadialGradient();

    /**
     * @brief Constructs a radial gradient with a center point and radius.
     * @param point The center point of the gradient.
     * @param radius The radius of the gradient.
     */
    explicit RadialGradient(PointF point, float radius);

    /**
     * @brief Constructs a radial gradient with color stops.
     * @param point The center point of the gradient.
     * @param radius The radius of the gradient.
     * @param colorStops Array of color stops.
     */
    explicit RadialGradient(PointF point, float radius, ColorStopArray colorStops);

    /**
     * @brief Constructs a radial gradient with start and end colors.
     * @param point The center point of the gradient.
     * @param radius The radius of the gradient.
     * @param startColor The starting color.
     * @param endColor The ending color.
     */
    explicit RadialGradient(PointF point, float radius, ColorW startColor, ColorW endColor);
};

} // namespace Brisk
