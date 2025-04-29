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
#include <brisk/core/Text.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Log.hpp>
#include <unistd.h>

namespace Brisk {

namespace {

const fs::path& userHome() {
    static fs::path homeDir;
    if (homeDir.empty()) {
        const char* home = std::getenv("HOME");
        BRISK_ASSERT(home != nullptr);
        // Before Ubuntu 19, the patched sudo in Ubuntu retained the HOME environment variable, while other
        // Linux distributions changed HOME to the root's home directory
        homeDir = std::string(home);
    }
    return homeDir;
}

const std::map<std::string, std::string, std::less<>>& paths() {
    static std::map<std::string, std::string, std::less<>> cache;
    if (cache.empty()) {
        auto lines = readLines(userHome() / ".config" / "user-dirs.dirs");
        if (!lines.has_value())
            return cache;
        for (const std::string& s : *lines) {
            std::string name;
            std::string value;
            split(s, "=", name, value);
            if (name.empty() || name.find('#') != std::string::npos)
                continue;
            if (value.size() >= 2 && value.starts_with('"') && value.ends_with('"'))
                value = value.substr(1, value.size() - 2);
            if (value.find_first_of("\"'\\") != std::string::npos) {
                LOG_WARN(io, "Got problematic path for {}: {}", name, value);
                continue;
            }
            if (value.starts_with("$HOME")) {
                value.erase(0, 5);
                value.insert(0, userHome());
            }
            cache.insert_or_assign(std::move(name), std::move(value));
        }
    }
    return cache;
}

fs::path get(std::string_view envName, std::string fallback) {
    const auto& p = paths();
    if (auto it = p.find(envName); it != p.end()) {
        return it->second;
    }
    return userHome() / std::move(fallback);
}

} // namespace

fs::path platformDefaultFolder(DefaultFolder folder) {
    switch (folder) {
    case DefaultFolder::Home:
        return userHome();
    case DefaultFolder::Documents:
        return get("XDG_DOCUMENTS_DIR", "Documents");
    case DefaultFolder::Music:
        return get("XDG_MUSIC_DIR", "Music");
    case DefaultFolder::Pictures:
        return get("XDG_PICTURES_DIR", "Pictures");
    case DefaultFolder::SystemData:
        return "/usr/local/share/";
    case DefaultFolder::UserData:
        return get("XDG_DATA_HOME", ".local/share");
    default:
        BRISK_UNREACHABLE();
    }
}

std::vector<fs::path> fontFolders() {
    return {
        "/usr/share/fonts",
        "/usr/local/share/fonts",
        userHome() / ".local/share/fonts",
    };
}

fs::path executablePath() {
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len < 0)
        return {};
    return std::string(path, len);
}

} // namespace Brisk
