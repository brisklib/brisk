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
 */                                                                                                          \
#pragma once

#include "Geometry.hpp"

namespace Brisk {

/**
 * @brief Represents a 2D matrix of floating point values.
 *
 * This template class provides a 2D transformation matrix implementation with
 * support for translation, scaling, rotation, reflection, and skewing. It
 * works on any floating-point type (e.g., float, double).
 *
 * @tparam T The type of floating-point values (e.g., float or double).
 */
template <typename T>
struct MatrixOf {
    static_assert(std::is_floating_point_v<T>, "MatrixOf requires a floating-point type.");
    using vec_subtype = SIMD<T, 2>;
    using vec_type    = std::array<vec_subtype, 3>;

    union {
        vec_type v; ///< Array of SIMD vectors for efficient storage.

        struct {
            T a, b, c, d, e, f; ///< Individual matrix coefficients.
        };
    };

    /**
     * @brief Constructs an identity matrix.
     */
    constexpr MatrixOf() : MatrixOf(1, 0, 0, 1, 0, 0) {}

    /**
     * @brief Constructs a matrix with specified coefficients.
     *
     * @param a Matrix coefficient at position (0,0).
     * @param b Matrix coefficient at position (0,1).
     * @param c Matrix coefficient at position (1,0).
     * @param d Matrix coefficient at position (1,1).
     * @param e Matrix translation component along the x-axis.
     * @param f Matrix translation component along the y-axis.
     */
    constexpr MatrixOf(T a, T b, T c, T d, T e, T f) : v{ SIMD{ a, b }, SIMD{ c, d }, SIMD{ e, f } } {}

    /**
     * @brief Returns the matrix coefficients as an array.
     *
     * @return std::array<T, 6> The array of coefficients {a, b, c, d, e, f}.
     */
    constexpr std::array<T, 6> coefficients() const {
        return { { a, b, c, d, e, f } };
    }

    /**
     * @brief Constructs a matrix from a given vector type.
     *
     * @param v The vector type representing the matrix.
     */
    constexpr explicit MatrixOf(const vec_type& v) : v(v) {}

    /**
     * @brief Checks if the matrix is an identity matrix.
     *
     * @return true if the matrix is an identity matrix; false otherwise.
     */
    [[nodiscard]] constexpr bool isIdentity() const noexcept {
        return *this == MatrixOf{};
    }

    /**
     * @brief Translates the matrix by a given point offset.
     *
     * @param offset The point by which to translate.
     * @return MatrixOf The translated matrix.
     */
    [[nodiscard]] constexpr MatrixOf translate(PointOf<T> offset) const {
        vec_type m = v;
        m[2] += offset.v;
        return MatrixOf(m);
    }

    /**
     * @brief Translates the matrix by given x and y offsets.
     *
     * @param x The x-axis translation.
     * @param y The y-axis translation.
     * @return MatrixOf The translated matrix.
     */
    [[nodiscard]] constexpr MatrixOf translate(T x, T y) const {
        return translate({ x, y });
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @return MatrixOf The scaled matrix.
     */
    [[nodiscard]] constexpr MatrixOf scale(T x, T y) const {
        vec_type m = v;
        m[0] *= SIMD{ x, y };
        m[1] *= SIMD{ x, y };
        m[2] *= SIMD{ x, y };
        return MatrixOf(m);
    }

    /**
     * @brief Scales the matrix by the given scaling factor.
     *
     * @param x The scaling factor.
     * @return MatrixOf The scaled matrix.
     */
    [[nodiscard]] constexpr MatrixOf scale(T xy) const {
        vec_type m = v;
        m[0] *= xy;
        m[1] *= xy;
        m[2] *= xy;
        return MatrixOf(m);
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors with respect to an origin point.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @param origin The origin point.
     * @return MatrixOf The scaled matrix.
     */
    [[nodiscard]] constexpr MatrixOf scale(T x, T y, PointOf<T> origin) const {
        return translate(-origin).scale(x, y).translate(origin);
    }

    /**
     * @brief Scales the matrix by the given x and y scaling factors with respect to a specified origin.
     *
     * @param x The x-axis scaling factor.
     * @param y The y-axis scaling factor.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return MatrixOf The scaled matrix.
     */
    [[nodiscard]] constexpr MatrixOf scale(T x, T y, T originx, T originy) const {
        return scale(x, y, { originx, originy });
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @return MatrixOf The skewed matrix.
     */
    [[nodiscard]] constexpr MatrixOf skew(T x, T y) const {
        vec_type m;
        m[0] = SIMD{ v[0][0] + v[0][1] * x, v[0][0] * y + v[0][1] };
        m[1] = SIMD{ v[1][0] + v[1][1] * x, v[1][0] * y + v[1][1] };
        m[2] = SIMD{ v[2][0] + v[2][1] * x, v[2][0] * y + v[2][1] };
        return MatrixOf(m);
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients with respect to an origin point.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @param origin The origin point.
     * @return MatrixOf The skewed matrix.
     */
    [[nodiscard]] constexpr MatrixOf skew(T x, T y, PointOf<T> origin) const {
        return translate(-origin).skew(x, y).translate(origin);
    }

    /**
     * @brief Skews the matrix by the given x and y skewness coefficients with respect to a specified origin.
     *
     * @param x The x-axis skew coefficient.
     * @param y The y-axis skew coefficient.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return MatrixOf The skewed matrix.
     */
    [[nodiscard]] constexpr MatrixOf skew(T x, T y, T originx, T originy) const {
        return skew(x, y, { originx, originy });
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees).
     *
     * @param angle The angle in degrees.
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate(T angle) const {
        vec_type m     = v;
        vec_subtype sc = sincos(vec_subtype(deg2rad<T> * angle));
        vec_subtype cs = swapAdjacent(sc) * SIMD{ T(1), T(-1) };
        m[0]           = SIMD{ dot(m[0], cs), dot(m[0], sc) };
        m[1]           = SIMD{ dot(m[1], cs), dot(m[1], sc) };
        m[2]           = SIMD{ dot(m[2], cs), dot(m[2], sc) };
        return MatrixOf(m);
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees) with respect to an origin point.
     *
     * @param angle The angle in degrees.
     * @param origin The origin point for rotation.
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate(T angle, PointOf<T> origin) const {
        return translate(-origin).rotate(angle).translate(origin);
    }

    /**
     * @brief Rotates the matrix by the given angle (in degrees) with respect to a specified origin.
     *
     * @param angle The angle in degrees.
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate(T angle, T originx, T originy) const {
        return rotate(angle, { originx, originy });
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate90(int angle) const {
        MatrixOf result = *this;
        switch (unsigned(angle) % 4) {
        case 0:
        default:
            break;
        case 1:
            result = { -b, a, -d, c, -f, e };
            break;
        case 2:
            result = { -a, -b, -c, -d, -e, -f };
            break;
        case 3:
            result = { b, -a, d, -c, f, -e };
            break;
        }
        return result;
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees with respect to a point.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @param origin The origin point for rotation.
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate90(int angle, PointOf<T> origin) const {
        return translate(-origin).rotate90(angle).translate(origin);
    }

    /**
     * @brief Rotates the matrix by a multiple of 90 degrees with respect to an origin.
     *
     * @param angle The multiple of 90 degrees to rotate (e.g., 90, 180, 270).
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return MatrixOf The rotated matrix.
     */
    [[nodiscard]] constexpr MatrixOf rotate90(int angle, T originx, T originy) const {
        return rotate90(angle, { originx, originy });
    }

    /**
     * @brief Reflects the matrix over the specified axis.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @return MatrixOf The reflected matrix.
     */
    [[nodiscard]] constexpr MatrixOf reflect(FlipAxis axis) const {
        switch (axis) {
        case FlipAxis::X:
            return scale(-1, 1);
        case FlipAxis::Y:
            return scale(1, -1);
        case FlipAxis::Both:
            return scale(-1, -1);
        }
    }

    /**
     * @brief Reflects the matrix over the specified axis with respect to a point.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @param origin The origin point for the reflection.
     * @return MatrixOf The reflected matrix.
     */
    [[nodiscard]] constexpr MatrixOf reflect(FlipAxis axis, PointOf<T> origin) const {
        return translate(-origin).reflect(axis).translate(origin);
    }

    /**
     * @brief Reflects the matrix over the specified axis with respect to an origin.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @param originx The x-coordinate of the origin.
     * @param originy The y-coordinate of the origin.
     * @return MatrixOf The reflected matrix.
     */
    [[nodiscard]] constexpr MatrixOf reflect(FlipAxis axis, T originx, T originy) const {
        return reflect(axis, { originx, originy });
    }

    /**
     * @brief Creates a translation matrix.
     *
     * @param x Translation along the x-axis.
     * @param y Translation along the y-axis.
     * @return MatrixOf The translation matrix.
     */
    [[nodiscard]] static constexpr MatrixOf translation(T x, T y) {
        return MatrixOf{ 1, 0, 0, 1, x, y };
    }

    /**
     * @brief Creates a scaling matrix.
     *
     * @param x Scaling factor along the x-axis.
     * @param y Scaling factor along the y-axis.
     * @return MatrixOf The scaling matrix.
     */
    [[nodiscard]] static constexpr MatrixOf scaling(T x, T y) {
        return MatrixOf{ x, 0, 0, y, 0, 0 };
    }

    /**
     * @brief Creates a scaling matrix.
     *
     * @param xy Scaling factor.
     * @return MatrixOf The scaling matrix.
     */
    [[nodiscard]] static constexpr MatrixOf scaling(T xy) {
        return MatrixOf{ xy, 0, 0, xy, 0, 0 };
    }

    /**
     * @brief Creates a rotation matrix.
     *
     * @param angle The rotation angle in degrees.
     * @return MatrixOf The rotation matrix.
     */
    [[nodiscard]] static constexpr MatrixOf rotation(T angle) {
        vec_subtype sc = sincos(vec_subtype(deg2rad<T> * angle));
        return MatrixOf{ sc[1], sc[0], -sc[0], sc[1], 0, 0 };
    }

    /**
     * @brief Creates a 90-degree rotation matrix.
     *
     * @param angle The multiple of 90 degrees (0, 90, 180, or 270).
     * @return MatrixOf The 90-degree rotation matrix.
     */
    [[nodiscard]] static constexpr MatrixOf rotation90(int angle) {
        constexpr MatrixOf<T> m[4] = {
            { 1, 0, 0, 1, 0, 0 },
            { 0, 1, -1, 0, 0, 0 },
            { -1, 0, 0, -1, 0, 0 },
            { 0, -1, 1, 0, 0, 0 },
        };
        return m[angle % 4];
    }

    /**
     * @brief Creates a reflection matrix over the specified axis.
     *
     * @param axis The axis of reflection (X, Y, or Both).
     * @return MatrixOf The reflection matrix.
     */
    [[nodiscard]] static constexpr MatrixOf reflection(FlipAxis axis) {
        switch (axis) {
        case FlipAxis::X:
            return scaling(-1, 1);
        case FlipAxis::Y:
            return scaling(1, -1);
        case FlipAxis::Both:
            return scaling(-1, -1);
        }
    }

    /**
     * @brief Creates a skewness matrix.
     *
     * @param x The x-axis skew factor.
     * @param y The y-axis skew factor.
     * @return MatrixOf The skewness matrix.
     */
    [[nodiscard]] static constexpr MatrixOf skewness(T x, T y) {
        return MatrixOf{ 1, y, x, 1, 0, 0 };
    }

    /**
     * @brief Multiplies two matrices together.
     *
     * @param m The first matrix.
     * @param n The second matrix.
     * @return MatrixOf The resulting matrix.
     */
    constexpr friend MatrixOf<T> operator*(const MatrixOf<T>& m, const MatrixOf<T>& n) {
        using v2             = SIMD<T, 2>;
        using v3             = SIMD<T, 3>;
        using v4             = SIMD<T, 4>;
        using v8             = SIMD<T, 8>;

        std::array<v2, 3> mm = m.v;
        std::array<v3, 2> nn{ v3{ n.a, n.c, n.e }, v3{ n.b, n.d, n.f } };
        v4 n01   = concat(nn[0].firstn(size_constant<2>{}), nn[1].firstn(size_constant<2>{}));
        v4 m0n01 = mm[0].shuffle(size_constants<0, 1, 0, 1>{}) * n01;
        v4 m1n01 = mm[1].shuffle(size_constants<0, 1, 0, 1>{}) * n01;
        v4 m2n01 = mm[2].shuffle(size_constants<0, 1, 0, 1>{}) * n01;

        v8 m01   = concat(m0n01, m1n01).shuffle(size_constants<0, 2, 4, 6, 1, 3, 5, 7>{});
        v8 m23   = concat(m2n01, m2n01).shuffle(size_constants<0, 2, 4, 6, 1, 3, 5, 7>{});
        v4 t0    = m01.low() + m01.high();
        v4 t1    = m23.low() + m23.high();

        return {
            t0[0],            //
            t0[1],            //
            t0[2],            //
            t0[3],            //
            t1[0] + nn[0][2], //
            t1[1] + nn[1][2], //
        };
    }

    /**
     * @brief Transforms a point using the matrix.
     *
     * @param pt The point to transform.
     * @param m The transformation matrix.
     * @return PointOf<T> The transformed point.
     */
    constexpr friend PointOf<T> operator*(const PointOf<T>& pt, const MatrixOf<T>& m) {
        return m.transform(pt);
    }

    /**
     * @brief Flattens the matrix coefficients into a SIMD array.
     *
     * @return SIMD<T, 6> The flattened matrix.
     */
    constexpr SIMD<T, 6> flatten() const noexcept {
        SIMD<T, 6> result;
        std::memcpy(&result, v.data(), sizeof(*this));
        return result;
    }

    /**
     * @brief Checks if two matrices are equal.
     *
     * @param m The matrix to compare with.
     * @return true if the matrices are equal, false otherwise.
     */
    constexpr bool operator==(const MatrixOf<T>& m) const {
        return horizontalRMS(flatten() - m.flatten()) < 0.0001f;
    }

    /**
     * @brief Checks if two matrices are not equal.
     *
     * @param m The matrix to compare with.
     * @return true if the matrices are not equal, false otherwise.
     */
    constexpr bool operator!=(const MatrixOf<T>& m) const {
        return !operator==(m);
    }

    /**
     * @brief Transforms a rectangle using the matrix.
     *
     * @param pt The rectangle to transform.
     * @return RectangleOf<T> The transformed rectangle.
     */
    constexpr RectangleOf<T> transform(RectangleOf<T> pt) const {
        PointOf<T> points[4] = { pt.p1, pt.p2, { pt.x1, pt.y2 }, { pt.x2, pt.y1 } };
        transform(points);
        RectangleOf<T> result;
        result.x1 = std::min(std::min(points[0].x, points[1].x), std::min(points[2].x, points[3].x));
        result.y1 = std::min(std::min(points[0].y, points[1].y), std::min(points[2].y, points[3].y));
        result.x2 = std::max(std::max(points[0].x, points[1].x), std::max(points[2].x, points[3].x));
        result.y2 = std::max(std::max(points[0].y, points[1].y), std::max(points[2].y, points[3].y));
        return result;
    }

    /**
     * @brief Estimates the average scaling factor of the matrix.
     *
     * @return T The estimated scaling factor.
     */
    constexpr T estimateScale() const {
        return std::sqrt(a * a + c * c);
    }

    constexpr bool isUniformScale() const {
        constexpr float epsilon = 1e-4f;
        float scale1_sq         = a * a + c * c;
        float scale2_sq         = b * b + d * d;
        if (std::abs(b) < epsilon && std::abs(c) < epsilon) {
            return std::abs(std::abs(a) - std::abs(d)) < epsilon;
        }
        if (std::abs(scale1_sq - scale2_sq) > epsilon)
            return false;
        float dot_product = a * b + c * d;
        return std::abs(dot_product) < epsilon;
    }

    /**
     * @brief Transforms a point using the matrix.
     *
     * Applies a 2D transformation to a given point using the current matrix.
     * The transformation follows the formula:
     * \f$ x' = x \cdot a + y \cdot c + e \f$
     * \f$ y' = x \cdot b + y \cdot d + f \f$
     *
     * @param pt The point to transform.
     * @return PointOf<T> The transformed point.
     */
    constexpr PointOf<T> transform(PointOf<T> pt) const {
        // Formula: pt.x * a + pt.y * c + e, pt.x * b + pt.y * d + f;
        SIMD<T, 4> tmp = pt.v.shuffle(size_constants<0, 0, 1, 1>{}) * flatten().template firstn<4>();
        PointOf<T> result(tmp.low() + tmp.high() + flatten().template lastn<2>());
        return result;
    }

    /**
     * @brief Transforms a collection of points using the matrix.
     *
     * Applies a 2D transformation to a span of points in an optimized SIMD
     * approach, processing multiple points in parallel when possible.
     *
     * @param points The span of points to transform.
     */
    constexpr void transform(std::span<PointOf<T>> points) const {
        constexpr size_t N  = 8;
        constexpr size_t N2 = N * 2;
        size_t i            = 0;

        if (isAligned(points.data()) && points.size() >= N) {
            SIMD<T, N2> ad = repeat<N>(SIMD{ a, d });
            SIMD<T, N2> cb = repeat<N>(SIMD{ c, b });
            SIMD<T, N2> ef = repeat<N>(SIMD{ e, f });

            for (; i < points.size() && !isAligned<N>(points.data() + i); ++i) {
                points[i] = transform(points[i]);
            }
            for (; i + N - 1 < points.size(); i += N) {
                SIMD<T, N2> xy = *reinterpret_cast<const SIMD<T, N2>*>(points.data() + i);
                xy             = xy * ad + swapAdjacent(xy) * cb + ef;
                *reinterpret_cast<SIMD<T, N2>*>(points.data() + i) = xy;
            }
        }
        for (; i < points.size(); ++i) {
            points[i] = transform(points[i]);
        }
    }

    /**
     * @brief Inverts the matrix, if possible.
     *
     * @return std::optional<MatrixOf> The inverse of the matrix if it is invertible, or `std::nullopt` if
     * the matrix is singular (non-invertible).
     */
    std::optional<MatrixOf> invert() const {
        T det = a * d - b * c; // determinant

        if (det < std::numeric_limits<T>::epsilon()) {
            return std::nullopt; // Matrix is not invertible
        }

        MatrixOf result;
        // Calculate the inverse of the matrix
        result.a = d / det;
        result.b = -b / det;
        result.c = -c / det;
        result.d = a / det;
        result.e = (c * f - d * e) / det;
        result.f = (b * e - a * f) / det;
        return result;
    }
};

using Matrix = MatrixOf<float>;
} // namespace Brisk
