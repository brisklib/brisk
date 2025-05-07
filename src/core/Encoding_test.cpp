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
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include <ostream>
#include <string_view>
#include <brisk/core/Encoding.hpp>

using namespace Brisk;

namespace Brisk {
[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, UtfValidation v) {
    switch (v) {
    case UtfValidation::Valid:
        return os << "Valid";
    case UtfValidation::Truncated:
        return os << "Truncated";
    case UtfValidation::Overlong:
        return os << "Overlong";
    case UtfValidation::Invalid:
    default:
        return os << "Invalid";
    }
}
} // namespace Brisk

static void checkValid(std::u32string_view u32, std::u16string u16, std::string u8) {
    INFO("u32 = " << fmt::format("{:08X}",
                                 fmt::join(reinterpret_cast<const uint32_t*>(u32.data()),
                                           reinterpret_cast<const uint32_t*>(u32.data() + u32.size()), " ")));
    INFO("u16 = " << fmt::format("{:04X}",
                                 fmt::join(reinterpret_cast<const uint16_t*>(u16.data()),
                                           reinterpret_cast<const uint16_t*>(u16.data() + u16.size()), " ")));
    INFO("u8  = " << fmt::format("{:02X}",
                                 fmt::join(reinterpret_cast<const uint8_t*>(u8.data()),
                                           reinterpret_cast<const uint8_t*>(u8.data() + u8.size()), " ")));

    CHECK(utf32ToUtf8(u32) == u8);
    CHECK(utf8ToUtf32(u8) == u32);
    CHECK(utf32ToUtf16(u32) == u16);
    CHECK(utf16ToUtf32(u16) == u32);
    CHECK(utf16ToUtf8(u16) == u8);
    CHECK(utf8ToUtf16(u8) == u16);

    CHECK(utf8Codepoints(u8) == u32.size());
    CHECK(utf16Codepoints(u16) == u32.size());
    CHECK(utf32Codepoints(u32) == u32.size());

    CHECK(utf8Validate(u8) == UtfValidation::Valid);
    CHECK(utf16Validate(u16) == UtfValidation::Valid);
    CHECK(utf32Validate(u32) == UtfValidation::Valid);
}

TEST_CASE("Encoding") {
    CHECK(utf8ToUtf32("abc\n") == U"abc\n");
    CHECK(utf8ToUtf16("abc\n") == u"abc\n");
    CHECK(utf8ToUtf32("\U0001F603") == U"\U0001F603");
    CHECK(utf8ToUtf16("\U0001F603") == u"\U0001F603");
    CHECK(utf32ToUtf8(U"\U0001F603") == "\U0001F603");
    CHECK(utf32ToUtf16(U"\U0001F603") == u"\U0001F603");

    checkValid(U"", u"", "");
    checkValid(U"A", u"A", "A");
    checkValid(U"\U00000000", u"\U00000000", "\U00000000");
    checkValid(U"\U0000007F", u"\U0000007F", "\U0000007F");
    checkValid(U"\U00000080", u"\U00000080", "\U00000080");
    checkValid(U"\U000007FF", u"\U000007FF", "\U000007FF");
    checkValid(U"\U00000800", u"\U00000800", "\U00000800");
    checkValid(U"\U0000FFFF", u"\U0000FFFF", "\U0000FFFF");
    checkValid(U"\U00010000", u"\U00010000", "\U00010000");
    checkValid(U"\U0010FFFF", u"\U0010FFFF", "\U0010FFFF");

    checkValid(U"\U0000D7FF", u"\U0000D7FF", "\U0000D7FF");
    checkValid(U"\U0000E000", u"\U0000E000", "\U0000E000");
    checkValid(U"\U0000FFFD", u"\U0000FFFD", "\U0000FFFD");

    checkValid(U"\U00000000\U00000000\U00000000", u"\U00000000\U00000000\U00000000",
               "\U00000000\U00000000\U00000000");

    checkValid(U"\U00000000"
               U"\U0000007F"
               U"\U00000080"
               U"\U000007FF"
               U"\U00000800"
               U"\U0000FFFF"
               U"\U00010000"
               U"\U0010FFFF",
               u"\U00000000"
               u"\U0000007F"
               u"\U00000080"
               u"\U000007FF"
               u"\U00000800"
               u"\U0000FFFF"
               u"\U00010000"
               u"\U0010FFFF",
               "\U00000000"
               "\U0000007F"
               "\U00000080"
               "\U000007FF"
               "\U00000800"
               "\U0000FFFF"
               "\U00010000"
               "\U0010FFFF");

    CHECK(utf32Validate(std::u32string_view(std::array<char32_t, 1>{ 0x110000 }.data(), 1)) ==
          UtfValidation::Invalid);
    CHECK(utf16Validate(std::u16string_view(std::array<char16_t, 1>{ 0xD800 }.data(), 1)) ==
          UtfValidation::Truncated);
    CHECK(utf16Validate(std::u16string_view(std::array<char16_t, 1>{ 0xDFFF }.data(), 1)) ==
          UtfValidation::Invalid);
    CHECK(utf8Validate("\xC0\x80") == UtfValidation::Overlong);
    CHECK(utf8Validate("\xE0\x80\x80") == UtfValidation::Overlong);
    CHECK(utf8Validate("\xF0\x80\x80\x80") == UtfValidation::Overlong);
    CHECK(utf8Validate("\xC0") == UtfValidation::Truncated);
    CHECK(utf8Validate("\xED\xA0\x80") == UtfValidation::Invalid);
    CHECK(utf32Validate(std::u32string_view(std::array<char32_t, 1>{ 0xD800 }.data(), 1)) ==
          UtfValidation::Invalid);

    CHECK(utf8Cleanup("\xF0\x80\x80\x80", UtfPolicy::SkipInvalid) == "");
    CHECK(utf8Cleanup("_\xF0\x80\x80\x80\x00", UtfPolicy::SkipInvalid) == "_\x00");
    CHECK(utf8Cleanup("_\xC0", UtfPolicy::SkipInvalid) == "_");

    CHECK(utf16Cleanup(std::u16string_view(std::array<char16_t, 1>{ 0xD800 }.data(), 1),
                       UtfPolicy::SkipInvalid) == u"");
    CHECK(utf16Cleanup(std::u16string_view(std::array<char16_t, 1>{ 0xD800 }.data(), 1),
                       UtfPolicy::ReplaceInvalid) == u"" REPLACEMENT_CHAR_S);

    CHECK(utf8Cleanup("\x80", UtfPolicy::ReplaceInvalid) == "" REPLACEMENT_CHAR_S);
    CHECK(utf8Cleanup("_\xC0", UtfPolicy::ReplaceInvalid) == "_" REPLACEMENT_CHAR_S);
    CHECK(utf8Cleanup("\xF0\x80\x80\x80", UtfPolicy::ReplaceInvalid) == "" REPLACEMENT_CHAR_S);
    CHECK(utf8Cleanup("_\xF0\x80\x80\x80\x00", UtfPolicy::ReplaceInvalid) == "_" REPLACEMENT_CHAR_S "\x00");

    CHECK(utf8Cleanup("", UtfPolicy::SkipInvalid) == "");

    CHECK(utf8Validate("\xC0\x01") == UtfValidation::Invalid);
    CHECK(utf8Validate("\xE4\x8B\x13") == UtfValidation::Invalid);
    CHECK(utf8Validate("\xE0\x80\x01") == UtfValidation::Invalid);

    constexpr std::string_view long_string =
        "\x78\x66\xdc\x14\x35\xfc\xb6\x36\xa6\x89\xbf\x5e\xe4\x8b\x13\x7d\x17\xdf\x27\xd0\x5b\x3c\x95\xb5\xd0"
        "\x44\x54\x30\x29\x92\x87\x91\xe8\xaf\x97\xcd\xdf\x34\x91\xfd\xb8\xbc\x9e\xae\x05\x2d\x42\x7f\x88\x5c"
        "\x1f\xaf\x67\x68\xfe\x15\x36\x59\x37\x4c\x7a\x91\x86\xb4\x14\x76\x1a\x5f\x6b\x57\x17\x99\x9d\xec\x59"
        "\xd4\x37\x7d\x89\xc2\x39\x55\x05\x31\xb8\xc6\xb7\xb5\x71\xf9\x4d\x4d\x1f\x46\xe0\x44\x67\x40\xb8\xe3"
        "\x32\x04\x96\x06\xa0\x5b\x67\x00\x0f\xae\x5e\xd1\x39\xd4\xff\x84\x00\xbc\xe4\x0e\x8c\xcc\x09\xf9\xac"
        "\x1a\x67\x13\x35\xfc\x13\x9c\xb8\xd8\x63\x92\x2c\x8b\x38\x6b\x43\x94\x66\x0e\xc6\x02\xdb\xbc\x0f\xfc"
        "\xe8\x65\x77\xd0\x57\x13\xdb\xbc\x34\x6f\x63\x9f\xb0\x43\x70\xd0\x7d\xf6\x90\x9f\xdb\x20\xa4\x92\xb2"
        "\xcc\x8b\xd1\xfc\xff\xe3\xfb\x76\xbe\xb5\x94\x17\x18\x28\xf2\xc5\xce\x51\xda\xf8\x20\xd9\xfc\x45\x62"
        "\x59\x0f\x30\x74\xc9\x73\xc4\xf1\x51\xcb\x4e\x75\xe6\x09\x1a\xd8\x49\xcb\x2c\x97\xae\x28\xc4\xed\x03"
        "\xc4\xc8\x95\xd9\x9f\xa4\x82\x15\xf8\x84\xfe\x9e\x20\x2b\x4d\x76\xc1\xd8\xdb\x95\xc8\x48\x2d\xb0\x2f"
        "\x7a\x6c\xa4\xbb\xd0\xdd";

    CHECK(utf8Codepoints(long_string, UtfPolicy::SkipInvalid) == 60);
    CHECK(utf8Codepoints(long_string, UtfPolicy::ReplaceInvalid) == 104);

    for (char32_t ch : utf8Iterate("\U0001F603")) {
        CHECK(ch == 0x1F603);
    }
    for (char32_t ch : utf16Iterate(u"\U0001F603")) {
        CHECK(ch == 0x1F603);
    }
    for (char32_t ch : utf32Iterate(U"\U0001F603")) {
        CHECK(ch == 0x1F603);
    }
}

TEST_CASE("UtfMisc") {
    using namespace std::string_literals;
    CHECK(utf8ToUtf32("") == U""s);
    CHECK(utf8ToUtf32("123") == U"123"s);
    CHECK(utf8ToUtf32("猫") == U"\x732B"s);
    CHECK(utf8ToUtf32("🐈") == U"\U0001F408"s);

    CHECK(utf8ToUtf32("") == U""s);
    CHECK(utf8ToUtf32("<123>") == U"<123>"s);
    CHECK(utf8ToUtf32("<猫>") == U"<\x732B>"s);
    CHECK(utf8ToUtf32("<🐈>") == U"<\U0001F408>"s);

    CHECK(utf8ToUtf16("") == u""s);
    CHECK(utf8ToUtf16("123") == u"123"s);
    CHECK(utf8ToUtf16("猫") == u"\x732B"s);
    CHECK(utf8ToUtf16("🐈") == u"\xd83d\xdc08"s);

    CHECK(utf8ToUtf16("") == u""s);
    CHECK(utf8ToUtf16("<123>") == u"<123>"s);
    CHECK(utf8ToUtf16("<猫>") == u"<\x732B>"s);
    CHECK(utf8ToUtf16("<🐈>") == u"<\xd83d\xdc08>"s);

    CHECK("" == utf32ToUtf8(U""s));
    CHECK("123" == utf32ToUtf8(U"123"s));
    CHECK("猫" == utf32ToUtf8(U"\x732B"s));
    CHECK("🐈" == utf32ToUtf8(U"\U0001F408"s));

    CHECK("" == utf32ToUtf8(U""s));
    CHECK("<123>" == utf32ToUtf8(U"<123>"s));
    CHECK("<猫>" == utf32ToUtf8(U"<\x732B>"s));
    CHECK("<🐈>" == utf32ToUtf8(U"<\U0001F408>"s));

    CHECK("" == utf16ToUtf8(u""s));
    CHECK("123" == utf16ToUtf8(u"123"s));
    CHECK("猫" == utf16ToUtf8(u"\x732B"s));
    CHECK("🐈" == utf16ToUtf8(u"\xd83d\xdc08"s));

    CHECK("" == utf16ToUtf8(u""s));
    CHECK("<123>" == utf16ToUtf8(u"<123>"s));
    CHECK("<猫>" == utf16ToUtf8(u"<\x732B>"s));
    CHECK("<🐈>" == utf16ToUtf8(u"<\xd83d\xdc08>"s));
}

TEST_CASE("utf_skip_bom") {
    CHECK(utf8SkipBom("\xEF\xBB\xBF") == ""sv);
    CHECK(utf8SkipBom("\xEF\xBB\xBF_") == "_"sv);
    CHECK(utf8SkipBom("_\xEF\xBB\xBF") == "_\xEF\xBB\xBF"sv);

    CHECK(utf16SkipBom(u"\uFEFF") == u""sv);
    CHECK(utf16SkipBom(u"\uFEFF_") == u"_"sv);
    CHECK(utf16SkipBom(u"_\uFEFF") == u"_\uFEFF"sv);

    CHECK(utf32SkipBom(U"\uFEFF") == U""sv);
    CHECK(utf32SkipBom(U"\uFEFF_") == U"_"sv);
    CHECK(utf32SkipBom(U"_\uFEFF") == U"_\uFEFF"sv);
}

TEST_CASE("utf_normalize") {
    CHECK(utf8Normalize("\u00C5", UtfNormalization::NFC) == "\u00C5"sv);
    CHECK(utf8Normalize("\u00C5", UtfNormalization::NFD) == "\u0041\u030A"sv);
    CHECK(utf8Normalize("\u00C5", UtfNormalization::NFKC) == "\u00C5"sv);
    CHECK(utf8Normalize("\u00C5", UtfNormalization::NFKD) == "\u0041\u030A"sv);

    CHECK(utf8Normalize("\u00F4", UtfNormalization::NFC) == "\u00F4"sv);
    CHECK(utf8Normalize("\u00F4", UtfNormalization::NFD) == "\u006F\u0302"sv);
    CHECK(utf8Normalize("\u00F4", UtfNormalization::NFKC) == "\u00F4"sv);
    CHECK(utf8Normalize("\u00F4", UtfNormalization::NFKD) == "\u006F\u0302"sv);

    CHECK(utf8Normalize("\u1E69", UtfNormalization::NFC) == "\u1E69"sv);
    CHECK(utf8Normalize("\u1E69", UtfNormalization::NFD) == "\u0073\u0323\u0307"sv);
    CHECK(utf8Normalize("\u1E69", UtfNormalization::NFKC) == "\u1E69"sv);
    CHECK(utf8Normalize("\u1E69", UtfNormalization::NFKD) == "\u0073\u0323\u0307"sv);

    CHECK(utf8Normalize("\u1E0B\u0323", UtfNormalization::NFC) == "\u1E0D\u0307"sv);
    CHECK(utf8Normalize("\u1E0B\u0323", UtfNormalization::NFD) == "\u0064\u0323\u0307"sv);
    CHECK(utf8Normalize("\u1E0B\u0323", UtfNormalization::NFKC) == "\u1E0D\u0307"sv);
    CHECK(utf8Normalize("\u1E0B\u0323", UtfNormalization::NFKD) == "\u0064\u0323\u0307"sv);

    CHECK(utf8Normalize("\u0071\u0307\u0323", UtfNormalization::NFC) == "\u0071\u0323\u0307"sv);
    CHECK(utf8Normalize("\u0071\u0307\u0323", UtfNormalization::NFD) == "\u0071\u0323\u0307"sv);
    CHECK(utf8Normalize("\u0071\u0307\u0323", UtfNormalization::NFKC) == "\u0071\u0323\u0307"sv);
    CHECK(utf8Normalize("\u0071\u0307\u0323", UtfNormalization::NFKD) == "\u0071\u0323\u0307"sv);

    CHECK(utf8Normalize("\uFB01", UtfNormalization::NFC) == "\uFB01"sv);
    CHECK(utf8Normalize("\uFB01", UtfNormalization::NFD) == "\uFB01"sv);
    CHECK(utf8Normalize("\uFB01", UtfNormalization::NFKC) == "\u0066\u0069"sv);
    CHECK(utf8Normalize("\uFB01", UtfNormalization::NFKD) == "\u0066\u0069"sv);

    CHECK(utf8Normalize("\u0032\u2075", UtfNormalization::NFC) == "\u0032\u2075"sv);
    CHECK(utf8Normalize("\u0032\u2075", UtfNormalization::NFD) == "\u0032\u2075"sv);
    CHECK(utf8Normalize("\u0032\u2075", UtfNormalization::NFKC) == "\u0032\u0035"sv);
    CHECK(utf8Normalize("\u0032\u2075", UtfNormalization::NFKD) == "\u0032\u0035"sv);

    CHECK(utf8Normalize("\u1E9B\u0323", UtfNormalization::NFC) == "\u1E9B\u0323"sv);
    CHECK(utf8Normalize("\u1E9B\u0323", UtfNormalization::NFD) == "\u017F\u0323\u0307"sv);
    CHECK(utf8Normalize("\u1E9B\u0323", UtfNormalization::NFKC) == "\u1E69"sv);
    CHECK(utf8Normalize("\u1E9B\u0323", UtfNormalization::NFKD) == "\u0073\u0323\u0307"sv);

    CHECK(utf16Normalize(u"\u00C5", UtfNormalization::NFC) == u"\u00C5"sv);
    CHECK(utf16Normalize(u"\u00C5", UtfNormalization::NFD) == u"\u0041\u030A"sv);
    CHECK(utf16Normalize(u"\u00C5", UtfNormalization::NFKC) == u"\u00C5"sv);
    CHECK(utf16Normalize(u"\u00C5", UtfNormalization::NFKD) == u"\u0041\u030A"sv);

    CHECK(utf16Normalize(u"\u00F4", UtfNormalization::NFC) == u"\u00F4"sv);
    CHECK(utf16Normalize(u"\u00F4", UtfNormalization::NFD) == u"\u006F\u0302"sv);
    CHECK(utf16Normalize(u"\u00F4", UtfNormalization::NFKC) == u"\u00F4"sv);
    CHECK(utf16Normalize(u"\u00F4", UtfNormalization::NFKD) == u"\u006F\u0302"sv);

    CHECK(utf16Normalize(u"\u1E69", UtfNormalization::NFC) == u"\u1E69"sv);
    CHECK(utf16Normalize(u"\u1E69", UtfNormalization::NFD) == u"\u0073\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u1E69", UtfNormalization::NFKC) == u"\u1E69"sv);
    CHECK(utf16Normalize(u"\u1E69", UtfNormalization::NFKD) == u"\u0073\u0323\u0307"sv);

    CHECK(utf16Normalize(u"\u1E0B\u0323", UtfNormalization::NFC) == u"\u1E0D\u0307"sv);
    CHECK(utf16Normalize(u"\u1E0B\u0323", UtfNormalization::NFD) == u"\u0064\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u1E0B\u0323", UtfNormalization::NFKC) == u"\u1E0D\u0307"sv);
    CHECK(utf16Normalize(u"\u1E0B\u0323", UtfNormalization::NFKD) == u"\u0064\u0323\u0307"sv);

    CHECK(utf16Normalize(u"\u0071\u0307\u0323", UtfNormalization::NFC) == u"\u0071\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u0071\u0307\u0323", UtfNormalization::NFD) == u"\u0071\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u0071\u0307\u0323", UtfNormalization::NFKC) == u"\u0071\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u0071\u0307\u0323", UtfNormalization::NFKD) == u"\u0071\u0323\u0307"sv);

    CHECK(utf16Normalize(u"\uFB01", UtfNormalization::NFC) == u"\uFB01"sv);
    CHECK(utf16Normalize(u"\uFB01", UtfNormalization::NFD) == u"\uFB01"sv);
    CHECK(utf16Normalize(u"\uFB01", UtfNormalization::NFKC) == u"\u0066\u0069"sv);
    CHECK(utf16Normalize(u"\uFB01", UtfNormalization::NFKD) == u"\u0066\u0069"sv);

    CHECK(utf16Normalize(u"\u0032\u2075", UtfNormalization::NFC) == u"\u0032\u2075"sv);
    CHECK(utf16Normalize(u"\u0032\u2075", UtfNormalization::NFD) == u"\u0032\u2075"sv);
    CHECK(utf16Normalize(u"\u0032\u2075", UtfNormalization::NFKC) == u"\u0032\u0035"sv);
    CHECK(utf16Normalize(u"\u0032\u2075", UtfNormalization::NFKD) == u"\u0032\u0035"sv);

    CHECK(utf16Normalize(u"\u1E9B\u0323", UtfNormalization::NFC) == u"\u1E9B\u0323"sv);
    CHECK(utf16Normalize(u"\u1E9B\u0323", UtfNormalization::NFD) == u"\u017F\u0323\u0307"sv);
    CHECK(utf16Normalize(u"\u1E9B\u0323", UtfNormalization::NFKC) == u"\u1E69"sv);
    CHECK(utf16Normalize(u"\u1E9B\u0323", UtfNormalization::NFKD) == u"\u0073\u0323\u0307"sv);

    CHECK(utf32Normalize(U"\u00C5", UtfNormalization::NFC) == U"\u00C5"sv);
    CHECK(utf32Normalize(U"\u00C5", UtfNormalization::NFD) == U"\u0041\u030A"sv);
    CHECK(utf32Normalize(U"\u00C5", UtfNormalization::NFKC) == U"\u00C5"sv);
    CHECK(utf32Normalize(U"\u00C5", UtfNormalization::NFKD) == U"\u0041\u030A"sv);

    CHECK(utf32Normalize(U"\u00F4", UtfNormalization::NFC) == U"\u00F4"sv);
    CHECK(utf32Normalize(U"\u00F4", UtfNormalization::NFD) == U"\u006F\u0302"sv);
    CHECK(utf32Normalize(U"\u00F4", UtfNormalization::NFKC) == U"\u00F4"sv);
    CHECK(utf32Normalize(U"\u00F4", UtfNormalization::NFKD) == U"\u006F\u0302"sv);

    CHECK(utf32Normalize(U"\u1E69", UtfNormalization::NFC) == U"\u1E69"sv);
    CHECK(utf32Normalize(U"\u1E69", UtfNormalization::NFD) == U"\u0073\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u1E69", UtfNormalization::NFKC) == U"\u1E69"sv);
    CHECK(utf32Normalize(U"\u1E69", UtfNormalization::NFKD) == U"\u0073\u0323\u0307"sv);

    CHECK(utf32Normalize(U"\u1E0B\u0323", UtfNormalization::NFC) == U"\u1E0D\u0307"sv);
    CHECK(utf32Normalize(U"\u1E0B\u0323", UtfNormalization::NFD) == U"\u0064\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u1E0B\u0323", UtfNormalization::NFKC) == U"\u1E0D\u0307"sv);
    CHECK(utf32Normalize(U"\u1E0B\u0323", UtfNormalization::NFKD) == U"\u0064\u0323\u0307"sv);

    CHECK(utf32Normalize(U"\u0071\u0307\u0323", UtfNormalization::NFC) == U"\u0071\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u0071\u0307\u0323", UtfNormalization::NFD) == U"\u0071\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u0071\u0307\u0323", UtfNormalization::NFKC) == U"\u0071\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u0071\u0307\u0323", UtfNormalization::NFKD) == U"\u0071\u0323\u0307"sv);

    CHECK(utf32Normalize(U"\uFB01", UtfNormalization::NFC) == U"\uFB01"sv);
    CHECK(utf32Normalize(U"\uFB01", UtfNormalization::NFD) == U"\uFB01"sv);
    CHECK(utf32Normalize(U"\uFB01", UtfNormalization::NFKC) == U"\u0066\u0069"sv);
    CHECK(utf32Normalize(U"\uFB01", UtfNormalization::NFKD) == U"\u0066\u0069"sv);

    CHECK(utf32Normalize(U"\u0032\u2075", UtfNormalization::NFC) == U"\u0032\u2075"sv);
    CHECK(utf32Normalize(U"\u0032\u2075", UtfNormalization::NFD) == U"\u0032\u2075"sv);
    CHECK(utf32Normalize(U"\u0032\u2075", UtfNormalization::NFKC) == U"\u0032\u0035"sv);
    CHECK(utf32Normalize(U"\u0032\u2075", UtfNormalization::NFKD) == U"\u0032\u0035"sv);

    CHECK(utf32Normalize(U"\u1E9B\u0323", UtfNormalization::NFC) == U"\u1E9B\u0323"sv);
    CHECK(utf32Normalize(U"\u1E9B\u0323", UtfNormalization::NFD) == U"\u017F\u0323\u0307"sv);
    CHECK(utf32Normalize(U"\u1E9B\u0323", UtfNormalization::NFKC) == U"\u1E69"sv);
    CHECK(utf32Normalize(U"\u1E9B\u0323", UtfNormalization::NFKD) == U"\u0073\u0323\u0307"sv);
}
