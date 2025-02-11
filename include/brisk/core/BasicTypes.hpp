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
#include "Brisk.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <cstddef>
#include <cstdint>
#include "RC.hpp"
#include "internal/Span.hpp"
#include "internal/Constants.hpp"
#include <bit>
#include "Reflection.hpp"
#include <fmt/ranges.h>

namespace Brisk {

using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;

/**
 * @brief Creates a `std::span` representing a view over a single mutable element.
 *
 * This function template constructs a `std::span<T>` from a reference to a single element of type `T`.
 * The span provides read and write access to the element.
 *
 * @tparam T The type of the element.
 * @param value A reference to the element of type `T`. This element will be used to create the span.
 *
 * @return A `std::span<T>` representing a view over the single element `value`.
 *         The span has a size of 1 and points to the memory address of `value`.
 */
template <typename T>
std::span<T> one(T& value) noexcept {
    return std::span<T>{ &value, 1 };
}

/**
 * @brief Creates a `std::span` representing a view over a single immutable element.
 *
 * This function template constructs a `std::span<const T>` from a reference to a single constant element of
 * type `T`. The span provides read-only access to the element.
 *
 * @tparam T The type of the element.
 * @param value A reference to the constant element of type `T`. This element will be used to create the span.
 *
 * @return A `std::span<const T>` representing a view over the single element `value`.
 *         The span has a size of 1 and points to the memory address of `value`.
 */
template <typename T>
std::span<const T> one(const T& value) noexcept {
    return std::span<const T>{ &value, 1 };
}

/**
 * @brief Type alias for a single byte (8-bit unsigned integer).
 */
using byte               = uint8_t;

/**
 * @brief Type alias for a vector of bytes.
 */
using Bytes              = std::vector<byte>;

/**
 * @brief Type alias for a non-modifiable view of a sequence of bytes.
 */
using BytesView         = std::span<const byte>;

/**
 * @brief Type alias for a modifiable view of a sequence of bytes.
 */
using BytesMutableView = std::span<byte>;

#ifdef BRISK_USE_CHAR8_T
/**
 * @brief Type alias for UTF-8 characters.
 */
using U8Char                     = char8_t;

/**
 * @brief Type alias for UTF-8 strings.
 */
using U8String                   = std::u8string;

/**
 * @brief Type alias for UTF-8 string views.
 */
using U8StringView               = std::u8string_view;

/**
 * @brief Type alias for ASCII characters.
 */
using AsciiChar                  = char;

/**
 * @brief Type alias for ASCII strings.
 */
using AsciiString                = std::string;

/**
 * @brief Type alias for ASCII string views.
 */
using AsciiStringView            = std::string_view;

/**
 * @brief A constant indicating that the character type is not UTF-8.
 */
constexpr inline bool charIsUtf8 = false;
#else
/**
 * @brief Type alias for UTF-8 characters (fallback to ASCII character type).
 */
using U8Char                     = char;

/**
 * @brief Type alias for UTF-8 strings (fallback to ASCII string type).
 */
using U8String                   = std::string;

/**
 * @brief Type alias for UTF-8 string views (fallback to ASCII string view type).
 */
using U8StringView               = std::string_view;

/**
 * @brief Type alias for ASCII characters.
 */
using AsciiChar                  = char;

/**
 * @brief Type alias for ASCII strings.
 */
using AsciiString                = std::string;

/**
 * @brief Type alias for ASCII string views.
 */
using AsciiStringView            = std::string_view;

/**
 * @brief A constant indicating that the character type is UTF-8.
 */
constexpr inline bool charIsUtf8 = true;
#endif

/**
 * @brief Type alias for UTF-16 characters.
 */
using UChar16       = char16_t;

/**
 * @brief Type alias for UTF-32 characters.
 */
using UChar32       = char32_t;

/**
 * @brief Type alias for wide characters.
 */
using WChar         = wchar_t;

/**
 * @brief Type alias for UTF-16 strings.
 */
using U16String     = std::u16string;

/**
 * @brief Type alias for UTF-16 string views.
 */
using U16StringView = std::u16string_view;

/**
 * @brief Type alias for UTF-32 strings.
 */
using U32String     = std::u32string;

/**
 * @brief Type alias for UTF-32 string views.
 */
using U32StringView = std::u32string_view;

/**
 * @brief Type alias for wide strings.
 */
using WString       = std::wstring;

/**
 * @brief Type alias for wide string views.
 */
using WStringView   = std::wstring_view;

/**
 * @brief A compile-time constant determining if a type has a simple memory representation.
 *
 * A type is considered to have a simple memory representation if it has unique object representations,
 * is a floating-point type, or is a boolean type, and its alignment is less than or equal to its size.
 */
template <typename T>
constexpr inline bool simpleMemoryRepresentation =
    (std::has_unique_object_representations_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool>) &&
    (alignof(T) <= sizeof(T));

/**
 * @brief Converts an object of type `T` to a non-modifiable view of bytes.
 *
 * This function requires that the type `T` has a simple memory representation.
 *
 * @param value The object to convert.
 * @return A `BytesView` representing the object as a sequence of bytes.
 */
template <typename Type>
inline BytesView asBytesView(const Type& value)
    requires simpleMemoryRepresentation<Type>
{
    return BytesView(reinterpret_cast<const byte*>(std::addressof(value)), sizeof(Type));
}

/**
 * @brief Converts a container to a non-modifiable view of bytes.
 *
 * This function works for containers that may be converted to a `std::span<const T>`.
 *
 * @param cont The container to convert.
 * @return A `BytesView` representing the container's elements as a sequence of bytes.
 */
template <typename Container, typename T = typename std::decay_t<Container>::value_type>
inline BytesView toBytesView(Container&& cont)
    requires std::is_constructible_v<std::span<const T>, Container&>
{
    std::span<const T> sp(cont);
    return BytesView(reinterpret_cast<const byte*>(sp.data()), sp.size_bytes());
}

/**
 * @brief Converts a null-terminated C-string to a non-modifiable view of bytes.
 *
 * @param str The null-terminated C-string to convert.
 * @return A `BytesView` representing the C-string as a sequence of bytes, excluding null byte.
 */
template <size_t N>
inline BytesView toBytesView(const char (&str)[N]) {
    return BytesView(reinterpret_cast<const byte*>(str), N - 1);
}

/**
 * @brief Converts a null-terminated UTF-8 C-string to a non-modifiable view of bytes.
 *
 * @param str The null-terminated UTF-8 C-string to convert.
 * @return A `BytesView` representing the UTF-8 C-string as a sequence of bytes, excluding null byte.
 */
template <size_t N>
inline BytesView toBytesView(const char8_t (&str)[N]) {
    return BytesView(reinterpret_cast<const byte*>(str), N - 1);
}

/**
 * @brief Converts a null-terminated UTF-16 C-string to a non-modifiable view of bytes.
 *
 * @param str The null-terminated UTF-16 C-string to convert.
 * @return A `BytesView` representing the UTF-16 C-string as a sequence of bytes, excluding null.
 */
template <size_t N>
inline BytesView toBytesView(const char16_t (&str)[N]) {
    return BytesView(reinterpret_cast<const byte*>(str), (N - 1) * sizeof(char16_t));
}

/**
 * @brief Converts a null-terminated UTF-32 C-string to a non-modifiable view of bytes.
 *
 * @param str The null-terminated UTF-32 C-string to convert.
 * @return A `BytesView` representing the UTF-32 C-string as a sequence of bytes, excluding null.
 */
template <size_t N>
inline BytesView toBytesView(const char32_t (&str)[N]) {
    return BytesView(reinterpret_cast<const byte*>(str), (N - 1) * sizeof(char32_t));
}

/**
 * @brief Converts a null-terminated wide C-string to a non-modifiable view of bytes.
 *
 * @param str The null-terminated wide C-string to convert.
 * @return A `BytesView` representing the wide C-string as a sequence of bytes, excluding null.
 */
template <size_t N>
inline BytesView toBytesView(const wchar_t (&str)[N]) {
    return BytesView(reinterpret_cast<const byte*>(str), (N - 1) * sizeof(wchar_t));
}

/**
 * @brief Converts a container to a modifiable view of bytes.
 *
 * This function works for containers that may be converted to a `std::span<T>`.
 *
 * @param cont The container to convert.
 * @return A `BytesMutableView` representing the container's elements as a sequence of bytes.
 */
template <typename Container>
inline BytesMutableView toBytesMutableView(Container&& cont)
    requires requires { std::span{ cont }; }
{
    std::span sp(cont);
    return BytesMutableView(reinterpret_cast<byte*>(sp.data()), sp.size_bytes());
}

/**
 * @brief Converts an object to a vector of bytes.
 *
 * This function first converts the object to a `BytesView`, then creates a `Bytes` vector from it.
 *
 * @param value The object to convert.
 * @return A `Bytes` vector representing the object as a sequence of bytes.
 */
template <typename T>
inline Bytes toBytes(T&& value) {
    BytesView v = toBytesView(value);
    return Bytes(v.begin(), v.end());
}

/**
 * @brief Converts an object to a `std::string_view`.
 *
 * This function first converts the object to a `BytesView`, then creates a `std::string_view` from it.
 *
 * @param value The object to convert.
 * @return A `std::string_view` representing the object as a sequence of characters.
 */
template <typename T>
inline std::string_view toStringView(T&& value) {
    BytesView v = toBytesView(value);
    return std::string_view(reinterpret_cast<const char*>(v.data()), v.size());
}

/**
 * @enum Encoding
 * @brief Enum class representing different text encodings.
 *
 * This enum class defines various text encodings that can be used to represent characters in a digital
 * format.
 */
enum class Encoding : uint8_t {
    Utf8,  /**< UTF-8 encoding, which uses one to four bytes for each character. */
    Utf16, /**< UTF-16 encoding, which uses one or two 16-bit code units for each character. */
    Utf32  /**< UTF-32 encoding, which uses a single 32-bit code unit for each character. */
};

/**
 * @brief Names for the Encoding enum values.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<Encoding>> defaultNames<Encoding>{
    { "Utf8", Encoding::Utf8 },   /**< Name for the Utf8 encoding. */
    { "Utf16", Encoding::Utf16 }, /**< Name for the Utf16 encoding. */
    { "Utf32", Encoding::Utf32 }, /**< Name for the Utf32 encoding. */
};

enum class Orientation : uint8_t {
    Horizontal = 0,
    Vertical   = 1,

    Direct     = 0,
    Invert     = 1,
};

enum class FlipAxis : uint8_t {
    X,
    Y,
    Both,
};

enum class Direction : uint8_t {
    LeftToRight = 0,
    RightToLeft = 1,
    TopToBottom = 2,
    BottomToTop = 3,
};

enum class Order : uint8_t {
    Next,
    Previous,
};

enum class HorizontalDirection : uint8_t {
    LeftToRight = 0,
    RightToLeft = 1,
};

enum class VerticalDirection : uint8_t {
    TopToBottom = 0,
    BottomToTop = 1,
};

enum class LogicalDirection {
    UpOrLeft    = 0,
    DownOrRight = 1,
};

enum class CornerFlags {
    All         = 0b1111,
    None        = 0b0000,

    TopLeft     = 0b0001,
    TopRight    = 0b0010,
    BottomLeft  = 0b0100,
    BottomRight = 0b1000,

    Top         = TopLeft | TopRight,
    Bottom      = BottomLeft | BottomRight,
    Left        = TopLeft | BottomLeft,
    Right       = TopRight | BottomRight,
};

constexpr int operator+(CornerFlags o) {
    return static_cast<int>(o);
}

constexpr Orientation toOrientation(Direction d) {
    return static_cast<Orientation>(static_cast<int>(d) >> 1);
}

constexpr bool isInverted(Direction d) {
    return static_cast<int>(d) & 1;
}

constexpr int operator+(Orientation o) {
    return static_cast<int>(o);
}

constexpr int operator-(Orientation o) {
    return static_cast<int>(o) ^ 1;
}

constexpr Orientation operator^(Orientation x, Orientation y) {
    return static_cast<Orientation>(static_cast<int>(x) ^ static_cast<int>(y));
}

constexpr Orientation operator!(Orientation o) {
    return static_cast<Orientation>(static_cast<int>(o) ^ 1);
}

constexpr Direction operator!(Direction o) {
    return static_cast<Direction>(static_cast<int>(o) ^ 1);
}

enum class Edge : uint8_t {
    Left,
    Top,
    Right,
    Bottom,
};

constexpr auto operator+(Edge value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @brief Converts a `const char*` to a `std::string_view`, handling `nullptr` gracefully.
 *
 * This function takes a pointer to a null-terminated C-string. If the pointer is non-null,
 * it returns a `std::string_view` that references the string. If the pointer is `nullptr`,
 * it returns a `std::string_view` with the value "(null)" to represent the absence of a string.
 *
 * @param s A pointer to a null-terminated C-string. Can be `nullptr`.
 *
 * @return A `std::string_view` that either references the input C-string or is set to "(null)"
 *         if the input pointer is `nullptr`.
 */
inline std::string_view safeCharPtr(const char* s) {
    if (s)
        return s;
    return "(null)";
}

/**
 * @brief A template class representing a range of values.
 *
 * The `Range` class template defines a range with a minimum and maximum value of type `T`.
 * It provides various methods to perform operations such as calculating distance, union,
 * intersection, and checking if a value is contained within the range.
 *
 * @tparam T The type of the values in the range. It must support arithmetic operations.
 */
template <typename T, bool inclusive = false>
struct Range {
    T min; ///< The minimum value of the range.
    T max; ///< The maximum value of the range.

    constexpr static std::tuple Reflection{
        ReflectionField{ "min", &Range::min },
        ReflectionField{ "max", &Range::max },
    };

    /**
     * @brief Computes the distance between the minimum and maximum values of the range.
     *
     * @return The distance between `max` and `min`.
     */
    constexpr T distance() const noexcept {
        return max - min;
    }

    /**
     * @brief Computes the union of this range with another range.
     *
     * The union of two ranges is a range that spans from the minimum of the two ranges
     * to the maximum of the two ranges.
     *
     * @param b The other range to compute the union with.
     * @return A new `Range` representing the union of this range and `b`.
     */
    constexpr Range union_(const Range& b) const noexcept {
        return { std::min(min, b.min), std::max(max, b.max) };
    }

    /**
     * @brief Computes the intersection of this range with another range.
     *
     * The intersection of two ranges is a range that spans from the maximum of the two
     * ranges' minimums to the minimum of the two ranges' maximums. If the ranges do not
     * overlap, the result will be an empty range.
     *
     * @param b The other range to compute the intersection with.
     * @return A new `Range` representing the intersection of this range and `b`.
     */
    constexpr Range intersection(const Range& b) const noexcept {
        return { std::max(min, b.min), std::min(max, b.max) };
    }

    /**
     * @brief Checks if a value is contained within the range.
     *
     * @param value The value to check for containment.
     * @return `true` if `value` is greater than or equal to `min` and less than `max`;
     *         otherwise, `false`.
     */
    constexpr bool contains(T value) const noexcept {
        if constexpr (inclusive)
            return value >= min && value <= max;
        else
            return value >= min && value < max;
    }

    /**
     * @brief Checks if the range is empty.
     *
     * A range is considered empty if `max` is less than or equal to `min`.
     *
     * @return `true` if `max` is less than or equal to `min`; otherwise, `false`.
     */
    constexpr bool empty() const noexcept {
        if constexpr (inclusive)
            return max < min;
        else
            return max <= min;
    }

    /**
     * @brief Checks if this range intersects with another range.
     *
     * Two ranges intersect if their intersection is not empty.
     *
     * @param other The other range to check for intersection.
     * @return `true` if this range intersects with `other`; otherwise, `false`.
     */
    constexpr bool intersects(const Range<T>& other) const noexcept {
        return !intersection(other).empty();
    }

    /**
     * @brief Shifts the range by adding a value to both the minimum and maximum.
     *
     * @param b The value to add to both `min` and `max`.
     * @return A new `Range` that represents the shifted range.
     */
    constexpr Range operator+(T b) const noexcept {
        return Range(*this) += b;
    }

    /**
     * @brief Shifts the range by subtracting a value from both the minimum and maximum.
     *
     * @param b The value to subtract from both `min` and `max`.
     * @return A new `Range` that represents the shifted range.
     */
    constexpr Range operator-(T b) const noexcept {
        return Range(*this) -= b;
    }

    /**
     * @brief Increments both the minimum and maximum values of the range.
     *
     * @param b The value to add to both `min` and `max`.
     * @return A reference to the modified `Range`.
     */
    constexpr Range& operator+=(T b) noexcept {
        min += b;
        max += b;
        return *this;
    }

    /**
     * @brief Decrements both the minimum and maximum values of the range.
     *
     * @param b The value to subtract from both `min` and `max`.
     * @return A reference to the modified `Range`.
     */
    constexpr Range& operator-=(T b) noexcept {
        min -= b;
        max -= b;
        return *this;
    }

    /**
     * @brief Checks for equality between this range and another range.
     *
     * @param b The range to compare with.
     * @return `true` if both `min` and `max` values are equal; otherwise, `false`.
     */
    constexpr bool operator==(const Range& b) const noexcept = default;

    template <typename U>
    operator Range<U, inclusive>() const noexcept
        requires std::is_convertible_v<T, U>
    {
        return { static_cast<U>(min), static_cast<U>(max) };
    }

    /**
     * @brief A nested iterator class for the Range.
     */
    struct RangeIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        explicit RangeIterator(T current) : m_current(current) {}

        reference operator*() const {
            return m_current;
        }

        pointer operator->() const {
            return &m_current;
        }

        RangeIterator& operator++() {
            ++m_current;
            return *this;
        }

        RangeIterator operator++(int) {
            RangeIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const RangeIterator& other) const {
            return m_current == other.m_current;
        }

        bool operator!=(const RangeIterator& other) const {
            return !(*this == other);
        }

    private:
        T m_current;
    };

    /**
     * @brief Returns an iterator to the beginning of the range.
     *
     * @return An iterator pointing to the minimum value.
     */
    RangeIterator begin() const noexcept
        requires(!inclusive)
    {
        return RangeIterator(min);
    }

    /**
     * @brief Returns an iterator to the end of the range.
     *
     * The end iterator points just past the maximum value.
     *
     * @return An iterator pointing just past the maximum value.
     */
    RangeIterator end() const noexcept
        requires(!inclusive)
    {
        return RangeIterator(max);
    }
};

template <typename T>
Range(T, T) -> Range<T, false>;

template <typename T>
using InclusiveRange = Range<T>;

template <typename T>
constexpr inline bool isBitFlags = false;

template <typename T>
concept BitFlags = isBitFlags<T>;

template <BitFlags T>
constexpr std::underlying_type_t<T> operator+(T x) noexcept {
    return static_cast<std::underlying_type_t<T>>(x);
}

template <BitFlags T>
constexpr bool operator&&(T flags, T flag) noexcept {
    return (+flags & +flag) != 0;
}

template <BitFlags T>
constexpr T operator|(T x, T y) noexcept {
    return static_cast<T>(+x | +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T operator&(T x, T y) noexcept {
    return static_cast<T>(+x & +y);
}

template <BitFlags T>
constexpr T& operator|=(T& x, T y) noexcept {
    return x = static_cast<T>(+x | +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T& operator^=(T& x, T y) noexcept {
    return x = static_cast<T>(+x ^ +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T operator^(T x, T y) noexcept {
    return static_cast<T>(+x ^ +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T operator~(T x) noexcept {
    return static_cast<T>(~+x); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T& operator&=(T& x, T y) noexcept {
    return x = static_cast<T>(+x & +y);
} /*constexpr T operator<<(T& x, int y) { return static_cast<T>(+x << y); }  */

template <BitFlags T>
constexpr T operator>>(T& x, int y) noexcept {
    return static_cast<T>(+x >> y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T& operator<<=(T& x, int y) noexcept {
    return x = static_cast<T>(+x << y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T& operator>>=(T& x, int y) noexcept {
    return x = static_cast<T>(+x >> y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T operator+(T x, T y) noexcept {
    return static_cast<T>(+x + +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr T& operator+=(T& x, T y) noexcept {
    return x = static_cast<T>(+x + +y); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
}

template <BitFlags T>
constexpr void toggle(T& x, T y, bool flag) noexcept {
    if (flag)
        x |= y;
    else
        x &= ~y;
}

struct Empty {};

namespace InternalHelper {
template <typename F, typename Ret, typename A, typename... Rest>
A helper(Ret (F::*)(A, Rest...));

template <typename F, typename Ret, typename A, typename... Rest>
A helper(Ret (F::*)(A, Rest...) const);

template <typename F>
struct LambdaArgument {
    using type = decltype(helper(&F::operator()));
};

template <typename R, typename A>
struct LambdaArgument<R (*)(A)> {
    using type = A;
};

template <typename R, typename A>
struct LambdaArgument<R (&)(A)> {
    using type = A;
};
} // namespace InternalHelper

template <typename Fn, typename Arg = std::decay_t<typename InternalHelper::LambdaArgument<Fn>::type>,
          typename Ret = std::decay_t<std::invoke_result_t<Fn, Arg>>>
struct RefAdapter {
    Fn fn;
    Ret& ref;
    Arg val{};

    operator Arg&() && {
        return static_cast<Arg&>(val);
    }

    ~RefAdapter() {
        ref = fn(val);
    }
};

template <typename Fn, typename T>
RefAdapter(Fn, T&) -> RefAdapter<Fn>;

// Retrieves ParentClass and Class from member field pointer
template <typename Func>
struct MemberFieldTraits;

template <typename Class_, typename Type_>
struct MemberFieldTraits<Type_ Class_::*> {
    using Class = Class_;
    using Type  = Type_;
};

#ifdef __INTELLISENSE__
#define __asm__
#define __volatile__
#define __attribute__(...)
#define __builtin_va_list va_list
#endif

template <typename T>
struct TypeID {
    using Type = T;
};

template <typename... T>
struct TypeIDs {};

struct PassThrough {
    template <typename T>
    BRISK_INLINE T&& operator()(T&& x) noexcept {
        return std::forward<T>(x);
    }
};

template <typename... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overload(Ts&&...) -> Overload<Ts...>;

template <size_t Factor = 1, typename T>
bool isAligned(T* pointer) {
    constexpr size_t Alignment = sizeof(T) * Factor;
    if constexpr (std::has_single_bit(Alignment)) {
        return (reinterpret_cast<uintptr_t>(pointer) & (Alignment - 1)) == 0;
    } else {
        return false;
    }
}

} // namespace Brisk

namespace fmt {
template <typename T, bool inclusive, typename Char>
struct range_format_kind<Brisk::Range<T, inclusive>, Char>
    : std::integral_constant<range_format, range_format::disabled> {};

} // namespace fmt
