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

#include <brisk/core/Brisk.h>
#include <brisk/core/Bytes.hpp>
#include <unordered_map>
#include <optional>

namespace Brisk {

/**
 * @brief Platform-specific type definition for clipboard format identifiers.
 *
 * Defines the appropriate clipboard format based on the target operating system.
 */
#ifdef BRISK_WINDOWS
using ClipboardFormat = uint32_t;
#endif
#ifdef BRISK_APPLE
using ClipboardFormat = std::string;
#endif
#ifdef BRISK_LINUX
using ClipboardFormat = int32_t;
#endif

/**
 * @brief Structure representing the content of the clipboard.
 *
 * This structure holds optional text content and a map of format-specific data.
 */
struct ClipboardContent {
    /// Optional text content from the clipboard.
    std::optional<std::string> text;

    /// A map of format-specific data stored in the clipboard, keyed by ClipboardFormat.
    std::unordered_map<ClipboardFormat, Bytes> formats;
};

/**
 * @brief Registers a custom clipboard format.
 *
 * @param formatID The identifier of the format to register.
 * @return The registered ClipboardFormat.
 */
ClipboardFormat registerClipboardFormat(std::string_view formatID);

/**
 * @brief Global variable that represents the text format identifier for the clipboard.
 */
extern ClipboardFormat textFormat;

/**
 * @brief Checks if the clipboard contains data in the specified format.
 *
 * @param format The format to check for.
 * @return True if the clipboard has data in the specified format, otherwise false.
 */
bool clipboardHasFormat(ClipboardFormat format);

/**
 * @brief Checks if the clipboard contains text data.
 *
 * @return True if the clipboard has text data, otherwise false.
 */
inline bool clipboardHasText() {
    return clipboardHasFormat(textFormat);
}

/**
 * @brief Sets the content of the clipboard.
 *
 * @param content The ClipboardContent to set in the clipboard.
 * @return True if the clipboard content was successfully set, otherwise false.
 */
bool setClipboardContent(const ClipboardContent& content);

/**
 * @brief Retrieves the content of the clipboard for the specified formats.
 *
 * @param formats A list of ClipboardFormats to retrieve.
 * @return The ClipboardContent containing the data in the requested formats.
 */
[[nodiscard]] ClipboardContent getClipboardContent(std::initializer_list<ClipboardFormat> formats);

/**
 * @brief Copies text content to the clipboard.
 *
 * @param content The text content to copy to the clipboard.
 * @return True if the text content was successfully copied, otherwise false.
 */
inline bool copyTextToClipboard(std::string_view content) {
    return setClipboardContent(ClipboardContent{ std::string(content), {} });
}

/**
 * @brief Copies binary data to the clipboard for a specific format.
 *
 * @param content The binary data to copy.
 * @param format The ClipboardFormat to associate with the binary data.
 * @return True if the binary data was successfully copied, otherwise false.
 */
inline bool copyBytesToClipboard(BytesView content, ClipboardFormat format) {
    return setClipboardContent(ClipboardContent{ std::nullopt, { { format, toBytes(content) } } });
}

/**
 * @brief Retrieves text content from the clipboard.
 *
 * @return An optional string containing the clipboard text if available, otherwise std::nullopt.
 */
inline std::optional<std::string> getTextFromClipboard() {
    return getClipboardContent({ textFormat }).text;
}

/**
 * @brief Retrieves binary data from the clipboard for a specific format.
 *
 * @param format The ClipboardFormat to retrieve data for.
 * @return An optional vector of bytes containing the clipboard data for the specified format, or std::nullopt
 * if not found.
 */
inline std::optional<Bytes> getBytesFromClipboard(ClipboardFormat format) {
    ClipboardContent content = getClipboardContent({ format });
    if (auto it = content.formats.find(format); it != content.formats.end()) {
        return std::move(it->second);
    }
    return std::nullopt;
}

} // namespace Brisk
