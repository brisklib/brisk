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
#pragma once

#include "Brisk.h"
#include <cstdint>
#include <cmath>
#include <brisk/core/Reflection.hpp>
#include "internal/Optional.hpp"
#include <numbers>
#include <concepts>

namespace Brisk {

template <typename T>
inline T withCurvature(T x, std::type_identity_t<T> curvature) {
    return (x * x) * curvature + x * (T(1) - curvature);
}

template <typename T>
inline T withCurvatureInv(T x, std::type_identity_t<T> curvature) {
    return std::sqrt(x) * curvature + x * (T(1) - curvature);
}

/**
 * @brief A template structure for representing and manipulating fractions.
 *
 * This structure allows operations with fractions, including arithmetic operations, normalization,
 * and conversions between fractions and other numeric types. The fraction is represented as a numerator
 * and a denominator, with the denominator being non-negative and the fraction in its reduced form.
 *
 * @tparam T The type used for the numerator and denominator. It must support basic arithmetic operations
 *           and the `std::abs` function.
 */
template <typename T>
struct Fraction {
    /**
     * @brief Constructs a Fraction with the given numerator and denominator.
     *
     * The constructor normalizes the fraction such that the denominator is positive and the fraction
     * is reduced to its simplest form.
     *
     * @param num The numerator of the fraction. Default is 0.
     * @param den The denominator of the fraction. Default is 1.
     */
    Fraction(T num = 0, T den = 1) : numerator(num), denominator(den) {
        normalize();
    }

    /**
     * @brief Deleted constructor for float types.
     *
     * This constructor is disabled to prevent the creation of a Fraction with a float type.
     */
    Fraction(float)  = delete;

    /**
     * @brief Deleted constructor for double types.
     *
     * This constructor is disabled to prevent the creation of a Fraction with a double type.
     */
    Fraction(double) = delete;

    /**
     * @brief Normalizes the fraction.
     *
     * Ensures that the denominator is positive and the fraction is reduced to its simplest form.
     */
    void normalize() {
        if (denominator < 0) {
            denominator = -denominator;
            numerator   = -numerator;
        }
        if (denominator) {
            const T z = gcd(abs(numerator), abs(denominator));
            numerator /= z;
            denominator /= z;
        }
    }

    /**
     * @brief Computes the absolute value of the given value.
     *
     * @param v The value whose absolute value is to be computed.
     * @return The absolute value of the given value.
     */
    static T abs(T v) {
        if constexpr (std::is_signed_v<T>)
            return std::abs(v);
        else
            return v;
    }

    T numerator;   ///< The numerator of the fraction.
    T denominator; ///< The denominator of the fraction.

    /**
     * @brief Reflection data for the Fraction structure.
     */
    constexpr static std::tuple reflection{
        ReflectionField{ "num", &Fraction::numerator },
        ReflectionField{ "den", &Fraction::denominator },
    };

    /**
     * @brief Unary plus operator.
     *
     * @return A copy of the fraction.
     */
    Fraction operator+() const {
        return *this;
    }

    /**
     * @brief Unary minus operator.
     *
     * @return A fraction with the same denominator but negated numerator.
     */
    Fraction operator-() const {
        return Fraction(-numerator, denominator);
    }

    /**
     * @brief Converts the fraction to a boolean.
     *
     * @return `true` if the numerator is non-zero, `false` otherwise.
     */
    explicit operator bool() const {
        return numerator != 0;
    }

    /**
     * @brief Converts the fraction to a double.
     *
     * @return The floating-point representation of the fraction.
     */
    explicit operator double() const {
        return static_cast<double>(numerator) / denominator;
    }

    /**
     * @brief Converts the fraction to a float.
     *
     * @return The floating-point representation of the fraction.
     */
    explicit operator float() const {
        return static_cast<float>(numerator) / denominator;
    }

    /**
     * @brief Converts the fraction to the template type T.
     *
     * @return The fractional value as type T.
     */
    explicit operator T() const {
        return static_cast<T>(numerator) / denominator;
    }

    /**
     * @brief Defines floating-point operations for the Fraction type.
     *
     * Provides operator overloads for arithmetic operations with float and double types.
     */
#define FLOATING_POINT_OP(op)                                                                                \
    friend float operator op(float x, const Fraction& y) {                                                   \
        return x op static_cast<float>(y);                                                                   \
    }                                                                                                        \
    friend double operator op(double x, const Fraction& y) {                                                 \
        return x op static_cast<double>(y);                                                                  \
    }                                                                                                        \
    friend float operator op(const Fraction& x, float y) {                                                   \
        return static_cast<float>(x) op y;                                                                   \
    }                                                                                                        \
    friend double operator op(const Fraction& x, double y) {                                                 \
        return static_cast<double>(x) op y;                                                                  \
    }                                                                                                        \
    friend Fraction operator op(const Fraction& x, int y) {                                                  \
        return x op Fraction(y);                                                                             \
    }                                                                                                        \
    friend Fraction operator op(int x, const Fraction& y) {                                                  \
        return Fraction(x) op y;                                                                             \
    }

    FLOATING_POINT_OP(+)
    FLOATING_POINT_OP(-)
    FLOATING_POINT_OP(*)
    FLOATING_POINT_OP(/)

#undef FLOATING_POINT_OP

    /**
     * @brief Adds two fractions.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return The result of adding the two fractions.
     */
    friend Fraction operator+(const Fraction& x, const Fraction& y) {
        return Fraction(x.numerator * y.denominator + y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }

    /**
     * @brief Subtracts one fraction from another.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return The result of subtracting the second fraction from the first.
     */
    friend Fraction operator-(const Fraction& x, const Fraction& y) {
        return Fraction(x.numerator * y.denominator - y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }

    /**
     * @brief Multiplies two fractions.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return The result of multiplying the two fractions.
     */
    friend Fraction operator*(const Fraction& x, const Fraction& y) {
        return Fraction(x.numerator * y.numerator, x.denominator * y.denominator);
    }

    /**
     * @brief Divides one fraction by another.
     *
     * @param x The numerator fraction.
     * @param y The denominator fraction.
     * @return The result of dividing the first fraction by the second.
     */
    friend Fraction operator/(const Fraction& x, const Fraction& y) {
        return Fraction(x.numerator * y.denominator, x.denominator * y.numerator);
    }

    /**
     * @brief Checks for equality between two fractions.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the fractions are equal, `false` otherwise.
     */
    friend bool operator==(const Fraction& x, const Fraction& y) {
        return x.numerator == y.numerator && x.denominator == y.denominator;
    }

    /**
     * @brief Checks for inequality between two fractions.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the fractions are not equal, `false` otherwise.
     */
    friend bool operator!=(const Fraction& x, const Fraction& y) {
        return !(operator==(x, y));
    }

    /**
     * @brief Checks if the first fraction is less than the second.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the first fraction is less than the second, `false` otherwise.
     */
    friend bool operator<(const Fraction& x, const Fraction& y) {
        return x.numerator * y.denominator < y.numerator * x.denominator;
    }

    /**
     * @brief Checks if the first fraction is less than or equal to the second.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the first fraction is less than or equal to the second, `false` otherwise.
     */
    friend bool operator<=(const Fraction& x, const Fraction& y) {
        return x.numerator * y.denominator <= y.numerator * x.denominator;
    }

    /**
     * @brief Checks if the first fraction is greater than the second.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the first fraction is greater than the second, `false` otherwise.
     */
    friend bool operator>(const Fraction& x, const Fraction& y) {
        return x.numerator * y.denominator > y.numerator * x.denominator;
    }

    /**
     * @brief Checks if the first fraction is greater than or equal to the second.
     *
     * @param x The first fraction.
     * @param y The second fraction.
     * @return `true` if the first fraction is greater than or equal to the second, `false` otherwise.
     */
    friend bool operator>=(const Fraction& x, const Fraction& y) {
        return x.numerator * y.denominator >= y.numerator * x.denominator;
    }

    /**
     * @brief Adds another fraction to this fraction.
     *
     * @param y The fraction to be added.
     * @return A reference to this fraction after addition.
     */
    Fraction& operator+=(const Fraction& y) {
        *this = *this + y;
        return *this;
    }

    /**
     * @brief Subtracts another fraction from this fraction.
     *
     * @param y The fraction to be subtracted.
     * @return A reference to this fraction after subtraction.
     */
    Fraction& operator-=(const Fraction& y) {
        *this = *this - y;
        return *this;
    }

    /**
     * @brief Multiplies this fraction by another fraction.
     *
     * @param y The fraction to multiply by.
     * @return A reference to this fraction after multiplication.
     */
    Fraction& operator*=(const Fraction& y) {
        *this = *this * y;
        return *this;
    }

    /**
     * @brief Divides this fraction by another fraction.
     *
     * @param y The fraction to divide by.
     * @return A reference to this fraction after division.
     */
    Fraction& operator/=(const Fraction& y) {
        *this = *this / y;
        return *this;
    }

private:
    /**
     * @brief Computes the greatest common divisor of two values.
     *
     * @param a The first value.
     * @param b The second value.
     * @return The greatest common divisor of the two values.
     */
    static T gcd(T a, T b) {
        T r;
        while (b > 0) {
            r = a % b;
            a = b;
            b = r;
        }
        return a;
    }

    /**
     * @brief Computes the least common multiple of two values.
     *
     * @param a The first value.
     * @param b The second value.
     * @return The least common multiple of the two values.
     */
    static T lcm(T a, T b) {
        return abs(a * b) / gcd(a, b);
    }
};

/** @brief Defines optional operators for arithmetic operations. */
#define OPTIONAL_OPERATOR(op)                                                                                \
    template <typename T1, typename T2>                                                                      \
    inline std::optional<decltype(std::declval<T1>() op std::declval<T2>())> operator op(                    \
        const std::optional<T1>& x, const std::optional<T2>& y) {                                            \
        if (x && y)                                                                                          \
            return *x op * y;                                                                                \
        return std::nullopt;                                                                                 \
    }                                                                                                        \
    template <typename T1, typename T2>                                                                      \
    inline std::optional<decltype(std::declval<T1>() op std::declval<T2>())> operator op(                    \
        const T1 & x, const std::optional<T2>& y) {                                                          \
        if (y)                                                                                               \
            return x op * y;                                                                                 \
        return std::nullopt;                                                                                 \
    }                                                                                                        \
    template <typename T1, typename T2>                                                                      \
    inline std::optional<decltype(std::declval<T1>() op std::declval<T2>())> operator op(                    \
        const std::optional<T1>& x, const T2 & y) {                                                          \
        if (x)                                                                                               \
            return *x op y;                                                                                  \
        return std::nullopt;                                                                                 \
    }

OPTIONAL_OPERATOR(+)
OPTIONAL_OPERATOR(-)
OPTIONAL_OPERATOR(*)
OPTIONAL_OPERATOR(/)

#undef OPTIONAL_OPERATOR

/**
 * @brief Computes the square of a value.
 *
 * @tparam T The type of the value. Must support multiplication.
 * @param x The value to be squared.
 * @return The square of the given value.
 */
template <typename T>
constexpr T sqr(T x) noexcept {
    return x * x;
}

/**
 * @brief Converts degrees to radians.
 *
 * @tparam T The numeric type.
 */
template <typename T>
constexpr inline T deg2rad = std::numbers::pi_v<T> / T(180);

/**
 * @brief Converts radians to degrees.
 *
 * @tparam T The numeric type.
 */
template <typename T>
constexpr inline T rad2deg = T(180) / std::numbers::pi_v<T>;

/**
 * @brief Performs linear interpolation between two values.
 *
 * @tparam T The type of the values.
 * @param t The interpolation factor, ranging from 0 to 1.
 * @param x The starting value.
 * @param y The ending value.
 * @return The interpolated value between `x` and `y`.
 */
template <typename T>
constexpr T mix(float t, T x, T y) {
    return x * (1 - t) + y * t;
}

/**
 * @brief Computes the fractional part of a floating-point value.
 *
 * @tparam T A floating-point type.
 * @param x The value to be processed.
 * @return The fractional part of the given value.
 */
template <std::floating_point T>
constexpr T fract(T x) {
    return x - std::floor(x);
}

} // namespace Brisk
