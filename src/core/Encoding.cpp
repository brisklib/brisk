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
#include <brisk/core/Encoding.hpp>
#include <bit>
#include <string_view>
#include "utf8proc.h"

namespace Brisk {

static_assert(sizeof(char) == 1);
static_assert(sizeof(char16_t) == 2);
static_assert(sizeof(char32_t) == 4);

static_assert(sizeof(wchar_t) == sizeof(char16_t) || sizeof(wchar_t) == sizeof(char32_t));

U8StringView utf8_bom   = "\U0000FEFF";
U16StringView utf16_bom = u"\U0000FEFF";
U32StringView utf32_bom = U"\U0000FEFF";

template <typename OutChar, typename InChar>
std::basic_string_view<OutChar> utfSkipBom(std::basic_string_view<InChar> text) {
    if (text.empty())
        return text;
    const InChar* textp = text.data();
    char32_t ch         = utfRead(textp, text.data() + text.size());
    if (ch == U'\U0000FEFF') {
        return text.substr(textp - text.data());
    }
    return text;
}

template std::basic_string_view<char> utfSkipBom<char>(std::basic_string_view<char>);
template std::basic_string_view<char16_t> utfSkipBom<char16_t>(std::basic_string_view<char16_t>);
template std::basic_string_view<char32_t> utfSkipBom<char32_t>(std::basic_string_view<char32_t>);

template <UtfPolicy p>
using CUtfPolicy = constant<UtfPolicy, p>;

inline bool utfIsError(char32_t ch) {
    return static_cast<int32_t>(ch) < 0;
}

inline bool isUtf8Continuation(char ch) {
    return (ch & 0b11000000) == 0b10000000;
}

inline bool isValidCodepoint(char32_t ch) {
    return (ch & 0xF800) != 0xD800;
}

template <int bytes>
inline char32_t consumeUtf8(const char*& text, const char* end);

template <>
inline char32_t consumeUtf8<0>(const char*& text, const char* end) {
    text += 1;
    return UtfInvalid;
}

template <>
inline char32_t consumeUtf8<1>(const char*& text, const char* end) {
    if (text + 1 > end) {
        text = end;
        return UtfTruncated;
    }
    char32_t result = (text[0] & 0b01111111);
    text += 1;
    return result;
}

template <>
inline char32_t consumeUtf8<2>(const char*& text, const char* end) {
    if (text + 2 > end) {
        text = end;
        return UtfTruncated;
    }
    if (!isUtf8Continuation(text[1])) {
        ++text;
        return UtfInvalid;
    }
    char32_t result = (text[0] & 0b00011111) << 6 | (text[1] & 0b00111111);
    text += 2;
    if (!isValidCodepoint(result)) {
        return UtfInvalid;
    }
    return result > 0x7Fu ? result : UtfOverlong;
}

template <>
inline char32_t consumeUtf8<3>(const char*& text, const char* end) {
    if (text + 3 > end) {
        text = end;
        return UtfTruncated;
    }
    if (!isUtf8Continuation(text[1]) || !isUtf8Continuation(text[2])) {
        ++text;
        return UtfInvalid;
    }
    char32_t result = (text[0] & 0b00001111) << 12 | (text[1] & 0b00111111) << 6 | (text[2] & 0b00111111);
    text += 3;
    if (!isValidCodepoint(result)) {
        return UtfInvalid;
    }
    return result > 0x7FFu ? result : UtfOverlong;
}

template <>
inline char32_t consumeUtf8<4>(const char*& text, const char* end) {
    if (text + 4 > end) {
        text = end;
        return UtfTruncated;
    }
    if (static_cast<uint8_t>(*text) > 0b11110111) {
        ++text;
        // longer than 4 bytes
        return UtfInvalid;
    }
    if (!isUtf8Continuation(text[1]) || !isUtf8Continuation(text[2]) || !isUtf8Continuation(text[3])) {
        ++text;
        return UtfInvalid;
    }
    char32_t result = (text[0] & 0b00000111) << 18 | (text[1] & 0b00111111) << 12 |
                      (text[2] & 0b00111111) << 6 | (text[3] & 0b00111111);
    text += 4;
    if (!isValidCodepoint(result)) {
        return UtfInvalid;
    }
    return result > 0xFFFFu ? result : UtfOverlong;
}

template <int bytes>
inline void produceUtf8(char*& text, char* end, char32_t ch) {}

template <>
inline void produceUtf8<0>(char*& text, char* end, char32_t ch) {
    //
}

template <>
inline void produceUtf8<1>(char*& text, char* end, char32_t ch) {
    *text++ = ch;
}

template <>
inline void produceUtf8<2>(char*& text, char* end, char32_t ch) {
    if (text + 2 > end)
        return;
    *text++ = 0b11000000 | ((ch >> 6) & 0x1F);
    *text++ = 0b10000000 | (ch & 0x3F);
}

template <>
inline void produceUtf8<3>(char*& text, char* end, char32_t ch) {
    if (text + 3 > end)
        return;
    *text++ = 0b11100000 | ((ch >> 12) & 0x0F);
    *text++ = 0b10000000 | ((ch >> 6) & 0x3F);
    *text++ = 0b10000000 | (ch & 0x3F);
}

template <>
inline void produceUtf8<4>(char*& text, char* end, char32_t ch) {
    if (text + 4 > end)
        return;
    *text++ = 0b11110000 | ((ch >> 18) & 0x07);
    *text++ = 0b10000000 | ((ch >> 12) & 0x3F);
    *text++ = 0b10000000 | ((ch >> 6) & 0x3F);
    *text++ = 0b10000000 | (ch & 0x3F);
}

using fn_consume                  = char32_t (*)(const char*& text, const char* end);
using fn_produce                  = void (*)(char*& text, char* end, char32_t ch);

static fn_consume utf8ReadTable[] = {
    /*0b0000xxxx 0_ */ &consumeUtf8<1>,
    /*0b0001xxxx 1_ */ &consumeUtf8<1>,
    /*0b0010xxxx 2_ */ &consumeUtf8<1>,
    /*0b0011xxxx 3_ */ &consumeUtf8<1>,
    /*0b0100xxxx 4_ */ &consumeUtf8<1>,
    /*0b0101xxxx 5_ */ &consumeUtf8<1>,
    /*0b0110xxxx 6_ */ &consumeUtf8<1>,
    /*0b0111xxxx 7_ */ &consumeUtf8<1>,
    /*0b1000xxxx 8_ */ &consumeUtf8<0>,
    /*0b1001xxxx 9_ */ &consumeUtf8<0>,
    /*0b1010xxxx A_ */ &consumeUtf8<0>,
    /*0b1011xxxx B_ */ &consumeUtf8<0>,
    /*0b1100xxxx C_ */ &consumeUtf8<2>,
    /*0b1101xxxx D_ */ &consumeUtf8<2>,
    /*0b1110xxxx E_ */ &consumeUtf8<3>,
    /*0b1111xxxx F_ */ &consumeUtf8<4>,
};

static fn_produce utf8WriteTable[] = {
    /*00*/ &produceUtf8<1>,
    /*01*/ &produceUtf8<1>,
    /*02*/ &produceUtf8<1>,
    /*03*/ &produceUtf8<1>,
    /*04*/ &produceUtf8<1>,
    /*05*/ &produceUtf8<1>,
    /*06*/ &produceUtf8<1>,
    /*07*/ &produceUtf8<1>,
    /*08*/ &produceUtf8<2>,
    /*09*/ &produceUtf8<2>,
    /*10*/ &produceUtf8<2>,
    /*11*/ &produceUtf8<2>,
    /*12*/ &produceUtf8<3>,
    /*13*/ &produceUtf8<3>,
    /*14*/ &produceUtf8<3>,
    /*15*/ &produceUtf8<3>,
    /*16*/ &produceUtf8<3>,
    /*17*/ &produceUtf8<4>,
    /*18*/ &produceUtf8<4>,
    /*19*/ &produceUtf8<4>,
    /*20*/ &produceUtf8<4>,
    /*21*/ &produceUtf8<4>,
    /*22*/ &produceUtf8<0>,
    /*23*/ &produceUtf8<0>,
    /*24*/ &produceUtf8<0>,
    /*25*/ &produceUtf8<0>,
    /*26*/ &produceUtf8<0>,
    /*27*/ &produceUtf8<0>,
    /*28*/ &produceUtf8<0>,
    /*29*/ &produceUtf8<0>,
    /*30*/ &produceUtf8<0>,
    /*31*/ &produceUtf8<0>,
};

char32_t utfRead(const char*& text, const char* end) {
    return utf8ReadTable[static_cast<uint8_t>(*text) >> 4](text, end);
}

char32_t utfRead(const char16_t*& text, const char16_t* end) {
    uint16_t ch = static_cast<uint16_t>(*text);
    if ((ch & 0b11111000'00000000) != 0b11011000'00000000)
        return *text++;
    if (ch & 0b00000100'00000000) {
        ++text; // high surrogate
        return UtfInvalid;
    }
    if (text + 2 > end) {
        text = end;
        return UtfTruncated;
    }
    char32_t result = 0x10000u + ((text[0] & 0x3FF) << 10) | (text[1] & 0x3FF);
    text += 2;
    return result;
}

char32_t utfRead(const char32_t*& text, const char32_t* end) {
    if (static_cast<uint32_t>(*text) > 0x0010FFFF) {
        ++text;
        return UtfInvalid;
    }
    if (!isValidCodepoint(*text)) {
        ++text;
        return UtfInvalid;
    }
    return *text++;
}

char32_t utfRead(const wchar_t*& text, const wchar_t* end) {
    if constexpr (sizeof(wchar_t) == sizeof(char16_t)) {
        return utfRead(reinterpret_cast<const char16_t*&>(text), reinterpret_cast<const char16_t*>(end));
    } else if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        return utfRead(reinterpret_cast<const char32_t*&>(text), reinterpret_cast<const char32_t*>(end));
    }
}

BRISK_INLINE static uint32_t leadingZeros(uint32_t value) {
    return std::countl_zero(value);
}

void utfWrite(char*& text, char* end, char32_t ch) {
    uint32_t code = static_cast<uint32_t>(ch);
    uint32_t bits = 32 - leadingZeros(code | 0x7F) & 0x1F; // ensure that code != 0
    return utf8WriteTable[bits](text, end, ch);
}

void utfWrite(char16_t*& text, char16_t* end, char32_t ch) {
    uint32_t code = static_cast<uint32_t>(ch);
    if (code > 0x10FFFFu) {
        // skip
        return;
    }
    if (code <= 0xFFFF) {
        *text++ = ch;
        return;
    }
    if (text + 2 > end) {
        // overflow
        return;
    }
    code -= 0x10000u;
    *text++ = 0xD800 | ((code >> 10) & 0x3FF);
    *text++ = 0xDC00 | (code & 0x3FF);
}

void utfWrite(char32_t*& text, char32_t* end, char32_t ch) {
    *text++ = ch;
}

void utfWrite(wchar_t*& text, wchar_t* end, char32_t ch) {
    if constexpr (sizeof(wchar_t) == sizeof(char16_t)) {
        return utfWrite(reinterpret_cast<char16_t*&>(text), reinterpret_cast<char16_t*>(end), ch);
    } else if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        return utfWrite(reinterpret_cast<char32_t*&>(text), reinterpret_cast<char32_t*>(end), ch);
    }
}

constexpr static size_t utfMaxElements(char ch) {
    return 4;
}

constexpr static size_t utfMaxElements(char16_t ch) {
    return 2;
}

constexpr static size_t utfMaxElements(char32_t ch) {
    return 1;
}

[[maybe_unused]] constexpr static size_t utfMaxElements(wchar_t ch) {
    if constexpr (sizeof(wchar_t) == sizeof(char16_t)) {
        return utfMaxElements(char16_t{});
    } else if constexpr (sizeof(wchar_t) == sizeof(char32_t)) {
        return utfMaxElements(char32_t{});
    }
}

template <typename Char, UtfPolicy policy = UtfPolicy::ReplaceInvalid>
static size_t utfCodepoints(const Char* text, const Char* end, CUtfPolicy<policy> = CUtfPolicy<policy>{}) {
    if constexpr (sizeof(Char) == sizeof(char32_t) && policy == UtfPolicy::ReplaceInvalid) {
        return end - text;
    }
    size_t result = 0;
    while (text < end) {
        [[maybe_unused]] char32_t ch = utfRead(text, end);
        if constexpr (policy == UtfPolicy::SkipInvalid) {
            if (!utfIsError(ch))
                ++result;
        } else {
            ++result;
        }
    }
    return result;
}

template <typename Char>
static size_t utfCodepoints(const Char* text, const Char* end, UtfPolicy policy) {
    if (policy == UtfPolicy::ReplaceInvalid)
        return utfCodepoints(text, end, CUtfPolicy<UtfPolicy::ReplaceInvalid>{});
    else
        return utfCodepoints(text, end, CUtfPolicy<UtfPolicy::SkipInvalid>{});
}

template <typename Char>
size_t utfCodepoints(std::basic_string_view<Char> sv, UtfPolicy policy) {
    if (policy == UtfPolicy::ReplaceInvalid)
        return utfCodepoints(sv.data(), sv.data() + sv.size(), CUtfPolicy<UtfPolicy::ReplaceInvalid>{});
    else
        return utfCodepoints(sv.data(), sv.data() + sv.size(), CUtfPolicy<UtfPolicy::SkipInvalid>{});
}

template size_t utfCodepoints<char>(std::basic_string_view<char> sv, UtfPolicy policy);
template size_t utfCodepoints<char16_t>(std::basic_string_view<char16_t> sv, UtfPolicy policy);
template size_t utfCodepoints<char32_t>(std::basic_string_view<char32_t> sv, UtfPolicy policy);

template <typename OutChar, typename InChar, typename Fn, UtfPolicy policy = UtfPolicy::ReplaceInvalid>
static OutChar* utfConvert(OutChar* dest, OutChar* dest_end, const InChar* src, const InChar* src_end,
                           Fn&& fn, CUtfPolicy<policy> = CUtfPolicy<policy>{}) {
    while (src < src_end && dest < dest_end) {
        char32_t ch = utfRead(src, src_end);
        if constexpr (policy == UtfPolicy::SkipInvalid) {
            if (!utfIsError(ch))
                utfWrite(dest, dest_end, fn(ch));
        } else {
            utfWrite(dest, dest_end, utfIsError(ch) ? replacementChar : fn(ch));
        }
    }
    return dest;
}

template <typename OutChar, typename InChar, UtfPolicy policy = UtfPolicy::ReplaceInvalid>
static OutChar* utfConvert(OutChar* dest, OutChar* dest_end, const InChar* src, const InChar* src_end,
                           CUtfPolicy<policy> = CUtfPolicy<policy>{}) {
    return utfConvert<OutChar, InChar, std::identity, policy>(dest, dest_end, src, src_end, std::identity{},
                                                              CUtfPolicy<policy>{});
}

template <typename InChar>
static UtfValidation utfValidate(const InChar* src, const InChar* src_end) {
    while (src < src_end) {
        char32_t ch = utfRead(src, src_end);
        if (ch == UtfTruncated)
            return UtfValidation::Truncated;
        if (ch == UtfInvalid)
            return UtfValidation::Invalid;
        if (ch == UtfOverlong)
            return UtfValidation::Overlong;
    }
    return UtfValidation::Valid;
}

template <typename Char>
UtfValidation utfValidate(std::basic_string_view<Char> text) {
    return utfValidate(text.data(), text.data() + text.size());
}

template UtfValidation utfValidate<char>(std::basic_string_view<char> sv);
template UtfValidation utfValidate<char16_t>(std::basic_string_view<char16_t> sv);
template UtfValidation utfValidate<char32_t>(std::basic_string_view<char32_t> sv);

template <typename OutChar, typename InChar, UtfPolicy policy = UtfPolicy::ReplaceInvalid>
std::basic_string<OutChar> utfToUtf(std::basic_string_view<InChar> text,
                                    CUtfPolicy<policy> = CUtfPolicy<policy>{}) {
    const size_t len =
        text.empty() ? 0 : utfMaxElements(OutChar{}) * utfCodepoints(text.data(), text.data() + text.size());
    std::basic_string<OutChar> result(len, ' ');
    result.resize(utfConvert(result.data(), result.data() + result.size(), text.data(),
                             text.data() + text.size(), CUtfPolicy<policy>{}) -
                  result.data());
    return result;
}

template <typename OutChar, typename InChar>
std::basic_string<OutChar> utfToUtf(std::basic_string_view<InChar> text, UtfPolicy policy) {
    if (policy == UtfPolicy::ReplaceInvalid)
        return utfToUtf<OutChar, InChar>(text, CUtfPolicy<UtfPolicy::ReplaceInvalid>{});
    else
        return utfToUtf<OutChar, InChar>(text, CUtfPolicy<UtfPolicy::SkipInvalid>{});
}

template std::basic_string<char> utfToUtf<char, char>(std::basic_string_view<char> sv, UtfPolicy policy);
template std::basic_string<char16_t> utfToUtf<char16_t, char>(std::basic_string_view<char> sv,
                                                              UtfPolicy policy);
template std::basic_string<char32_t> utfToUtf<char32_t, char>(std::basic_string_view<char> sv,
                                                              UtfPolicy policy);
template std::basic_string<wchar_t> utfToUtf<wchar_t, char>(std::basic_string_view<char> sv,
                                                            UtfPolicy policy);

template std::basic_string<char> utfToUtf<char, char16_t>(std::basic_string_view<char16_t> sv,
                                                          UtfPolicy policy);
template std::basic_string<char16_t> utfToUtf<char16_t, char16_t>(std::basic_string_view<char16_t> sv,
                                                                  UtfPolicy policy);
template std::basic_string<char32_t> utfToUtf<char32_t, char16_t>(std::basic_string_view<char16_t> sv,
                                                                  UtfPolicy policy);
template std::basic_string<wchar_t> utfToUtf<wchar_t, char16_t>(std::basic_string_view<char16_t> sv,
                                                                UtfPolicy policy);

template std::basic_string<char> utfToUtf<char, char32_t>(std::basic_string_view<char32_t> sv,
                                                          UtfPolicy policy);
template std::basic_string<char16_t> utfToUtf<char16_t, char32_t>(std::basic_string_view<char32_t> sv,
                                                                  UtfPolicy policy);
template std::basic_string<char32_t> utfToUtf<char32_t, char32_t>(std::basic_string_view<char32_t> sv,
                                                                  UtfPolicy policy);
template std::basic_string<wchar_t> utfToUtf<wchar_t, char32_t>(std::basic_string_view<char32_t> sv,
                                                                UtfPolicy policy);

template std::basic_string<char> utfToUtf<char, wchar_t>(std::basic_string_view<wchar_t> sv,
                                                         UtfPolicy policy);
template std::basic_string<char16_t> utfToUtf<char16_t, wchar_t>(std::basic_string_view<wchar_t> sv,
                                                                 UtfPolicy policy);
template std::basic_string<char32_t> utfToUtf<char32_t, wchar_t>(std::basic_string_view<wchar_t> sv,
                                                                 UtfPolicy policy);
template std::basic_string<wchar_t> utfToUtf<wchar_t, wchar_t>(std::basic_string_view<wchar_t> sv,
                                                               UtfPolicy policy);

template <typename InChar, UtfPolicy policy = UtfPolicy::ReplaceInvalid>
std::basic_string<InChar> utfTransform(std::basic_string_view<InChar> text,
                                       function_ref<char32_t(char32_t)> fn,
                                       CUtfPolicy<policy> = CUtfPolicy<policy>{}) {
    const size_t len =
        text.empty() ? 0 : utfMaxElements(InChar{}) * utfCodepoints(text.data(), text.data() + text.size());
    std::basic_string<InChar> result(len, ' ');
    result.resize(utfConvert(result.data(), result.data() + result.size(), text.data(),
                             text.data() + text.size(), fn, CUtfPolicy<policy>{}) -
                  result.data());
    return result;
}

template <typename InChar>
std::basic_string<InChar> utfTransform(std::basic_string_view<InChar> text,
                                       function_ref<char32_t(char32_t)> fn, UtfPolicy policy) {
    if (policy == UtfPolicy::ReplaceInvalid)
        return utfTransform<InChar>(text, fn, CUtfPolicy<UtfPolicy::ReplaceInvalid>{});
    else
        return utfTransform<InChar>(text, fn, CUtfPolicy<UtfPolicy::SkipInvalid>{});
}

template std::basic_string<char> utfTransform<char>(std::basic_string_view<char> sv,
                                                    function_ref<char32_t(char32_t)> fn, UtfPolicy policy);
template std::basic_string<char16_t> utfTransform<char16_t>(std::basic_string_view<char16_t> sv,
                                                            function_ref<char32_t(char32_t)> fn,
                                                            UtfPolicy policy);
template std::basic_string<char32_t> utfTransform<char32_t>(std::basic_string_view<char32_t> sv,
                                                            function_ref<char32_t(char32_t)> fn,
                                                            UtfPolicy policy);

template <typename Char>
std::basic_string<Char> utfCleanup(std::basic_string_view<Char> text, UtfPolicy policy) {
    return utfToUtf<Char, Char>(text, policy);
}

template std::basic_string<char> utfCleanup<char>(std::basic_string_view<char> sv, UtfPolicy policy);
template std::basic_string<char16_t> utfCleanup<char16_t>(std::basic_string_view<char16_t> sv,
                                                          UtfPolicy policy);
template std::basic_string<char32_t> utfCleanup<char32_t>(std::basic_string_view<char32_t> sv,
                                                          UtfPolicy policy);

template <typename Char>
static void utfAppend(std::basic_string<Char>& str, char32_t ch) {
    Char buf[utfMaxElements(Char{})];
    Char* ptr = buf;
    utfWrite(ptr, ptr + std::size(buf), ch);
    str.append(buf, ptr - buf);
}

template <typename Char>
std::basic_string<Char> utfNormalize(std::basic_string_view<Char> text, UtfNormalization normalization,
                                     UtfPolicy policy) {

    int opt = 0;
    if (normalization && UtfNormalization::Compose)
        opt |= utf8proc_option_t::UTF8PROC_COMPOSE;
    if (normalization && UtfNormalization::Decompose)
        opt |= utf8proc_option_t::UTF8PROC_DECOMPOSE;
    if (normalization && UtfNormalization::Compat)
        opt |= utf8proc_option_t::UTF8PROC_COMPAT;
    std::string u8        = toUtf8(text);
    utf8proc_uint8_t* dst = nullptr;
    if (utf8proc_ssize_t sz =
            utf8proc_map(reinterpret_cast<utf8proc_uint8_t*>(u8.data()), u8.size(), &dst,
                         utf8proc_option_t(opt)); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        sz >= 0) {
        std::string result(dst, dst + sz);
        std::free(dst);
        return utfToUtf<Char, char>(result);
    }
    return {};
}

template std::basic_string<char> utfNormalize<char>(std::basic_string_view<char> text,
                                                    UtfNormalization normalization, UtfPolicy policy);
template std::basic_string<char16_t> utfNormalize<char16_t>(std::basic_string_view<char16_t> text,
                                                            UtfNormalization normalization, UtfPolicy policy);
template std::basic_string<char32_t> utfNormalize<char32_t>(std::basic_string_view<char32_t> text,
                                                            UtfNormalization normalization, UtfPolicy policy);

std::string asciiTransform(std::string_view text, function_ref<char32_t(char32_t)> fn) {
    std::string result(text.size(), ' ');
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] = fn(text[i]);
    }
    return result;
}

bool isAscii(std::string_view text) {
    for (size_t i = 0; i < text.size(); ++i) {
        if (static_cast<uint8_t>(text[i]) >= 0x80)
            return false;
    }
    return true;
}

} // namespace Brisk

namespace std {

bool toJson(Brisk::Json& j, const std::u32string& s) {
    return j.from(Brisk::utf32ToUtf8(s));
}

bool toJson(Brisk::Json& j, const std::u16string& s) {
    return j.from(Brisk::utf16ToUtf8(s));
}

bool toJson(Brisk::Json& j, const std::wstring& s) {
    return j.from(Brisk::wcsToUtf8(s));
}

bool fromJson(const Brisk::Json& j, std::u32string& s) {
    return j.to<std::string>(Brisk::RefAdapter{ [](const std::string& s) {
                                                   return Brisk::utf8ToUtf32(s);
                                               },
                                                s });
}

bool fromJson(const Brisk::Json& j, std::u16string& s) {
    return j.to<std::string>(Brisk::RefAdapter{ [](const std::string& s) {
                                                   return Brisk::utf8ToUtf16(s);
                                               },
                                                s });
}

bool fromJson(const Brisk::Json& j, std::wstring& s) {
    return j.to<std::string>(Brisk::RefAdapter{ [](const std::string& s) {
                                                   return Brisk::utf8ToWcs(s);
                                               },
                                                s });
}
} // namespace std
