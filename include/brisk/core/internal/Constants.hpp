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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */
#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <limits>

namespace Brisk {

/**
 * @brief A template alias for std::integral_constant.
 *
 * This alias allows defining compile-time constants of any type.
 *
 * @tparam T The type of the constant.
 * @tparam N The value of the constant.
 */
template <typename T, T N>
using constant = std::integral_constant<T, N>;

/**
 * @brief A template alias for a size_t integral constant.
 *
 * This alias is a shorthand for creating compile-time constants of type size_t.
 *
 * @tparam N The value of the constant.
 */
template <size_t N>
using size_constant = constant<size_t, N>;

/**
 * @brief A template alias for std::integer_sequence.
 *
 * This alias allows the creation of sequences of compile-time integers.
 *
 * @tparam T The type of the sequence elements.
 * @tparam N The values of the sequence elements.
 */
template <typename T, T... N>
using constants = std::integer_sequence<T, N...>;

/**
 * @brief A template alias for creating a size_t integer sequence.
 *
 * This alias is a shorthand for creating sequences of size_t values.
 *
 * @tparam N The values of the sequence elements.
 */
template <size_t... N>
using size_constants = constants<size_t, N...>;

/**
 * @brief A template alias for creating an integer sequence.
 *
 * This alias uses std::make_integer_sequence to create a sequence of integers.
 *
 * @tparam T The type of the sequence elements.
 * @tparam N The number of elements in the sequence.
 */
template <typename T, size_t N>
using sequence = std::make_integer_sequence<T, N>;

/**
 * @brief A template alias for creating a size_t integer sequence.
 *
 * This alias is a shorthand for creating a sequence of size_t values.
 *
 * @tparam N The number of elements in the sequence.
 */
template <size_t N>
using size_sequence = sequence<size_t, N>;

/**
 * @brief Adds a constant to a sequence of constants.
 *
 * This operator overload allows the addition of a constant to a sequence of constants.
 *
 * @param lhs A sequence of constants.
 * @param rhs A constant to add.
 * @return A new sequence of constants resulting from the addition.
 */
template <typename T, T... x, T y>
constexpr auto operator+(constants<T, x...>, constant<T, y>) noexcept -> constants<T, (x + y)...> {
    return {};
}

/**
 * @brief Subtracts a constant from a sequence of constants.
 *
 * This operator overload allows the subtraction of a constant from a sequence of constants.
 *
 * @param lhs A sequence of constants.
 * @param rhs A constant to subtract.
 * @return A new sequence of constants resulting from the subtraction.
 */
template <typename T, T... x, T y>
constexpr auto operator-(constants<T, x...>, constant<T, y>) noexcept -> constants<T, (x - y)...> {
    return {};
}

/**
 * @brief Bitwise XORs a constant with a sequence of constants.
 *
 * This operator overload allows bitwise XOR of a constant with a sequence of constants.
 *
 * @param lhs A sequence of constants.
 * @param rhs A constant to XOR.
 * @return A new sequence of constants resulting from the bitwise XOR.
 */
template <typename T, T... x, T y>
constexpr auto operator^(constants<T, x...>, constant<T, y>) noexcept -> constants<T, (x ^ y)...> {
    return {};
}

/**
 * @brief Computes the modulus of a sequence of constants by a constant.
 *
 * This operator overload allows computing the modulus of a sequence of constants by a constant.
 *
 * @param lhs A sequence of constants.
 * @param rhs A constant to compute the modulus with.
 * @return A new sequence of constants resulting from the modulus operation.
 */
template <typename T, T... x, T y>
constexpr auto operator%(constants<T, x...>, constant<T, y>) noexcept -> constants<T, (x % y)...> {
    return {};
}

/**
 * @brief Adds a constant to a sequence of constants (reverse order).
 *
 * This operator overload allows the addition of a constant to a sequence of constants
 * with the constant on the left-hand side.
 *
 * @param lhs A constant to add.
 * @param rhs A sequence of constants.
 * @return A new sequence of constants resulting from the addition.
 */
template <typename T, T x, T... y>
constexpr auto operator+(constant<T, x>, constants<T, y...>) noexcept -> constants<T, (x + y)...> {
    return {};
}

/**
 * @brief Subtracts a sequence of constants from a constant (reverse order).
 *
 * This operator overload allows the subtraction of a sequence of constants from a constant
 * with the constant on the left-hand side.
 *
 * @param lhs A constant to subtract from.
 * @param rhs A sequence of constants.
 * @return A new sequence of constants resulting from the subtraction.
 */
template <typename T, T x, T... y>
constexpr auto operator-(constant<T, x>, constants<T, y...>) noexcept -> constants<T, (x - y)...> {
    return {};
}

/**
 * @brief Bitwise XORs a sequence of constants with a constant (reverse order).
 *
 * This operator overload allows bitwise XOR of a sequence of constants with a constant
 * with the constant on the left-hand side.
 *
 * @param lhs A constant to XOR with.
 * @param rhs A sequence of constants.
 * @return A new sequence of constants resulting from the bitwise XOR.
 */
template <typename T, T x, T... y>
constexpr auto operator^(constant<T, x>, constants<T, y...>) noexcept -> constants<T, (x ^ y)...> {
    return {};
}

/**
 * @brief Computes the modulus of a constant by a sequence of constants (reverse order).
 *
 * This operator overload allows computing the modulus of a constant by a sequence of constants
 * with the constant on the left-hand side.
 *
 * @param lhs A constant to compute the modulus of.
 * @param rhs A sequence of constants.
 * @return A new sequence of constants resulting from the modulus operation.
 */
template <typename T, T x, T... y>
constexpr auto operator%(constant<T, x>, constants<T, y...>) noexcept -> constants<T, (x % y)...> {
    return {};
}

namespace Internal {

/**
 * @brief Finds the appropriate integral type based on the given range.
 *
 * This helper function determines the smallest integral type that can
 * represent the range [Min, Max].
 *
 * @tparam Min The minimum value of the range.
 * @tparam Max The maximum value of the range.
 * @return The integral type that can represent the specified range.
 */
template <std::integral auto Min, std::integral auto Max>
constexpr auto findIntegralType() {
    if constexpr (std::cmp_less(Min, 0)) {
        // signed
        constexpr auto AbsMax = std::cmp_greater_equal(-(Min + 1), Max) ? -(Min + 1) : Max;
        if constexpr (std::cmp_less_equal(AbsMax, int8_t(INT8_MAX))) {
            return int8_t{};
        } else if constexpr (std::cmp_less_equal(AbsMax, INT16_MAX)) {
            return int16_t{};
        } else if constexpr (std::cmp_less_equal(AbsMax, INT32_MAX)) {
            return int32_t{};
        } else {
            return int64_t{};
        }
    } else {
        // unsigned
        if constexpr (std::cmp_less_equal(Max, UINT8_MAX)) {
            return uint8_t{};
        } else if constexpr (std::cmp_less_equal(Max, UINT16_MAX)) {
            return uint16_t{};
        } else if constexpr (std::cmp_less_equal(Max, UINT32_MAX)) {
            return uint32_t{};
        } else {
            return uint64_t{};
        }
    }
}

template <std::integral Tout, std::integral Tin>
constexpr bool fitsIntType(Tin in) {
    return std::cmp_less_equal(in, std::numeric_limits<Tout>::max()) &&
           std::cmp_greater_equal(in, std::numeric_limits<Tout>::min());
}

} // namespace Internal

/**
 * @brief Finds the integral type that can represent the given range.
 *
 * This alias evaluates to the integral type that can safely represent the values
 * in the range [Min, Max].
 *
 * @tparam Min The minimum value of the range.
 * @tparam Max The maximum value of the range.
 */
template <auto Min, auto Max>
using findIntegralType = decltype(Internal::findIntegralType<Min, Max>());

} // namespace Brisk
