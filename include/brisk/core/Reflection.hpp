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

#include <string_view>
#include <tuple>
#include <type_traits>
#include <iostream>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/ostream.h>
#include "internal/Constants.hpp"
#include "internal/Span.hpp"

namespace Brisk {

/**
 * @brief Enum representing flags for reflection metadata.
 */
enum class ReflectionFlag : int {
    Default           = 0,      /**< No specific flag (default). */
    SkipSerialization = 1 << 0, /**< Skip field during serialization. */
    SkipPrinting      = 1 << 1, /**< Skip field during printing. */
};

/**
 * @brief Converts a `ReflectionFlag` to its underlying integer type.
 *
 * @param x The ReflectionFlag to convert.
 * @return The underlying integer value of the flag.
 */
constexpr std::underlying_type_t<ReflectionFlag> operator+(ReflectionFlag x) {
    return static_cast<std::underlying_type_t<ReflectionFlag>>(x);
}

/**
 * @brief Checks if a flag is set in a combination of `ReflectionFlag` values.
 *
 * @param flags The combination of flags.
 * @param flag The flag to check.
 * @return true if the flag is set, false otherwise.
 */
constexpr bool operator&&(ReflectionFlag flags, ReflectionFlag flag) {
    return (+flags & +flag) != 0;
}

/**
 * @brief Combines two `ReflectionFlag` values using bitwise OR.
 *
 * @param x The first flag.
 * @param y The second flag.
 * @return The combined flag result.
 */
constexpr ReflectionFlag operator|(ReflectionFlag x, ReflectionFlag y) {
    return static_cast<ReflectionFlag>(+x | +y);
}

/**
 * @brief Represents metadata for a reflected field in a class.
 *
 * @tparam Class_ The class type containing the field.
 * @tparam FieldType_ The type of the field being reflected.
 */
template <typename Class_, typename FieldType_>
struct ReflectionField {
    using Class     = Class_;     /**< The class type. */
    using FieldType = FieldType_; /**< The field type. */

    std::string_view name;                          /**< Name of the field. */
    FieldType Class::* pointerToField;              /**< Pointer to the field in the class. */
    ReflectionFlag flags = ReflectionFlag::Default; /**< Reflection flags for the field. */
};

/**
 * @brief Deduction guide for constructing `ReflectionField` without flags.
 *
 * @tparam N Size of the field name.
 * @tparam Class The class type containing the field.
 * @tparam FieldType The type of the field being reflected.
 */
template <size_t N, typename Class, typename FieldType>
ReflectionField(const char (&)[N], FieldType Class::*) -> ReflectionField<Class, FieldType>;

/**
 * @brief Deduction guide for constructing `ReflectionField` with flags.
 *
 * @tparam N Size of the field name.
 * @tparam Class The class type containing the field.
 * @tparam FieldType The type of the field being reflected.
 */
template <size_t N, typename Class, typename FieldType>
ReflectionField(const char (&)[N], FieldType Class::*, ReflectionFlag) -> ReflectionField<Class, FieldType>;

template <typename T>
struct ReflectionOf {
    constexpr static bool hasReflection = false;
};

template <typename T>
    requires std::is_class_v<std::remove_cvref_t<T>> && requires { std::remove_cvref_t<T>::reflection; }
struct ReflectionOf<T> {
    constexpr static bool hasReflection = true;

    static decltype(auto) reflection() {
        return std::remove_cvref_t<T>::reflection;
    }
};

/**
 * @brief Concept that checks if a type `T` has reflection metadata.
 */
template <typename T>
concept HasReflection = ReflectionOf<T>::hasReflection;

/**
 * @brief Retrieves the reflection metadata of a type `T`.
 *
 * @tparam T The type with reflection metadata.
 * @return The reflection metadata tuple.
 */
template <HasReflection T>
constexpr decltype(auto) reflectionOf() {
    return ReflectionOf<T>::reflection();
}

/**
 * @brief Retrieves the reflection metadata of an object of type `T`.
 *
 * @tparam T The type with reflection metadata.
 * @param obj The object whose reflection metadata is retrieved.
 * @return The reflection metadata tuple.
 */
template <HasReflection T>
constexpr decltype(auto) reflectionOf(T&& obj) {
    return ReflectionOf<T>::reflection();
}

/**
 * @brief Iterates over each reflected field of a type `T` and applies a function to each field.
 *
 * @tparam T The type with reflection metadata.
 * @tparam Fn The function type to apply to each field.
 * @param fn The function to apply to each reflected field.
 */
template <HasReflection T, typename Fn>
constexpr void forEachField(Fn&& fn) {
    decltype(auto) reflection = reflectionOf<T>();
    std::apply(
        [&fn](auto&&... field) {
            (static_cast<void>(fn(field)), ...);
        },
        reflection);
}

template <HasReflection T, size_t index>
using FieldType =
    std::remove_reference_t<decltype(std::declval<T>().*std::get<index>(reflectionOf<T>()).pointerToField)>;

template <size_t index, HasReflection T>
constexpr decltype(auto) accessField(T&& obj) {
    return obj.*(std::get<index>(reflectionOf(std::forward<T>(obj))).pointerToField);
}

/**
 * @brief Returns the number of enum values based on the last enum value.
 *
 * @tparam T The enum type.
 * @tparam Last The last enum value (default to T::Last).
 */
template <typename T, T Last = T::Last>
inline constexpr size_t enumSize = static_cast<size_t>(static_cast<std::underlying_type_t<T>>(Last)) + 1;

/**
 * @brief Looks up a value in an array by its corresponding enum value.
 *
 * @tparam T The type of the array elements.
 * @tparam N The size of the array.
 * @tparam E The enum type.
 * @param array The array to search.
 * @param value The enum value to look up.
 * @param fallback The fallback value if the enum value is out of range.
 * @return The corresponding value from the array or the fallback value if not found.
 */
template <typename T, size_t N, typename E>
inline T lookupByEnum(T (&array)[N], E value, T fallback = T{}) {
    size_t index = static_cast<size_t>(static_cast<std::underlying_type_t<E>>(value));
    if (index >= N)
        return std::move(fallback);
    return array[index];
}

/**
 * @brief Placeholder for default names of an enum type.
 *
 * @tparam T The enum type.
 */
template <typename T>
inline constexpr std::nullptr_t defaultNames{};

template <typename T>
concept HasDefaultNames = requires() {
    std::begin(defaultNames<T>);
    std::end(defaultNames<T>);
};

/**
 * @brief Represents a name-value pair for an enum.
 *
 * @tparam T The type of the value.
 */
template <typename T>
using NameValuePair = std::pair<std::string_view, T>;

/**
 * @brief Represents a span of name-value pairs for an enum.
 *
 * @tparam T The type of the values.
 */
template <typename T>
using NameValuePairs = std::span<NameValuePair<T>>;

/**
 * @brief Converts an enum value to its string representation using default names.
 *
 * @tparam T The enum type.
 * @param value The enum value to convert.
 * @return The corresponding string name of the enum value, or "(unknown)" if not found.
 */
template <HasDefaultNames T>
constexpr std::string_view defaultToString(T value) {
    for (const std::pair<std::string_view, T>& kv : defaultNames<T>) {
        if (kv.second == value)
            return kv.first;
    }
    return "(unknown)";
}

/**
 * @brief Macro to define a reflection field for a class member.
 *
 * @param f The class member field.
 */
#define BRISK_REFLECTION_FIELD(f)                                                                            \
    ReflectionField {                                                                                        \
        #f, &This::f                                                                                         \
    }

namespace Internal {

template <typename T>
inline const T& reflectQ(const T& x) {
    return x;
}

inline std::string reflectQ(const std::string& x) {
    return "\"" + x + "\"";
}

template <typename Char, size_t... indices, typename T, typename Class, typename... FieldType>
std::basic_string<Char> reflectFormat(size_constants<indices...>, const T& val,
                                      const std::tuple<ReflectionField<Class, FieldType>...>& fields) {
    auto out = fmt::basic_memory_buffer<Char>();

    ((fmt::format_to(std::back_inserter(out), "{}:{},", std::get<indices>(fields).name,
                     reflectQ(val.*(std::get<indices>(fields).pointerToField))),
      void()),
     ...);
    std::basic_string<Char> str = fmt::to_string(out);
    if (str.empty())
        str = "{}";
    else
        str = "{" + str.substr(0, str.size() - 1) + "}";
    return str;
}
} // namespace Internal

/**
 * @brief Formats an object's fields using reflection and returns a formatted string.
 *
 * @tparam Char The character type for the output string (default is `char`).
 * @tparam T The type of the object being reflected.
 * @param val The object to format.
 * @return A formatted string representing the object's fields.
 */
template <typename Char = char, typename T>
std::basic_string<Char> reflectFormat(const T& val) {
    constexpr static auto numFields = std::tuple_size_v<decltype(reflectionOf<T>())>;
    return Internal::reflectFormat<char>(size_sequence<numFields>{}, val, reflectionOf<T>());
}

} // namespace Brisk

/**
 * @brief Overloads the `<<` operator to output an object's fields using reflection.
 *
 * @tparam T The type of the object being reflected.
 * @param os The output stream.
 * @param val The object to output.
 * @return The output stream with the reflected object.
 */
template <Brisk::HasReflection T>
std::ostream& operator<<(std::ostream& os, const T& val) {
    constexpr static auto numFields = std::tuple_size_v<decltype(Brisk::reflectionOf<T>())>;

    std::string str = Brisk::Internal::reflectFormat<char>(Brisk::size_sequence<numFields>{}, val,
                                                           Brisk::reflectionOf<T>());
    os << str;
    return os;
}

/**
 * @brief Provides a custom formatter for types that have reflection metadata, for use with `fmt::format`.
 *
 * @tparam T The type of the object being reflected.
 * @tparam Char The character type for the output format.
 */
template <Brisk::HasReflection T, typename Char>
struct fmt::formatter<T, Char> : fmt::formatter<std::basic_string<Char>, Char> {
    constexpr static auto numFields = std::tuple_size_v<decltype(Brisk::reflectionOf<T>())>;

    template <typename FormatContext>
    auto format(const T& val, FormatContext& ctx) const {
        std::basic_string<Char> str = Brisk::Internal::reflectFormat<Char>(Brisk::size_sequence<numFields>{},
                                                                           val, Brisk::reflectionOf<T>());
        return formatter<std::basic_string<Char>, char>::format(str, ctx);
    }
};

/**
 * @brief Provides a custom formatter for enum types with default names, for use with `fmt::format`.
 *
 * @tparam T The enum type.
 * @tparam Char The character type for the output format.
 */
template <Brisk::HasDefaultNames T, typename Char>
struct fmt::formatter<T, Char> : fmt::formatter<std::string_view, Char> {
    template <typename FormatContext>
    auto format(const T& val, FormatContext& ctx) const {
        return formatter<std::string_view, Char>::format(Brisk::defaultToString(val), ctx);
    }
};
