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
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * For commercial licensing, please visit: https://brisklib.com/
 */
#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace Brisk {
namespace Internal {

/**
 * @brief A compile-time fixed-size string class.
 *
 * This class provides a way to work with fixed-size strings during compile-time.
 * The string's size is determined at compile time, and it is stored in an array of characters.
 *
 * @tparam N The size of the string (not including the null terminator).
 */
template <size_t N>
struct FixedString {
    /**
     * @brief Constructs a FixedString from a string literal.
     *
     * This constructor takes a string literal and initializes the FixedString
     * with its content. The size of the string must match the template parameter N.
     *
     * @param str A string literal to initialize the FixedString.
     */
    constexpr FixedString(const char (&str)[N + 1]) {
        std::copy_n(str, N, content.begin());
    }

    /**
     * @brief Constructs a FixedString from a pointer to a character array.
     *
     * This constructor is explicit and allows initialization from a character
     * pointer. The size of the string must still match the template parameter N.
     *
     * @param str A pointer to a null-terminated character array.
     * @param dummy A dummy parameter to disambiguate this constructor.
     */
    constexpr explicit FixedString(const char* str, int dummy) {
        std::copy_n(str, N, content.begin());
    }

    /**
     * @brief Returns a string view of the FixedString content.
     *
     * This function provides a view of the string content as a std::string_view.
     *
     * @return A std::string_view representing the string content.
     */
    constexpr std::string_view string() const {
        return std::string_view{ content.data(), content.data() + N };
    }

    /// The underlying storage for the string content.
    std::array<char, N> content;

    /**
     * @brief Compares two FixedStrings for ordering.
     *
     * This operator is defaulted to provide a three-way comparison based on
     * the lexicographical order of the string contents.
     *
     * @param other The other FixedString to compare against.
     * @return A comparison result.
     */
    auto operator<=>(const FixedString&) const noexcept = default;
};

/**
 * @brief Deduction guide for creating FixedString from string literals.
 *
 * This deduction guide allows the compiler to automatically deduce the
 * template parameter N when initializing a FixedString with a string literal.
 *
 * @param str A string literal.
 */
template <size_t N>
FixedString(const char (&)[N]) -> FixedString<N - 1>;

} // namespace Internal
} // namespace Brisk
