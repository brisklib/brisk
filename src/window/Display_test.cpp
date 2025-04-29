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
#include <catch2/catch_all.hpp>
#include "../core/Catch2Utils.hpp"
#include <brisk/window/Display.hpp>
#include <brisk/core/Reflection.hpp>

using namespace Brisk;

static void showDisplay(Rc<Display> display) {
    fmt::println("            pointer = {}", (void*)display.get());
    fmt::println("               name = {}", display->name());
    fmt::println("                 id = {}", display->id());
    fmt::println("        adapterName = {}", display->adapterName());
    fmt::println("          adapterId = {}", display->adapterId());
    fmt::println("           position = {}", display->position());
    fmt::println("               size = {}", display->size());
    fmt::println("         resolution = {}", display->resolution());
    fmt::println("   nativeResolution = {}", display->nativeResolution());
    fmt::println("           workarea = {}", display->workarea());
    fmt::println("       physicalSize = {}", display->physicalSize());
    fmt::println("                dpi = {}", display->dpi());
    fmt::println("       contentScale = {}", display->contentScale());
    fmt::println("        refreshRate = {}", display->refreshRate());
    fmt::println(" backingScaleFactor = {}", display->backingScaleFactor());
    fmt::println("              flags = {:08X}", (uint32_t)display->flags());
    fmt::println("");
}

TEST_CASE("Display tests") {

    Internal::updateDisplays();

    auto primary = Display::primary();
    REQUIRE(primary.get());
    showDisplay(primary);
    fmt::println("==============================================================\n");
    for (auto display : Display::all()) {
        showDisplay(display);
    }
    fmt::println("==============================================================\n");
    for (auto display : Display::all()) {
        showDisplay(display);
    }
}
