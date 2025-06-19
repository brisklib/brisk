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

enum class Placement : uint8_t {
    Normal,   // In-flow
    Absolute, // Absolute in parent widget
    Window,   // Absolute in parent window
};

enum class WidgetClip : uint8_t {
    SelfRect       = 1, // Clip to this widget rectangle
    ParentRect     = 2, // Clip to the parent rectangle
    ParentClipRect = 4, // Inherit parent clip rect
    None           = 0, // Don't clip

    Normal         = ParentClipRect | SelfRect,

    Inherit        = ParentClipRect, // Deprecated alias
    All            = Normal,         // Deprecated alias
};

template <>
constexpr inline bool isBitFlags<WidgetClip> = true;

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
    AffectHint       = 1 << 9,
    AffectVisibility = 1 << 10,

    Compound         = 1 << 11,
};

template <>
constexpr inline bool isBitFlags<PropFlags> = true;

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
