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

#include <brisk/core/Reflection.hpp>
#include <brisk/core/internal/Argument.hpp>
#include <brisk/graphics/Color.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include "internal/Animation.hpp"
#include <brisk/window/Types.hpp>
#include <stdint.h>
#include "Layout.hpp"
#include <bitset>

namespace Brisk {

template <typename T>
using OptConstRef = std::conditional_t<std::is_trivially_copyable_v<T>, T, const T&>;

static_assert(std::is_same_v<OptConstRef<int>, int>);
static_assert(std::is_same_v<OptConstRef<ColorF>, ColorF>);
static_assert(std::is_same_v<OptConstRef<std::string_view>, std::string_view>);
static_assert(std::is_same_v<OptConstRef<std::string>, const std::string&>);
static_assert(std::is_same_v<OptConstRef<std::vector<int>>, const std::vector<int>&>);

enum class Placement : uint8_t {
    Normal,   // In-flow
    Absolute, // Absolute in parent widget
    Window,   // Absolute in parent window
};

enum class WidgetClip : uint8_t {
    Children, // Only children
    All,      // Children and self
    Inherit,  // Inherit parent clipping
    None,     // Disable clipping
};

enum class ZOrder : uint8_t {
    Normal,
    TopMost,
};

enum class TextAutoSize : uint8_t {
    None,
    FitWidth,
    FitHeight,
    FitSize,
};

enum class Layout : uint8_t {
    Horizontal = 0, // important
    Vertical   = 1, // important
};

constexpr uint8_t operator+(Layout v) {
    return static_cast<uint8_t>(v);
}

constexpr uint8_t operator-(Layout v) {
    return static_cast<uint8_t>(v) ^ uint8_t(1);
}

enum class LayoutOrder : uint8_t {
    Direct  = 0,
    Reverse = 1,
};

enum class Rotation : uint8_t {
    NoRotation = 0,
    Rotate90   = 1,
    Rotate180  = 2,
    Rotate270  = 3,
};

constexpr Orientation toOrientation(Rotation r) {
    return static_cast<int>(r) & 1 ? Orientation::Vertical : Orientation::Horizontal;
}

enum class TextAlign : uint8_t {
    Start,
    Center,
    End,
};

constexpr float toFloatAlign(TextAlign align) {
    return static_cast<int>(align) * 0.5f;
}

enum class AlignToViewport : uint8_t {
    None = 0,
    X    = 1,
    Y    = 2,
    XY   = 3,
};

template <>
constexpr inline bool isBitFlags<AlignToViewport> = true;

enum FontSize : uint8_t {
    Small    = 10,
    Normal   = 12,
    Bigger   = 16,
    Headline = 24,
};

using Classes = SmallVector<std::string, 1>;

enum class PropFlags : uint16_t {
    None             = 0,
    AffectLayout     = 1 << 0,
    AffectStyle      = 1 << 1,
    Transition       = 1 << 2,
    Resolvable       = 1 << 3,
    AffectResolve    = 1 << 4,
    AffectFont       = 1 << 5,
    Inheritable      = 1 << 6,
    RelativeToParent = 1 << 7,
    AffectPaint      = 1 << 8,

    Compound         = 1 << 9,
};

template <>
constexpr inline bool isBitFlags<PropFlags> = true;

namespace Internal {
constexpr size_t indexFromFlags(PropFlags flags) noexcept {
    return (static_cast<uint16_t>(flags) >> 8) & 0b11;
}

enum class PropState : uint8_t {
    None      = 0,
    Overriden = 1, // Don't apply style
    Inherited = 2, // Copy from parent

    Mask      = Overriden | Inherited,
};

constexpr inline size_t propStateBits = std::bit_width(static_cast<uint8_t>(PropState::Mask));

} // namespace Internal

template <>
constexpr inline bool isBitFlags<Internal::PropState> = true;

namespace Tag {
struct PropertyTag {};

struct StyleVarTag {};
} // namespace Tag

template <typename T>
concept PropertyTag = std::derived_from<T, Tag::PropertyTag> && requires { typename T::Type; };

template <typename T>
concept StyleVarTag = std::derived_from<T, Tag::StyleVarTag> && requires { typename T::Type; };

struct Inherit {};

constexpr inline Inherit inherit{};

template <>
inline constexpr std::initializer_list<NameValuePair<Layout>> defaultNames<Layout>{
    { "Horizontal", Layout::Horizontal },
    { "Vertical", Layout::Vertical },
};

} // namespace Brisk
