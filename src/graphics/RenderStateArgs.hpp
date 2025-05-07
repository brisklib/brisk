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

#include <brisk/graphics/RenderState.hpp>

namespace Brisk {

namespace Tag {

struct SubpixelMode {
    using Type = Brisk::SubpixelMode;

    static void apply(const Type& value, RenderStateEx& state) {
        state.subpixelMode = value;
    }
};

struct FillColor {
    using Type = ColorF;

    static void apply(const Type& value, RenderStateEx& state) {
        state.fillColor1 = value;
        state.fillColor2 = value;
    }
};

struct StrokeColor {
    using Type = ColorF;

    static void apply(const Type& value, RenderStateEx& state) {
        state.strokeColor1 = value;
        state.strokeColor2 = value;
    }
};

struct FillColors {
    using Type = GradientColors;

    static void apply(const Type& value, RenderStateEx& state) {
        state.fillColor1 = value.color1;
        state.fillColor2 = value.color2;
    }
};

struct StrokeColors {
    using Type = GradientColors;

    static void apply(const Type& value, RenderStateEx& state) {
        state.strokeColor1 = value.color1;
        state.strokeColor2 = value.color2;
    }
};

struct PaintOpacity {
    using Type = float;

    static void apply(const Type& value, RenderStateEx& state) {
        state.opacity = value;
    }
};

struct StrokeWidth {
    using Type = float;

    static void apply(const Type& value, RenderStateEx& state) {
        state.strokeWidth = value;
    }
};

struct Multigradient {
    using Type = Rc<GradientResource>;

    static void apply(const Type& value, RenderStateEx& state) {
        state.gradientHandle = value;
    }
};

template <GradientType grad_type>
struct FillGradient {
    using Type = GradientPoints;

    static void apply(const Type& value, RenderStateEx& state) {
        state.gradient       = grad_type;
        state.gradientPoint1 = value.point1;
        state.gradientPoint2 = value.point2;
    }
};

struct Scissor {
    using Type = Quad3;

    static void apply(const Type& value, RenderStateEx& state) {
        state.scissorQuad = value;
    }
};

struct Patterns {
    using Type = PatternCodes;

    static void apply(const Type& value, RenderStateEx& state) {
        state.pattern = value;
    }
};

struct BlurRadius {
    using Type = float;

    static void apply(const Type& value, RenderStateEx& state) {
        state.blurRadius = value;
    }
};

struct BlurDirections {
    using Type = int;

    static void apply(const Type& value, RenderStateEx& state) {
        state.blurDirections = value;
    }
};

struct TextureChannel {
    using Type = int;

    static void apply(const Type& value, RenderStateEx& state) {
        state.textureChannel = value;
    }
};

struct CoordMatrix {
    using Type = Matrix;

    static void apply(const Type& value, RenderStateEx& state) {
        state.coordMatrix = state.coordMatrix * value;
    }
};

struct SamplerMode {
    using Type = Brisk::SamplerMode;

    static void apply(const Type& value, RenderStateEx& state) {
        state.samplerMode = value;
    }
};

struct Scissors {
    using Type = Quad3;

    static void apply(const Type& value, RenderStateEx& state) {
        state.scissorQuad = value;
    }
};

} // namespace Tag

inline namespace Arg {

constexpr inline Argument<Tag::FillColor> fillColor{};
constexpr inline Argument<Tag::StrokeColor> strokeColor{};
constexpr inline Argument<Tag::FillColors> fillColors{};
constexpr inline Argument<Tag::StrokeColors> strokeColors{};
constexpr inline Argument<Tag::StrokeWidth> strokeWidth{};
constexpr inline Argument<Tag::PaintOpacity> paintOpacity{};
constexpr inline Argument<Tag::FillGradient<GradientType::Linear>> linearGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Radial>> radialGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Angle>> angleGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Reflected>> reflectedGradient{};
constexpr inline Argument<Tag::Multigradient> multigradient{};
constexpr inline Argument<Tag::Patterns> patterns{};
constexpr inline Argument<Tag::BlurRadius> blurRadius{};
constexpr inline Argument<Tag::BlurDirections> blurDirections{};
constexpr inline Argument<Tag::TextureChannel> textureChannel{};
constexpr inline Argument<Tag::CoordMatrix> coordMatrix{};
constexpr inline Argument<Tag::SamplerMode> samplerMode{};
constexpr inline Argument<Tag::Scissors> scissors{};

} // namespace Arg

} // namespace Brisk
