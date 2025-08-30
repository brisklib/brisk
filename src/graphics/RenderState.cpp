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
}

RenderStateEx::RenderStateEx(ShaderType shader, RenderStateExArgs args) {
    this->shader = shader;
    args.apply(this);
}

RenderStateEx::RenderStateEx(ShaderType shader, int instances, RenderStateExArgs args) {
    this->instances = instances;
    this->shader    = shader;
    args.apply(this);
}
} // namespace Brisk
