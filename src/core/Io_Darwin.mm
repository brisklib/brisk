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
#include <mach-o/dyld.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <brisk/core/Io.hpp>
#include <sys/stat.h>
#include <brisk/core/Text.hpp>

namespace Brisk {

fs::path platformDefaultFolder(DefaultFolder folder) {
    struct passwd* pwd = getpwuid(getuid());
    fs::path home      = pwd->pw_dir;
    fs::path root      = "/";
    fs::path sys       = "/System";
    switch (folder) {
    case DefaultFolder::Home:
        return home;
    case DefaultFolder::Documents:
        return home / "Documents";
    case DefaultFolder::Music:
        return home / "Music";
    case DefaultFolder::Pictures:
        return home / "Pictures";
    case DefaultFolder::UserData:
        return home / "Library" / "Application Support";
    case DefaultFolder::SystemData:
        return root / "Library" / "Application Support";
    default:
        return home / "Documents";
    }
}

std::vector<fs::path> fontFolders() {
    return {
        "/System/Library/Fonts", // System font folder must be first
        "/Library/Fonts",
        platformDefaultFolder(DefaultFolder::Home) / "Library/Fonts",
    };
}

fs::path executablePath() {
    char path[PATH_MAX] = { 0 };
    uint32_t size       = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
        return path;
    return {};
}

} // namespace Brisk
