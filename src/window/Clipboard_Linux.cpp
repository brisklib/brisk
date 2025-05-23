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
#include <brisk/core/Threading.hpp>
#include <brisk/window/Clipboard.hpp>
#include <brisk/core/Encoding.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Log.hpp>

#include <GLFW/glfw3.h>

namespace Brisk {

Clipboard::Format Clipboard::textFormat = {};

bool Clipboard::setContent(const Content& content) {
    if (content.text) {
        mainScheduler->dispatch([text = content.text]() {
            glfwSetClipboardString(nullptr, text->c_str());
        });
        return true;
    }
    return false;
}

auto Clipboard::getContent(std::initializer_list<Format> formats) -> Content {
    Content content;

    content.text = mainScheduler->dispatchAndWait([]() -> std::optional<std::string> {
        if (const char* str = glfwGetClipboardString(nullptr)) {
            return str;
        }
        return std::nullopt;
    });
    return content;
}

bool Clipboard::hasFormat(Format format) {
    if (format == textFormat) {
        return glfwGetClipboardString(nullptr) != nullptr;
    }
    return false;
}

auto Clipboard::registerFormat(std::string_view formatId) -> Format {
    LOG_WARN(clipboard, "Not implemented");
    return {};
}

} // namespace Brisk
