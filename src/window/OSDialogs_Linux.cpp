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
#include <brisk/window/OSDialogs.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/core/internal/Expected.hpp>
#include <brisk/core/Localization.hpp>
#include <brisk/core/Text.hpp>

namespace Brisk {

// Helper function to execute commands and retrieve output
static std::pair<std::string, int> execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return { {}, INT32_MAX };
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int status = pclose(pipe);
    return { std::move(result), status };
}

static std::string escapeShellArg(std::string_view arg) {
    std::string escaped;
    escaped += '\'';
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\\''"; // Escape single quotes
        } else {
            escaped += c;
        }
    }
    escaped += '\'';
    return escaped;
}

DialogResult Shell::showDialog(std::string_view title, std::string_view message, DialogButtons buttons,
                               MessageBoxType type) {
    std::vector<std::pair<std::string, DialogResult>> localizedLabels;
    std::string buttonSetup;

    for (DialogButtons btn : { DialogButtons::OK, DialogButtons::Yes, DialogButtons::Retry,
                               DialogButtons::Close, DialogButtons::Cancel, DialogButtons::No }) {
        if (btn && buttons) {
            std::string localized = locale->translate(fmt::to_string(btn));
            if (buttonSetup.empty())
                buttonSetup += "--ok-label=" + escapeShellArg(localized);
            else
                buttonSetup += " --extra-button=" + escapeShellArg(localized);
            localizedLabels.push_back(std::pair{ std::move(localized), static_cast<DialogResult>(btn) });
        }
    }

    // Determine dialog type
    std::string dialogType;
    switch (type) {
    case MessageBoxType::Info:
        dialogType = "--info";
        break;
    case MessageBoxType::Warning:
        dialogType = "--warning";
        break;
    case MessageBoxType::Error:
        dialogType = "--error";
        break;
    default:
        dialogType = "--info";
        break;
    }

    // Construct zenity command
    std::string command = "zenity " + dialogType + " --title=" + escapeShellArg(title) +
                          " --text=" + escapeShellArg(message) + " " + buttonSetup + " 2>/dev/null";

    // Execute the command
    std::pair<std::string, int> result = execCommand(command);
    if (result.second == INT32_MAX) {
        return DialogResult::Other;
    }

    std::string resultBtn = trim(result.first);
    if (resultBtn.empty()) {
        return localizedLabels.front().second;
    } else {
        return keyToValue(localizedLabels, resultBtn).value_or(DialogResult::Other);
    }
}

void Shell::openURLInBrowser(std::string_view url) {
    std::string command                = "xdg-open " + escapeShellArg(url) + " &";
    std::pair<std::string, int> result = execCommand(command);
    if (result.second != 0) {
        LOG_ERROR(dialogs, "xdg-open failed with exit code {}", result.second);
    }
}

void Shell::openFileInDefaultApp(const fs::path& path) {
    openURLInBrowser(path.string());
}

void Shell::openFolder(const fs::path& path) {
    openURLInBrowser(path.string());
}

std::optional<fs::path> Shell::showOpenDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::string command = "zenity --file-selection --title=" + escapeShellArg("Open file"_tr);
    if (!defaultPath.empty()) {
        command += " --filename=" + escapeShellArg(defaultPath.string());
    }

    std::pair<std::string, int> result = execCommand(command);
    if (result.second != 0) {
        LOG_ERROR(dialogs, "zenity failed with exit code {}", result.second);
        return std::nullopt;
    }

    return fs::path(trim(result.first)); // Remove trailing newline
}

std::vector<fs::path> Shell::showOpenDialogMulti(std::span<const FileDialogFilter> filters,
                                                 const fs::path& defaultPath) {
    std::string command =
        "zenity --file-selection --multiple --separator=':' --title=" + escapeShellArg("Open file"_tr);
    if (!defaultPath.empty()) {
        command += " --filename=" + escapeShellArg(defaultPath.string());
    }

    std::pair<std::string, int> result = execCommand(command);
    if (result.second != 0) {
        LOG_ERROR(dialogs, "zenity failed with exit code {}", result.second);
        return {};
    }
    result.first                        = trim(result.first);
    std::vector<std::string_view> paths = split(result.first, ':');
    std::vector<fs::path> list(paths.begin(), paths.end());
    return list;
}

std::optional<fs::path> Shell::showSaveDialog(std::span<const FileDialogFilter> filters,
                                              const fs::path& defaultPath) {
    std::string command =
        "zenity --file-selection --save --confirm-overwrite --title=" + escapeShellArg("Save file"_tr);
    if (!defaultPath.empty()) {
        command += " --filename=" + escapeShellArg(defaultPath.string());
    }

    std::pair<std::string, int> result = execCommand(command);
    if (result.second != 0) {
        LOG_ERROR(dialogs, "zenity failed with exit code {}", result.second);
        return std::nullopt;
    }

    return fs::path(trim(result.first)); // Remove trailing newline
}

std::optional<fs::path> Shell::showFolderDialog(const fs::path& defaultPath) {
    std::string command = "zenity --file-selection --directory --title=" + escapeShellArg("Select folder"_tr);
    if (!defaultPath.empty()) {
        command += " --filename=" + escapeShellArg(defaultPath.string() + "/");
    }

    std::pair<std::string, int> result = execCommand(command);
    if (result.second != 0) {
        LOG_ERROR(dialogs, "zenity failed with exit code {}", result.second);
        return std::nullopt;
    }

    return fs::path(trim(result.first)); // Remove trailing newline
}
} // namespace Brisk
