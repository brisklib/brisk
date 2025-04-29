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
#include <brisk/core/System.hpp>
#include <charconv>
#include <sys/utsname.h>

#include <brisk/core/Text.hpp>

#include <fmt/format.h>

namespace Brisk {

static int toInt(std::string_view str) {
    int val;
    auto [ptr, ec] = std::from_chars(str.begin(), str.end(), val);
    if (ec != std::errc()) {
        return 0;
    }
    return val;
}

OsVersion osVersion() {
    utsname vers{};
    if (uname(&vers))
        return {};
    std::vector<std::string_view> parts = split(vers.release, ".");
    if (parts.size() < 3)
        return { 0, 0, 0 };
    return {
        static_cast<uint16_t>(toInt(parts[0])),
        static_cast<uint16_t>(toInt(parts[1])),
        static_cast<uint32_t>(toInt(parts[2])),
    };
}

std::string osName() {
    return "Linux";
}

} // namespace Brisk
