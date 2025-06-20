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

#include <bit>
#include <limits>
#include <functional>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <fmt/format.h>

namespace Brisk {

enum class LengthUnit : uint8_t {
    Undefined, // Value ignored
    Auto,      // Value ignored

    Pixels,        // GUI pixels
    DevicePixels,  // Device (physical) pixels
    AlignedPixels, // GUI pixels aligned to device pixels before layout
    Em,            // Current font EM square

    Vw,   // Viewport width
    Vh,   // Viewport height
    Vmin, // Minimum of viewport width and height (min(vw, vh))
    Vmax, // Maximum of viewport width and height (max(vw, vh))

    Percent, // Range from 0 to 100

    Last    = Percent,
    Default = Pixels,

    // Order is important:
    // 1. Valueless units (if any). Undefined is first if present.
    // 2. Default unit.
    // 3. Value units (if any).
};

template <>
inline constexpr std::initializer_list<NameValuePair<LengthUnit>> defaultNames<LengthUnit>{
    { "Undefined", LengthUnit::Undefined },         //
    { "Auto", LengthUnit::Auto },                   //
    { "Pixels", LengthUnit::Pixels },               //
    { "DevicePixels", LengthUnit::DevicePixels },   //
    { "AlignedPixels", LengthUnit::AlignedPixels }, //
    { "Em", LengthUnit::Em },                       //
    { "Percent", LengthUnit::Percent },             //
};

constexpr auto operator+(LengthUnit value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

static_assert(+LengthUnit::Last - +LengthUnit::Default <= 15);
static_assert(+LengthUnit::Default <= 15);

template <typename T>
concept IsLengthUnit = std::is_enum_v<T> && requires(T val) {
    { T::Default } -> std::convertible_to<T>;
    { T::Last } -> std::convertible_to<T>;
    { +val } -> std::integral;
};
template <typename T>
concept HasUndefined = std::is_enum_v<T> && requires(T val) {
    { T::Undefined } -> std::convertible_to<T>;
};

struct Undefined {};

constexpr inline Undefined undef{};

template <IsLengthUnit Unit>
struct LengthOf {
    constexpr LengthOf() noexcept
        requires(HasUndefined<Unit>)
        : m_packed(pack(0.f, Unit::Undefined)) {}

    constexpr LengthOf(float value, Unit unit) noexcept : m_packed(pack(value, unit)) {}

    constexpr LengthOf(Undefined) noexcept
        requires(HasUndefined<Unit>)
        : m_packed(pack(0.f, Unit::Undefined)) {}

    constexpr /*implicit*/ LengthOf(float value) noexcept : m_packed(pack(value, Unit::Default)) {}

    constexpr bool hasValue() const noexcept {
        return !isValueless(unit());
    }

    constexpr bool isUndefined() const noexcept
        requires(HasUndefined<Unit>)
    {
        return unit() == Unit::Undefined;
    }

    constexpr float valueOr(float fallback) const noexcept {
        return hasValue() ? value() : fallback;
    }

    constexpr Unit unit() const noexcept {
        return unpackUnit(m_packed);
    }

    constexpr float value() const noexcept {
        return unpackValue(m_packed);
    }

    template <Unit srcUnit, Unit dstUnit = Unit::Default>
    constexpr LengthOf convert(float scale) const noexcept {
        if (unit() == srcUnit) {
            return { value() * scale, dstUnit };
        } else {
            return *this;
        }
    }

    friend constexpr bool operator==(const LengthOf& x, const Undefined& y) noexcept
        requires(HasUndefined<Unit>)
    {
        return x.unit() == Unit::Undefined;
    }

    friend constexpr bool operator==(const Undefined& x, const LengthOf& y) noexcept
        requires(HasUndefined<Unit>)
    {
        return y.unit() == Unit::Undefined;
    }

    friend constexpr bool operator==(const LengthOf& x, const LengthOf& y) noexcept {
        if (x.unit() != y.unit())
            return false;
        return !x.hasValue() || x.value() == y.value();
    }

    friend constexpr LengthOf operator-(LengthOf value) noexcept {
        return { -value.value(), value.unit() };
    }

    friend constexpr LengthOf operator+(LengthOf value) noexcept {
        return value;
    }

    friend constexpr LengthOf operator*(float factor, LengthOf value) noexcept {
        return { factor * value.value(), value.unit() };
    }

    friend constexpr LengthOf operator*(LengthOf value, float factor) noexcept {
        return { factor * value.value(), value.unit() };
    }

    constexpr static uint32_t unitBits = std::bit_width(+Unit::Last);

private:
    constexpr static uint32_t unitMask  = (1u << unitBits) - 1u;
    constexpr static uint32_t valueMask = ~unitMask;

    static constexpr bool isValueless(Unit unit) noexcept {
        return +unit < +Unit::Default;
    }

    constexpr static uint32_t pack(float value, Unit unit) noexcept {
        if (unit >= Unit::Default) {
            return (std::bit_cast<uint32_t>(value) & valueMask) |
                   static_cast<uint32_t>(+unit - +Unit::Default);
        } else {
            return special + +unit;
        }
    }

    constexpr static float unpackValue(uint32_t value) noexcept {
        if ((value & valueMask) == special) {
            return std::numeric_limits<float>::quiet_NaN();
        } else {
            return std::bit_cast<float>(value & valueMask);
        }
    }

    constexpr static Unit unpackUnit(uint32_t value) noexcept {
        if ((value & valueMask) == special) {
            return static_cast<Unit>(value & unitMask);
        } else {
            // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
            return static_cast<Unit>((value & unitMask) + +Unit::Default);
            // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)
        }
    }

    uint32_t m_packed                 = 0;

    constexpr static uint32_t special = std::bit_cast<uint32_t>(std::numeric_limits<float>::quiet_NaN());

    // True for both x86* and arm*
    static_assert(special == 0b0'11111111'10000000000000000000000u);
};

using Length = LengthOf<LengthUnit>;

static_assert(sizeof(Length) == 4);
static_assert(Length::unitBits <= 4);

using SizeL    = SizeOf<Length>;
using PointL   = PointOf<Length>;
using EdgesL   = EdgesOf<Length>;
using CornersL = CornersOf<Length>;

/**
 * @brief Constant representing a Length with an auto size.
 */
constexpr inline Length auto_{ 0.f, LengthUnit::Auto };

/**
 * @brief UDL operator to convert a value in scalable pixels to Length.
 * @param value The value in scalable pixels.
 * @return A Length object with the Pixels unit.
 */
constexpr Length operator""_px(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Pixels };
}

/**
 * @brief UDL operator to convert a value in device pixels to Length.
 * @param value The value in device pixels.
 * @return A Length object with the DevicePixels unit.
 */
constexpr Length operator""_dpx(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::DevicePixels };
}

/**
 * @brief UDL operator to convert a value in aligned pixels to Length.
 * @param value The value in aligned pixels.
 * @return A Length object with the AlignedPixels unit.
 */
constexpr Length operator""_apx(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::AlignedPixels };
}

/**
 * @brief UDL operator to convert a value in Em-units to Length.
 * @param value The value in Em-units.
 * @return A Length object with the Em unit.
 */
constexpr Length operator""_em(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Em };
}

/**
 * @brief UDL operator to convert a value in percents to Length.
 * @param value The value in percents.
 * @return A Length object with the Percent unit.
 */
constexpr Length operator""_perc(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Percent };
}

constexpr Length operator""_vw(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vw };
}

constexpr Length operator""_vh(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vh };
}

constexpr Length operator""_vmin(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vmin };
}

constexpr Length operator""_vmax(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vmax };
}

/**
 * @brief UDL operator to convert a value in scalable pixels (long double) to Length.
 * @param value The value in scalable pixels.
 * @return A Length object with the Pixels unit.
 */
constexpr Length operator""_px(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Pixels };
}

/**
 * @brief UDL operator to convert a value in device pixels (long double) to Length.
 * @param value The value in device pixels.
 * @return A Length object with the DevicePixels unit.
 */
constexpr Length operator""_dpx(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::DevicePixels };
}

/**
 * @brief UDL operator to convert a value in aligned pixels (long double) to Length.
 * @param value The value in aligned pixels.
 * @return A Length object with the AlignedPixels unit.
 */
constexpr Length operator""_apx(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::AlignedPixels };
}

/**
 * @brief UDL operator to convert a value in Em-units (long double) to Length.
 * @param value The value in Em-units.
 * @return A Length object with the Em unit.
 */
constexpr Length operator""_em(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Em };
}

/**
 * @brief UDL operator to convert a value in percents (long double) to Length.
 * @param value The value in percents.
 * @return A Length object with the Percent unit.
 */
constexpr Length operator""_perc(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Percent };
}

constexpr Length operator""_vw(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vw };
}

constexpr Length operator""_vh(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vh };
}

constexpr Length operator""_vmin(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vmin };
}

constexpr Length operator""_vmax(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Vmax };
}

/**
 * @brief Specifies the flex container's main axis direction.
 *
 * Defines the direction in which flex items are placed within a flex container.
 */
enum class FlexDirection : uint8_t {
    Column,        /**< Items are placed in a column (vertical direction). */
    ColumnReverse, /**< Items are placed in a column, but in reverse order. */
    Row,           /**< Items are placed in a row (horizontal direction). */
    RowReverse,    /**< Items are placed in a row, but in reverse order. */
};

/**
 * @brief Converts a FlexDirection value to its underlying integer type.
 */
constexpr auto operator+(FlexDirection value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @brief Specifies how flex items are aligned along the main axis.
 *
 * Defines how the items are distributed along the flex container's main axis.
 */
enum class Justify : uint8_t {
    FlexStart,    /**< Items are aligned at the start of the main axis. */
    Center,       /**< Items are aligned at the center of the main axis. */
    FlexEnd,      /**< Items are aligned at the end of the main axis. */
    SpaceBetween, /**< Items are spaced with the first item at the start and the last at the end. */
    SpaceAround,  /**< Items are spaced with equal space around them. */
    SpaceEvenly,  /**< Items are spaced with equal space between them. */
};

/**
 * @brief Converts a Justify value to its underlying integer type.
 */
constexpr auto operator+(Justify value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @brief Specifies how flex items are aligned along the cross axis.
 *
 * Defines how items are aligned perpendicular to the main axis of the flex container.
 */
enum class Align : uint8_t {
    Auto,         /**< Items are aligned based on their default behavior. */
    FlexStart,    /**< Items are aligned at the start of the cross axis. */
    Center,       /**< Items are aligned at the center of the cross axis. */
    FlexEnd,      /**< Items are aligned at the end of the cross axis. */
    Stretch,      /**< Items are stretched to fill the available space along the cross axis. */
    Baseline,     /**< Items are aligned based on their baseline. */
    SpaceBetween, /**< Items are spaced with the first item at the start and the last at the end. */
    SpaceAround,  /**< Items are spaced with equal space around them. */
    SpaceEvenly,  /**< Items are spaced with equal space between them. */
};

/**
 * @brief Converts an Align value to its underlying integer type.
 */
constexpr auto operator+(Align value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @brief Alias for Align, used for aligning items along the cross axis.
 */
using AlignItems   = Align;

/**
 * @brief Alias for Align, used for aligning individual flex items.
 */
using AlignSelf    = Align;

/**
 * @brief Alias for Align, used for aligning flex container's content along the cross axis.
 */
using AlignContent = Align;

/**
 * @brief Specifies whether flex items wrap onto multiple lines.
 *
 * Defines how the flex container's items wrap within the container.
 */
enum class Wrap : uint8_t {
    NoWrap,      /**< Items do not wrap, staying in a single line. */
    Wrap,        /**< Items wrap onto multiple lines as needed. */
    WrapReverse, /**< Items wrap in reverse order, starting from the bottom/right. */
};

/**
 * @brief Converts a Wrap value to its underlying integer type.
 */
constexpr auto operator+(Wrap value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @enum OverflowScroll
 * @brief Defines the visibility of scrollbars for a container.
 */
enum class OverflowScroll : uint8_t {
    Disable, ///< Scrollbars are always hidden.
    Enable,  ///< Scrollbars are always visible.
    Auto,    ///< Scrollbars are visible only when content overflows.
};

using OverflowScrollBoth = SizeOf<OverflowScroll>;

/**
 * @enum ContentOverflow
 * @brief Defines how content overflow affects container sizing.
 */
enum class ContentOverflow : uint8_t {
    Default, ///< Default sizing behavior is applied.
    Allow,   ///< Content overflow does not affect the container's size.
};

using ContentOverflowBoth = SizeOf<ContentOverflow>;

/**
 * @brief Specifies the gutter (spacing) direction for the flex container.
 *
 * Defines the direction in which gutter spacing is applied between flex items.
 */
enum class Gutter : uint8_t {
    Column, /**< Gutter applies between items in a column layout. */
    Row,    /**< Gutter applies between items in a row layout. */
    All,    /**< Gutter applies between all items regardless of layout direction. */
};

/**
 * @brief Converts a Gutter value to its underlying integer type.
 */
constexpr auto operator+(Gutter value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class BoxSizingPerAxis : uint8_t {
    BorderBox   = 0,
    ContentBoxX = 1,
    ContentBoxY = 2,
    ContentBox  = 3,
};

template <>
constexpr inline bool isBitFlags<BoxSizingPerAxis> = true;

enum class Dimension : uint8_t {
    Width,
    Height,
};

constexpr auto operator+(Dimension value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class OptFloatUnit : uint8_t {
    Undefined,
    Default,
    Last = Default,
};

constexpr auto operator+(OptFloatUnit value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

using OptFloat = LengthOf<OptFloatUnit>;

enum class MeasureMode : uint8_t {
    Undefined,
    Exactly,
    AtMost,
    Last    = AtMost,
    Default = Exactly,
};

constexpr auto operator+(MeasureMode value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

using AvailableLength = LengthOf<MeasureMode>;
using AvailableSize   = SizeOf<AvailableLength>;

static_assert(sizeof(AvailableLength) == 4);
static_assert(AvailableLength::unitBits == 2);

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::Length> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(Brisk::Length val, FormatContext& ctx) const {
        std::vector<std::string_view> list;
        using enum Brisk::LengthUnit;
        switch (val.unit()) {
        case Undefined:
            return fmt::format_to(ctx.out(), "undefined");
        case Auto:
            return fmt::format_to(ctx.out(), "auto");
        case Pixels:
            return fmt::format_to(ctx.out(), "{}px", val.value());
        case Em:
            return fmt::format_to(ctx.out(), "{}em", val.value());
        case Percent:
            return fmt::format_to(ctx.out(), "{}%", val.value());
        default:
            return fmt::format_to(ctx.out(), "(unknown)");
        }
    }
};

template <>
struct fmt::formatter<Brisk::AvailableLength> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(Brisk::AvailableLength val, FormatContext& ctx) const {
        std::vector<std::string_view> list;
        using enum Brisk::MeasureMode;
        switch (val.unit()) {
        case Undefined:
            return fmt::format_to(ctx.out(), "undefined");
        case Exactly:
            return fmt::format_to(ctx.out(), "=={}", val.value());
        case AtMost:
            return fmt::format_to(ctx.out(), "<={}", val.value());
        default:
            return fmt::format_to(ctx.out(), "(unknown)");
        }
    }
};
