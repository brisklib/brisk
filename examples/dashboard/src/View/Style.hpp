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

#include <brisk/gui/Styles.hpp>
#include <brisk/widgets/Graphene.hpp>

namespace App {
using namespace Brisk;

using namespace Graphene;

const inline auto globalStyle = Rules{
    Arg::stylesheet     = Graphene::stylesheet(),
    mainColor           = 0x2b3445_rgb,
    windowColor         = 0x202330_rgb,
    selectedColor       = 0x269426_rgb,
    linkColor           = 0x378AFF_rgb,
    editorColor         = 0xFDFDFD_rgb,
    boxRadius           = 0.f,
    menuColor           = 0xFDFDFD_rgb,
    animationSpeed      = 0.5f,
    boxBorderColor      = 0x070709_rgb,
    shadeColor          = 0x000000'88_rgba,
    deepColor           = 0x000000_rgb,
    focusFrameColor     = 0x03a1fc'c0_rgba,
    hintBackgroundColor = 0xFFE9AD_rgb,
    hintShadowColor     = 0x000000'AA_rgba,
    hintTextColor       = 0x000000_rgb,

    fontFamily          = "Titillium,@icons",
    fontSize            = 16,
};
} // namespace App
