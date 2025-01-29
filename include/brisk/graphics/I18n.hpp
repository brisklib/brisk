#pragma once

#include "brisk/core/BasicTypes.hpp"
#include "brisk/core/RC.hpp"
#include "brisk/core/Reflection.hpp"
#include <cstdint>
#include <vector>
#include <string_view>
#include <type_traits>

namespace Brisk {

enum class TextBreakMode {
    Grapheme,
    Word,
    Line,
};

constexpr auto operator+(TextBreakMode value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

enum class TextDirection : uint8_t {
    LTR,
    RTL,
};

template <>
inline constexpr std::initializer_list<NameValuePair<TextDirection>> defaultNames<TextDirection>{
    { "LTR", TextDirection::LTR },
    { "RTL", TextDirection::RTL },
};

namespace Internal {

class TextBreakIterator {
public:
    virtual ~TextBreakIterator() {}

    virtual std::optional<uint32_t> next() = 0;
};

RC<TextBreakIterator> textBreakIterator(std::u32string_view text, TextBreakMode mode);

class BidiTextIterator {
public:
    virtual ~BidiTextIterator() {}

    struct TextFragment {
        Range<uint32_t> codepointRange;
        uint32_t visualOrder;
        TextDirection direction;
    };

    virtual std::optional<TextFragment> next() = 0;
};

RC<BidiTextIterator> bidiTextIterator(std::u32string_view text, TextDirection defaultDirection);

} // namespace Internal

} // namespace Brisk
