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
 */                                                                                                          \
#pragma once

#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/RC.hpp>
#include <brisk/core/Reflection.hpp>
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
