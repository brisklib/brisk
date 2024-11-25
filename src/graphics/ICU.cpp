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

#include <brisk/core/internal/Debug.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <utf8proc.h>

#ifdef ICUDT_SIZE

#include <brisk/core/Time.hpp>
#include <brisk/core/Embed.hpp>
#include <unicode/putil.h>
#include <unicode/uclean.h>
#include <unicode/ubidi.h>
#include <unicode/brkiter.h>
#include <resources/icudt.hpp>

// Externally declare the ICU data array. This array will hold the ICU data.
extern "C" {
unsigned char icudt74_dat[ICUDT_SIZE];
}

namespace Brisk {

bool icuAvailable = true;

// Uncompress and initialize ICU data.
static void uncompressICUData() {
    static bool icuDataInit = false;
    if (icuDataInit)
        return;

    // Unpack the ICU data.
    auto&& b = icudt();

    // Assert that the size of the retrieved data matches the expected size.
    BRISK_ASSERT(b.size() == ICUDT_SIZE);

    // Copy the uncompressed ICU data into the external data array.
    memcpy(icudt74_dat, b.data(), b.size());

    icuDataInit     = true;

    // Initialize ICU with error checking.
    UErrorCode uerr = U_ZERO_ERROR;
    u_init(&uerr);

    if (uerr != UErrorCode::U_ZERO_ERROR) {
        // Throw an exception if there was an error, including the error name.
        throwException(EUnicode("ICU Error: {}", u_errorName(uerr)));
    }
}

struct UBiDiDeleter {
    void operator()(UBiDi* ptr) {
        ubidi_close(ptr);
    }
};

[[noreturn]] static void handleICUErr(UErrorCode err) {
    throwException(EUnicode("ICU Error: {}", safeCharPtr(u_errorName(err))));
}

static std::mutex breakIteratorMutex;
static std::unique_ptr<icu::BreakIterator> breakIterators[3];

static std::unique_ptr<icu::BreakIterator> createBreakIterator(TextBreakMode mode) {
    uncompressICUData();
    UErrorCode uerr = U_ZERO_ERROR;
    std::unique_ptr<icu::BreakIterator> iter;
    switch (mode) {
    case TextBreakMode::Grapheme:
        iter.reset(icu::BreakIterator::createCharacterInstance(icu::Locale::getDefault(), uerr));
        break;
    case TextBreakMode::Word:
        iter.reset(icu::BreakIterator::createWordInstance(icu::Locale::getDefault(), uerr));
        break;
    case TextBreakMode::Line:
        iter.reset(icu::BreakIterator::createLineInstance(icu::Locale::getDefault(), uerr));
        break;
    default:
        BRISK_UNREACHABLE();
    }
    if (U_FAILURE(uerr)) {
        handleICUErr(uerr);
    } else {
        return iter;
    }
}

template <typename Fn>
static auto withBreakIterator(TextBreakMode mode, Fn&& fn) {
    std::unique_lock<std::mutex> lk(breakIteratorMutex, std::defer_lock_t{});
    if (lk.try_lock()) {
        if (!breakIterators[+mode])
            breakIterators[+mode] = createBreakIterator(mode);
        return fn(*breakIterators[+mode]);
    } else {
        std::unique_ptr<icu::BreakIterator> iter(createBreakIterator(mode));
        return fn(*iter);
    }
}

std::vector<uint32_t> textBreakPositions(std::u32string_view text, TextBreakMode mode) {
    return withBreakIterator(mode, [text](icu::BreakIterator& iter) {
        std::vector<uint32_t> result;
        u16string u16 = utf32ToUtf16(text);
        icu::UnicodeString ustr(u16.data(), u16.size());
        iter.setText(ustr);
        size_t codepoints = 0;
        result.clear();
        result.push_back(0);
        int32_t p    = iter.next();
        int32_t oldp = 0;
        while (p != icu::BreakIterator::DONE) {
            codepoints +=
                utf16Codepoints(std::u16string_view{ u16.data() + oldp, static_cast<size_t>(p - oldp) });
            result.push_back(codepoints);
            oldp = p;
            p    = iter.next();
        }
        return result;
    });
}

static TextDirection toDir(UBiDiDirection direction) {
    return direction == UBIDI_LTR ? TextDirection::LTR : TextDirection::RTL;
}

static TextDirection toDir(UBiDiLevel level) {
    return (level & 1) ? TextDirection::RTL : TextDirection::LTR;
}

#define HANDLE_UERROR(returncode)                                                                            \
    if (U_FAILURE(uerr)) {                                                                                   \
        handleICUErr(uerr);                                                                                  \
        return returncode;                                                                                   \
    }

namespace Internal {
// Split text into runs of the same direction
std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection) {
    uncompressICUData();
    std::vector<TextRun> textRuns;
    UErrorCode uerr = U_ZERO_ERROR;
    std::unique_ptr<UBiDi, UBiDiDeleter> bidi(ubidi_openSized(0, 0, &uerr));
    HANDLE_UERROR(textRuns)

    std::u16string u16 = utf32ToUtf16(text);
    ubidi_setPara(bidi.get(), u16.data(), u16.size(),
                  defaultDirection == TextDirection::LTR ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL, nullptr,
                  &uerr);
    HANDLE_UERROR(textRuns)

    UBiDiDirection direction = ubidi_getDirection(bidi.get());
    if (direction != UBIDI_MIXED) {
        textRuns.push_back(TextRun{
            toDir(direction),
            0,
            (int32_t)text.size(),
            0,
            nullptr,
        });
    } else {
        int32_t count = ubidi_countRuns(bidi.get(), &uerr);
        HANDLE_UERROR(textRuns)
        int32_t codepoints = 0;
        int32_t u16chars   = 0;
        for (int i = 0; i < count; ++i) {
            TextRun r;
            r.face = nullptr;
            int32_t u16length;
            UBiDiLevel level;
            ubidi_getLogicalRun(bidi.get(), u16chars, &u16length, &level);
            u16length -= u16chars;
            r.direction   = toDir(level);
            r.begin       = codepoints;
            r.end         = codepoints + utf16Codepoints(u16string_view(u16).substr(u16chars, u16length));
            codepoints    = r.end;
            r.visualOrder = ubidi_getVisualIndex(bidi.get(), u16chars, &uerr);
            HANDLE_UERROR(textRuns)
            textRuns.push_back(r);
            u16chars += u16length;
        }
    }
    return textRuns;
}
} // namespace Internal

} // namespace Brisk

#else

namespace Brisk {

bool icuAvailable = false;

static bool isCategoryWithin(char32_t codepoint, int32_t categoryFirst, int32_t categoryLast) {
    const int32_t category = utf8proc_category(codepoint);
    return category >= categoryFirst && category <= categoryLast;
}

static bool isSplit(char32_t previous, char32_t current, TextBreakMode mode) {
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

std::vector<int32_t> textBreakPositions(std::u32string_view text, TextBreakMode mode) {
    std::vector<int32_t> result;
    result.push_back(0);
    if (text.empty())
        return result;
    char32_t previous = text.front();
    for (size_t i = 1; i < text.size(); ++i) {
        char32_t current = text[i];
        if (isSplit(previous, current, mode))
            result.push_back(i);
        previous = current;
    }
    result.push_back(text.size());
    return result;
}

namespace Internal {
std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection) {
    return std::vector<TextRun>{
        TextRun{ TextDirection::LTR, 0, static_cast<int32_t>(text.size()), 0, nullptr },
    };
}
} // namespace Internal

} // namespace Brisk
#endif
