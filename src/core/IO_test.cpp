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
#include <brisk/core/RC.hpp>
#include <brisk/core/Stream.hpp>
#include "Catch2Utils.hpp"
#include <type_traits>

namespace Brisk {

struct Slice {
    BytesView bytes;
    size_t offset = 0;

    Transferred read(uint8_t* data, size_t size) {
        return {};
    }

    Transferred write(const uint8_t* data, size_t size) {
        return {};
    }

    bool flush() {
        return true;
    }
};

TEST_CASE("stdoutStream") {
    CHECK(stdoutStream()->canWrite());
    CHECK(!stdoutStream()->canRead());
    CHECK(stdoutStream()->write("stdout\n\n") == 8);
    CHECK(stderrStream()->write("stderr\n\n") == 8);
    CHECK(!stdinStream()->canWrite());
    CHECK(stdinStream()->canRead());
}

TEST_CASE("defaultPaths") {
    fmt::println("DefaultFolder::Documents = {}", defaultFolder(DefaultFolder::Documents).string());
    fmt::println("DefaultFolder::Pictures = {}", defaultFolder(DefaultFolder::Pictures).string());
    fmt::println("DefaultFolder::Music = {}", defaultFolder(DefaultFolder::Music).string());
    fmt::println("DefaultFolder::UserData = {}", defaultFolder(DefaultFolder::UserData).string());
    fmt::println("DefaultFolder::SystemData = {}", defaultFolder(DefaultFolder::SystemData).string());
    fmt::println("DefaultFolder::Home = {}", defaultFolder(DefaultFolder::Home).string());
    fmt::println("DefaultFolder::VendorUserData = {}", defaultFolder(DefaultFolder::VendorUserData).string());
    fmt::println("DefaultFolder::VendorSystemData = {}",
                 defaultFolder(DefaultFolder::VendorSystemData).string());
    fmt::println("DefaultFolder::VendorHome = {}", defaultFolder(DefaultFolder::VendorHome).string());
    fmt::println("DefaultFolder::AppUserData = {}", defaultFolder(DefaultFolder::AppUserData).string());
    fmt::println("DefaultFolder::AppSystemData = {}", defaultFolder(DefaultFolder::AppSystemData).string());
    fmt::println("DefaultFolder::AppHome = {}", defaultFolder(DefaultFolder::AppHome).string());
}

TEST_CASE("executablePath") {
    fmt::println("executablePath = {}", executablePath().string());
}

} // namespace Brisk
