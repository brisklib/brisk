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

#include <brisk/graphics/I18n.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <utf8proc.h>

namespace Brisk {

bool icuAvailable                                 = false;

Internal::TextBreakIterator::~TextBreakIterator() = default;

namespace Internal {
// Split text into runs of the same direction
std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection) {
    return { TextRun{ defaultDirection, 0, int32_t(text.size()), 0, nullptr } };
}
} // namespace Internal

namespace {

bool isCategoryWithin(char32_t codepoint, int32_t categoryFirst, int32_t categoryLast) {
    const int32_t category = utf8proc_category(codepoint);
    return category >= categoryFirst && category <= categoryLast;
}

bool isSplit(char32_t previous, char32_t current, TextBreakMode mode) {
    switch (mode) {
    case TextBreakMode::Grapheme:
        return utf8proc_grapheme_break(previous, current);
    case TextBreakMode::Word:
        return isCategoryWithin(previous, UTF8PROC_CATEGORY_LU, UTF8PROC_CATEGORY_LO) !=
               isCategoryWithin(current, UTF8PROC_CATEGORY_LU, UTF8PROC_CATEGORY_LO);
    case TextBreakMode::Line:
        return isCategoryWithin(previous, UTF8PROC_CATEGORY_ZS, UTF8PROC_CATEGORY_ZP) &&
               !isCategoryWithin(current, UTF8PROC_CATEGORY_ZS, UTF8PROC_CATEGORY_ZP);
    default:
        BRISK_UNREACHABLE();
    }
}

class TextBreakIteratorSimple final : public Internal::TextBreakIterator {
public:
    TextBreakMode mode;
    std::u32string text;
    size_t pos = 1;

    TextBreakIteratorSimple(std::u32string_view text, TextBreakMode mode) : mode(mode), text(text) {}

    ~TextBreakIteratorSimple() = default;

    std::optional<uint32_t> next() {
        if (text.empty())
            return std::nullopt;

        for (; pos <= text.size(); ++pos) {
            if (pos == text.size() || isSplit(text[pos - 1], text[pos], mode)) {
                return pos++;
            }
        }
        return std::nullopt;
    }
};
} // namespace

RC<Internal::TextBreakIterator> Internal::textBreakIterator(std::u32string_view text, TextBreakMode mode) {
    return rcnew TextBreakIteratorSimple(text, mode);
}
} // namespace Brisk
