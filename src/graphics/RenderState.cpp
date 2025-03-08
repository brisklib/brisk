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
#include <brisk/graphics/RenderState.hpp>
#include <cstring>

namespace Brisk {

bool RenderState::operator==(const RenderState& state) const {
    return memcmp(this, &state, sizeof(RenderState)) == 0;
}

bool RenderState::compare(const RenderState& second) const {
    return std::memcmp(reinterpret_cast<const uint8_t*>(this) + RenderState::compare_offset,
                       reinterpret_cast<const uint8_t*>(&second) + RenderState::compare_offset,
                       sizeof(RenderState) - RenderState::compare_offset) == 0;
}

void RenderState::premultiply() {
    fillColor1   = fillColor1.premultiply();
    fillColor2   = fillColor2.premultiply();
    strokeColor1 = strokeColor1.premultiply();
    strokeColor2 = strokeColor2.premultiply();
}

void Tag::SubpixelMode::apply(const Type& value, RenderStateEx& state) {
    state.subpixelMode = value;
}

void Tag::FillColor::apply(const ColorF& value, RenderStateEx& state) {
    state.fillColor1 = value;
    state.fillColor2 = value;
}

void Tag::StrokeColor::apply(const ColorF& value, RenderStateEx& state) {
    state.strokeColor1 = value;
    state.strokeColor2 = value;
}

void Tag::FillColors::apply(const GradientColors& value, RenderStateEx& state) {
    state.fillColor1 = value.color1;
    state.fillColor2 = value.color2;
}

void Tag::StrokeColors::apply(const GradientColors& value, RenderStateEx& state) {
    state.strokeColor1 = value.color1;
    state.strokeColor2 = value.color2;
}

void Tag::PaintOpacity::apply(const Tag::PaintOpacity::Type& value, RenderStateEx& state) {
    state.opacity = value;
}

void Tag::ContourSize::apply(const Tag::ContourSize::Type& value, RenderStateEx& state) {
    state.strokeWidth = value * 2;
}

void Tag::ContourColor::apply(const Tag::ContourColor::Type& value, RenderStateEx& state) {
    state.fillColor1 = value;
    state.fillColor2 = value;
}

void Tag::StrokeWidth::apply(const Tag::StrokeWidth::Type& value, RenderStateEx& state) {
    state.strokeWidth = value;
}

void Tag::Multigradient::apply(const Tag::Multigradient::Type& value, RenderStateEx& state) {
    state.gradientHandle = value;
}

void Tag::Scissor::apply(const Tag::Scissor::Type& value, RenderStateEx& state) {
    state.scissor = value;
}

void Tag::Patterns::apply(const Tag::Patterns::Type& value, RenderStateEx& state) {
    state.hpattern     = value.hpattern;
    state.vpattern     = value.vpattern;
    state.patternScale = std::max(1, value.scale);
}

void Tag::BlurRadius::apply(const Tag::BlurRadius::Type& value, RenderStateEx& state) {
    state.blurRadius = value;
}

void Tag::BlurDirections::apply(const Tag::BlurDirections::Type& value, RenderStateEx& state) {
    state.blurDirections = value;
}

void Tag::TextureChannel::apply(const Tag::TextureChannel::Type& value, RenderStateEx& state) {
    state.textureChannel = value;
}

void Tag::CoordMatrix::apply(const Tag::CoordMatrix::Type& value, RenderStateEx& state) {
    state.coordMatrix = value;
}

void Tag::SamplerMode::apply(const Tag::SamplerMode::Type& value, RenderStateEx& state) {
    state.samplerMode = value;
}

} // namespace Brisk
