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
#include <brisk/core/internal/Debug.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <utf8proc.h>

#ifndef ICUDT_SIZE
#error ICUDT_SIZE must be defined
#endif

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

struct LockableICUBreakIterator {
    std::mutex mutex;
    std::unique_ptr<icu::BreakIterator> iterator;
};

static LockableICUBreakIterator cachedBreakIterators[3];

static std::unique_ptr<icu::BreakIterator> createICUBreakIterator(TextBreakMode mode) {
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

struct LockedICUBreakIterator {
    LockableICUBreakIterator* locked;
    icu::BreakIterator* iterator;

    icu::BreakIterator* operator->() const {
        return iterator;
    }

    LockedICUBreakIterator(const LockedICUBreakIterator&) noexcept            = delete;
    LockedICUBreakIterator(LockedICUBreakIterator&&) noexcept                 = delete;
    LockedICUBreakIterator& operator=(const LockedICUBreakIterator&) noexcept = delete;
    LockedICUBreakIterator& operator=(LockedICUBreakIterator&&) noexcept      = delete;

    LockedICUBreakIterator(TextBreakMode mode) {
        if (cachedBreakIterators[+mode].mutex.try_lock()) {
            if (!cachedBreakIterators[+mode].iterator) {
                cachedBreakIterators[+mode].iterator = createICUBreakIterator(mode);
            }
            locked   = &cachedBreakIterators[+mode];
            iterator = cachedBreakIterators[+mode].iterator.get();
        } else {
            locked   = nullptr;
            iterator = createICUBreakIterator(mode).release();
        }
    }

    ~LockedICUBreakIterator() {
        if (locked) {
            locked->mutex.unlock();
        } else {
            delete iterator;
        }
    }
};

LockedICUBreakIterator acquireICUBreakIterator(TextBreakMode mode) {
    return LockedICUBreakIterator(mode);
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

namespace Brisk {

Internal::TextBreakIterator::~TextBreakIterator() = default;

namespace {

class TextBreakIteratorICU final : public Internal::TextBreakIterator {
public:
    LockedICUBreakIterator icu;
    icu::UnicodeString ustr;
    size_t codepoints = 0;
    int32_t oldp      = 0;

    TextBreakIteratorICU(std::u32string_view text, TextBreakMode mode) : icu(mode) {
        std::u16string u16 = utf32ToUtf16(text);
        ustr.setTo(u16.data(), u16.size());
        icu->setText(ustr);
    }

    ~TextBreakIteratorICU() = default;

    std::optional<uint32_t> next() {
        int32_t p = icu->next();
        if (p == icu::BreakIterator::DONE) {
            return std::nullopt;
        }
        codepoints +=
            utf16Codepoints(std::u16string_view{ ustr.getBuffer() + oldp, static_cast<size_t>(p - oldp) });
        oldp = p;
        return codepoints;
    }
};
} // namespace

RC<Internal::TextBreakIterator> Internal::textBreakIterator(std::u32string_view text, TextBreakMode mode) {
    return rcnew TextBreakIteratorICU(text, mode);
}
} // namespace Brisk
