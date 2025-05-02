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
 */
#pragma once

#include "BasicTypes.hpp"
#include <string_view>
#include "Encoding.hpp"
#include <brisk/core/internal/FixedString.hpp>
#include <brisk/core/Encoding.hpp>

namespace Brisk {

/**
 * @brief Enum representing case transformation modes.
 */
enum class CaseTransformation {
    Lower, ///< Transform to lowercase.
    Upper, ///< Transform to uppercase.
};

/**
 * @brief Transforms the case of a UTF-8 string based on the specified mode.
 *
 * @param str The input UTF-8 string to transform.
 * @param mode The case transformation mode (lower or upper).
 * @return Transformed string in the specified case.
 */
U8String transformCase(U8StringView str, CaseTransformation mode);

/**
 * @brief Transforms the case of a UTF-16 string based on the specified mode.
 *
 * @param str The input UTF-16 string to transform.
 * @param mode The case transformation mode (lower or upper).
 * @return Transformed string in the specified case.
 */
U16String transformCase(U16StringView str, CaseTransformation mode);

/**
 * @brief Transforms the case of a UTF-32 string based on the specified mode.
 *
 * @param str The input UTF-32 string to transform.
 * @param mode The case transformation mode (lower or upper).
 * @return Transformed string in the specified case.
 */
U32String transformCase(U32StringView str, CaseTransformation mode);

/**
 * @brief Converts a UTF-8 string to lowercase.
 *
 * @param str The input UTF-8 string to convert.
 * @return The converted lowercase string.
 */
inline U8String lowerCase(U8StringView str) {
    return transformCase(str, CaseTransformation::Lower);
}

/**
 * @brief Converts a UTF-8 string to uppercase.
 *
 * @param str The input UTF-8 string to convert.
 * @return The converted uppercase string.
 */
inline U8String upperCase(U8StringView str) {
    return transformCase(str, CaseTransformation::Upper);
}

/**
 * @brief Converts a UTF-16 string to lowercase.
 *
 * @param str The input UTF-16 string to convert.
 * @return The converted lowercase string.
 */
inline U16String lowerCase(U16StringView str) {
    return transformCase(str, CaseTransformation::Lower);
}

/**
 * @brief Converts a UTF-16 string to uppercase.
 *
 * @param str The input UTF-16 string to convert.
 * @return The converted uppercase string.
 */
inline U16String upperCase(U16StringView str) {
    return transformCase(str, CaseTransformation::Upper);
}

/**
 * @brief Converts a UTF-32 string to lowercase.
 *
 * @param str The input UTF-32 string to convert.
 * @return The converted lowercase string.
 */
inline U32String lowerCase(U32StringView str) {
    return transformCase(str, CaseTransformation::Lower);
}

/**
 * @brief Converts a UTF-32 string to uppercase.
 *
 * @param str The input UTF-32 string to convert.
 * @return The converted uppercase string.
 */
inline U32String upperCase(U32StringView str) {
    return transformCase(str, CaseTransformation::Upper);
}

/**
 * @brief Word-wraps a string to the specified number of columns.
 *
 * @param text The input string to wrap.
 * @param columns The number of columns for wrapping.
 * @return The word-wrapped string.
 */
std::string wordWrap(std::string text, size_t columns);

/**
 * @brief Splits a string into a list of substrings using a delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string to split.
 * @param delimiter The delimiter used to split the string.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(
    std::basic_string_view<Char> text, std::basic_string_view<std::type_identity_t<Char>> delimiter) {
    size_t prev_pos  = 0;
    size_t start_pos = 0;
    std::vector<std::basic_string_view<Char>> list;
    while ((start_pos = text.find(delimiter, prev_pos)) != std::basic_string_view<Char>::npos) {
        list.push_back(std::basic_string_view<Char>(text.substr(prev_pos, start_pos - prev_pos)));
        prev_pos = start_pos + delimiter.size();
    }
    list.push_back(std::basic_string_view<Char>(text.substr(prev_pos)));
    return list;
}

/**
 * @brief Splits a string into a list of substrings using a single character as a delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string to split.
 * @param delimiter The delimiter character used to split the string.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(std::basic_string_view<Char> text,
                                                std::type_identity_t<Char> delimiter) {
    std::basic_string_view<Char> r = text;
    size_t prev_pos                = 0;
    size_t start_pos               = 0;
    std::vector<std::basic_string_view<Char>> list;
    while ((start_pos = r.find(delimiter, prev_pos)) != std::basic_string_view<Char>::npos) {
        list.push_back(text.substr(prev_pos, start_pos - prev_pos));
        prev_pos = start_pos + 1;
    }
    list.push_back(text.substr(prev_pos));
    return list;
}

/**
 * @brief Splits a basic string into a list of substrings using a delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string to split.
 * @param delimiter The string delimiter used to split the text.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(
    const std::basic_string<Char>& text, std::basic_string_view<std::type_identity_t<Char>> delimiter) {
    return split(std::basic_string_view<Char>(text), delimiter);
}

/**
 * @brief Splits a basic string into a list of substrings using a single character as a delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string to split.
 * @param delimiter The delimiter character used to split the text.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(const std::basic_string<Char>& text,
                                                std::type_identity_t<Char> delimiter) {
    return split(std::basic_string_view<Char>(text), delimiter);
}

/**
 * @brief Splits a character array (C-string) into a list of substrings using a string delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input C-string to split.
 * @param delimiter The string delimiter used to split the text.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(
    const Char* text, std::basic_string_view<std::type_identity_t<Char>> delimiter) {
    return split(std::basic_string_view<Char>(text), delimiter);
}

/**
 * @brief Splits a character array (C-string) into a list of substrings using a single character as a
 * delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input C-string to split.
 * @param delimiter The delimiter character used to split the text.
 * @return A vector of string views representing the split substrings.
 */
template <typename Char>
std::vector<std::basic_string_view<Char>> split(Char* text, std::type_identity_t<Char> delimiter) {
    return split(std::basic_string_view<Char>(text), delimiter);
}

/**
 * @brief Converts a container of elements to a vector of basic strings.
 *
 * @tparam Char The character type for the output strings.
 * @tparam T The container type.
 * @param value The container with elements to convert.
 * @return A vector of basic strings converted from the container elements.
 */
template <typename Char = char, typename T>
std::vector<std::basic_string<Char>> toStrings(T&& value) {
    std::vector<std::basic_string<Char>> result(value.begin(), value.end());
    return result;
}

/**
 * @brief Joins a list of strings into a single string with a specified delimiter.
 *
 * @tparam Char The character type of the strings.
 * @param list The list of strings to join.
 * @param delimiter The delimiter used to join the strings.
 * @return A single string with the elements joined by the delimiter.
 */
template <typename Char>
std::basic_string<Char> join(const std::vector<std::basic_string<Char>>& list,
                             std::basic_string_view<std::type_identity_t<Char>> delimiter) {
    std::basic_string<Char> result;
    bool first = true;
    for (const std::basic_string<Char>& t : list) {
        if (!first)
            result += delimiter;
        result += t;
        first = false;
    }
    return result;
}

/**
 * @brief Joins a list of basic strings into a single string with a character delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param list A vector of basic strings to join.
 * @param delimiter The character used to separate the joined strings.
 * @return A single string with the individual strings joined by the delimiter.
 */
template <typename Char>
std::basic_string<Char> join(const std::vector<std::basic_string<Char>>& list,
                             std::type_identity_t<Char> delimiter) {
    std::basic_string<Char> result;
    bool first = true;
    for (const std::basic_string<Char>& t : list) {
        if (!first)
            result += delimiter;
        result += t;
        first = false;
    }
    return result;
}

/**
 * @brief Joins a list of basic string views into a single string with a string delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param list A vector of basic string views to join.
 * @param delimiter The string delimiter used to separate the joined string views.
 * @return A single string with the individual string views joined by the delimiter.
 */
template <typename Char>
std::basic_string<Char> join(const std::vector<std::basic_string_view<Char>>& list,
                             std::basic_string_view<std::type_identity_t<Char>> delimiter) {
    std::basic_string<Char> result;
    bool first = true;
    for (std::basic_string_view<Char> t : list) {
        if (!first)
            result += delimiter;
        result += t;
        first = false;
    }
    return result;
}

/**
 * @brief Joins a list of basic string views into a single string with a character delimiter.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param list A vector of basic string views to join.
 * @param delimiter The character used to separate the joined string views.
 * @return A single string with the individual string views joined by the delimiter.
 */
template <typename Char>
std::basic_string<Char> join(const std::vector<std::basic_string_view<Char>>& list,
                             std::type_identity_t<Char> delimiter) {
    std::basic_string<Char> result;
    bool first = true;
    for (std::basic_string_view<Char> t : list) {
        if (!first)
            result += delimiter;
        result += t;
        first = false;
    }
    return result;
}

/**
 * @brief Replaces all occurrences of a substring in a string with another substring.
 *
 * @tparam Char The character type of the string.
 * @param str The string in which to perform replacements.
 * @param from The substring to be replaced.
 * @param to The substring to replace with.
 * @return The string with all replacements applied.
 */
template <typename Char>
std::basic_string<Char> replaceAll(std::basic_string<Char> str,
                                   std::basic_string_view<std::type_identity_t<Char>> from,
                                   std::basic_string_view<std::type_identity_t<Char>> to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

/**
 * @brief Replaces all occurrences of a character in a basic string with another character.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param str The input string where replacements are made.
 * @param from The character to replace.
 * @param to The character to replace with.
 * @return A new string with all occurrences of the 'from' character replaced by the 'to' character.
 */
template <typename Char>
std::basic_string<Char> replaceAll(std::basic_string<Char> str, std::type_identity_t<Char> from,
                                   const std::type_identity_t<Char>& to) {
    for (Char& c : str) {
        if (c == from)
            c = to;
    }
    return str;
}

/**
 * @brief Replaces all occurrences of a substring in a basic string view with another substring.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string view where replacements are made.
 * @param from The substring to replace.
 * @param to The substring to replace with.
 * @return A new string with all occurrences of the 'from' substring replaced by the 'to' substring.
 */
template <typename Char>
std::basic_string<Char> replaceAll(std::basic_string_view<Char> text,
                                   std::basic_string_view<std::type_identity_t<Char>> from,
                                   std::basic_string_view<std::type_identity_t<Char>> to) {
    return replaceAll(std::basic_string<Char>(text), from, to);
}

/**
 * @brief Replaces all occurrences of a character in a basic string view with another character.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input string view where replacements are made.
 * @param from The character to replace.
 * @param to The character to replace with.
 * @return A new string with all occurrences of the 'from' character replaced by the 'to' character.
 */
template <typename Char>
std::basic_string<Char> replaceAll(std::basic_string_view<Char> text, std::type_identity_t<Char> from,
                                   std::type_identity_t<Char> to) {
    return replaceAll(std::basic_string<Char>(text), from, to);
}

/**
 * @brief Replaces all occurrences of a substring in a C-string with another substring.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input C-string where replacements are made.
 * @param from The substring to replace.
 * @param to The substring to replace with.
 * @return A new string with all occurrences of the 'from' substring replaced by the 'to' substring.
 */
template <typename Char>
std::basic_string<Char> replaceAll(const Char* text, std::basic_string_view<std::type_identity_t<Char>> from,
                                   std::basic_string_view<std::type_identity_t<Char>> to) {
    return replaceAll(std::basic_string<Char>(text), from, to);
}

/**
 * @brief Replaces all occurrences of a character in a C-string with another character.
 *
 * @tparam Char The character type (e.g., char, wchar_t).
 * @param text The input C-string where replacements are made.
 * @param from The character to replace.
 * @param to The character to replace with.
 * @return A new string with all occurrences of the 'from' character replaced by the 'to' character.
 */
template <typename Char>
std::basic_string<Char> replaceAll(Char* text, std::type_identity_t<Char> from,
                                   std::type_identity_t<Char> to) {
    return replaceAll(std::basic_string<Char>(text), from, to);
}

/**
 * @brief Checks if a string contains a specific substring.
 *
 * @tparam Char The character type of the string.
 * @param str The string to search in.
 * @param substr The substring to search for.
 * @return true if the substring is found, false otherwise.
 */
template <typename Char>
inline bool contains(const std::basic_string<Char>& str,
                     const std::basic_string<std::type_identity_t<Char>>& substr) {
    return str.size() >= substr.size() && str.find(substr) != std::basic_string<Char>::npos;
}

/**
 * @brief Trims whitespace from the left side of a string.
 *
 * @param s The string to trim.
 * @return The left-trimmed string.
 */
std::string ltrim(std::string s);

/**
 * @brief Trims whitespace from the right side of a string.
 *
 * @param s The string to trim.
 * @return The right-trimmed string.
 */
std::string rtrim(std::string s);

/**
 * @brief Trims whitespace from both sides of a string.
 *
 * @param s The string to trim.
 * @return The trimmed string.
 */
std::string trim(std::string s);

/**
 * @brief Trims whitespace from the left side of a string view.
 *
 * @param s The string view to trim.
 * @return The left-trimmed string view.
 */
std::string_view ltrim(std::string_view s);

/**
 * @brief Trims whitespace from the right side of a string view.
 *
 * @param s The string view to trim.
 * @return The right-trimmed string view.
 */
std::string_view rtrim(std::string_view s);

/**
 * @brief Trims whitespace from both sides of a string view.
 *
 * @param s The string view to trim.
 * @return The trimmed string view.
 */
std::string_view trim(std::string_view s);

inline std::string_view ltrim(const char* s) {
    return ltrim(std::string_view(s));
}

inline std::string_view rtrim(const char* s) {
    return rtrim(std::string_view(s));
}

inline std::string_view trim(const char* s) {
    return trim(std::string_view(s));
}

/**
 * @brief Splits a string view into two substrings using a delimiter.
 *
 * @param text The input string to split.
 * @param delimiter The delimiter used to split the string.
 * @param s1 Output string view for the first part.
 * @param s2 Output string view for the second part.
 */
void split(std::string_view text, std::string_view delimiter, std::string_view& s1, std::string_view& s2);

inline void split(std::string_view text, std::string_view delimiter, std::string& s1, std::string& s2) {
    std::string_view sv1, sv2;
    split(text, delimiter, sv1, sv2);
    s1 = sv1;
    s2 = sv2;
}

/**
 * @brief Shortens a string to a specified maximum length.
 *
 * @param str The string to shorten.
 * @param maxLength The maximum length of the output string in unicode codepoints.
 * @param position Where to cut the string: left (0), right (1), or middle (0.5).
 * @param ellipsis The ellipsis string to append if shortening occurs.
 * @return The shortened string.
 */
std::u32string shorten(std::u32string str, size_t maxLength, float position,
                       std::u32string_view ellipsis = U"…");

/**
 * @brief Shortens a UTF-8 string to a specified maximum length.
 *
 * @param str The UTF-8 string to shorten.
 * @param maxLength The maximum length of the output string in unicode codepoints.
 * @param position Where to cut the string: left (0), right (1), or middle (0.5).
 * @param ellipsis The ellipsis string to append if shortening occurs.
 * @return The shortened UTF-8 string.
 */
inline std::string shorten(const std::string& str, size_t maxLength, float position,
                           std::u32string_view ellipsis = U"…") {
    return utf32ToUtf8(shorten(utf8ToUtf32(str), maxLength, position, ellipsis));
}

/**
 * @struct FormatString
 * @brief A compile-time format string wrapper for type-safe string formatting.
 * @tparam key A FixedString representing the format string.
 */
template <Internal::FixedString key>
struct FormatString {
    /**
     * @brief Checks if the format string is valid for the given argument types.
     * @tparam Args Variadic template for format argument types.
     * @throws Compile-time error if the format string is invalid for the arguments.
     */
    template <typename... Args>
    static consteval void checkFormatArgs() {
        [[maybe_unused]] fmt::format_string<Args...> fmt(key.string());
    }

    /**
     * @brief Formats the string using the provided arguments.
     * @tparam Args Variadic template for format argument types.
     * @param args Arguments to be formatted into the string.
     * @return A formatted string based on the key and arguments.
     */
    template <typename... Args>
    std::string operator()(Args&&... args) const {
        checkFormatArgs<Args...>();
        return fmt::format(key.string(), std::forward<Args>(args)...);
    }
};

/**
 * @brief User-defined literal operator to create a FormatString from a string literal.
 * @tparam s The FixedString created from the string literal.
 * @return A FormatString object for the given string literal.
 * @code
 * std::string str = "Hello, {}!"_fmt("World"); // Produces "Hello, World!"
 * @endcode
 */
template <Internal::FixedString s>
constexpr FormatString<s> operator""_fmt() noexcept {
    return {};
}

} // namespace Brisk
