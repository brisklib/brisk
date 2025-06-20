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

#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Simd.hpp>

namespace Brisk {

namespace Internal {
template <typename T>
struct FloatTypeFor {
    using Type = float;
};

template <>
struct FloatTypeFor<double> {
    using Type = double;
};
} // namespace Internal

template <typename T>
struct PointOf;
template <typename T>
struct SizeOf;
template <typename T>
struct EdgesOf;
template <typename T>
struct CornersOf;
template <typename T>
struct RectangleOf;

/**
 * @brief A template struct representing a point in polar coordinates.
 *
 * This struct holds the radius and angle of a point in a polar coordinate system.
 * The `radius` represents the distance from the origin, while the `angle` represents
 * the direction of the point in radians.
 *
 * @tparam T The type of the radius and angle, typically a numeric type such as float or double.
 */
template <typename T>
struct PolarOf {
    /**
     * @brief The radius of the polar coordinate.
     *
     * This value represents the distance from the origin (0, 0).
     */
    T radius;

    /**
     * @brief The angle of the polar coordinate.
     *
     * This value represents the angle in radians.
     */
    T angle;

    /**
     * @brief Equality operator for comparing two PolarOf objects.
     */
    constexpr bool operator==(const PolarOf& c) const noexcept = default;

    /**
     * @brief Inequality operator for comparing two PolarOf objects.
     */
    constexpr bool operator!=(const PolarOf& c) const noexcept = default;

    constexpr static std::tuple reflection{
        ReflectionField{ "radius", &PolarOf::radius },
        ReflectionField{ "angle", &PolarOf::angle },
    };
};

using PolarF = PolarOf<float>;

/**
 * @brief A template struct representing a point in 2D Cartesian coordinates.
 *
 * The struct holds the x and y coordinates of a point. It provides various constructors,
 * operator overloads, and utility functions for manipulating points in a Cartesian coordinate system.
 *
 * @tparam T The type of the coordinates, typically a numeric type such as float or double.
 */
template <typename T>
struct PointOf {
    /**
     * @brief Default constructor.
     *
     * Initializes the coordinates to zero.
     */
    constexpr PointOf() noexcept : x(), y() {}

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the coordinates with the given values.
     *
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     */
    constexpr PointOf(T x, T y) noexcept : x(x), y(y) {}

    /**
     * @brief Copy constructor.
     *
     * Initializes a new PointOf instance by copying the given point.
     *
     * @param p The PointOf instance to copy.
     */
    constexpr PointOf(const PointOf& p) noexcept = default;

    constexpr PointOf(const SizeOf<T>& size) noexcept : x(size.x), y(size.y) {}

    /**
     * @brief Equality operator.
     *
     * Compares two PointOf instances for equality based on their coordinates.
     *
     * @param c The PointOf instance to compare against.
     * @return true if the points are equal, false otherwise.
     */
    constexpr bool operator==(const PointOf& c) const noexcept {
        return x == c.x && y == c.y;
    }

    /**
     * @brief Inequality operator.
     *
     * Compares two PointOf instances for inequality.
     *
     * @param c The PointOf instance to compare against.
     * @return true if the points are not equal, false otherwise.
     */
    constexpr bool operator!=(const PointOf& c) const noexcept {
        return !operator==(c);
    }

    /**
     * @brief Flips the point coordinates.
     *
     * Swaps the x and y coordinates of the point.
     *
     * @return A new PointOf instance with flipped coordinates.
     */
    constexpr PointOf flipped() const noexcept {
        return PointOf(y, x);
    }

    /**
     * @brief Conditionally flips the point coordinates.
     *
     * If the flip parameter is true, the coordinates are flipped; otherwise, the original point is returned.
     *
     * @param flip A boolean indicating whether to flip the coordinates.
     * @return A new PointOf instance, either flipped or unchanged.
     */
    constexpr PointOf flippedIf(bool flip) const noexcept {
        return flip ? flipped() : *this;
    }

    /**
     * @brief Access operator for point components.
     *
     * Provides read access to the point's components via index.
     *
     * @param i The index of the component (0 for x, 1 for y).
     * @return The component at the specified index.
     */
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /**
     * @brief Access operator for point components.
     *
     * Provides write access to the point's components via index.
     *
     * @param i The index of the component (0 for x, 1 for y).
     * @return A reference to the component at the specified index.
     */
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    union {
        T components[2]; ///< Array of components for indexed access.

        struct {
            T x; ///< The x-coordinate.
            T y; ///< The y-coordinate.
        };
    };

    /**
     * @brief Reflection data for introspection.
     *
     * This static member provides reflection information about the PointOf struct,
     * including its fields and their corresponding names.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "x", &PointOf::x },
        ReflectionField{ "y", &PointOf::y },
    };
};

/**
 * @brief A specialized template struct for SIMD-compatible points in 2D Cartesian coordinates.
 *
 * This version of PointOf uses SIMD (Single Instruction, Multiple Data) types for efficient
 * vectorized computations.
 *
 * @tparam T The SIMD type representing the point coordinates, must be SIMD compatible.
 */
template <SimdCompatible T>
struct PointOf<T> {
    using Tfloat = typename Internal::FloatTypeFor<T>::Type; ///< The floating-point type corresponding to T.

    /**
     * @brief Default constructor.
     *
     * Initializes the SIMD vector to zero.
     */
    constexpr PointOf() noexcept : v() {}

    /**
     * @brief Parameterized constructor from a SIMD vector.
     *
     * Initializes the point with the given SIMD vector.
     *
     * @param v The SIMD vector representing the point.
     */
    constexpr explicit PointOf(const Simd<T, 2>& v) noexcept : v(v) {}

    /**
     * @brief Parameterized constructor from two coordinates.
     *
     * Initializes the point with the specified x and y values.
     *
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     */
    constexpr PointOf(T x, T y) noexcept : v(x, y) {}

    /**
     * @brief Constructor from a SizeOf object.
     *
     * Initializes the point using the size vector from a SizeOf object.
     *
     * @param sz The SizeOf object from which to initialize the point.
     */
    constexpr PointOf(const SizeOf<T>& sz) noexcept : v(sz.v) {}

    /**
     * @brief Copy constructor.
     *
     * Initializes a new PointOf instance by copying the given point.
     *
     * @param p The PointOf instance to copy.
     */
    constexpr PointOf(const PointOf& p) noexcept = default;

    /**
     * @brief Conversion operator to a PointOf<U>.
     *
     * Converts this PointOf instance to another type PointOf<U>.
     *
     * @tparam U The target type for conversion.
     * @return A PointOf<U> instance.
     */
    template <typename U>
    operator PointOf<U>() const noexcept {
        if constexpr (SimdCompatible<U>) {
            return PointOf<U>(rescale<U, 1, 1>(v));
        } else if constexpr (std::convertible_to<T, U>) {
            return PointOf<U>{ static_cast<U>(x), static_cast<U>(y) };
        } else {
            static_assert(sizeof(U) == 0, "Cannot convert");
        }
    }

    /**
     * @brief Equality operator.
     *
     * Compares two PointOf instances for equality based on their SIMD vector.
     *
     * @param c The PointOf instance to compare against.
     * @return true if the points are equal, false otherwise.
     */
    constexpr bool operator==(const PointOf& c) const noexcept {
        return v == c.v;
    }

    /**
     * @brief Inequality operator.
     *
     * Compares two PointOf instances for inequality.
     *
     * @param c The PointOf instance to compare against.
     * @return true if the points are not equal, false otherwise.
     */
    constexpr bool operator!=(const PointOf& c) const noexcept {
        return !operator==(c);
    }

    /**
     * @brief Constructor from a PolarOf<T>.
     *
     * Converts a polar coordinate to Cartesian coordinates.
     *
     * @param p The PolarOf instance to convert.
     */
    constexpr PointOf(const PolarOf<T>& p) noexcept {
        static_assert(std::is_floating_point_v<T>);
        x = p.radius * std::cos(p.angle);
        y = p.radius * std::sin(p.angle);
    }

    /**
     * @brief Conversion operator to PolarOf<T>.
     *
     * Converts this Cartesian point to a polar coordinate representation.
     *
     * @return A PolarOf<T> instance representing the polar coordinates.
     */
    constexpr operator PolarOf<T>() noexcept {
        static_assert(std::is_floating_point_v<T>);
        return {
            std::sqrt(x * x + y * y),
            std::atan2(y, x),
        };
    }

    /**
     * @brief Addition operator.
     *
     * Adds two PointOf instances and returns a new PointOf instance.
     *
     * @param p1 The first PointOf instance.
     * @param p2 The second PointOf instance.
     * @return A new PointOf instance representing the sum.
     */
    constexpr friend PointOf operator+(const PointOf& p1, const PointOf& p2) noexcept {
        return PointOf(p1.v + p2.v);
    }

    /**
     * @brief Subtraction operator.
     *
     * Subtracts one PointOf instance from another and returns a new PointOf instance.
     *
     * @param p1 The first PointOf instance.
     * @param p2 The second PointOf instance.
     * @return A new PointOf instance representing the difference.
     */
    constexpr friend PointOf operator-(const PointOf& p1, const PointOf& p2) noexcept {
        return PointOf(p1.v - p2.v);
    }

    /**
     * @brief Multiplication operator.
     *
     * Multiplies two PointOf instances and returns a new PointOf instance.
     *
     * @param p1 The first PointOf instance.
     * @param p2 The second PointOf instance.
     * @return A new PointOf instance representing the product.
     */
    constexpr friend PointOf operator*(const PointOf& p1, const PointOf& p2) noexcept {
        return PointOf(p1.v * p2.v);
    }

    /**
     * @brief Division operator.
     *
     * Divides one PointOf instance by another and returns a new PointOf instance.
     *
     * @param p1 The first PointOf instance.
     * @param p2 The second PointOf instance.
     * @return A new PointOf instance representing the quotient.
     */
    constexpr friend PointOf operator/(const PointOf& p1, const PointOf& p2) noexcept {
        return PointOf(p1.v / p2.v);
    }

    /**
     * @brief Negation operator.
     *
     * Negates the PointOf instance and returns a new PointOf instance.
     *
     * @param p1 The PointOf instance to negate.
     * @return A new PointOf instance representing the negated point.
     */
    constexpr friend PointOf operator-(const PointOf& p1) noexcept {
        return PointOf(-p1.v);
    }

    /**
     * @brief Scalar multiplication operator.
     *
     * Multiplies a PointOf instance by a scalar and returns a new PointOf instance.
     *
     * @param p1 The PointOf instance.
     * @param v2 The scalar value.
     * @return A new PointOf instance representing the product.
     */
    constexpr friend PointOf operator*(const PointOf& p1, const T& v2) noexcept {
        return PointOf(p1.v * v2);
    }

    /**
     * @brief Scalar multiplication operator.
     *
     * Multiplies a scalar by a PointOf instance and returns a new PointOf instance.
     *
     * @param v1 The scalar value.
     * @param p2 The PointOf instance.
     * @return A new PointOf instance representing the product.
     */
    constexpr friend PointOf operator*(const T& v1, const PointOf& p2) noexcept {
        return PointOf(v1 * p2.v);
    }

    /**
     * @brief Scalar division operator.
     *
     * Divides a PointOf instance by a scalar and returns a new PointOf instance.
     *
     * @param p1 The PointOf instance.
     * @param v2 The scalar value.
     * @return A new PointOf instance representing the quotient.
     */
    constexpr friend PointOf operator/(const PointOf& p1, const T& v2) noexcept {
        return PointOf(p1.v / v2);
    }

    /**
     * @brief Scalar division operator.
     *
     * Divides a scalar by a PointOf instance and returns a new PointOf instance.
     *
     * @param v1 The scalar value.
     * @param p2 The PointOf instance.
     * @return A new PointOf instance representing the quotient.
     */
    constexpr friend PointOf operator/(const T& v1, const PointOf& p2) noexcept {
        return PointOf(v1 / p2.v);
    }

    /**
     * @brief Compound addition assignment operator.
     *
     * Adds another PointOf instance to this one and returns the updated instance.
     *
     * @param p2 The PointOf instance to add.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator+=(const PointOf& p2) noexcept {
        return v += p2.v, *this;
    }

    /**
     * @brief Compound subtraction assignment operator.
     *
     * Subtracts another PointOf instance from this one and returns the updated instance.
     *
     * @param p2 The PointOf instance to subtract.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator-=(const PointOf& p2) noexcept {
        return v -= p2.v, *this;
    }

    /**
     * @brief Compound multiplication assignment operator.
     *
     * Multiplies this PointOf instance by another and returns the updated instance.
     *
     * @param p2 The PointOf instance to multiply by.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator*=(const PointOf& p2) noexcept {
        return v *= p2.v, *this;
    }

    /**
     * @brief Compound division assignment operator.
     *
     * Divides this PointOf instance by another and returns the updated instance.
     *
     * @param p2 The PointOf instance to divide by.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator/=(const PointOf& p2) noexcept {
        return v /= p2.v, *this;
    }

    /**
     * @brief Compound addition assignment operator for scalar.
     *
     * Adds a scalar value to this PointOf instance and returns the updated instance.
     *
     * @param v2 The scalar value to add.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator+=(T v2) noexcept {
        return v += v2, *this;
    }

    /**
     * @brief Compound subtraction assignment operator for scalar.
     *
     * Subtracts a scalar value from this PointOf instance and returns the updated instance.
     *
     * @param v2 The scalar value to subtract.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator-=(T v2) noexcept {
        return v -= v2, *this;
    }

    /**
     * @brief Compound multiplication assignment operator for scalar.
     *
     * Multiplies this PointOf instance by a scalar value and returns the updated instance.
     *
     * @param v2 The scalar value to multiply by.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator*=(T v2) noexcept {
        return v *= v2, *this;
    }

    /**
     * @brief Compound division assignment operator for scalar.
     *
     * Divides this PointOf instance by a scalar value and returns the updated instance.
     *
     * @param v2 The scalar value to divide by.
     * @return A reference to the updated PointOf instance.
     */
    constexpr PointOf& operator/=(T v2) noexcept {
        return v /= v2, *this;
    }

    /**
     * @brief Flips the point coordinates.
     *
     * Swaps the x and y coordinates of the point.
     *
     * @return A new PointOf instance with flipped coordinates.
     */
    constexpr PointOf flipped() const noexcept {
        return PointOf(swapAdjacent(v));
    }

    /**
     * @brief Conditionally flips the point coordinates.
     *
     * If the flip parameter is true, the coordinates are flipped; otherwise, the original point is returned.
     *
     * @param flip A boolean indicating whether to flip the coordinates.
     * @return A new PointOf instance, either flipped or unchanged.
     */
    constexpr PointOf flippedIf(bool flip) const noexcept {
        return flip ? flipped() : *this;
    }

    /**
     * @brief Calculates the Euclidean distance to another point.
     *
     * Computes the distance from this point to the specified point.
     *
     * @param pt The PointOf instance to calculate the distance to.
     * @return The Euclidean distance to the other point.
     */
    T distance(const PointOf& pt) const noexcept {
        return std::sqrt(horizontalSum(sqr(pt.v - v)));
    }

    /**
     * @brief Calculates the Manhattan distance to another point.
     *
     * Computes the Manhattan distance from this point to the specified point.
     *
     * @param pt The PointOf instance to calculate the distance to.
     * @return The Manhattan distance to the other point.
     */
    T distanceManhattan(const PointOf& pt) const noexcept {
        return horizontalMax(abs(pt.v - v));
    }

    /**
     * @brief Access operator for point components.
     *
     * Provides read access to the point's components via index.
     *
     * @param i The index of the component (0 for x, 1 for y).
     * @return The component at the specified index.
     */
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /**
     * @brief Access operator for point components.
     *
     * Provides write access to the point's components via index.
     *
     * @param i The index of the component (0 for x, 1 for y).
     * @return A reference to the component at the specified index.
     */
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /**
     * @brief Creates an aligned rectangle around the point.
     *
     * Generates a rectangle centered around the point with specified dimensions and alignment.
     *
     * @param innerSize The size of the rectangle.
     * @param alignment The alignment of the rectangle.
     * @return A RectangleOf instance representing the aligned rectangle.
     */
    RectangleOf<T> alignedRect(const SizeOf<T>& innerSize, const PointOf<Tfloat>& alignment) const noexcept {
        const SizeOf<T> sz  = innerSize;
        const SizeOf<T> gap = -sz;
        const Simd<T, 2> p  = v + Simd<T, 2>(Simd<Tfloat, 2>(gap.v) * alignment.v);
        return RectangleOf<T>(concat(p, p + sz.v));
    }

    /**
     * @brief Creates an aligned rectangle around the point with specified dimensions.
     *
     * Generates a rectangle centered around the point with specified width, height, and alignment.
     *
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     * @param alignX The alignment factor in the X direction.
     * @param alignY The alignment factor in the Y direction.
     * @return A RectangleOf instance representing the aligned rectangle.
     */
    RectangleOf<T> alignedRect(T width, T height, Tfloat alignX, Tfloat alignY) const noexcept {
        return alignedRect({ width, height }, { alignX, alignY });
    }

    /**
     * @brief Rounds the point components.
     *
     * Applies rounding to the x and y components of the point.
     *
     * @return A new PointOf instance with rounded components.
     */
    constexpr PointOf round() const noexcept {
        return PointOf(Brisk::round(v));
    }

    /**
     * @brief Applies floor to the point components.
     *
     * Computes the largest integer values less than or equal to the x and y components of the point.
     *
     * @return A new PointOf instance with floored components.
     */
    constexpr PointOf floor() const noexcept {
        return PointOf(Brisk::floor(v));
    }

    /**
     * @brief Applies ceil to the point components.
     *
     * Computes the smallest integer values greater than or equal to the x and y components of the point.
     *
     * @return A new PointOf instance with ceiled components.
     */
    constexpr PointOf ceil() const noexcept {
        return PointOf(Brisk::ceil(v));
    }

    /**
     * @brief Applies truncation to the point components.
     *
     * Computes the integer part of the x and y components of the point.
     *
     * @return A new PointOf instance with truncated components.
     */
    constexpr PointOf trunc() const noexcept {
        return PointOf(Brisk::trunc(v));
    }

    /**
     * @brief Type representing the SIMD vector type.
     */
    using vec_type = Simd<T, 2>;

    union {
        vec_type v;      ///< SIMD vector representation of the point.
        T components[2]; ///< Array of components for direct access.

        struct {
            T x; ///< X component of the point.
            T y; ///< Y component of the point.
        };
    };

    /**
     * @brief Reflection metadata for the PointOf struct.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "x", &PointOf::x },
        ReflectionField{ "y", &PointOf::y },
    };
};

/**
 * @brief Computes the minimum point from two PointOf instances.
 *
 * @tparam T The type of the point components.
 * @param a The first PointOf instance.
 * @param b The second PointOf instance.
 * @return A new PointOf instance representing the minimum point.
 */
template <typename T>
BRISK_INLINE PointOf<T> min(const PointOf<T>& a, const PointOf<T>& b) noexcept {
    return PointOf<T>(min(a.v, b.v));
}

/**
 * @brief Computes the maximum point from two PointOf instances.
 *
 * @tparam T The type of the point components.
 * @param a The first PointOf instance.
 * @param b The second PointOf instance.
 * @return A new PointOf instance representing the maximum point.
 */
template <typename T>
BRISK_INLINE PointOf<T> max(const PointOf<T>& a, const PointOf<T>& b) noexcept {
    return PointOf<T>(max(a.v, b.v));
}

/**
 * @brief A structure representing a 2D size with width and height.
 *
 * This structure allows operations on sizes, such as addition, subtraction,
 * and comparison. It can also represent the size as a union of its components
 * for easy access.
 *
 * @tparam T The type of the components (e.g., float, int).
 */
template <typename T>
struct SizeOf {
    /// @brief Default constructor that initializes size to (0, 0).
    constexpr SizeOf() noexcept : x{}, y{} {}

    /// @brief Constructor that initializes size with specific values.
    /// @param x The width component.
    /// @param y The height component.
    constexpr SizeOf(T x, T y) noexcept : x(x), y(y) {}

    /// @brief Constructor that initializes width and height to the same value.
    /// @param xy The value for both width and height.
    constexpr SizeOf(T xy) noexcept : x(xy), y(xy) {}

    /// @brief Constructor that converts from a type convertible to T.
    /// @tparam U The type to convert from.
    /// @param v The value to convert and initialize.
    template <std::convertible_to<T> U>
    constexpr SizeOf(U v) noexcept : SizeOf(static_cast<T>(v)) {}

    /// @brief Copy constructor.
    /// @param s The SizeOf instance to copy from.
    constexpr SizeOf(const SizeOf& s) noexcept = default;

    constexpr SizeOf(const PointOf<T>& pt) noexcept : x(pt.x), y(pt.y) {}

    /// @brief Access operator for retrieving component by index.
    /// @param i The index of the component (0 for x, 1 for y).
    /// @return The component value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying component by index.
    /// @param i The index of the component (0 for x, 1 for y).
    /// @return A reference to the component value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Returns a new SizeOf instance with x and y flipped.
    /// @return A SizeOf instance with flipped dimensions.
    constexpr SizeOf flipped() const noexcept {
        return SizeOf(y, x);
    }

    /// @brief Conditionally flips the dimensions based on the provided flag.
    /// @param flip If true, dimensions are flipped; otherwise, unchanged.
    /// @return A SizeOf instance that is either flipped or unchanged.
    constexpr SizeOf flippedIf(bool flip) const noexcept {
        return flip ? flipped() : *this;
    }

    /// @brief Checks if two SizeOf instances are equal.
    /// @param c The SizeOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const SizeOf& c) const noexcept {
        return x == c.x && y == c.y;
    }

    /// @brief Checks if two SizeOf instances are not equal.
    /// @param c The SizeOf instance to compare against.
    /// @return True if both instances are not equal; otherwise false.
    constexpr bool operator!=(const SizeOf& c) const noexcept {
        return !operator==(c);
    }

    union {
        /// @brief An array to access the components directly.
        T components[2];

        /// @brief Struct for accessing components as x and y.
        struct {
            T x; ///< The width component.
            T y; ///< The height component.
        };

        /// @brief Struct for accessing components as width and height.
        struct {
            T width;  ///< The width component.
            T height; ///< The height component.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x", &SizeOf::x },
        ReflectionField{ "y", &SizeOf::y },
    };
};

/**
 * @brief A specialized SizeOf structure for SIMD-compatible types.
 *
 * This specialization leverages SIMD operations for better performance
 * on two-dimensional sizes. It allows for various arithmetic and utility
 * operations using SIMD vectors.
 *
 * @tparam T The SIMD-compatible type of the components.
 */
template <SimdCompatible T>
struct SizeOf<T> {
    /// @brief Default constructor that initializes size to (0, 0).
    constexpr SizeOf() noexcept : x{}, y{} {}

    /// @brief Constructor that initializes size with specific values.
    /// @param x The width component.
    /// @param y The height component.
    constexpr SizeOf(T x, T y) noexcept : x(x), y(y) {}

    /// @brief Constructor that initializes width and height to the same value.
    /// @param xy The value for both width and height.
    constexpr explicit SizeOf(T xy) noexcept : x(xy), y(xy) {}

    /// @brief Constructor that initializes from a SIMD vector.
    /// @param v The SIMD vector to initialize from.
    SizeOf(const Simd<T, 2>& v) noexcept : v(v) {}

    /// @brief Copy constructor.
    /// @param s The SizeOf instance to copy from.
    constexpr SizeOf(const SizeOf& s) noexcept = default;

    constexpr SizeOf(const PointOf<T>& pt) noexcept : v(pt.v) {}

    /// @brief Conversion operator to a different SizeOf type.
    /// @tparam U The type to convert to.
    /// @return A new SizeOf instance of type U.
    template <typename U>
    operator SizeOf<U>() const noexcept {
        if constexpr (SimdCompatible<U>) {
            return SizeOf<U>(rescale<U, 1, 1>(v));
        } else if constexpr (std::convertible_to<T, U>) {
            return SizeOf<U>{ static_cast<U>(x), static_cast<U>(y) };
        } else {
            static_assert(sizeof(U) == 0, "Cannot convert");
        }
    }

    /// @brief Checks if the size is empty (both components <= 0).
    /// @return True if the size is empty; otherwise false.
    constexpr bool empty() const noexcept {
        return x <= 0 && y <= 0;
    }

    /// @brief Gets the length of the shortest side.
    /// @return The value of the shortest side.
    constexpr T shortestSide() const noexcept {
        return horizontalMin(v);
    }

    /// @brief Gets the length of the longest side.
    /// @return The value of the longest side.
    constexpr T longestSide() const noexcept {
        return horizontalMax(v);
    }

    /// @brief Rounds the dimensions to the nearest integer.
    /// @return A SizeOf instance with rounded dimensions.
    constexpr SizeOf round() const noexcept {
        return SizeOf(Brisk::round(v));
    }

    /// @brief Floors the dimensions to the nearest lower integer.
    /// @return A SizeOf instance with floored dimensions.
    constexpr SizeOf floor() const noexcept {
        return SizeOf(Brisk::floor(v));
    }

    /// @brief Ceils the dimensions to the nearest higher integer.
    /// @return A SizeOf instance with ceiled dimensions.
    constexpr SizeOf ceil() const noexcept {
        return SizeOf(Brisk::ceil(v));
    }

    /// @brief Truncates the dimensions towards zero.
    /// @return A SizeOf instance with truncated dimensions.
    constexpr SizeOf trunc() const noexcept {
        return SizeOf(Brisk::trunc(v));
    }

    /// @brief Returns a new SizeOf instance with dimensions flipped.
    /// @return A SizeOf instance with flipped dimensions.
    constexpr SizeOf flipped() const noexcept {
        return SizeOf(swapAdjacent(v));
    }

    /// @brief Conditionally flips dimensions based on a provided flag.
    /// @param flip If true, dimensions are flipped; otherwise, unchanged.
    /// @return A SizeOf instance that is either flipped or unchanged.
    constexpr SizeOf flippedIf(bool flip) const noexcept {
        return flip ? flipped() : *this;
    }

    /// @brief Addition operator for SizeOf instances.
    /// @param s1 The first SizeOf instance.
    /// @param s2 The second SizeOf instance.
    /// @return A new SizeOf instance that is the sum of s1 and s2.
    constexpr friend SizeOf operator+(const SizeOf& s1, const SizeOf& s2) noexcept {
        return SizeOf(s1.v + s2.v);
    }

    /// @brief Subtraction operator for SizeOf instances.
    /// @param s1 The first SizeOf instance.
    /// @param s2 The second SizeOf instance.
    /// @return A new SizeOf instance that is the difference of s1 and s2.
    constexpr friend SizeOf operator-(const SizeOf& s1, const SizeOf& s2) noexcept {
        return SizeOf(s1.v - s2.v);
    }

    /// @brief Multiplication operator for SizeOf instances.
    /// @param s1 The first SizeOf instance.
    /// @param s2 The second SizeOf instance.
    /// @return A new SizeOf instance that is the product of s1 and s2.
    constexpr friend SizeOf operator*(const SizeOf& s1, const SizeOf& s2) noexcept {
        return SizeOf(s1.v * s2.v);
    }

    /// @brief Division operator for SizeOf instances.
    /// @param s1 The first SizeOf instance.
    /// @param s2 The second SizeOf instance.
    /// @return A new SizeOf instance that is the quotient of s1 and s2.
    constexpr friend SizeOf operator/(const SizeOf& s1, const SizeOf& s2) noexcept {
        return SizeOf(s1.v / s2.v);
    }

    /// @brief Multiplication operator with a scalar.
    /// @param s1 The SizeOf instance.
    /// @param s2 The scalar value.
    /// @return A new SizeOf instance that is the product of s1 and s2.
    constexpr friend SizeOf operator*(const SizeOf& s1, T s2) noexcept {
        return SizeOf(s1.v * s2);
    }

    /// @brief Multiplication operator with a scalar on the left.
    /// @param s1 The scalar value.
    /// @param s2 The SizeOf instance.
    /// @return A new SizeOf instance that is the product of s1 and s2.
    constexpr friend SizeOf operator*(T s1, const SizeOf& s2) noexcept {
        return SizeOf(s1 * s2.v);
    }

    /// @brief Division operator with a scalar.
    /// @param s1 The SizeOf instance.
    /// @param s2 The scalar value.
    /// @return A new SizeOf instance that is the quotient of s1 and s2.
    constexpr friend SizeOf operator/(const SizeOf& s1, T s2) noexcept {
        return SizeOf(s1.v / s2);
    }

    /// @brief Division operator with a scalar on the left.
    /// @param s1 The scalar value.
    /// @param s2 The SizeOf instance.
    /// @return A new SizeOf instance that is the quotient of s1 and s2.
    constexpr friend SizeOf operator/(T s1, const SizeOf& s2) noexcept {
        return SizeOf(s1 / s2.v);
    }

    /// @brief Unary negation operator.
    /// @param s1 The SizeOf instance.
    /// @return A new SizeOf instance that is the negation of s1.
    constexpr friend SizeOf operator-(const SizeOf& s1) noexcept {
        return SizeOf(-s1.v);
    }

    /// @brief Calculates the area of the size.
    /// @return The area as the product of width and height.
    constexpr T area() const noexcept {
        return x * y;
    }

    /// @brief Access operator for retrieving component by index.
    /// @param i The index of the component (0 for x, 1 for y).
    /// @return The component value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying component by index.
    /// @param i The index of the component (0 for x, 1 for y).
    /// @return A reference to the component value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Checks if two SizeOf instances are equal.
    /// @param c The SizeOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const SizeOf& c) const noexcept {
        return v == c.v;
    }

    /// @brief Checks if two SizeOf instances are not equal.
    /// @param c The SizeOf instance to compare against.
    /// @return True if both instances are not equal; otherwise false.
    constexpr bool operator!=(const SizeOf& c) const noexcept {
        return !operator==(c);
    }

    using vec_type = Simd<T, 2>; ///< The underlying SIMD vector type.

    union {
        vec_type v; ///< The SIMD vector representing size.

        /// @brief An array to access the components directly.
        T components[2];

        struct {
            T x; ///< The width component.
            T y; ///< The height component.
        };

        struct {
            T width;  ///< The width component.
            T height; ///< The height component.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x", &SizeOf::x },
        ReflectionField{ "y", &SizeOf::y },
    };
};

/**
 * @brief Computes the element-wise minimum of two SizeOf instances.
 *
 * @tparam T The type of the components.
 * @param a The first SizeOf instance.
 * @param b The second SizeOf instance.
 * @return A SizeOf instance representing the minimum size.
 */
template <typename T>
BRISK_INLINE SizeOf<T> min(const SizeOf<T>& a, const SizeOf<T>& b) noexcept {
    return SizeOf<T>(min(a.v, b.v));
}

/**
 * @brief Computes the element-wise maximum of two SizeOf instances .
 *
 * @tparam T The type of the components.
 * @param a The first SizeOf instance.
 * @param b The second SizeOf instance.
 * @return A SizeOf instance representing the maximum size.
 */
template <typename T>
BRISK_INLINE SizeOf<T> max(const SizeOf<T>& a, const SizeOf<T>& b) noexcept {
    return SizeOf<T>(max(a.v, b.v));
}

/**
 * @brief A structure representing the edges of a rectangle or bounding box in 2D space.
 *
 * This structure defines the edges using four coordinates: (x1, y1) for one corner
 * and (x2, y2) for the opposite corner. It provides methods for constructing, accessing,
 * and manipulating these edges.
 *
 * @tparam T The type of the edge coordinates (e.g., int, float).
 */
template <typename T>
struct EdgesOf {
    /// @brief Default constructor that initializes edges to (0, 0, 0, 0).
    constexpr EdgesOf() noexcept : x1{}, y1{}, x2{}, y2{} {}

    /// @brief Constructor that initializes all edges to a specific value.
    /// @param value The value to initialize all edge coordinates.
    constexpr EdgesOf(T value) noexcept : x1(value), y1(value), x2(value), y2(value) {}

    /// @brief Constructor that initializes edges from a convertible type.
    /// @tparam U The type to convert from.
    /// @param v The value to convert and initialize edges.
    template <std::convertible_to<T> U>
    constexpr EdgesOf(U v) noexcept : EdgesOf(static_cast<T>(v)) {}

    /// @brief Constructor that initializes edges with specified horizontal and vertical values.
    /// @param h The horizontal value for x1 and x2.
    /// @param v The vertical value for y1 and y2.
    constexpr EdgesOf(T h, T v) noexcept : x1(h), y1(v), x2(h), y2(v) {}

    /// @brief Constructor that initializes edges with four specified coordinates.
    /// @param x1 The x-coordinate of the first edge.
    /// @param y1 The y-coordinate of the first edge.
    /// @param x2 The x-coordinate of the second edge.
    /// @param y2 The y-coordinate of the second edge.
    constexpr EdgesOf(T x1, T y1, T x2, T y2) noexcept : x1(x1), y1(y1), x2(x2), y2(y2) {}

    /// @brief Copy constructor.
    /// @param b The EdgesOf instance to copy from.
    constexpr EdgesOf(const EdgesOf& b) noexcept = default;

    /// @brief Access operator for retrieving edge coordinate by index.
    /// @param i The index of the coordinate (0 for x1, 1 for y1, 2 for x2, 3 for y2).
    /// @return The coordinate value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying edge coordinate by index.
    /// @param i The index of the coordinate (0 for x1, 1 for y1, 2 for x2, 3 for y2).
    /// @return A reference to the coordinate value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Checks if two EdgesOf instances are equal.
    /// @param c The EdgesOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const EdgesOf& c) const noexcept {
        return x1 == c.x1 && y1 == c.y1 && x2 == c.x2 && y2 == c.y2;
    }

    /// @brief Checks if two EdgesOf instances are not equal.
    /// @param c The EdgesOf instance to compare against.
    /// @return True if both instances are not equal; otherwise false.
    constexpr bool operator!=(const EdgesOf& c) const noexcept {
        return !(operator==(c));
    }

    union {
        /// @brief An array to access the edge coordinates directly.
        T components[4];

        struct {
            T x1; ///< The x-coordinate of the first edge.
            T y1; ///< The y-coordinate of the first edge.
            T x2; ///< The x-coordinate of the second edge.
            T y2; ///< The y-coordinate of the second edge.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x1", &EdgesOf::x1 },
        ReflectionField{ "y1", &EdgesOf::y1 },
        ReflectionField{ "x2", &EdgesOf::x2 },
        ReflectionField{ "y2", &EdgesOf::y2 },
    };
};

/**
 * @brief A specialized EdgesOf structure for SIMD-compatible types.
 *
 * This specialization leverages SIMD operations for better performance
 * when working with edges. It allows for various arithmetic and utility
 * operations using SIMD vectors.
 *
 * @tparam T The SIMD-compatible type of the edge coordinates.
 */
template <SimdCompatible T>
struct EdgesOf<T> {
    /// @brief Default constructor that initializes edges to (0, 0, 0, 0).
    constexpr EdgesOf() noexcept : v() {}

    /// @brief Constructor that initializes all edges to a specific value.
    /// @param value The value to initialize all edge coordinates.
    constexpr explicit EdgesOf(T value) noexcept : v(value) {}

    /// @brief Constructor that initializes edges with specified horizontal and vertical values.
    /// @param h The horizontal value for x1 and x2.
    /// @param v The vertical value for y1 and y2.
    constexpr EdgesOf(T h, T v) noexcept : v(h, v, h, v) {}

    /// @brief Constructor that initializes edges with four specified coordinates.
    /// @param x1 The x-coordinate of the first edge.
    /// @param y1 The y-coordinate of the first edge.
    /// @param x2 The x-coordinate of the second edge.
    /// @param y2 The y-coordinate of the second edge.
    constexpr EdgesOf(T x1, T y1, T x2, T y2) noexcept : v(x1, y1, x2, y2) {}

    /// @brief Constructor that initializes from a SIMD vector.
    /// @param v The SIMD vector to initialize edges from.
    constexpr explicit EdgesOf(const Simd<T, 4>& v) noexcept : v(v) {}

    /// @brief Copy constructor.
    /// @param b The EdgesOf instance to copy from.
    constexpr EdgesOf(const EdgesOf& b) noexcept = default;

    /// @brief Conversion operator to a different EdgesOf type.
    /// @tparam U The type to convert to.
    /// @return A new EdgesOf instance of type U.
    template <typename U>
    operator EdgesOf<U>() const noexcept {
        if constexpr (SimdCompatible<U>) {
            return EdgesOf<U>(rescale<U, 1, 1>(v));
        } else if constexpr (std::convertible_to<T, U>) {
            return EdgesOf<U>{ static_cast<U>(x1), static_cast<U>(y1), static_cast<U>(x2),
                               static_cast<U>(y2) };
        } else {
            static_assert(sizeof(U) == 0, "Cannot convert");
        }
    }

    /// @brief Rounds the edge coordinates to the nearest integer.
    /// @return A new EdgesOf instance with rounded coordinates.
    constexpr EdgesOf round() const noexcept {
        return EdgesOf(Brisk::round(v));
    }

    /// @brief Floors the edge coordinates to the nearest lower integer.
    /// @return A new EdgesOf instance with floored coordinates.
    constexpr EdgesOf floor() const noexcept {
        return EdgesOf(Brisk::floor(v));
    }

    /// @brief Ceils the edge coordinates to the nearest higher integer.
    /// @return A new EdgesOf instance with ceiled coordinates.
    constexpr EdgesOf ceil() const noexcept {
        return EdgesOf(Brisk::ceil(v));
    }

    /// @brief Truncates the edge coordinates towards zero.
    /// @return A new EdgesOf instance with truncated coordinates.
    constexpr EdgesOf trunc() const noexcept {
        return EdgesOf(Brisk::trunc(v));
    }

    /// @brief Returns the size of the edges as a SizeOf instance.
    /// @return A SizeOf instance representing the total width and height.
    SizeOf<T> size() const noexcept {
        return SizeOf<T>(v.low() + v.high());
    }

    /// @brief Multiplies the edges by a scalar value.
    /// @param value The scalar value to multiply.
    /// @return A new EdgesOf instance with multiplied edges.
    EdgesOf operator*(T value) const noexcept {
        return EdgesOf(v * value);
    }

    /// @brief Adds a scalar value to the edges.
    /// @param value The scalar value to add.
    /// @return A new EdgesOf instance with added edges.
    EdgesOf operator+(T value) const noexcept {
        return EdgesOf(v + value);
    }

    /// @brief Subtracts a scalar value from the edges.
    /// @param value The scalar value to subtract.
    /// @return A new EdgesOf instance with subtracted edges.
    EdgesOf operator-(T value) const noexcept {
        return EdgesOf(v - value);
    }

    /// @brief Gets the horizontal size of the edges.
    /// @return The width of the edges.
    T horizontal() const noexcept {
        return size().x;
    }

    /// @brief Gets the vertical size of the edges.
    /// @return The height of the edges.
    T vertical() const noexcept {
        return size().y;
    }

    /// @brief Checks if the edges are empty.
    /// @return True if the sum of horizontal dimensions is zero; otherwise false.
    bool empty() const noexcept {
        return horizontalSum(v) == 0;
    }

    /// @brief Gets the minimum coordinate among the edges.
    /// @return The minimum edge coordinate.
    T min() const noexcept {
        return horizontalMin(v);
    }

    /// @brief Gets the maximum coordinate among the edges.
    /// @return The maximum edge coordinate.
    T max() const noexcept {
        return horizontalMax(v);
    }

    /// @brief Returns the leading edge point.
    /// @return A PointOf instance representing the leading edge.
    PointOf<T> leading() const noexcept {
        return PointOf<T>(v.low());
    }

    /// @brief Returns the trailing edge point.
    /// @return A PointOf instance representing the trailing edge.
    PointOf<T> trailing() const noexcept {
        return PointOf<T>(v.high());
    }

    /// @brief Access operator for retrieving edge coordinate by index.
    /// @param i The index of the coordinate (0 for x1, 1 for y1, 2 for x2, 3 for y2).
    /// @return The coordinate value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying edge coordinate by index.
    /// @param i The index of the coordinate (0 for x1, 1 for y1, 2 for x2, 3 for y2).
    /// @return A reference to the coordinate value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Adds two EdgesOf instances.
    /// @param b1 The first EdgesOf instance.
    /// @param b2 The second EdgesOf instance.
    /// @return A new EdgesOf instance representing the sum of both.
    constexpr friend EdgesOf operator+(const EdgesOf& b1, const EdgesOf& b2) noexcept {
        return EdgesOf(b1.v + b2.v);
    }

    /// @brief Checks if two EdgesOf instances are equal.
    /// @param c The EdgesOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const EdgesOf& c) const noexcept {
        return v == c.v;
    }

    /// @brief Checks if two EdgesOf instances are not equal.
    /// @param c The EdgesOf instance to compare against.
    /// @return True if both instances are not equal; otherwise false.
    constexpr bool operator!=(const EdgesOf& c) const noexcept {
        return !(operator==(c));
    }

    union {
        /// @brief The SIMD vector representing the edges.
        Simd<T, 4> v;

        /// @brief An array to access the edge coordinates directly.
        T components[4];

        struct {
            T x1; ///< The x-coordinate of the first edge.
            T y1; ///< The y-coordinate of the first edge.
            T x2; ///< The x-coordinate of the second edge.
            T y2; ///< The y-coordinate of the second edge.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x1", &EdgesOf::x1 },
        ReflectionField{ "y1", &EdgesOf::y1 },
        ReflectionField{ "x2", &EdgesOf::x2 },
        ReflectionField{ "y2", &EdgesOf::y2 },
    };
};

/**
 * @brief Computes the element-wise minimum of two EdgesOf instances.
 *
 * @tparam T The type of the edge coordinates.
 * @param a The first EdgesOf instance.
 * @param b The second EdgesOf instance.
 * @return A new EdgesOf instance representing the minimum edges.
 */
template <typename T>
BRISK_INLINE EdgesOf<T> min(const EdgesOf<T>& a, const EdgesOf<T>& b) noexcept {
    return EdgesOf<T>(min(a.v, b.v));
}

/**
 * @brief Computes the element-wise maximum of two EdgesOf instances.
 *
 * @tparam T The type of the edge coordinates.
 * @param a The first EdgesOf instance.
 * @param b The second EdgesOf instance.
 * @return A new EdgesOf instance representing the maximum edges.
 */
template <typename T>
BRISK_INLINE EdgesOf<T> max(const EdgesOf<T>& a, const EdgesOf<T>& b) noexcept {
    return EdgesOf<T>(max(a.v, b.v));
}

/**
 * @brief A structure representing the corners of a rectangle or bounding box in 2D space.
 *
 * This structure defines four corner points of a rectangle using two-dimensional coordinates:
 * (x1y1) for the top-left corner, (x2y1) for the top-right corner,
 * (x1y2) for the bottom-left corner, and (x2y2) for the bottom-right corner. It provides methods for
 * constructing, accessing, and manipulating these corners.
 *
 * @tparam T The type of the corner coordinates (e.g., int, float).
 */
template <typename T>
struct CornersOf {
    /// @brief Default constructor that initializes corners to zero.
    constexpr CornersOf() noexcept : x1y1{}, x2y1{}, x1y2{}, x2y2{} {}

    /// @brief Constructor that initializes all corners to a specific value.
    /// @param value The value to initialize all corner coordinates.
    constexpr CornersOf(T value) noexcept : x1y1(value), x2y1(value), x1y2(value), x2y2(value) {}

    /// @brief Constructor that initializes corners from a convertible type.
    /// @tparam U The type to convert from.
    /// @param v The value to convert and initialize corners.
    template <std::convertible_to<T> U>
    constexpr CornersOf(U v) noexcept : CornersOf(static_cast<T>(v)) {}

    /// @brief Constructor that initializes corners with specified coordinates.
    /// @param x1y1 The coordinate of the top-left corner.
    /// @param x2y1 The coordinate of the top-right corner.
    /// @param x1y2 The coordinate of the bottom-left corner.
    /// @param x2y2 The coordinate of the bottom-right corner.
    constexpr CornersOf(T x1y1, T x2y1, T x1y2, T x2y2) noexcept
        : x1y1(x1y1), x2y1(x2y1), x1y2(x1y2), x2y2(x2y2) {}

    /// @brief Copy constructor.
    /// @param b The CornersOf instance to copy from.
    constexpr CornersOf(const CornersOf& b) noexcept = default;

    /// @brief Access operator for retrieving corner coordinate by index.
    /// @param i The index of the coordinate (0 for x1y1, 1 for x2y1, 2 for x1y2, 3 for x2y2).
    /// @return The coordinate value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying corner coordinate by index.
    /// @param i The index of the coordinate (0 for x1y1, 1 for x2y1, 2 for x1y2, 3 for x2y2).
    /// @return A reference to the coordinate value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Checks if two CornersOf instances are equal.
    /// @param c The CornersOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const CornersOf& c) const noexcept {
        return x1y1 == c.x1y1 && x2y1 == c.x2y1 && x1y2 == c.x1y2 && x2y2 == c.x2y2;
    }

    union {
        /// @brief An array to access the corner coordinates directly.
        T components[4];

        struct {
            T x1y1; ///< The coordinate of the bottom-left corner.
            T x2y1; ///< The coordinate of the bottom-right corner.
            T x1y2; ///< The coordinate of the top-left corner.
            T x2y2; ///< The coordinate of the top-right corner.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x1y1", &CornersOf::x1y1 },
        ReflectionField{ "x2y1", &CornersOf::x2y1 },
        ReflectionField{ "x1y2", &CornersOf::x1y2 },
        ReflectionField{ "x2y2", &CornersOf::x2y2 },
    };
};

/**
 * @brief A specialized CornersOf structure for SIMD-compatible types.
 *
 * This specialization leverages SIMD operations for better performance when working with corners.
 * It allows for various arithmetic and utility operations using SIMD vectors.
 *
 * @tparam T The SIMD-compatible type of the corner coordinates.
 */
template <SimdCompatible T>
struct CornersOf<T> {
    /// @brief Default constructor that initializes corners to zero.
    constexpr CornersOf() noexcept : v() {}

    /// @brief Constructor that initializes all corners to a specific value.
    /// @param value The value to initialize all corner coordinates.
    constexpr CornersOf(T value) noexcept : v(value) {}

    /// @brief Constructor that initializes corners from a convertible type.
    /// @tparam U The type to convert from.
    /// @param v The value to convert and initialize corners.
    template <std::convertible_to<T> U>
    constexpr CornersOf(U v) noexcept : CornersOf(static_cast<T>(v)) {}

    /// @brief Constructor that initializes corners with specified coordinates.
    /// @param x1y1 The coordinate of the top-left corner.
    /// @param x2y1 The coordinate of the top-right corner.
    /// @param x1y2 The coordinate of the bottom-left corner.
    /// @param x2y2 The coordinate of the bottom-right corner.
    constexpr CornersOf(T x1y1, T x2y1, T x1y2, T x2y2) noexcept : v(x1y1, x2y1, x1y2, x2y2) {}

    constexpr CornersOf(Simd<T, 4> v) noexcept : v(v) {}

    /// @brief Copy constructor.
    /// @param b The CornersOf instance to copy from.
    constexpr CornersOf(const CornersOf& b) noexcept = default;

    /// @brief Conversion operator to a different CornersOf type.
    /// @tparam U The type to convert to.
    /// @return A new CornersOf instance of type U.
    template <typename U>
    operator CornersOf<U>() const noexcept {
        if constexpr (SimdCompatible<U>) {
            return CornersOf<U>(rescale<U, 1, 1>(v));
        } else if constexpr (std::convertible_to<T, U>) {
            return CornersOf<U>{ static_cast<U>(x1y1), static_cast<U>(x2y1), static_cast<U>(x1y2),
                                 static_cast<U>(x2y2) };
        } else {
            static_assert(sizeof(U) == 0, "Cannot convert");
        }
    }

    /// @brief Access operator for retrieving corner coordinate by index.
    /// @param i The index of the coordinate (0 for x1y1, 1 for x2y1, 2 for x1y2, 3 for x2y2).
    /// @return The coordinate value at index i.
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /// @brief Access operator for modifying corner coordinate by index.
    /// @param i The index of the coordinate (0 for x1y1, 1 for x2y1, 2 for x1y2, 3 for x2y2).
    /// @return A reference to the coordinate value at index i.
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    /// @brief Checks if two CornersOf instances are equal.
    /// @param c The CornersOf instance to compare against.
    /// @return True if both instances are equal; otherwise false.
    constexpr bool operator==(const CornersOf& c) const noexcept {
        return v == c.v;
    }

    /// @brief Gets the minimum coordinate among the corners.
    /// @return The minimum corner coordinate.
    T min() const noexcept {
        return horizontalMin(v);
    }

    /// @brief Gets the maximum coordinate among the corners.
    /// @return The maximum corner coordinate.
    T max() const noexcept {
        return horizontalMax(v);
    }

    /// @brief Checks if the corners are empty.
    /// @return True if the sum of corner coordinates is zero; otherwise false.
    bool empty() const noexcept {
        return horizontalSum(v) == 0;
    }

    CornersOf operator+(T value) const noexcept {
        return CornersOf(v + Simd<T, 4>(value));
    }

    union {
        /// @brief The SIMD vector representing the corners.
        Simd<T, 4> v;

        /// @brief An array to access the corner coordinates directly.
        T components[4];

        struct {
            T x1y1; ///< The coordinate of the bottom-left corner.
            T x2y1; ///< The coordinate of the bottom-right corner.
            T x1y2; ///< The coordinate of the top-left corner.
            T x2y2; ///< The coordinate of the top-right corner.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x1y1", &CornersOf::x1y1 },
        ReflectionField{ "x2y1", &CornersOf::x2y1 },
        ReflectionField{ "x1y2", &CornersOf::x1y2 },
        ReflectionField{ "x2y2", &CornersOf::x2y2 },
    };
};

/**
 * @brief A template structure representing a rectangle in a 2D space.
 *
 * This structure provides methods for manipulating and querying a rectangle's properties
 * defined by two corner points (x1, y1) and (x2, y2), where (x1, y1) is the top-left
 * corner and (x2, y2) is the bottom-right corner.
 *
 * @tparam T The data type for the coordinates.
 */
template <typename T>
struct RectangleOf {
    using Tfloat = typename Internal::FloatTypeFor<T>::Type; ///< The floating-point type associated with T.

    /**
     * @brief Constructs a rectangle using the given corner coordinates.
     *
     * @param x1 The x-coordinate of the top-left corner.
     * @param y1 The y-coordinate of the top-left corner.
     * @param x2 The x-coordinate of the bottom-right corner.
     * @param y2 The y-coordinate of the bottom-right corner.
     */
    constexpr RectangleOf(T x1, T y1, T x2, T y2) noexcept : v(x1, y1, x2, y2) {}

    /**
     * @brief Constructs a rectangle from a SIMD vector.
     *
     * @param v A SIMD vector representing the rectangle's corners.
     */
    constexpr explicit RectangleOf(const Simd<T, 4>& v) noexcept : v(v) {}

    /**
     * @brief Copy constructor for the rectangle.
     *
     * @param r The rectangle to copy from.
     */
    constexpr RectangleOf(const RectangleOf& r) noexcept = default;

    /**
     * @brief Converts this rectangle to a rectangle of a different type.
     *
     * @tparam U The type to convert to.
     * @return A new rectangle of type U.
     */
    template <typename U>
    operator RectangleOf<U>() const noexcept {
        if constexpr (SimdCompatible<U>) {
            return RectangleOf<U>(rescale<U, 1, 1>(v));
        } else if constexpr (std::convertible_to<T, U>) {
            return RectangleOf<U>{ static_cast<U>(x1), static_cast<U>(y1), static_cast<U>(x2),
                                   static_cast<U>(y2) };
        } else {
            static_assert(sizeof(U) == 0, "Cannot convert");
        }
    }

    /**
     * @brief Constructs a rectangle from a point and size.
     *
     * @param point The starting point of the rectangle.
     * @param size The size of the rectangle.
     */
    constexpr RectangleOf(const PointOf<T>& point, const SizeOf<T>& size) noexcept
        : v(concat(point.v, point.v + size.v)) {}

    /**
     * @brief Constructs a rectangle from two corner points.
     *
     * @param point1 The first corner point of the rectangle.
     * @param point2 The second corner point of the rectangle.
     */
    constexpr RectangleOf(const PointOf<T>& point1, const PointOf<T>& point2) noexcept
        : v(concat(point1.v, point2.v)) {}

    /**
     * @brief Constructs an aligned rectangle given a base point, size, and alignment.
     *
     * @param base The base point of the rectangle.
     * @param dim The size of the rectangle.
     * @param alignment The alignment for the rectangle.
     */
    constexpr RectangleOf(const PointOf<T>& base, const SizeOf<T>& dim,
                          const PointOf<Tfloat>& alignment) noexcept
        : RectangleOf(base.alignedRect(dim, alignment)) {}

    /** @brief Default constructor for the rectangle. */
    constexpr RectangleOf() noexcept : v() {}

    /**
     * @brief Checks if the rectangle is empty.
     *
     * The rectangle is considered empty if its width or height is less than or equal to 0.
     *
     * @return True if the rectangle is empty, false otherwise.
     */
    constexpr bool empty() const noexcept {
        return horizontalAny(le(v.high(), v.low()));
    }

    /**
     * @brief Gets the size of the rectangle.
     *
     * @return The size of the rectangle as a SizeOf<T> object.
     */
    constexpr SizeOf<T> size() const noexcept {
        return v.high() - v.low();
    }

    /**
     * @brief Calculates the area of the rectangle.
     *
     * @return The area of the rectangle.
     */
    constexpr T area() const noexcept {
        return size().area();
    }

    /**
     * @brief Calculates the width of the rectangle.
     *
     * @return The width of the rectangle.
     */
    constexpr T width() const noexcept {
        return x2 - x1;
    }

    /**
     * @brief Calculates the height of the rectangle.
     *
     * @return The height of the rectangle.
     */
    constexpr T height() const noexcept {
        return y2 - y1;
    }

    /**
     * @brief Gets the shortest side length of the rectangle.
     *
     * @return The length of the shortest side.
     */
    constexpr T shortestSide() const noexcept {
        return size().shortestSide();
    }

    /**
     * @brief Gets the longest side length of the rectangle.
     *
     * @return The length of the longest side.
     */
    constexpr T longestSide() const noexcept {
        return size().longestSize();
    }

    /**
     * @brief Determines the orientation of the rectangle.
     *
     * @return The orientation (Horizontal or Vertical).
     */
    Orientation orientation() const noexcept {
        return width() > height() ? Orientation::Horizontal : Orientation::Vertical;
    }

    /**
     * @brief Creates a slice of the rectangle based on the given orientation.
     *
     * @param orientation The orientation of the slice.
     * @param start The normalized start coordinate.
     * @param stop The normalized stop coordinate.
     * @return A new RectangleOf that represents the slice.
     */
    RectangleOf slice(Orientation orientation, Tfloat start, Tfloat stop) const noexcept {
        if (orientation == Orientation::Horizontal)
            return RectangleOf{ at(start, 0.f), at(stop, 1.f) };
        else
            return RectangleOf{ at(0.f, start), at(1.f, stop) };
    }

    /**
     * @brief Gets the center point of the rectangle.
     *
     * @return The center point as a PointOf<T>.
     */
    PointOf<T> center() const noexcept {
        return at(0.5f, 0.5f);
    }

    /**
     * @brief Converts a point to normalized coordinates based on the rectangle.
     *
     * @param pt The point to normalize.
     * @return The normalized coordinates as a PointOf<Tfloat>.
     */
    PointOf<Tfloat> toNormCoord(const PointOf<T>& pt) const noexcept {
        return PointOf<T>((pt.v - p1.v) / (p1.v - p1.v));
    }

    /**
     * @brief Converts a point to normalized coordinates with a fallback point if outside.
     *
     * @param pt The point to normalize.
     * @param ifoutside The fallback point if the original point is outside the rectangle.
     * @return The normalized coordinates or the fallback point.
     */
    PointOf<Tfloat> toNormCoord(const PointOf<T>& pt, const PointOf<T>& ifoutside) const noexcept {
        if (!contains(pt))
            return ifoutside;
        return PointOf<T>((pt.v - p1.v) / (p2.v - p1.v));
    }

    /**
     * @brief Splits the rectangle based on a point and size.
     *
     * @param point1 The starting point of the split.
     * @param size The size of the split rectangle.
     * @return A new RectangleOf representing the split area.
     */
    RectangleOf split(const PointOf<Tfloat>& point1, const SizeOf<Tfloat>& size) const noexcept {
        const Simd<Tfloat, 2> point2 = point1.v + size.v;
        return RectangleOf(
            concat(Simd<T, 2>(p1.v + this->size().v * point1.v), Simd<T, 2>(p1.v + this->size().v * point2)));
    }

    /**
     * @brief Splits the rectangle based on individual coordinates.
     *
     * @param x The x-coordinate of the top-left corner of the split rectangle.
     * @param y The y-coordinate of the top-left corner of the split rectangle.
     * @param w The width of the split rectangle.
     * @param h The height of the split rectangle.
     * @return A new RectangleOf representing the split area.
     */
    RectangleOf split(Tfloat x, Tfloat y, Tfloat w, Tfloat h) const noexcept {
        return split({ x, y }, { w, h });
    }

    /**
     * @brief Gets the point at a normalized coordinate within the rectangle.
     *
     * @param pt The normalized coordinates.
     * @return The point as a PointOf<T>.
     */
    PointOf<T> at(const PointOf<Tfloat>& pt) const noexcept {
        return p1 + PointOf<T>(Simd<T, 2>(pt.v * Simd<Tfloat, 2>(size().v)));
    }

    /**
     * @brief Gets the point at a normalized coordinate within the rectangle.
     *
     * @param x The normalized x-coordinate.
     * @param y The normalized y-coordinate.
     * @return The point as a PointOf<T>.
     */
    PointOf<T> at(Tfloat x, Tfloat y) const noexcept {
        return p1 + PointOf<T>(Simd<T, 2>{ Simd<Tfloat, 2>{ x, y } * Simd<Tfloat, 2>(size().v) });
    }

    /**
     * @brief Applies a new starting point to the rectangle.
     *
     * @param p The new starting point.
     */
    void applyStart(const PointOf<T>& p) noexcept {
        v = concat(p.v, p.v + size());
    }

    /**
     * @brief Applies a new starting point using individual coordinates.
     *
     * @param x The x-coordinate of the new starting point.
     * @param y The y-coordinate of the new starting point.
     */
    void applyStart(T x, T y) noexcept {
        v = concat(Simd{ x, y }, Simd{ x, y } + size().v);
    }

    /**
     * @brief Applies a new size to the rectangle.
     *
     * @param s The new size.
     */
    void applySize(const SizeOf<T>& s) noexcept {
        v = concat(v.low(), v.low() + s.v);
    }

    /**
     * @brief Applies a new size using individual dimensions.
     *
     * @param w The new width.
     * @param h The new height.
     */
    void applySize(T w, T h) noexcept {
        v = concat(v.low(), v.low() + pack(w, h));
    }

    /**
     * @brief Applies a new width to the rectangle, keeping the height unchanged.
     *
     * @param w The new width.
     */
    void applyWidth(T w) noexcept {
        applySize(w, height());
    }

    /**
     * @brief Applies a new height to the rectangle, keeping the width unchanged.
     *
     * @param h The new height.
     */
    void applyHeight(T h) noexcept {
        applySize(width(), h);
    }

    /**
     * @brief Applies an offset to the rectangle's position.
     *
     * @param x The x offset.
     * @param y The y offset.
     */
    void applyOffset(T x, T y) noexcept {
        v += Simd<T, 4>(x, y, x, y);
    }

    /**
     * @brief Applies an offset using a point.
     *
     * @param p The point to offset by.
     */
    void applyOffset(const PointOf<T>& p) noexcept {
        v += concat(p.v, p.v);
    }

    /**
     * @brief Applies a scale factor to the rectangle.
     *
     * @param x The x scale factor.
     * @param y The y scale factor.
     */
    void applyScale(T x, T y) noexcept {
        v *= Simd<T, 4>(x, y, x, y);
    }

    /**
     * @brief Applies a margin to the rectangle.
     *
     * @param h The horizontal margin.
     * @param v The vertical margin.
     */
    void applyMargin(T h, T v) noexcept {
        v += Simd<T, 4>(-h, -v, +h, +v);
    }

    /**
     * @brief Applies padding to the rectangle.
     *
     * @param h The horizontal padding.
     * @param v The vertical padding.
     */
    void applyPadding(T h, T v) noexcept {
        v += Simd<T, 4>(+h, +v, -h, -v);
    }

    /**
     * @brief Applies a uniform margin to the rectangle.
     *
     * @param m The margin value.
     */
    void applyMargin(T m) noexcept {
        v += Simd<T, 4>(-m, -m, +m, +m);
    }

    /**
     * @brief Applies a uniform padding to the rectangle.
     *
     * @param p The padding value.
     */
    void applyPadding(T p) noexcept {
        v += Simd<T, 4>(+p, +p, -p, -p);
    }

    /**
     * @brief Applies a margin defined by edges to the rectangle.
     *
     * @param m The margin edges.
     */
    void applyMargin(const EdgesOf<T>& m) noexcept {
        v += Simd<T, 4>(-1, -1, 1, 1) * m.v;
    }

    /**
     * @brief Applies padding defined by edges to the rectangle.
     *
     * @param p The padding edges.
     */
    void applyPadding(const EdgesOf<T>& p) noexcept {
        v += Simd<T, 4>(1, 1, -1, -1) * p.v;
    }

    /**
     * @brief Applies padding to the rectangle using individual dimensions.
     *
     * @param x1 The left padding.
     * @param y1 The top padding.
     * @param x2 The right padding.
     * @param y2 The bottom padding.
     */
    void applyPadding(T x1, T y1, T x2, T y2) noexcept {
        v += Simd<T, 4>(+x1, +y1, -x2, -y2);
    }

    /**
     * @brief Creates an aligned rectangle with specified inner size and alignment.
     *
     * @param innerSize The inner size of the rectangle.
     * @param alignment The alignment for the rectangle.
     * @return A new aligned RectangleOf.
     */
    RectangleOf alignedRect(const SizeOf<T>& innerSize, const PointOf<Tfloat>& alignment) const noexcept {
        const SizeOf<T> sz  = innerSize;
        const SizeOf<T> gap = size() - sz;
        const Simd<T, 2> p  = p1.v + Simd<T, 2>(Simd<Tfloat, 2>(gap.v) * alignment.v);
        return RectangleOf(concat(p, p + sz.v));
    }

    /**
     * @brief Creates an aligned rectangle using width, height, and alignment factors.
     *
     * @param width The width of the rectangle.
     * @param height The height of the rectangle.
     * @param alignX The horizontal alignment factor.
     * @param alignY The vertical alignment factor.
     * @return A new aligned RectangleOf.
     */
    RectangleOf alignedRect(T width, T height, Tfloat alignX, Tfloat alignY) const noexcept {
        return alignedRect({ width, height }, { alignX, alignY });
    }

    /**
     * @brief Creates a new rectangle with a specified starting point.
     *
     * @param p The new starting point.
     * @return A new RectangleOf with the specified starting point.
     */
    constexpr RectangleOf withStart(const PointOf<T>& p) const noexcept {
        return RectangleOf(concat(p.v, p.v + size()));
    }

    /**
     * @brief Creates a new rectangle with a specified starting point using coordinates.
     *
     * @param x The x-coordinate of the new starting point.
     * @param y The y-coordinate of the new starting point.
     * @return A new RectangleOf with the specified starting point.
     */
    constexpr RectangleOf withStart(T x, T y) const noexcept {
        return RectangleOf(concat(Simd{ x, y }, Simd{ x, y } + size()));
    }

    /**
     * @brief Creates a new rectangle with a specified size.
     *
     * @param s The new size.
     * @return A new RectangleOf with the specified size.
     */
    constexpr RectangleOf withSize(const SizeOf<T>& s) const noexcept {
        return RectangleOf(concat(v.low(), v.low() + s.v));
    }

    /**
     * @brief Creates a new rectangle with a specified size using dimensions.
     *
     * @param w The new width.
     * @param h The new height.
     * @return A new RectangleOf with the specified size.
     */
    constexpr RectangleOf withSize(T w, T h) const noexcept {
        return RectangleOf(concat(v.low(), v.low() + Simd{ w, h }));
    }

    /**
     * @brief Creates a new rectangle with a specified width.
     *
     * @param w The new width.
     * @return A new RectangleOf with the specified width.
     */
    constexpr RectangleOf withWidth(T w) const noexcept {
        return withSize(w, height());
    }

    /**
     * @brief Creates a new rectangle with a specified height.
     *
     * @param h The new height.
     * @return A new RectangleOf with the specified height.
     */
    constexpr RectangleOf withHeight(T h) const noexcept {
        return withSize(width(), h);
    }

    /**
     * @brief Creates a new rectangle with a specified offset.
     *
     * @param p The point to offset by.
     * @return A new RectangleOf with the specified offset.
     */
    constexpr RectangleOf withOffset(const PointOf<T>& p) const noexcept {
        return RectangleOf(v + Simd<T, 4>(concat(p.v, p.v)));
    }

    /**
     * @brief Creates a new rectangle with a specified offset using coordinates.
     *
     * @param x The x offset.
     * @param y The y offset.
     * @return A new RectangleOf with the specified offset.
     */
    constexpr RectangleOf withOffset(T x, T y) const noexcept {
        return RectangleOf(v + Simd<T, 4>(x, y, x, y));
    }

    /**
     * @brief Creates a new rectangle with applied scaling factors.
     *
     * @param x The x scaling factor.
     * @param y The y scaling factor.
     * @return A new RectangleOf with the applied scaling factors.
     */
    constexpr RectangleOf withScale(T x, T y) const noexcept {
        return RectangleOf(v * Simd<T, 4>(x, y, x, y));
    }

    /**
     * @brief Creates a new rectangle with applied margin.
     *
     * @param h The horizontal margin.
     * @param v The vertical margin.
     * @return A new RectangleOf with the applied margin.
     */
    constexpr RectangleOf withMargin(T h, T v) const noexcept {
        return RectangleOf(this->v + Simd<T, 4>(-h, -v, +h, +v));
    }

    /**
     * @brief Creates a new rectangle with applied padding.
     *
     * @param h The horizontal padding.
     * @param v The vertical padding.
     * @return A new RectangleOf with the applied padding.
     */
    constexpr RectangleOf withPadding(T h, T v) const noexcept {
        return RectangleOf(this->v + Simd<T, 4>(+h, +v, -h, -v));
    }

    /**
     * @brief Creates a new rectangle with applied padding using dimensions.
     *
     * @param x1 The left padding.
     * @param y1 The top padding.
     * @param x2 The right padding.
     * @param y2 The bottom padding.
     * @return A new RectangleOf with the applied padding.
     */
    constexpr RectangleOf withPadding(T x1, T y1, T x2, T y2) const noexcept {
        return RectangleOf(v + Simd<T, 4>(+x1, +y1, -x2, -y2));
    }

    /**
     * @brief Creates a new rectangle with applied margin.
     *
     * @param m The margin value.
     * @return A new RectangleOf with the applied margin.
     */
    constexpr RectangleOf withMargin(T m) const noexcept {
        return RectangleOf(v + Simd<T, 4>(-m, -m, +m, +m));
    }

    /**
     * @brief Creates a new rectangle with applied padding.
     *
     * @param p The padding value.
     * @return A new RectangleOf with the applied padding.
     */
    constexpr RectangleOf withPadding(T p) const noexcept {
        return RectangleOf(v + Simd<T, 4>(+p, +p, -p, -p));
    }

    /**
     * @brief Creates a new rectangle with applied padding defined by edges.
     *
     * @param p The padding edges.
     * @return A new RectangleOf with the applied padding.
     */
    constexpr RectangleOf withPadding(const EdgesOf<T>& p) const noexcept {
        return RectangleOf(v + Simd<T, 4>(1, 1, -1, -1) * p.v);
    }

    /**
     * @brief Creates a new rectangle with applied margin defined by edges.
     *
     * @param m The margin edges.
     * @return A new RectangleOf with the applied margin.
     */
    constexpr RectangleOf withMargin(const EdgesOf<T>& m) const noexcept {
        return RectangleOf(v + Simd<T, 4>(-1, -1, 1, 1) * m.v);
    }

    /**
     * @brief Creates a new rectangle that is flipped horizontally.
     *
     * @return A new RectangleOf that is flipped.
     */
    constexpr RectangleOf flipped() const noexcept {
        return RectangleOf(swapAdjacent(v));
    }

    /**
     * @brief Creates a new rectangle that is conditionally flipped.
     *
     * @param flip A flag indicating whether to flip the rectangle.
     * @return A new RectangleOf that is flipped or the original.
     */
    constexpr RectangleOf flippedIf(bool flip) const noexcept {
        return flip ? flipped() : *this;
    }

    /**
     * @brief Rounds the rectangle's coordinates.
     *
     * @return A new RectangleOf with rounded coordinates.
     */
    constexpr RectangleOf round() const noexcept {
        return RectangleOf(Brisk::round(v));
    }

    /**
     * @brief Floors the rectangle's coordinates.
     *
     * @return A new RectangleOf with floored coordinates.
     */
    constexpr RectangleOf floor() const noexcept {
        return RectangleOf(Brisk::floor(v));
    }

    /**
     * @brief Expands the rectangle by rounding its boundaries outward.
     *
     * This function enlarges the rectangle by applying the floor operation to the
     * minimum corner and the ceil operation to the maximum corner.
     *
     * @return A new RectangleOf<int32_t> with expanded boundaries.
     */
    constexpr RectangleOf<int32_t> roundOutward() const noexcept {
        return RectangleOf(blend<0, 0, 1, 1>(Brisk::floor(v), Brisk::ceil(v)));
    }

    /**
     * @brief Shrinks the rectangle by rounding its boundaries inward.
     *
     * This function reduces the rectangle by applying the ceil operation to the
     * minimum corner and the floor operation to the maximum corner.
     *
     * @return A new RectangleOf<int32_t> with reduced boundaries.
     */
    constexpr RectangleOf<int32_t> roundInward() const noexcept {
        return RectangleOf(blend<1, 1, 0, 0>(Brisk::floor(v), Brisk::ceil(v)));
    }

    /**
     * @brief Ceils the rectangle's coordinates.
     *
     * @return A new RectangleOf with ceiled coordinates.
     */
    constexpr RectangleOf ceil() const noexcept {
        return RectangleOf(Brisk::ceil(v));
    }

    /**
     * @brief Truncates the rectangle's coordinates.
     *
     * @return A new RectangleOf with truncated coordinates.
     */
    constexpr RectangleOf trunc() const noexcept {
        return RectangleOf(Brisk::trunc(v));
    }

    /**
     * @brief Checks if a point is contained within the rectangle.
     *
     * @param pt The point to check.
     * @return True if the point is contained; otherwise, false.
     */
    bool contains(const PointOf<T>& pt) const noexcept {
        return pt.x >= x1 && pt.y >= y1 && pt.x < x2 && pt.y < y2;
    }

    /**
     * @brief Checks if a point is contained within the rectangle using the << operator.
     *
     * @param pt The point to check.
     * @return True if the point is contained; otherwise, false.
     */
    bool operator<<(const PointOf<T>& pt) const noexcept {
        return contains(pt);
    }

    /**
     * @brief Compares two rectangles for equality.
     *
     * @param c The rectangle to compare against.
     * @return True if both rectangles are equal; otherwise, false.
     */
    constexpr bool operator==(const RectangleOf& c) const noexcept {
        return v == c.v;
    }

    /**
     * @brief Compares two rectangles for inequality.
     *
     * @param c The rectangle to compare against.
     * @return True if the rectangles are not equal; otherwise, false.
     */
    constexpr bool operator!=(const RectangleOf& c) const noexcept {
        return !(*this == c);
    }

    /**
     * @brief Creates a new rectangle that is the union of this rectangle and another.
     *
     * @param c The other rectangle.
     * @return A new RectangleOf that is the union of both rectangles.
     */
    constexpr RectangleOf union_(const RectangleOf& c) const noexcept {
        return RectangleOf(blend<0, 0, 1, 1>(min(v, c.v), max(v, c.v)));
    }

    /**
     * @brief Creates a new rectangle that is the intersection of this rectangle and another.
     *
     * @param c The other rectangle.
     * @return A new RectangleOf that is the intersection of both rectangles.
     */
    constexpr RectangleOf intersection(const RectangleOf& c) const noexcept {
        return RectangleOf(blend<1, 1, 0, 0>(min(v, c.v), max(v, c.v)));
    }

    /**
     * @brief Checks whether this rectangle intersects with another rectangle.
     *
     * @param c The other rectangle to check intersection with.
     * @return True if this rectangle intersects with rectangle c, false otherwise.
     */
    constexpr bool intersects(const RectangleOf& c) const noexcept {
        return !intersection(c).empty();
    }

    /**
     * @brief Accesses a specific component of the rectangle.
     *
     * @param i The index of the component (0: x1, 1: y1, 2: x2, 3: y2).
     * @return The component at the specified index.
     */
    T operator[](size_t i) const noexcept {
        return components[i];
    }

    /**
     * @brief Accesses a specific component of the rectangle.
     *
     * @param i The index of the component (0: x1, 1: y1, 2: x2, 3: y2).
     * @return A reference to the component at the specified index.
     */
    T& operator[](size_t i) noexcept {
        return components[i];
    }

    union {
        Simd<T, 4> v;    ///< SIMD representation of the rectangle.
        T components[4]; ///< Array representation of the rectangle components.

        struct {
            T x1; ///< The x-coordinate of the top-left corner.
            T y1; ///< The y-coordinate of the top-left corner.
            T x2; ///< The x-coordinate of the bottom-right corner.
            T y2; ///< The y-coordinate of the bottom-right corner.
        };

        struct {
            PointOf<T> p1; ///< The top-left corner point.
            PointOf<T> p2; ///< The bottom-right corner point.
        };
    };

    constexpr static std::tuple reflection{
        ReflectionField{ "x1", &RectangleOf::x1 },
        ReflectionField{ "y1", &RectangleOf::y1 },
        ReflectionField{ "x2", &RectangleOf::x2 },
        ReflectionField{ "y2", &RectangleOf::y2 },
    };
};

/**
 * @brief Type alias for a point using 32-bit integers.
 */
using Point      = PointOf<int32_t>;

/**
 * @brief Type alias for a point using single-precision floating-point numbers.
 */
using PointF     = PointOf<float>;

/**
 * @brief Type alias for a size using 32-bit integers.
 */
using Size       = SizeOf<int32_t>;

/**
 * @brief Type alias for a size using single-precision floating-point numbers.
 */
using SizeF      = SizeOf<float>;

/**
 * @brief Type alias for edges using 32-bit integers.
 */
using Edges      = EdgesOf<int32_t>;

/**
 * @brief Type alias for edges using single-precision floating-point numbers.
 */
using EdgesF     = EdgesOf<float>;

/**
 * @brief Type alias for corners using 32-bit integers.
 */
using Corners    = CornersOf<int32_t>;

/**
 * @brief Type alias for corners using single-precision floating-point numbers.
 */
using CornersF   = CornersOf<float>;

/**
 * @brief Type alias for a rectangle using 32-bit integers.
 */
using Rectangle  = RectangleOf<int32_t>;

/**
 * @brief Type alias for a rectangle using single-precision floating-point numbers.
 */
using RectangleF = RectangleOf<float>;

/**
 * @brief A constant rectangle that represents no clipping area.
 */
constexpr inline Rectangle noClipRect{ INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX };

} // namespace Brisk
