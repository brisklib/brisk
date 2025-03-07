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

constexpr inline Argument<StyleVariableTag<ColorW, "mainColor"_hash>> mainColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "linkColor"_hash>> linkColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "editorColor"_hash>> editorColor{};
constexpr inline Argument<StyleVariableTag<float, "boxRadius"_hash>> boxRadius{};
constexpr inline Argument<StyleVariableTag<ColorW, "menuColor"_hash>> menuColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "boxBorderColor"_hash>> boxBorderColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "shadeColor"_hash>> shadeColor{};
constexpr inline Argument<StyleVariableTag<ColorW, "deepColor"_hash>> deepColor{};

RC<const Stylesheet> stylesheet();
Rules lightColors();
Rules darkColors();

} // namespace Graphene

} // namespace Brisk
