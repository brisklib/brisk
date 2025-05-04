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
#include <brisk/core/Text.hpp>
#include <algorithm>
#include <brisk/core/Encoding.hpp>
#include "utf8proc.h"

namespace Brisk {

U8String transformCase(U8StringView str, CaseTransformation mode) {
    if (isAscii(str))
        return asciiTransform(str, [mode](char32_t x) -> char32_t {
            return mode == CaseTransformation::Lower
                       ? static_cast<char32_t>(std::tolower(static_cast<char>(x)))
                       : static_cast<char32_t>(std::toupper(static_cast<char>(x)));
        });
    else
        return utf8Transform(str, mode == CaseTransformation::Lower ? &utf8proc_tolower : &utf8proc_toupper);
}

U16String transformCase(U16StringView str, CaseTransformation mode) {
    return utf16Transform(str, mode == CaseTransformation::Lower ? &utf8proc_tolower : &utf8proc_toupper);
}

U32String transformCase(U32StringView str, CaseTransformation mode) {
    return utf32Transform(str, mode == CaseTransformation::Lower ? &utf8proc_tolower : &utf8proc_toupper);
}

std::string textWordWrap(std::string text, size_t columns) {
    // p points to the first char in line
    size_t p = 0;

    for (;;) {
        // find the next line feed
        size_t e = text.find('\n', p);
        if (e == std::string::npos)
            e = text.size();
        if (e - p > columns) {
            // find the last space before line feed
            size_t s = text.rfind(' ', p + columns);
            if (s != std::string::npos && s > p) {
                text.at(s) = '\n'; // replace by the line feed
                p          = s + 1;
            } else {
                text.insert(p + columns, "\n"); // insert the line feed
                p = p + columns + 1;
            }
        } else {
            p = e + 1;
        }
        if (p >= text.size())
            return text;
    }
}

static bool isASCIISpace(unsigned ch) {
    return ch <= ' ';
}

static bool isASCIINonSpace(unsigned ch) {
    return ch > ' ';
}

std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), isASCIINonSpace));
    return s;
}

std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), isASCIINonSpace).base(), s.end());
    return s;
}

std::string trim(std::string s) {
    return rtrim(ltrim(std::move(s)));
}

std::string_view ltrim(std::string_view s) {
    auto firstNonSpace = std::find_if(s.begin(), s.end(), isASCIINonSpace);
    return s.substr(firstNonSpace - s.begin());
}

std::string_view rtrim(std::string_view s) {
    auto lastNonSpace = std::find_if(s.rbegin(), s.rend(), isASCIINonSpace).base();
    return s.substr(0, lastNonSpace - s.begin());
}

std::string_view trim(std::string_view s) {
    return rtrim(ltrim(std::move(s)));
}

void split(std::string_view text, std::string_view delimiter, std::string_view& s1, std::string_view& s2) {
    size_t prev_pos  = 0;
    size_t start_pos = 0;
    if ((start_pos = text.find(delimiter, prev_pos)) != std::string_view::npos) {
        s1       = text.substr(prev_pos, start_pos - prev_pos);
        prev_pos = start_pos + delimiter.size();
    }
    s2 = text.substr(prev_pos);
}

std::u32string shorten(std::u32string s, size_t maxLength, float position, std::u32string_view ellipsis) {
    if (s.size() <= maxLength)
        return s;
    size_t cutNum = s.size() - maxLength + ellipsis.size();
    size_t cutPos = (s.size() - cutNum) * std::max(std::min(position, 1.f), 0.f);
    s.replace(cutPos, cutNum, ellipsis);
    return s;
}
} // namespace Brisk
