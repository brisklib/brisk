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

#include <brisk/core/Time.hpp>
#include <unicode/putil.h>
#include <unicode/uclean.h>
#include <unicode/ubidi.h>
#include <unicode/brkiter.h>
#include <unicode/udata.h>
#include <brisk/core/Resources.hpp>

#include "unicode/utypes.h"
#include "unicode/udata.h"

typedef struct alignas(16) {
    uint16_t headerSize;
    uint8_t magic1, magic2;
    UDataInfo info;
    char padding[8];
    uint32_t count, reserved;
    /*
    const struct {
    const char *const name;
    const void *const data;
    } toc[1];
    */
    uint64_t fakeNameAndData[2]; /* TODO:  Change this header type from */
                                 /*        pointerTOC to OffsetTOC.     */
} ICU_Data_Header;

extern "C" U_EXPORT const ICU_Data_Header U_ICUDATA_ENTRY_POINT alignas(16) = {
    32,   /* headerSize */
    0xda, /* magic1,  (see struct MappedData in udata.c)  */
    0x27, /* magic2     */
    {
        /*UDataInfo   */
        sizeof(UDataInfo), /* size        */
        0,                 /* reserved    */

#if U_IS_BIG_ENDIAN
        1,
#else
        0,
#endif

        U_CHARSET_FAMILY,
        sizeof(char16_t),
        0,                          /* reserved      */
        { 0x54, 0x6f, 0x43, 0x50 }, /* data format identifier: "ToCP" */
        { 1, 0, 0, 0 },             /* format version major, minor, milli, micro */
        { 0, 0, 0, 0 }              /* dataVersion   */
    },
    { 's', 't', 'u', 'b', 'd', 'a', 't', 'a' }, /* Padding[8] */
    0,                                          /* count        */
    0,                                          /* Reserved     */
    {
        /*  TOC structure */
        0, 0 /* name and data entries.  Count says there are none,  */
             /*  but put one in just in case.                       */
    }
};

namespace Brisk {

bool icuAvailable = true;

// Uncompress and initialize ICU data.
static void uncompressICUData() {
    static bool icuDataInit = false;
    if (icuDataInit)
        return;

    // Unpack the ICU data.
    static const Bytes icudt = Resources::load("internal/icudt.dat");

    UErrorCode uerr          = U_ZERO_ERROR;
    udata_setCommonData(icudt.data(), &uerr);
    if (uerr != UErrorCode::U_ZERO_ERROR) {
        // Throw an exception if there was an error, including the error name.
        throwException(EUnicode("ICU setCommonData Error: {}", u_errorName(uerr)));
    }

    icuDataInit = true;

    uerr        = U_ZERO_ERROR;
    u_init(&uerr);

    if (uerr != UErrorCode::U_ZERO_ERROR) {
        // Throw an exception if there was an error, including the error name.
        throwException(EUnicode("ICU Init Error: {}", u_errorName(uerr)));
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

#define HANDLE_UERROR(...)                                                                                   \
    if (U_FAILURE(uerr)) {                                                                                   \
        handleICUErr(uerr);                                                                                  \
        return __VA_ARGS__;                                                                                  \
    }

namespace Internal {

namespace {

class TextBreakIteratorICU final : public TextBreakIterator {
public:
    LockedICUBreakIterator icu;
    icu::UnicodeString ustr;
    size_t codepoints = 0;
    int32_t oldp      = 0;

    TextBreakIteratorICU(std::u32string_view text, TextBreakMode mode) : icu(mode) {
        uncompressICUData();
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

class BidiTextIteratorICU final : public BidiTextIterator {
public:
    std::unique_ptr<UBiDi, UBiDiDeleter> bidi;
    std::u16string u16;
    int32_t codepoints = 0;
    int32_t u16chars   = 0;

    BidiTextIteratorICU(std::u32string_view text, TextDirection defaultDirection) {
        uncompressICUData();
        UErrorCode uerr = U_ZERO_ERROR;
        bidi.reset(ubidi_openSized(0, 0, &uerr));
        HANDLE_UERROR();
        u16 = utf32ToUtf16(text);
        ubidi_setPara(bidi.get(), u16.data(), u16.size(),
                      defaultDirection == TextDirection::LTR ? UBIDI_DEFAULT_LTR : UBIDI_DEFAULT_RTL, nullptr,
                      &uerr);
        HANDLE_UERROR();
    }

    ~BidiTextIteratorICU() = default;

    bool isMixed() const {
        return ubidi_getDirection(bidi.get()) == UBIDI_MIXED;
    }

    std::optional<TextFragment> next() {
        if (u16chars == u16.size())
            return std::nullopt;
        UErrorCode uerr = U_ZERO_ERROR;
        TextFragment r;
        int32_t u16length;
        UBiDiLevel level;
        ubidi_getLogicalRun(bidi.get(), u16chars, &u16length, &level);
        u16length -= u16chars;
        r.direction          = toDir(level);
        r.codepointRange.min = codepoints;
        r.codepointRange.max =
            codepoints + utf16Codepoints(std::u16string_view(u16).substr(u16chars, u16length));
        codepoints    = r.codepointRange.max;
        r.visualOrder = ubidi_getVisualIndex(bidi.get(), u16chars, &uerr);
        HANDLE_UERROR(std::nullopt)
        u16chars += u16length;
        return r;
    }
};
} // namespace

RC<TextBreakIterator> textBreakIterator(std::u32string_view text, TextBreakMode mode) {
    return rcnew TextBreakIteratorICU(text, mode);
}

RC<BidiTextIterator> bidiTextIterator(std::u32string_view text, TextDirection defaultDirection) {
    return rcnew BidiTextIteratorICU(text, defaultDirection);
}
} // namespace Internal
} // namespace Brisk
