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
#pragma once

#include <optional>
#include <string_view>
#include "Color.hpp"

namespace Brisk {

/**
 * @brief Interface for handling SAX-like HTML parsing events.
 */
struct HtmlSax {
    /**
     * @brief Called before parsing begins.
     */
    virtual void openDocument() {}

    /**
     * @brief Called after parsing is complete, even if parsing fails.
     */
    virtual void closeDocument() {}

    /**
     * @brief Called when an HTML tag is opened.
     * @param tagName Pointer to the original HTML string containing the tag name.
     *        Self-closing tags will trigger closeTag immediately after this.
     */
    virtual void openTag(std::string_view tagName) {}

    /**
     * @brief Called when an attribute name is parsed.
     * @param name Pointer to the original HTML string containing the attribute name.
     */
    virtual void attrName(std::string_view name) {}

    /**
     * @brief Called when a fragment of an attribute value is parsed.
     * @param value Temporary memory containing the value fragment. Copy if needed outside this callback.
     */
    virtual void attrValueFragment(std::string_view value) {}

    /**
     * @brief Called when an attribute parsing is complete.
     */
    virtual void attrFinished() {}

    /**
     * @brief Called when a fragment of text is parsed.
     * @param text Temporary memory containing the text fragment. Copy if needed outside this callback.
     */
    virtual void textFragment(std::string_view text) {}

    /**
     * @brief Called when text parsing is complete.
     */
    virtual void textFinished() {}

    /**
     * @brief Called when an HTML tag is closed.
     */
    virtual void closeTag() {}
};

/**
 * @brief Parses an HTML string and triggers HtmlSax callbacks during parsing.
 * @param html The HTML content to parse.
 * @param sax The HtmlSax implementation to handle parsing events.
 * @return true if parsing succeeds, false otherwise.
 */
bool parseHtml(std::string_view html, HtmlSax* sax);

/**
 * @brief Decodes an HTML character entity (e.g., "&gt;" -> ">", "&nbsp;" -> "\xA0", non-breaking space).
 * @param name The entity name to decode.
 * @return The decoded character as a string view.
 */
std::string_view htmlDecodeChar(std::string_view name);

/**
 * @brief Parses an HTML color string into a Color object.
 * @param colorText The color string in formats like #RGB, #RGBA, #RRGGBB, #RRGGBBAA, or a named color.
 * @return A Color object if parsing succeeds, or std::nullopt if it fails.
 */
std::optional<Color> parseHtmlColor(std::string_view colorText);

} // namespace Brisk
