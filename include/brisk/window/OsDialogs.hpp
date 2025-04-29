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

#include <brisk/core/internal/Optional.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/App.hpp>

namespace Brisk {

/**
 * @brief Types of message boxes.
 *
 * Defines the different types of message boxes that can be displayed to the user.
 */
enum class MessageBoxType {
    None,     ///< No message box type specified.
    Info,     ///< Informational message box.
    Warning,  ///< Warning message box.
    Error,    ///< Error message box.
    Security, ///< Security-related message box.
};

/**
 * @brief Dialog button configurations.
 *
 * This enum defines various dialog buttons and allows the combination of buttons
 * using bitwise OR operations.
 */
enum class DialogButtons {
    None        = 0,      ///< No button.
    OK          = 1 << 0, ///< "OK" button.
    Yes         = 1 << 1, ///< "Yes" button.
    Cancel      = 1 << 2, ///< "Cancel" button.
    No          = 1 << 3, ///< "No" button.
    Close       = 1 << 4, ///< "Close" button.
    Retry       = 1 << 5, ///< "Retry" button.

    OKCancel    = OK | Cancel,       ///< Combination of "OK" and "Cancel" buttons.
    YesNo       = Yes | No,          ///< Combination of "Yes" and "No" buttons.
    YesNoCancel = Yes | No | Cancel, ///< Combination of "Yes", "No", and "Cancel" buttons.
};

/**
 * @brief The result of a dialog interaction.
 *
 * This enum represents the outcome of a dialog, corresponding to the pressed button.
 * The `[[nodiscard]]` attribute ensures the result is not discarded.
 */
enum class [[nodiscard]] DialogResult {
    OK     = static_cast<int>(DialogButtons::OK),     ///< "OK" button pressed or equivalent action.
    Cancel = static_cast<int>(DialogButtons::Cancel), ///< "Cancel" button pressed or equivalent action.
    Yes    = static_cast<int>(DialogButtons::Yes),    ///< "Yes" button pressed or equivalent action.
    No     = static_cast<int>(DialogButtons::No),     ///< "No" button pressed or equivalent action.
    Close  = static_cast<int>(DialogButtons::Close),  ///< "Close" button pressed or equivalent action.
    Retry  = static_cast<int>(DialogButtons::Retry),  ///< "Retry" button pressed or equivalent action.

    Other  = 0, ///< Other result (not explicitly defined by a button).
};

class Shell {
public:
    /**
     * @brief Shows a dialog with the given title, message, and buttons.
     *
     * @param title The title of the dialog.
     * @param message The message to display in the dialog.
     * @param buttons The buttons to include in the dialog.
     * @param type The type of message box (default is Info).
     * @return DialogResult The result of the dialog based on the button clicked.
     */
    static DialogResult showDialog(std::string_view title, std::string_view message, DialogButtons buttons,
                                   MessageBoxType type = MessageBoxType::Info);

    /**
     * @brief Shows a dialog with the given message and buttons.
     *
     * This overload of showDialog automatically uses the application's name as the title.
     *
     * @param message The message to display in the dialog.
     * @param buttons The buttons to include in the dialog.
     * @param type The type of message box (default is Info).
     * @return DialogResult The result of the dialog based on the button clicked.
     */
    static DialogResult showDialog(std::string_view message, DialogButtons buttons,
                                   MessageBoxType type = MessageBoxType::Info) {
        return showDialog(appMetadata.name, message, buttons, type);
    }

    /**
     * @brief Displays a message box with a title and message.
     *
     * This function shows a message box with a single "OK" button.
     *
     * @param title The title of the message box.
     * @param message The message to display in the message box.
     * @param type The type of message box (default is Info).
     */
    static void showMessage(std::string_view title, std::string_view message,
                            MessageBoxType type = MessageBoxType::Info) {
        std::ignore = showDialog(title, message, DialogButtons::OK, type);
    }

    /**
     * @brief Displays a message box with a message.
     *
     * This function shows a message box with the application's name as the title
     * and a single "OK" button.
     *
     * @param message The message to display in the message box.
     * @param type The type of message box (default is Info).
     */
    static void showMessage(std::string_view message, MessageBoxType type = MessageBoxType::Info) {
        std::ignore = showDialog(appMetadata.name, message, DialogButtons::OK, type);
    }

    /**
     * @brief Opens the given URL in the default web browser.
     *
     * @param url The URL to open.
     */
    static void openURLInBrowser(std::string_view url);

    /**
     * @brief Opens a file in the default application.
     *
     * @param path The file path to open.
     */
    static void openFileInDefaultApp(const fs::path& path);

    /**
     * @brief Opens a folder in the system's file explorer.
     *
     * @param path The path to the folder to open.
     */
    static void openFolder(const fs::path& path);

    /**
     * @brief Structure representing a filter for file dialogs.
     *
     * Defines file type filters and a description for use in open/save file dialogs.
     */
    struct FileDialogFilter {
        std::vector<std::string> filters; ///< List of file filters (e.g., "*.jpg", "*.txt").
        std::string description;          ///< Description of the filter (e.g., "JPEG or Text").

        /**
         * @brief Constructs a FileDialogFilter with multiple filters.
         *
         * @param filters A vector of file filters.
         * @param description The description of the filter.
         */
        FileDialogFilter(std::vector<std::string> filters, std::string description) {
            this->filters     = std::move(filters);
            this->description = std::move(description);
        }

        /**
         * @brief Constructs a FileDialogFilter with a single filter.
         *
         * @param filter A single file filter.
         * @param description The description of the filter.
         */
        FileDialogFilter(std::string filter, std::string description) {
            this->filters.push_back(std::move(filter));
            this->description = std::move(description);
        }

        /**
         * @brief Constructs a FileDialogFilter with a single filter, using the filter as the description.
         *
         * @param filter A single file filter.
         */
        FileDialogFilter(std::string filter) {
            this->filters.push_back(std::move(filter));
            this->description = this->filters.back();
        }
    };

    /**
     * @brief Creates a filter for selecting any file type.
     *
     * @param description An optional description for the filter.
     * @return FileDialogFilter A file dialog filter for any file.
     */
    [[nodiscard]] static FileDialogFilter anyFile(std::string description = {}) {
        return FileDialogFilter{ std::vector<std::string>{ "*.*" },
                                 description.empty() ? "Any file" : std::move(description) };
    }

    /**
     * @brief Displays an open file dialog.
     *
     * @param filters A set of file filters for the dialog.
     * @param defaultPath The default path for the dialog.
     * @return std::optional<fs::path> The selected file path, or nullopt if the dialog was canceled.
     */
    [[nodiscard]] static std::optional<fs::path> showOpenDialog(std::span<const FileDialogFilter> filters,
                                                                const fs::path& defaultPath);

    /**
     * @brief Displays an open file dialog for multiple file selections.
     *
     * @param filters A set of file filters for the dialog.
     * @param defaultPath The default path for the dialog.
     * @return std::vector<fs::path> The list of selected file paths.
     */
    [[nodiscard]] static std::vector<fs::path> showOpenDialogMulti(std::span<const FileDialogFilter> filters,
                                                                   const fs::path& defaultPath);

    /**
     * @brief Displays a save file dialog.
     *
     * @param filters A set of file filters for the dialog.
     * @param defaultPath The default path for the dialog.
     * @return std::optional<fs::path> The file path to save, or std::nullopt if the dialog was canceled.
     */
    [[nodiscard]] static std::optional<fs::path> showSaveDialog(std::span<const FileDialogFilter> filters,
                                                                const fs::path& defaultPath);

    /**
     * @brief Displays a folder selection dialog.
     *
     * @param defaultPath The default folder path for the dialog.
     * @return std::optional<fs::path> The selected folder path, or std::nullopt if the dialog was canceled.
     */
    [[nodiscard]] static std::optional<fs::path> showFolderDialog(const fs::path& defaultPath);

    /**
     * @brief Displays an open file dialog using an initializer list for filters.
     *
     * @param filters An initializer list of file filters.
     * @param defaultPath The default path for the dialog.
     * @return std::optional<fs::path> The selected file path, or std::nullopt if the dialog was canceled.
     */
    [[nodiscard]] static std::optional<fs::path> showOpenDialog(
        std::initializer_list<FileDialogFilter> filters, const fs::path& defaultPath) {
        return showOpenDialog(std::span<const FileDialogFilter>{ filters }, defaultPath);
    }

    /**
     * @brief Displays an open file dialog for multiple selections using an initializer list for filters.
     *
     * @param filters An initializer list of file filters.
     * @param defaultPath The default path for the dialog.
     * @return std::vector<fs::path> The list of selected file paths.
     */
    [[nodiscard]] static std::vector<fs::path> showOpenDialogMulti(
        std::initializer_list<FileDialogFilter> filters, const fs::path& defaultPath) {
        return showOpenDialogMulti(std::span<const FileDialogFilter>{ filters }, defaultPath);
    }

    /**
     * @brief Displays a save file dialog using an initializer list for filters.
     *
     * @param filters An initializer list of file filters.
     * @param defaultPath The default path for the dialog.
     * @return std::optional<fs::path> The file path to save, or std::nullopt if the dialog was canceled.
     */
    [[nodiscard]] static std::optional<fs::path> showSaveDialog(
        std::initializer_list<FileDialogFilter> filters, const fs::path& defaultPath) {
        return showSaveDialog(std::span<const FileDialogFilter>{ filters }, defaultPath);
    }
};

template <>
inline constexpr std::initializer_list<NameValuePair<DialogButtons>> defaultNames<DialogButtons>{
    { "OK", DialogButtons::OK },         //
    { "Yes", DialogButtons::Yes },       //
    { "Cancel", DialogButtons::Cancel }, //
    { "No", DialogButtons::No },         //
    { "Close", DialogButtons::Close },   //
    { "Retry", DialogButtons::Retry },   //
};

/**
 * @brief Macro to enable bitwise operations for the DialogButtons enum.
 *
 * This macro allows the use of bitwise operators on DialogButtons, enabling
 * combinations of multiple button types.
 */
template <>
constexpr inline bool isBitFlags<DialogButtons> = true;

} // namespace Brisk
