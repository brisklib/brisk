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
 */                                                                                                          \
#pragma once

#include <array>
#include "Color.hpp"

namespace Brisk {

/**
 * @namespace Palette::Material
 * @brief A namespace for Material design colors.
 */
namespace Palette {
namespace Material {

/**
 * @brief Color definitions for Material design.
 */
constexpr inline Color red         = 0xF44336_rgb; ///< Red color.
constexpr inline Color pink        = 0xE91E63_rgb; ///< Pink color.
constexpr inline Color purple      = 0x9C27B0_rgb; ///< Purple color.
constexpr inline Color deep_purple = 0x673AB7_rgb; ///< Deep purple color.
constexpr inline Color indigo      = 0x3F51B5_rgb; ///< Indigo color.
constexpr inline Color blue        = 0x2196F3_rgb; ///< Blue color.
constexpr inline Color light_blue  = 0x03A9F4_rgb; ///< Light blue color.
constexpr inline Color cyan        = 0x00BCD4_rgb; ///< Cyan color.
constexpr inline Color teal        = 0x009688_rgb; ///< Teal color.
constexpr inline Color green       = 0x4CAF50_rgb; ///< Green color.
constexpr inline Color light_green = 0x8BC34A_rgb; ///< Light green color.
constexpr inline Color lime        = 0xCDDC39_rgb; ///< Lime color.
constexpr inline Color yellow      = 0xFFEB3B_rgb; ///< Yellow color.
constexpr inline Color amber       = 0xFFC107_rgb; ///< Amber color.
constexpr inline Color orange      = 0xFF9800_rgb; ///< Orange color.
constexpr inline Color deep_orange = 0xFF5722_rgb; ///< Deep orange color.
constexpr inline Color brown       = 0x795548_rgb; ///< Brown color.
constexpr inline Color grey        = 0x9E9E9E_rgb; ///< Grey color.
constexpr inline Color blue_grey   = 0x607D8B_rgb; ///< Blue-grey color.

} // namespace Material

/**
 * @namespace Palette::Standard
 * @brief A namespace for standard colors.
 */
namespace Standard {

/**
 * @brief Color definitions for standard colors.
 */
constexpr inline Color red     = 0xF22E2B_rgb; ///< Red color.
constexpr inline Color orange  = 0xFF681C_rgb; ///< Orange color.
constexpr inline Color amber   = 0xFC8D00_rgb; ///< Amber color.
constexpr inline Color yellow  = 0xE5AE00_rgb; ///< Yellow color.
constexpr inline Color lime    = 0xA4B600_rgb; ///< Lime color.
constexpr inline Color green   = 0x33BF3B_rgb; ///< Green color.
constexpr inline Color teal    = 0x00BAA7_rgb; ///< Teal color.
constexpr inline Color cyan    = 0x00A5EC_rgb; ///< Cyan color.
constexpr inline Color blue    = 0x0085FF_rgb; ///< Blue color.
constexpr inline Color indigo  = 0x6363ED_rgb; ///< Indigo color.
constexpr inline Color violet  = 0xBC3AB2_rgb; ///< Violet color.
constexpr inline Color fuchsia = 0xE90678_rgb; ///< Fuchsia color.

/**
 * @brief Array of all standard colors.
 */
constexpr inline Color all[]   = {
    Palette::Standard::red,  Palette::Standard::orange, Palette::Standard::amber,  Palette::Standard::yellow,
    Palette::Standard::lime, Palette::Standard::green,  Palette::Standard::teal,   Palette::Standard::cyan,
    Palette::Standard::blue, Palette::Standard::indigo, Palette::Standard::violet, Palette::Standard::fuchsia
};

/**
 * @brief Get a color by index from the standard color array.
 * @param x The index of the color.
 * @return The color at the specified index, wrapped around the array size.
 */
constexpr Color index(unsigned x) {
    return all[(x % std::size(all))];
}

} // namespace Standard
} // namespace Palette

} // namespace Brisk
