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

#include <brisk/gui/GUI.hpp>
#include <brisk/gui/Styles.hpp>
#include <brisk/core/RC.hpp>

namespace Brisk {

namespace Graphene {

constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 0>> mainColor{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 1>> linkColor{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 2>> editorColor{};
constexpr inline Argument<StyleVariableTag<float, styleVarCustomID + 3>> boxRadius{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 4>> menuColor{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 5>> boxBorderColor{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 6>> shadeColor{};
constexpr inline Argument<StyleVariableTag<ColorF, styleVarCustomID + 7>> deepColor{};

RC<const Stylesheet> stylesheet();
Rules lightColors();
Rules darkColors();

} // namespace Graphene

} // namespace Brisk
