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

#include "BasicTypes.hpp"
#include <string_view>
#include <vector>
#include "Json.hpp"
#include "internal/Function.hpp"

namespace Brisk {

/// @brief If CHAR_IS_UTF8 is defined, char and std::string are expected to contain utf-8, otherwise latin-1
/// (8-bit subset of Unicode)
#define CHAR_IS_UTF8 1

/**
 * @enum UtfPolicy
 * @brief Enum class representing policies for handling invalid UTF characters.
 */
enum class UtfPolicy {
    SkipInvalid,             ///< Skip invalid characters.
    ReplaceInvalid,          ///< Replace invalid characters with a replacement character.
    Default = ReplaceInvalid ///< Default policy for handling invalid characters.
};

/**
 * @brief The replacement character used for invalid UTF characters.
 */
constexpr inline char32_t replacementChar = U'\U0000FFFD';

/**
 * @brief The replacement character in char form.
 */
#define REPLACEMENT_CHAR '\U0000FFFD'

/**
 * @brief The replacement character in string literal form.
 */
#define REPLACEMENT_CHAR_S "\U0000FFFD"

/**
 * @brief UTF-8 Byte Order Mark (BOM) as a string view.
 */
extern U8StringView utf8_bom;

/**
 * @brief UTF-16 Byte Order Mark (BOM) as a string view.
 */
extern U16StringView utf16_bom;

/**
 * @brief UTF-32 Byte Order Mark (BOM) as a string view.
 */
extern U32StringView utf32_bom;

/**
 * @brief Skips BOM in a UTF encoded text.
 *
 * @param text A string view of the input text.
 * @return A string view of the text without BOM.
 */
template <typename OutChar, typename InChar>
std::basic_string_view<OutChar> utfSkipBom(std::basic_string_view<InChar> text);

/**
 * @brief Skips BOM in UTF-8 encoded text.
 *
 * @param text A string view of the input text.
 * @return A string view of the text without BOM.
 */
inline U8StringView utf8SkipBom(U8StringView text) {
    return utfSkipBom<char>(text);
}

/**
 * @brief Skips BOM in UTF-16 encoded text.
 *
 * @param text A string view of the input text.
 * @return A string view of the text without BOM.
 */
inline U16StringView utf16SkipBom(U16StringView text) {
    return utfSkipBom<char16_t>(text);
}

/**
 * @brief Skips BOM in UTF-32 encoded text.
 *
 * @param text A string view of the input text.
 * @return A string view of the text without BOM.
 */
inline U32StringView utf32SkipBom(U32StringView text) {
    return utfSkipBom<char32_t>(text);
}

/**
 * @brief Converts text from one UTF encoding to another.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A string of the text in the target UTF encoding.
 */
template <typename OutChar, typename InChar>
std::basic_string<OutChar> utfToUtf(std::basic_string_view<InChar> text, UtfPolicy policy);

/**
 * @brief Converts text from any encoding to UTF-8.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-8 encoded string.
 */
template <typename InChar>
inline U8String toUtf8(std::basic_string_view<InChar> text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char, InChar>(text, policy);
}

/**
 * @brief Converts text from any encoding to UTF-16.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-16 encoded string.
 */
template <typename InChar>
inline U16String toUtf16(std::basic_string_view<InChar> text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char16_t, InChar>(text, policy);
}

/**
 * @brief Converts text from any encoding to UTF-32.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-32 encoded string.
 */
template <typename InChar>
inline U32String toUtf32(std::basic_string_view<InChar> text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char32_t, InChar>(text, policy);
}

/**
 * @brief Converts UTF-8 text to UTF-16.
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-16 encoded string.
 */
inline std::u16string utf8ToUtf16(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char16_t, char>(text, policy);
}

/**
 * @brief Converts UTF-8 text to UTF-32.
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-32 encoded string.
 */
inline std::u32string utf8ToUtf32(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char32_t, char>(text, policy);
}

/**
 * @brief Converts UTF-16 text to UTF-8.
 *
 * @param text A UTF-16 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-8 encoded string.
 */
inline std::string utf16ToUtf8(U16StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char, char16_t>(text, policy);
}

/**
 * @brief Converts UTF-16 text to UTF-32.
 *
 * @param text A UTF-16 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-32 encoded string.
 */
inline std::u32string utf16ToUtf32(U16StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char32_t, char16_t>(text, policy);
}

/**
 * @brief Converts UTF-32 text to UTF-8.
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-8 encoded string.
 */
inline std::string utf32ToUtf8(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char, char32_t>(text, policy);
}

/**
 * @brief Converts UTF-32 text to UTF-16.
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-16 encoded string.
 */
inline std::u16string utf32ToUtf16(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char16_t, char32_t>(text, policy);
}

/**
 * @brief Converts wide character string (wchar_t) to UTF-8.
 *
 * @param text A wide character string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-8 encoded string.
 */
inline std::string wcsToUtf8(WStringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char, wchar_t>(text, policy);
}

/**
 * @brief Converts UTF-8 text to wide character string (wchar_t).
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A wide character string.
 */
inline std::wstring utf8ToWcs(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<wchar_t, char>(text, policy);
}

/**
 * @brief Converts wide character string (wchar_t) to UTF-32.
 *
 * @param text A wide character string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-32 encoded string.
 */
inline std::u32string wcsToUtf32(WStringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<char32_t, wchar_t>(text, policy);
}

/**
 * @brief Converts UTF-32 text to wide character string (wchar_t).
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A wide character string.
 */
inline std::wstring utf32ToWcs(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfToUtf<wchar_t, char32_t>(text, policy);
}

/**
 * @brief Counts the number of UTF codepoints in the text.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return The number of UTF codepoints in the text.
 */
template <typename Char>
size_t utfCodepoints(std::basic_string_view<Char> text, UtfPolicy policy = UtfPolicy::Default);

/**
 * @brief Counts the number of UTF-8 codepoints in the text.
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return The number of UTF-8 codepoints in the text.
 */
inline size_t utf8Codepoints(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCodepoints(text, policy);
}

/**
 * @brief Counts the number of UTF-16 codepoints in the text.
 *
 * @param text A UTF-16 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return The number of UTF-16 codepoints in the text.
 */
inline size_t utf16Codepoints(U16StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCodepoints(text, policy);
}

/**
 * @brief Counts the number of UTF-32 codepoints in the text.
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return The number of UTF-32 codepoints in the text.
 */
inline size_t utf32Codepoints(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCodepoints(text, policy);
}

/**
 * @brief Cleans up invalid UTF characters in the text.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A cleaned up string with invalid UTF characters handled according to the policy.
 */
template <typename Char>
std::basic_string<Char> utfCleanup(std::basic_string_view<Char> text, UtfPolicy policy = UtfPolicy::Default);

/**
 * @brief Cleans up invalid UTF-8 characters in the text.
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A cleaned up UTF-8 encoded string.
 */
inline std::string utf8Cleanup(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCleanup(text, policy);
}

/**
 * @brief Cleans up invalid UTF-16 characters in the text.
 *
 * @param text A UTF-16 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A cleaned up UTF-16 encoded string.
 */
inline std::u16string utf16Cleanup(U16StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCleanup(text, policy);
}

/**
 * @brief Cleans up invalid UTF-32 characters in the text.
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A cleaned up UTF-32 encoded string.
 */
inline std::u32string utf32Cleanup(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfCleanup(text, policy);
}

/**
 * @enum UtfValidation
 * @brief Enum class representing the types of UTF validation results.
 */
enum class UtfValidation {
    Valid,     ///< The UTF sequence is valid.
    Invalid,   ///< The UTF sequence is invalid.
    Overlong,  ///< The UTF sequence is overlong.
    Truncated, ///< The UTF sequence is truncated.
};

/**
 * @brief Checks if the text contains only ASCII characters.
 *
 * @param text A UTF-8 encoded string view.
 * @return True if the text is ASCII, otherwise false.
 */
bool isAscii(U8StringView text);

/**
 * @brief Validates a UTF encoded text.
 *
 * @param text A string view of the input text.
 * @return The validation result of the UTF text.
 */
template <typename Char>
UtfValidation utfValidate(std::basic_string_view<Char> text);

/**
 * @brief Validates UTF-8 encoded text.
 *
 * @param text A UTF-8 encoded string view.
 * @return The validation result of the UTF-8 text.
 */
inline UtfValidation utf8Validate(U8StringView text) {
    return utfValidate(text);
}

/**
 * @brief Validates UTF-16 encoded text.
 *
 * @param text A UTF-16 encoded string view.
 * @return The validation result of the UTF-16 text.
 */
inline UtfValidation utf16Validate(U16StringView text) {
    return utfValidate(text);
}

/**
 * @brief Validates UTF-32 encoded text.
 *
 * @param text A UTF-32 encoded string view.
 * @return The validation result of the UTF-32 text.
 */
inline UtfValidation utf32Validate(U32StringView text) {
    return utfValidate(text);
}

/**
 * @brief Enum values representing special UTF codepoints for error handling.
 */
enum : char32_t {
    UtfInvalid   = static_cast<char32_t>(-1), ///< Represents an invalid UTF codepoint.
    UtfOverlong  = static_cast<char32_t>(-2), ///< Represents an overlong UTF codepoint.
    UtfTruncated = static_cast<char32_t>(-3), ///< Represents a truncated UTF codepoint.
};

/**
 * @brief Transforms UTF text using a provided function.
 *
 * @param text A string view of the input text.
 * @param fn A function to transform each UTF codepoint.
 * @param policy The policy to handle invalid characters.
 * @return A transformed string.
 */
template <typename Char>
std::basic_string<Char> utfTransform(std::basic_string_view<Char> text,
                                     const function<char32_t(char32_t)>& fn,
                                     UtfPolicy policy = UtfPolicy::Default);

/**
 * @brief Transforms ASCII text using a provided function.
 *
 * @param text An ASCII string view.
 * @param fn A function to transform each UTF codepoint.
 * @return A transformed ASCII string.
 */
std::string asciiTransform(AsciiStringView text, const function<char32_t(char32_t)>& fn);

/**
 * @brief Transforms UTF-8 text using a provided function.
 *
 * @param text A UTF-8 encoded string view.
 * @param fn A function to transform each UTF codepoint.
 * @param policy The policy to handle invalid characters.
 * @return A transformed UTF-8 encoded string.
 */
inline std::string utf8Transform(U8StringView text, const function<char32_t(char32_t)>& fn,
                                 UtfPolicy policy = UtfPolicy::Default) {
    return utfTransform(text, fn, policy);
}

/**
 * @brief Transforms UTF-16 text using a provided function.
 *
 * @param text A UTF-16 encoded string view.
 * @param fn A function to transform each UTF codepoint.
 * @param policy The policy to handle invalid characters.
 * @return A transformed UTF-16 encoded string.
 */
inline std::u16string utf16Transform(U16StringView text, const function<char32_t(char32_t)>& fn,
                                     UtfPolicy policy = UtfPolicy::Default) {
    return utfTransform(text, fn, policy);
}

/**
 * @brief Transforms UTF-32 text using a provided function.
 *
 * @param text A UTF-32 encoded string view.
 * @param fn A function to transform each UTF codepoint.
 * @param policy The policy to handle invalid characters.
 * @return A transformed UTF-32 encoded string.
 */
inline std::u32string utf32Transform(U32StringView text, const function<char32_t(char32_t)>& fn,
                                     UtfPolicy policy = UtfPolicy::Default) {
    return utfTransform(text, fn, policy);
}

/**
 * @brief Reads a UTF codepoint from a text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @return The UTF codepoint read from the text.
 */
char32_t utfRead(const char*& text, const char* end);

/**
 * @brief Reads a UTF codepoint from a UTF-16 text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @return The UTF codepoint read from the text.
 */
char32_t utfRead(const char16_t*& text, const char16_t* end);

/**
 * @brief Reads a UTF codepoint from a UTF-32 text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @return The UTF codepoint read from the text.
 */
char32_t utfRead(const char32_t*& text, const char32_t* end);

/**
 * @brief Reads a UTF codepoint from a wide character text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @return The UTF codepoint read from the text.
 */
char32_t utfRead(const wchar_t*& text, const wchar_t* end);

/**
 * @brief Writes a UTF codepoint to a text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @param ch The UTF codepoint to write.
 */
void utfWrite(char*& text, char* end, char32_t ch);

/**
 * @brief Writes a UTF codepoint to a UTF-16 text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @param ch The UTF codepoint to write.
 */
void utfWrite(char16_t*& text, char16_t* end, char32_t ch);

/**
 * @brief Writes a UTF codepoint to a UTF-32 text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @param ch The UTF codepoint to write.
 */
void utfWrite(char32_t*& text, char32_t* end, char32_t ch);

/**
 * @brief Writes a UTF codepoint to a wide character text range.
 *
 * @param text Pointer to the current position in the text.
 * @param end Pointer to the end of the text.
 * @param ch The UTF codepoint to write.
 */
void utfWrite(wchar_t*& text, wchar_t* end, char32_t ch);

/**
 * @brief Utility struct for encoding a single Unicode character to UTF-8.
 */
struct Utf8Character {
    /**
     * @brief Buffer to store the UTF-8 encoded character.
     */
    char buf[4];

    /**
     * @brief The length of the UTF-8 encoded character in bytes (1-4).
     */
    uint32_t len;

    /**
     * @brief Constructs a Utf8Character from a Unicode codepoint.
     * @param codepoint The Unicode codepoint to encode as UTF-8.
     */
    explicit Utf8Character(char32_t codepoint) {
        char* data = buf;
        utfWrite(data, buf + std::size(buf), codepoint);
        len = static_cast<uint32_t>(data - buf);
    }

    /**
     * @brief Implicitly converts the Utf8Character to a string view.
     * @return A std::string_view representing the UTF-8 encoded character.
     */
    operator std::string_view() const noexcept {
        return std::string_view(buf, len);
    }
};

/**
 * @brief Struct representing a UTF iterator for iterating over UTF text.
 *
 * @tparam InChar The character type of the input text.
 */
template <typename InChar>
struct UtfIterator {
    using value_type = char32_t;

    struct end_iterator {};

    struct iterator {
        const InChar* it;
        const InChar* next;
        const InChar* end;
        char32_t ch;

        iterator(std::basic_string_view<InChar> text) {
            it   = text.data();
            next = text.data();
            end  = text.data() + text.size();
            if (it != end)
                ch = utfRead(next, end);
            else
                ch = UtfInvalid;
        }

        char32_t operator*() const noexcept {
            return ch;
        }

        iterator& operator++() noexcept {
            it = next;
            ch = utfRead(next, end);
            return *this;
        }

        iterator operator++(int) noexcept {
            iterator tmp{ *this };
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& other) const noexcept {
            return it == other.it;
        }

        bool operator==(const end_iterator&) const noexcept {
            return it == end;
        }

        bool operator!=(const iterator& other) const noexcept {
            return it != other.it;
        }

        bool operator!=(const end_iterator&) const noexcept {
            return it != end;
        }
    };

    using const_iterator = iterator;

    std::basic_string_view<InChar> text;

    iterator begin() const {
        return { text };
    }

    end_iterator end() const {
        return {};
    }
};

/**
 * @brief Creates a UTF iterator for iterating over UTF text.
 *
 * @param text A string view of the input text.
 * @param policy The policy to handle invalid characters.
 * @return A UTF iterator for the given text.
 */
template <typename InChar>
UtfIterator<InChar> utfIterate(std::basic_string_view<InChar> text, UtfPolicy policy = UtfPolicy::Default) {
    return { text };
}

/**
 * @brief Creates a UTF-8 iterator for iterating over UTF-8 text.
 *
 * @param text A UTF-8 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-8 iterator for the given text.
 */
inline UtfIterator<U8Char> utf8Iterate(U8StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfIterate(text, policy);
}

/**
 * @brief Creates a UTF-16 iterator for iterating over UTF-16 text.
 *
 * @param text A UTF-16 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-16 iterator for the given text.
 */
inline UtfIterator<char16_t> utf16Iterate(U16StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfIterate(text, policy);
}

/**
 * @brief Creates a UTF-32 iterator for iterating over UTF-32 text.
 *
 * @param text A UTF-32 encoded string view.
 * @param policy The policy to handle invalid characters.
 * @return A UTF-32 iterator for the given text.
 */
inline UtfIterator<char32_t> utf32Iterate(U32StringView text, UtfPolicy policy = UtfPolicy::Default) {
    return utfIterate(text, policy);
}

/**
 * @enum UtfNormalization
 * @brief Enum class representing the types of UTF normalization.
 */
enum class UtfNormalization {
    Compose   = 1,                  ///< Compose normalization.
    Decompose = 2,                  ///< Decompose normalization.
    Compat    = 4,                  ///< Compatibility normalization.
    NFC       = Compose,            ///< Compose normalization (alias for Compose).
    NFD       = Decompose,          ///< Decompose normalization (alias for Decompose).
    NFKC      = Compat | Compose,   ///< Compatibility and Compose normalization (alias for NFKC).
    NFKD      = Compat | Decompose, ///< Compatibility and Decompose normalization (alias for NFKD).
};

template <>
constexpr inline bool isBitFlags<UtfNormalization> = true;

/**
 * @brief Normalizes UTF text according to the specified normalization type.
 *
 * @param text A string view of the input text.
 * @param normalization The normalization type to apply.
 * @param policy The policy to handle invalid characters.
 * @return A normalized string.
 */
template <typename Char>
std::basic_string<Char> utfNormalize(std::basic_string_view<Char> text, UtfNormalization normalization,
                                     UtfPolicy policy = UtfPolicy::Default);

/**
 * @brief Normalizes UTF-8 text according to the specified normalization type.
 *
 * @param text A UTF-8 encoded string view.
 * @param normalization The normalization type to apply.
 * @param policy The policy to handle invalid characters.
 * @return A normalized UTF-8 encoded string.
 */
inline U8String utf8Normalize(U8StringView text, UtfNormalization normalization,
                              UtfPolicy policy = UtfPolicy::Default) {
    return utfNormalize(text, normalization, policy);
}

/**
 * @brief Normalizes UTF-16 text according to the specified normalization type.
 *
 * @param text A UTF-16 encoded string view.
 * @param normalization The normalization type to apply.
 * @param policy The policy to handle invalid characters.
 * @return A normalized UTF-16 encoded string.
 */
inline U16String utf16Normalize(U16StringView text, UtfNormalization normalization,
                                UtfPolicy policy = UtfPolicy::Default) {
    return utfNormalize(text, normalization, policy);
}

/**
 * @brief Normalizes UTF-32 text according to the specified normalization type.
 *
 * @param text A UTF-32 encoded string view.
 * @param normalization The normalization type to apply.
 * @param policy The policy to handle invalid characters.
 * @return A normalized UTF-32 encoded string.
 */
inline U32String utf32Normalize(U32StringView text, UtfNormalization normalization,
                                UtfPolicy policy = UtfPolicy::Default) {
    return utfNormalize(text, normalization, policy);
}

} // namespace Brisk

namespace std {

/**
 * @brief Serializes a UTF-32 string to JSON.
 *
 * @param j The JSON object to serialize to.
 * @param s The UTF-32 string to serialize.
 * @return True if serialization was successful, otherwise false.
 */
bool toJson(Brisk::Json& j, const std::u32string& s);

/**
 * @brief Serializes a UTF-16 string to JSON.
 *
 * @param j The JSON object to serialize to.
 * @param s The UTF-16 string to serialize.
 * @return True if serialization was successful, otherwise false.
 */
bool toJson(Brisk::Json& j, const std::u16string& s);

/**
 * @brief Serializes a wide string to JSON.
 *
 * @param j The JSON object to serialize to.
 * @param s The wide string to serialize.
 * @return True if serialization was successful, otherwise false.
 */
bool toJson(Brisk::Json& j, const std::wstring& s);

/**
 * @brief Deserializes a UTF-32 string from JSON.
 *
 * @param j The JSON object to deserialize from.
 * @param s The UTF-32 string to deserialize into.
 * @return True if deserialization was successful, otherwise false.
 */
bool fromJson(const Brisk::Json& j, std::u32string& s);

/**
 * @brief Deserializes a UTF-16 string from JSON.
 *
 * @param j The JSON object to deserialize from.
 * @param s The UTF-16 string to deserialize into.
 * @return True if deserialization was successful, otherwise false.
 */
bool fromJson(const Brisk::Json& j, std::u16string& s);

/**
 * @brief Deserializes a wide string from JSON.
 *
 * @param j The JSON object to deserialize from.
 * @param s The wide string to deserialize into.
 * @return True if deserialization was successful, otherwise false.
 */
bool fromJson(const Brisk::Json& j, std::wstring& s);

} // namespace std
