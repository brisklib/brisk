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
#include <catch2/catch_all.hpp>
#include <brisk/core/DynamicLibrary.hpp>
#include "Catch2Utils.hpp"
#include <thread>

using namespace Brisk;

TEST_CASE("DynamicLibrary") {
#ifdef BRISK_WINDOWS
    auto lib = DynamicLibrary::load("kernel32.dll");
    REQUIRE(lib);
    auto func = lib->func<unsigned long __stdcall()>("GetTickCount");
    fmt::println("GetTickCount#1 = {}", func());
    std::this_thread::sleep_for(std::chrono::milliseconds(32));
    fmt::println("GetTickCount#2 = {}", func());
    CHECK(func() > 0);
    lib = nullptr;
#endif
}
