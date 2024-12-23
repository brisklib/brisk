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

#ifdef BRISK_VIEWPORT_UNITS
    Vw,   // Viewport width
    Vh,   // Viewport height
    Vmin, // Minimum of viewport width and height (min(vw, vh))
    Vmax, // Maximum of viewport width and height (max(vw, vh))
#endif

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

    constexpr static uint32_t pack(float value, Unit unit) {
        if (unit >= Unit::Default) {
            return (std::bit_cast<uint32_t>(value) & valueMask) |
                   static_cast<uint32_t>(+unit - +Unit::Default);
        } else {
            return special + +unit;
        }
    }

    constexpr static float unpackValue(uint32_t value) {
        if ((value & valueMask) == special) {
            return std::numeric_limits<float>::quiet_NaN();
        } else {
            return std::bit_cast<float>(value & valueMask);
        }
    }

    constexpr static Unit unpackUnit(uint32_t value) {
        if ((value & valueMask) == special) {
            return static_cast<Unit>(value & unitMask);
        } else {
            return static_cast<Unit>((value & unitMask) + +Unit::Default);
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

constexpr inline Length auto_{ 0.f, LengthUnit::Auto };

constexpr Length operator""_px(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Pixels };
}

constexpr Length operator""_dpx(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::DevicePixels };
}

constexpr Length operator""_apx(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::AlignedPixels };
}

constexpr Length operator""_em(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Em };
}

constexpr Length operator""_perc(unsigned long long value) noexcept {
    return { static_cast<float>(value), LengthUnit::Percent };
}

#ifdef BRISK_VIEWPORT_UNITS
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
#endif

constexpr Length operator""_px(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Pixels };
}

constexpr Length operator""_dpx(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::DevicePixels };
}

constexpr Length operator""_apx(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::AlignedPixels };
}

constexpr Length operator""_em(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Em };
}

constexpr Length operator""_perc(long double value) noexcept {
    return { static_cast<float>(value), LengthUnit::Percent };
}

#ifdef BRISK_VIEWPORT_UNITS
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
#endif

enum class FlexDirection : uint8_t {
    Column,
    ColumnReverse,
    Row,
    RowReverse,
};

constexpr auto operator+(FlexDirection value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class Justify : uint8_t {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

constexpr auto operator+(Justify value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class Align : uint8_t {
    Auto,
    FlexStart,
    Center,
    FlexEnd,
    Stretch,
    Baseline,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
};

constexpr auto operator+(Align value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

using AlignItems   = Align;

using AlignSelf    = Align;

using AlignContent = Align;

enum class Wrap : uint8_t {
    NoWrap,
    Wrap,
    WrapReverse,
};

constexpr auto operator+(Wrap value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class Overflow : uint8_t {
    None       = 0,
    ScrollX    = 1,
    ScrollY    = 2,
    ScrollBoth = 3,
};

BRISK_FLAGS(Overflow)

enum class Gutter : uint8_t {
    Column,
    Row,
    All,
};

constexpr auto operator+(Gutter value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class BoxSizingPerAxis : uint8_t {
    BorderBox   = 0,
    ContentBoxX = 1,
    ContentBoxY = 2,
    ContentBox  = 3,
};

BRISK_FLAGS(BoxSizingPerAxis)

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
