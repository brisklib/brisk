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
#include <fmt/ranges.h>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/core/Utilities.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "../core/test/HelloWorld.hpp"
#include <brisk/core/Reflection.hpp>
#include "VisualTests.hpp"

namespace Brisk {

std::string glyphRunToString(const GlyphRun& run);
} // namespace Brisk

template <>
struct fmt::formatter<Brisk::FontMetrics> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FontMetrics& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{{ {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f}, {:.3f} }}", value.size,
                        value.ascender, value.descender, value.height, value.spaceAdvanceX,
                        value.lineThickness, value.xHeight, value.capitalHeight),
            ctx);
    }
};

template <>
struct fmt::formatter<Brisk::AscenderDescender> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::AscenderDescender& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(fmt::format("{}/{}", value.ascender, value.descender),
                                                   ctx);
    }
};

template <>
struct fmt::formatter<Brisk::FontManager::FontKey> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::FontManager::FontKey& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{} {} {}", std::get<0>(value), std::get<1>(value), std::get<2>(value)), ctx);
    }
};

template <>
struct fmt::formatter<Brisk::GlyphRun> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::GlyphRun& value, FormatContext& ctx) const {

        return fmt::formatter<std::string>::format(Brisk::glyphRunToString(value), ctx);
    }
};

template <>
struct fmt::formatter<Brisk::PreparedText> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::PreparedText& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("Prepared\n{}\n///////////////////////", fmt::join(value.runs, "\n----\n")), ctx);
    }
};

namespace Brisk {
static Rc<FontManager> fontManager;

static std::string codepointDetails(char32_t value) {
    if (uint32_t(value) >= 32)
        return fmt::format("{} ('{}')", unicodeChar(value),
                           Brisk::utf32ToUtf8(std::u32string_view(&value, 1)));
    else
        return fmt::format("{}      ", unicodeChar(value));
}

std::string glyphRunToString(const GlyphRun& run) {
    std::string result =
        fmt::format("Positioned at {{ {:6.2f}, {:6.2f} }}:\n", run.position.x, run.position.y);
    for (int i = 0; i < run.glyphs.size(); ++i) {
        const Internal::Glyph& g        = run.glyphs[i];
        Internal::GlyphData d           = g.load(run).value_or(Internal::GlyphData{});
        Brisk::FontManager::FontKey key = Brisk::fontManager->faceToKey(run.face);

        std::string flags;
        if (g.flags && Internal::GlyphFlags::AtLineBreak)
            flags += "ALB ";
        else
            flags += "    ";
        if (g.flags && Internal::GlyphFlags::SafeToBreak)
            flags += "STB ";
        else
            flags += "    ";

        result += fmt::format(
            "{{ {:5}, {}, cl={:2}..{:<2}, L={:6.2f}, R={:6.2f}, w={:6.2f}, pos={{ {:6.2f}, {:6.2f} }}, "
            "adv={:6.2f}, sz={:2d}x{:<2d}, sp={:08X}, {}, {} }}\n",
            g.glyph, codepointDetails(g.codepoint), g.begin_char, g.end_char, g.left_caret, g.right_caret,
            g.right_caret - g.left_caret, g.pos.x, g.pos.y, d.advance_x, d.size.x, d.size.y, UINT32_MAX, key,
            flags);
    }
    return result;
}

} // namespace Brisk

namespace Brisk {

TEST_CASE("textBreakPositions") {
    CHECK(utf32ToUtf16(U"abc").size() == 3);
    CHECK(utf32ToUtf16(U"αβγ").size() == 3);
    CHECK(utf32ToUtf16(U"一二三").size() == 3);
    CHECK(utf32ToUtf16(U"𠀀𠀁𠀂").size() == 6);
    CHECK(textBreakPositions(U"abc", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"αβγ", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"一二三", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 1, 2, 3 });
    CHECK(textBreakPositions(U"𠀀𠀁𠀂", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 1, 2, 3 });

    CHECK(textBreakPositions(U"abc abc", TextBreakMode::Grapheme) ==
          std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5, 6, 7 });
    CHECK(textBreakPositions(U"𠀀𠀁𠀂 𠀀𠀁𠀂", TextBreakMode::Grapheme) ==
          std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5, 6, 7 });

    CHECK(textBreakPositions(U"á", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 2 });
    CHECK(textBreakPositions(U"á̈", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 3 });

    CHECK(textBreakPositions(U"a\nb\nc", TextBreakMode::Grapheme) ==
          std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5 });
    CHECK(textBreakPositions(U"a\n\nb", TextBreakMode::Grapheme) == std::vector<uint32_t>{ 0, 1, 2, 3, 4 });

    CHECK(textBreakPositions(U"abc", TextBreakMode::Word) == std::vector<uint32_t>{ 0, 3 });
    CHECK(textBreakPositions(U"abc def", TextBreakMode::Word) == std::vector<uint32_t>{ 0, 3, 4, 7 });

    CHECK(textBreakPositions(U"abc", TextBreakMode::Line) == std::vector<uint32_t>{ 0, 3 });
    CHECK(textBreakPositions(U"abc def", TextBreakMode::Line) == std::vector<uint32_t>{ 0, 4, 7 });

    CHECK(textBreakPositions(U"A B C D E F", TextBreakMode::Line) ==
          std::vector<uint32_t>{ 0, 2, 4, 6, 8, 10, 11 });

    if (icuAvailable) {
        CHECK(textBreakPositions(U"a\u00ADb", TextBreakMode::Line) == std::vector<uint32_t>{ 0, 2, 3 });
    }
}

TEST_CASE("splitTextRuns") {
    CHECK(Internal::toVisualOrder(Internal::splitTextRuns(U"𠀀𠀁𠀂 𠀀𠀁𠀂", TextDirection::LTR)) ==
          std::vector<Internal::TextRun>{
              Internal::TextRun{
                  .direction = TextDirection::LTR, .begin = 0, .end = 7, .visualOrder = 0, .face = nullptr },
          });
    if (icuAvailable) {
        fmt::println("ICU is available");
        CHECK(Internal::toVisualOrder(
                  Internal::splitTextRuns(U"𠀀𠀁𠀂 \U0000200F123\U0000200E 𠀀𠀁𠀂", TextDirection::LTR)) ==
              std::vector<Internal::TextRun>{
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 0,
                                     .end         = 4,
                                     .visualOrder = 0,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 5,
                                     .end         = 8,
                                     .visualOrder = 7,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::RTL,
                                     .begin       = 4,
                                     .end         = 5,
                                     .visualOrder = 10,
                                     .face        = nullptr },
                  Internal::TextRun{ .direction   = TextDirection::LTR,
                                     .begin       = 8,
                                     .end         = 13,
                                     .visualOrder = 11,
                                     .face        = nullptr },
              });
    } else {
        fmt::println("ICU is not available, skipping some tests");
    }
}

TEST_CASE("FontManager") {

    fontManager = rcnew FontManager(nullptr, 1, 5000);

    auto ttf    = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(ttf.has_value());
    auto ttf2 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "GoNotoCurrent-Regular.ttf");
    REQUIRE(ttf2.has_value());
    auto ttf3 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "SourceCodePro-Medium.ttf");
    REQUIRE(ttf3.has_value());
    auto ttf4 = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Light.ttf");
    REQUIRE(ttf4.has_value());

    fontManager->addFont("lato", FontStyle::Normal, FontWeight::Regular, *ttf, true, FontFlags::Default);
    CHECK(fontManager->fontFamilyStyles("lato") == std::vector<FontStyleAndWeight>{ FontStyleAndWeight{
                                                       FontStyle::Normal,
                                                       FontWeight::Regular,
                                                   } });

    fontManager->addFont("noto", FontStyle::Normal, FontWeight::Regular, *ttf2, true, FontFlags::Default);

    fontManager->addFont("mono", FontStyle::Normal, FontWeight::Regular, *ttf3, true, FontFlags::Default);
    fontManager->addFont("latolight", FontStyle::Normal, FontWeight::Regular, *ttf4, true,
                         FontFlags::Default);

    Font font;
    font.fontSize       = 10;
    font.fontFamily     = "lato,noto";
    font.lineHeight     = 1.f;
    FontMetrics metrics = fontManager->metrics(font);
    CHECK(metrics == FontMetrics{ 10, 10, -3, 12, 2.531250, 0.750, 5.080, 7.180 });

    PreparedText run;

    run = fontManager->prepare(font, U""s);
    REQUIRE(run.runs.size() == 0);
    REQUIRE(run.graphemeBoundaries.size() == 1);
    REQUIRE(run.lines.size() == 1);
    CHECK(run.lines[0].runRange == Range{ 0u, 0u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 1u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 1);
    REQUIRE(run.ranges.size() == 0);
    CHECK(run.caretPositions[0] == 0);

    run = fontManager->prepare(font, U"abc"s);
    REQUIRE(run.runs.size() == 1);
    REQUIRE(run.graphemeBoundaries.size() == 4);
    CHECK(run.runs[0].charRange() == Range{ 0u, 3u });
    CHECK(run.runs[0].glyphs.size() == 3);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a');
    CHECK(run.runs[0].glyphs[1].codepoint == U'b');
    CHECK(run.runs[0].glyphs[2].codepoint == U'c');
    CHECK(run.runs[0].glyphs[1].pos.x > run.runs[0].glyphs[0].pos.x);
    CHECK(run.runs[0].glyphs[2].pos.x > run.runs[0].glyphs[1].pos.x);
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 1u }); // ltr
    CHECK(run.runs[0].glyphs[1].charRange() == Range{ 1u, 2u }); // ltr
    CHECK(run.runs[0].glyphs[2].charRange() == Range{ 2u, 3u }); // ltr
    CHECK(run.runs[0].glyphs[1].left_caret > run.runs[0].glyphs[0].left_caret);
    CHECK(run.runs[0].glyphs[2].left_caret > run.runs[0].glyphs[1].left_caret);
    CHECK(run.runs[0].glyphs[1].right_caret > run.runs[0].glyphs[1].left_caret);
    REQUIRE(run.lines.size() == 1);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 4u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 4);
    REQUIRE(run.ranges.size() == 3);
    CHECK(run.caretPositions[1] > run.caretPositions[0]);
    CHECK(run.caretPositions[2] > run.caretPositions[1]);
    CHECK(run.caretPositions[3] > run.caretPositions[2]);
    CHECK(run.graphemeToCaret(0) == PointF{ 0, 0 });
    CHECK(run.graphemeToCaret(1) == PointF{ run.runs[0].glyphs[0].right_caret, 0 });
    CHECK(run.graphemeToCaret(2) == PointF{ run.runs[0].glyphs[1].right_caret, 0 });
    CHECK(run.graphemeToCaret(3) == PointF{ run.runs[0].glyphs[2].right_caret, 0 });

    run = fontManager->prepare(font, U"ǎb"s);
    REQUIRE(run.runs.size() == 1);
    CHECK(run.runs[0].charRange() == Range{ 0u, 3u });
    CHECK(run.runs[0].glyphs.size() == 2);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a'); // ǎ represented by combined glyph
    CHECK(run.runs[0].glyphs[1].codepoint == U'b');
    CHECK(run.runs[0].glyphs[1].pos.x > run.runs[0].glyphs[0].pos.x);
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 2u }); // ltr
    CHECK(run.runs[0].glyphs[1].charRange() == Range{ 2u, 3u }); // ltr
    CHECK(run.runs[0].glyphs[1].left_caret > run.runs[0].glyphs[0].left_caret);
    CHECK(run.runs[0].glyphs[1].right_caret > run.runs[0].glyphs[1].left_caret);
    REQUIRE(run.lines.size() == 1);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 3u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 3);
    REQUIRE(run.ranges.size() == 2);
    CHECK(run.caretPositions[1] > run.caretPositions[0]);
    CHECK(run.caretPositions[2] > run.caretPositions[1]);
    CHECK(run.graphemeToCaret(0) == PointF{ 0, 0 });
    CHECK(run.graphemeToCaret(1) == PointF{ run.runs[0].glyphs[0].right_caret, 0 });
    CHECK(run.graphemeToCaret(2) == PointF{ run.runs[0].glyphs[1].right_caret, 0 });

    run = fontManager->prepare(font, U"a\nb"s);
    REQUIRE(run.runs.size() == 2);
    CHECK(run.runs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[0].glyphs.size() == 1);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a');
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[1].charRange() == Range{ 2u, 3u });
    CHECK(run.runs[1].glyphs.size() == 1);
    CHECK(run.runs[1].direction == TextDirection::LTR);
    CHECK(run.runs[1].glyphs[0].codepoint == U'b');
    CHECK(run.runs[1].glyphs[0].charRange() == Range{ 2u, 3u });
    REQUIRE(run.lines.size() == 2);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 2u });
    CHECK(run.lines[1].runRange == Range{ 1u, 2u });
    CHECK(run.lines[1].graphemeRange == Range{ 2u, 4u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 4);
    REQUIRE(run.ranges.size() == 3);
    CHECK(run.caretPositions[0] == 0);
    CHECK(run.caretPositions[1] > 0);
    CHECK(run.caretPositions[2] == 0);
    CHECK(run.caretPositions[3] > 0);
    CHECK(run.graphemeToCaret(0) == PointF{ 0, 0 });
    CHECK(run.graphemeToCaret(1) == PointF{ run.runs[0].glyphs[0].right_caret, 0 });
    CHECK(run.graphemeToCaret(2) == PointF{ 0, 12 });
    CHECK(run.graphemeToCaret(3) == PointF{ run.runs[1].glyphs[0].right_caret, 12 });

    run = fontManager->prepare(font, U"a\n"s);
    REQUIRE(run.runs.size() == 1);
    CHECK(run.runs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[0].glyphs.size() == 1);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a');
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 1u });
    REQUIRE(run.lines.size() == 2);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 2u });
    CHECK(run.lines[1].runRange == Range{ 1u, 1u });
    CHECK(run.lines[1].graphemeRange == Range{ 2u, 3u });

    run = fontManager->prepare(font, U"a\n\nb"s);
    REQUIRE(run.runs.size() == 2);
    CHECK(run.runs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[0].glyphs.size() == 1);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a');
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[1].charRange() == Range{ 3u, 4u });
    CHECK(run.runs[1].glyphs.size() == 1);
    CHECK(run.runs[1].direction == TextDirection::LTR);
    CHECK(run.runs[1].glyphs[0].codepoint == U'b');
    CHECK(run.runs[1].glyphs[0].charRange() == Range{ 3u, 4u });
    REQUIRE(run.lines.size() == 3);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 2u });
    CHECK(run.lines[1].runRange == Range{ 1u, 1u });
    CHECK(run.lines[1].graphemeRange == Range{ 2u, 3u });
    CHECK(run.lines[2].runRange == Range{ 1u, 2u });
    CHECK(run.lines[2].graphemeRange == Range{ 3u, 5u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 5);
    REQUIRE(run.ranges.size() == 4);
    CHECK(run.caretPositions[0] == 0);
    CHECK(run.caretPositions[1] > 0);
    CHECK(run.caretPositions[2] == 0);
    CHECK(run.caretPositions[3] == 0);
    CHECK(run.caretPositions[4] > 0);
    // CHECK(run.graphemeToCaret(0) == PointF{ 0, 0 });
    // CHECK(run.graphemeToCaret(1) == PointF{ run.runs[0].glyphs[0].right_caret, 0 });
    // CHECK(run.graphemeToCaret(2) == PointF{ 0, 12 });
    // CHECK(run.graphemeToCaret(3) == PointF{ run.runs[1].glyphs[0].right_caret, 12 });

    run = fontManager->prepare(font, TextWithOptions{ U"ab"s, TextOptions::WrapAnywhere }, 1.f);
    REQUIRE(run.runs.size() == 2);
    CHECK(run.runs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[0].glyphs.size() == 1);
    CHECK(run.runs[0].direction == TextDirection::LTR);
    CHECK(run.runs[0].glyphs[0].codepoint == U'a');
    CHECK(run.runs[0].glyphs[0].charRange() == Range{ 0u, 1u });
    CHECK(run.runs[1].charRange() == Range{ 1u, 2u });
    CHECK(run.runs[1].glyphs.size() == 1);
    CHECK(run.runs[1].direction == TextDirection::LTR);
    CHECK(run.runs[1].glyphs[0].codepoint == U'b');
    CHECK(run.runs[1].glyphs[0].charRange() == Range{ 1u, 2u });
    REQUIRE(run.lines.size() == 2);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 1u });
    CHECK(run.lines[1].runRange == Range{ 1u, 2u });
    CHECK(run.lines[1].graphemeRange == Range{ 1u, 3u });
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 3);
    REQUIRE(run.ranges.size() == 2);
    CHECK(run.caretPositions[0] == 0);
    CHECK(run.caretPositions[1] == 0);
    CHECK(run.caretPositions[2] > 0);
    CHECK(run.graphemeToCaret(0) == PointF{ 0, 0 });
    CHECK(run.graphemeToCaret(1) == PointF{ 0, 12 });
    CHECK(run.graphemeToCaret(2) == PointF{ run.runs[1].glyphs[0].right_caret, 12 });

    if (icuAvailable) {
        run = fontManager->prepare(font, U"ابت"s);
        REQUIRE(run.runs.size() == 1);
        CHECK(run.runs[0].charRange() == Range{ 0u, 3u });
        CHECK(run.runs[0].glyphs.size() == 3);
        CHECK(run.runs[0].direction == TextDirection::RTL);
        CHECK(run.runs[0].glyphs[0].codepoint == U'ت');
        CHECK(run.runs[0].glyphs[1].codepoint == U'ب');
        CHECK(run.runs[0].glyphs[2].codepoint == U'ا');
        CHECK(run.runs[0].glyphs[1].pos.x > run.runs[0].glyphs[0].pos.x);
        CHECK(run.runs[0].glyphs[2].pos.x > run.runs[0].glyphs[1].pos.x);
        CHECK(run.runs[0].glyphs[0].charRange() == Range{ 2u, 3u }); // rtl
        CHECK(run.runs[0].glyphs[1].charRange() == Range{ 1u, 2u }); // rtl
        CHECK(run.runs[0].glyphs[2].charRange() == Range{ 0u, 1u }); // rtl
        CHECK(run.runs[0].glyphs[1].left_caret > run.runs[0].glyphs[0].left_caret);
        CHECK(run.runs[0].glyphs[2].left_caret > run.runs[0].glyphs[1].left_caret);
        CHECK(run.runs[0].glyphs[1].right_caret > run.runs[0].glyphs[1].left_caret);
        REQUIRE(run.lines.size() == 1);
        CHECK(run.lines[0].runRange == Range{ 0u, 1u });
        CHECK(run.lines[0].graphemeRange == Range{ 0u, 4u });
        run.updateCaretData();
        REQUIRE(run.caretPositions.size() == 4);
        REQUIRE(run.ranges.size() == 3);
        CHECK(run.caretPositions[1] < run.caretPositions[0]); // rtl
        CHECK(run.caretPositions[2] < run.caretPositions[1]); // rtl
        CHECK(run.caretPositions[3] < run.caretPositions[2]); // rtl
        CHECK(run.graphemeToCaret(0) == PointF{ run.runs[0].glyphs[2].right_caret, 0 });
        CHECK(run.graphemeToCaret(1) == PointF{ run.runs[0].glyphs[1].right_caret, 0 });
        CHECK(run.graphemeToCaret(2) == PointF{ run.runs[0].glyphs[0].right_caret, 0 });
        CHECK(run.graphemeToCaret(3) == PointF{ run.runs[0].glyphs[0].left_caret, 0 });

        run = fontManager->prepare(font, U"abאבcd"s);
        REQUIRE(run.runs.size() == 3);
        CHECK(run.runs[0].charRange() == Range{ 0u, 2u });
        CHECK(run.runs[0].glyphs.size() == 2);
        CHECK(run.runs[0].direction == TextDirection::LTR);
        CHECK(run.runs[1].charRange() == Range{ 2u, 4u });
        CHECK(run.runs[1].glyphs.size() == 2);
        CHECK(run.runs[1].direction == TextDirection::RTL);
        CHECK(run.runs[2].charRange() == Range{ 4u, 6u });
        CHECK(run.runs[2].glyphs.size() == 2);
        CHECK(run.runs[2].direction == TextDirection::LTR);
        REQUIRE(run.lines.size() == 1);
        CHECK(run.lines[0].runRange == Range{ 0u, 3u });
        CHECK(run.lines[0].graphemeRange == Range{ 0u, 7u });
        run.updateCaretData();
        REQUIRE(run.caretPositions.size() == 7);
        REQUIRE(run.ranges.size() == 6);
        CHECK(run.caretPositions[0] == 0);
        CHECK(run.caretPositions[1] > run.caretPositions[0]);
        CHECK(run.caretPositions[2] > run.caretPositions[1]);
        CHECK(run.caretPositions[3] > run.caretPositions[2]);
        CHECK(run.caretPositions[4] < run.caretPositions[3]);
        CHECK(run.caretPositions[4] == run.caretPositions[2]);
        CHECK(run.caretPositions[5] > run.caretPositions[3]);
        CHECK(run.caretPositions[6] > run.caretPositions[5]);

        run = fontManager->prepare(font, TextWithOptions{ U"abאבcd"s, TextOptions::WrapAnywhere }, 22);
        REQUIRE(run.runs.size() == 4);
        CHECK(run.runs[0].charRange() == Range{ 0u, 2u });
        CHECK(run.runs[0].glyphs.size() == 2);
        CHECK(run.runs[0].direction == TextDirection::LTR);
        CHECK(run.runs[1].charRange() == Range{ 2u, 3u });
        CHECK(run.runs[1].glyphs.size() == 1);
        CHECK(run.runs[1].direction == TextDirection::RTL);
        CHECK(run.runs[2].charRange() == Range{ 3u, 4u });
        CHECK(run.runs[2].glyphs.size() == 1);
        CHECK(run.runs[2].direction == TextDirection::RTL);
        CHECK(run.runs[3].charRange() == Range{ 4u, 6u });
        CHECK(run.runs[3].glyphs.size() == 2);
        CHECK(run.runs[3].direction == TextDirection::LTR);
        REQUIRE(run.lines.size() == 2);
        CHECK(run.lines[0].runRange == Range{ 0u, 2u });
        CHECK(run.lines[0].graphemeRange == Range{ 0u, 3u });
        CHECK(run.lines[1].runRange == Range{ 2u, 4u });
        CHECK(run.lines[1].graphemeRange == Range{ 3u, 7u });
    }

    run = fontManager->prepare(font, U"fi"s);
    REQUIRE(run.runs.size() == 1);
    CHECK(run.runs[0].charRange() == Range{ 0u, 2u });
    CHECK(run.runs[0].glyphs.size() == 1);
    run.updateCaretData();
    REQUIRE(run.caretPositions.size() == 3);
    CHECK(run.caretPositions[0] == 0);
    CHECK(run.caretPositions[1] > run.caretPositions[0]);
    CHECK(run.caretPositions[2] > run.caretPositions[1]);
    CHECK(!std::isnan(run.caretPositions[2]));
    REQUIRE(run.lines.size() == 1);
    CHECK(run.lines[0].runRange == Range{ 0u, 1u });
    CHECK(run.lines[0].graphemeRange == Range{ 0u, 3u });

    run = fontManager->prepare(font, U"a"s);
    REQUIRE(run.lines.size() == 1);
    REQUIRE(run.lines[0].baseline == 0);
    run.updateCaretData();
    CHECK(run.graphemeToCaret(0).y == 0);
    CHECK(run.graphemeToCaret(1).y == 0);
    CHECK(run.bounds().height() == 12);

    run = fontManager->prepare(font, U"a\n"s);
    REQUIRE(run.lines.size() == 2);
    REQUIRE(run.lines[0].baseline == 0);
    REQUIRE(run.lines[1].baseline == 12);
    CHECK(run.lines[1].ascDesc == run.lines[0].ascDesc);
    run.updateCaretData();
    CHECK(run.graphemeToCaret(0).y == 0);
    CHECK(run.graphemeToCaret(1).y == 0);
    CHECK(run.graphemeToCaret(2).y == 12);
    CHECK(run.bounds().height() == 24);

    run = fontManager->prepare(font, U"a\nb"s);
    REQUIRE(run.lines.size() == 2);
    REQUIRE(run.lines[0].baseline == 0);
    REQUIRE(run.lines[1].baseline == 12);
    run.updateCaretData();
    CHECK(run.graphemeToCaret(0).y == 0);
    CHECK(run.graphemeToCaret(1).y == 0);
    CHECK(run.graphemeToCaret(2).y == 12);
    CHECK(run.bounds().height() == 24);

    run = fontManager->prepare(font, U"a\n\n"s);
    REQUIRE(run.lines.size() == 3);
    REQUIRE(run.lines[0].baseline == 0);
    REQUIRE(run.lines[1].baseline == 12);
    REQUIRE(run.lines[2].baseline == 24);
    run.updateCaretData();
    CHECK(run.graphemeToCaret(0).y == 0);
    CHECK(run.graphemeToCaret(1).y == 0);
    CHECK(run.graphemeToCaret(2).y == 12);
    CHECK(run.graphemeToCaret(3).y == 24);
    CHECK(run.bounds().height() == 36);

    RectangleF bounds          = fontManager->bounds(font, U"Hello, world!"s, GlyphRunBounds::Text);
    bounds                     = fontManager->bounds(font, U"  Hello, world!"s, GlyphRunBounds::Text);
    bounds                     = fontManager->bounds(font, U"  Hello, world!  "s, GlyphRunBounds::Text);
    bounds                     = fontManager->bounds(font, U"Hello, world!  "s, GlyphRunBounds::Text);

    [[maybe_unused]] Size size = { 512, 64 };

    Font bigFont{ "lato,noto", 36.f };

    static std::set<int> requiresIcu{
        3, 25, 47, 48, 71,
    };
    for (int i = 0; i < std::size(helloWorld); i++) {
        if (!icuAvailable && requiresIcu.contains(i)) {
            continue;
        }
        visualTestMono(fmt::format("hello{}", i), { 512, 64 }, [&](Rc<Image> image) {
            Font font{ "lato,noto", 36.f };
            auto run = fontManager->prepare(font, helloWorld[i]);
            fontManager->testRender(image, run, { 5, 42 });
        });
    }

    visualTestMono("nl-multi", { 256, 128 }, [&](Rc<Image> image) {
        auto run =
            fontManager->prepare(bigFont, TextWithOptions{ utf8ToUtf32("ABC\nDEF"), TextOptions::Default });
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("nl-single", { 256, 128 }, [&](Rc<Image> image) {
        auto run = fontManager->prepare(bigFont,
                                        TextWithOptions{ utf8ToUtf32("ABC\nDEF"), TextOptions::SingleLine });
        fontManager->testRender(image, run, { 5, 42 });
    });

    constexpr auto diac =
        UR"(a	à	â	ă	å	a̋	ä	a̧	ǎ	ã	á	ä́	á̈
i	ì	î	ĭ	i̊	i̋	ï	i̧	ǐ	ĩ	í	ḯ	í̈
q	q̀	q̂	q̆	q̊	q̋	q̈	q̧	q̌	q̃	q́	q̈́	q́̈
I	Ì	Î	Ĭ	I̊	I̋	Ï	I̧	Ǐ	Ĩ	Í	Ḯ	Í̈
Њ	Њ̀	Њ̂	Њ̆	Њ̊	Њ̋	Њ̈	Њ̧	Њ̌	Њ̃	Њ́	Њ̈́	Њ́̈
)";

    visualTestMono("diacritics-lato", { 650, 290 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        font.tabWidth = 5.f;
        auto run      = fontManager->prepare(font, diac);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("diacritics-noto", { 650, 290 }, [&](Rc<Image> image) {
        Font font{ "noto", 36.f };
        font.tabWidth = 5.f;
        auto run      = fontManager->prepare(font, diac);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("bounds-text", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"a");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text-bar", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"|");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text2", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"a  ");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text3", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"  a");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text4", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"    a");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text5", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"    aa");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("bounds-text6", { 128, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 36.f };
        auto run = fontManager->prepare(font, U"    a|");
        fontManager->testRender(image, run, { 5, 42 }, TestRenderFlags::TextBounds);
    });

    visualTestMono("empty-lines", { 128, 256 }, [&](Rc<Image> image) {
        Font font{ "lato", 24.f };
        auto run = fontManager->prepare(font, U"a\nb\n\nd\ne");
        fontManager->testRender(image, run, { 5, 32 }, TestRenderFlags::TextBounds);
    });

    visualTestMono("lineHeight1", { 64, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 16.f };
        font.lineHeight = 1.f;
        auto run        = fontManager->prepare(font, U"1st line\n2nd line");
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::TextBounds);
    });
    visualTestMono("lineHeight1dot5", { 64, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 16.f };
        font.lineHeight = 1.5f;
        auto run        = fontManager->prepare(font, U"1st line\n2nd line");
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::TextBounds);
    });

    visualTestMono("unicode-suppl", { 256, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 36.f };
        auto run = fontManager->prepare(
            font, U"\U00010140\U00010141\U00010142\U00010143\U00010144\U00010145\U00010146\U00010147");
        fontManager->testRender(image, run, { 5, 42 });
    });

    visualTestMono("wrapSpaces", { 64, 64 }, [&](Rc<Image> image) {
        Font font{ "lato", 16.f };
        font.lineHeight = 0.8f;
        auto run        = fontManager->prepare(font, U"1 2  3   4    5     6      ", 41);
        fontManager->testRender(image, run, { 3, 13 }, TestRenderFlags::TextBounds, { 3 + 41 });
    });

    if (icuAvailable) {
        visualTestMono("mixed", { 512, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 36.f };
            auto run = fontManager->prepare(font, U"abcdef مرحبا بالعالم!");
            fontManager->testRender(image, run, { 5, 42 });
        });

        visualTestMono("mixed2", { 512, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 36.f };
            auto run = fontManager->prepare(font, U"123456 مرحبا بالعالم!");
            fontManager->testRender(image, run, { 5, 42 });
        });

        visualTestMono("mixed3", { 512, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 36.f };
            auto run = fontManager->prepare(font, U"مرحبا (بالعالم)!");
            fontManager->testRender(image, run, { 5, 42 });
        });
    }

    visualTestMono("wrapped-abc", { 128, 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 18.f };
        auto t   = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto run = fontManager->prepare(font, t, 120);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 120 }, { 20 });
    });
    visualTestMono("wrapped-abc-big", { 4 * 128, 4 * 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 4 * 18.f };
        auto t   = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto run = fontManager->prepare(font, t, 4 * 120);
        fontManager->testRender(image, run, { 4 * 3, 4 * 20 }, TestRenderFlags::None,
                                { 4 * 3, 4 * 3 + 4 * 120 }, { 4 * 20 });
    });
    visualTestMono("wrapped-abc2", { 128, 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 18.f };
        auto t   = U"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z";
        auto run = fontManager->prepare(font, t, 110);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 110 }, { 20 });
    });
    visualTestMono("wrapped-abc3", { 128, 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 16.f };
        auto t   = U"ABCDEFGHIJKLM N O P Q R S T U V W X Y Z";
        auto run = fontManager->prepare(font, t, 100);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 100 }, { 20 });
    });
    visualTestMono("wrapped-abc4", { 128, 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 16.f };
        auto t   = U"A               B C D E F G H";
        auto run = fontManager->prepare(font, t, 24);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3, 3 + 24 }, { 20 });
    });
    if (icuAvailable) {
        visualTestMono("wrapped-rtl4", { 128, 128 }, [&](Rc<Image> image) {
            Font font{ "noto", 16.f };
            auto t        = U"א          ב ג ד ה ו ז ח ט י";
            auto run      = fontManager->prepare(font, t, 24);
            PointF offset = run.alignLines(1.f);
            fontManager->testRender(image, run, PointF{ 24, 0 } + PointF{ 3, 20 } + offset,
                                    TestRenderFlags::None, { 3, 3 + 24 }, { 20 });
        });
    }
    visualTestMono("wrapped-abc0", { 128, 128 }, [&](Rc<Image> image) {
        Font font{ "noto", 16.f };
        auto t   = U"ABCDEFGHIJKLM N O P Q R S T U V W X Y Z";
        auto run = fontManager->prepare(font, t, 0);
        fontManager->testRender(image, run, { 3, 20 }, TestRenderFlags::None, { 3 }, { 20 });
    });
    if (icuAvailable) {
        visualTestMono("wrapped-cn", { 490, 384 }, [&](Rc<Image> image) {
            Font font{ "noto", 32.f };
            auto t =
                U"人人生而自由，在尊严和权利上一律平等。他们赋有理性和良心，并应以兄弟关系的精神相对待。";
            auto run = fontManager->prepare(font, t, 460);
            fontManager->testRender(image, run, { 3, 36 }, TestRenderFlags::None, { 3, 3 + 460 }, { 36 });
        });
        visualTestMono("wrapped-ar-left", { 490, 384 }, [&](Rc<Image> image) {
            Font font{ "noto", 32.f };
            // clang-format off
        auto t = U"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق. وقد وهبوا عقلاً وضميرًا وعليهم أن يعامل بعضهم بعضًا بروح الإخاء.";
            // clang-format on
            auto run = fontManager->prepare(font, t, 360);
            fontManager->testRender(image, run, { 23, 36 }, TestRenderFlags::None, { 23, 23 + 360 }, { 36 });
        });
        visualTestMono("wrapped-ar-right", { 490, 384 }, [&](Rc<Image> image) {
            Font font{ "noto", 32.f };
            // clang-format off
        auto t = U"يولد جميع الناس أحرارًا متساوين في الكرامة والحقوق. وقد وهبوا عقلاً وضميرًا وعليهم أن يعامل بعضهم بعضًا بروح الإخاء.";
            // clang-format on
            auto run      = fontManager->prepare(font, t, 360);
            PointF offset = run.alignLines(1.f);
            fontManager->testRender(image, run,
                                    RectangleF{ 0, 0, 360, 384 }.at(1, 0.f) + PointF{ 23, 36 } + offset,
                                    TestRenderFlags::None, { 23, 23 + 360 }, { 36 });
        });
    }
    visualTestMono("letter-spacing", { 640, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        font.letterSpacing = 12.f;
        auto t             = U"Letter spacing fi fl ff áb́ć";
        auto run           = fontManager->prepare(font, t, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("word-spacing", { 640, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        font.wordSpacing = 12.f;
        auto t           = U"Word spacing fi fl ff áb́ć";
        auto run         = fontManager->prepare(font, t, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("letter-spacing-cn", { 640, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        font.letterSpacing = 12.f;
        auto t             = U"人人生而自由，在尊严和权利上一律平等。";
        auto run           = fontManager->prepare(font, t, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    visualTestMono("word-spacing-cn", { 640, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        font.wordSpacing = 12.f;
        auto t           = U"人人生而自由，在尊严和权利上一律平等。";
        auto run         = fontManager->prepare(font, t, HUGE_VALF);
        fontManager->testRender(image, run, { 5, 36 });
    });
    if (icuAvailable) {
        visualTestMono("letter-spacing-ar", { 640, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 22.f };
            font.letterSpacing = 12.f;
            auto t             = U"abcdef مرحبا بالعالم!";
            auto run           = fontManager->prepare(font, t, HUGE_VALF);
            fontManager->testRender(image, run, { 5, 36 });
        });
        visualTestMono("word-spacing-ar", { 640, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 22.f };
            font.wordSpacing = 12.f;
            auto t           = U"abcdef مرحبا بالعالم!";
            auto run         = fontManager->prepare(font, t, HUGE_VALF);
            fontManager->testRender(image, run, { 5, 36 });
        });
    }

    visualTestMono("alignment-ltr", { 256, 64 }, [&](Rc<Image> image) {
        Font font{ "noto", 32.f };
        auto t        = U"Hello, world!";
        auto run      = fontManager->prepare(font, t);
        PointF offset = run.alignLines(0.f, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(0, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });
    if (icuAvailable) {
        visualTestMono("alignment-rtl", { 256, 64 }, [&](Rc<Image> image) {
            Font font{ "noto", 32.f };
            auto t        = U"مرحبا بالعالم!";
            auto run      = fontManager->prepare(font, t);
            PointF offset = run.alignLines(1.f, 0.5f);
            fontManager->testRender(image, run,
                                    RectangleF(image->bounds().withPadding(2, 2)).at(1, 0.5f) + offset,
                                    TestRenderFlags::TextBounds);
        });
    }

    visualTestMono("wrapped-align-left", { 256, 256 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run      = fontManager->prepare(font, t, image->width() - 4);
        PointF offset = run.alignLines(0.f, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(0, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });
    visualTestMono("wrapped-align-center", { 256, 256 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run      = fontManager->prepare(font, t, image->width() - 4);
        PointF offset = run.alignLines(0.5f, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(0.5f, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });
    visualTestMono("wrapped-align-right", { 256, 256 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        auto t = U"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla scelerisque posuere urna "
                 U"sit amet luctus.";
        auto run      = fontManager->prepare(font, t, image->width() - 4);
        PointF offset = run.alignLines(1.f, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(1, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });
    visualTestMono("indented-left", { 256, 256 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        auto t        = U"0\n  2\n    4\n  2\n   3\n0";
        auto run      = fontManager->prepare(font, t, image->width() - 4);
        PointF offset = run.alignLines(0, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(0, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });
    visualTestMono("indented-right", { 256, 256 }, [&](Rc<Image> image) {
        Font font{ "noto", 22.f };
        auto t        = U"0\n  2\n    4\n  2\n   3\n0";
        auto run      = fontManager->prepare(font, t, image->width() - 4);
        PointF offset = run.alignLines(1, 0.5f);
        fontManager->testRender(image, run,
                                RectangleF(image->bounds().withPadding(2, 2)).at(1, 0.5f) + offset,
                                TestRenderFlags::TextBounds);
    });

    Font noto24{ "noto", 24 };
    Font noto36{ "noto", 36 };
    Font lato24{ "lato", 24 };
    Font lato26{ "lato", 26 };
    Font lato48{ "lato", 48 };
    Font latolight20{ "latolight", 20.f };
    Font mono20{ "mono", 20 };

    visualTestMono("richtext1", { 192, 64 }, [&](Rc<Image> image) {
        FontAndColor fonts[2]{ { noto24 }, { noto36 } };
        uint32_t offsets[1]{ 3 };

        auto run = fontManager->prepare(TextWithOptions{ utf8ToUtf32("ABCDEF"), TextOptions::Default }, fonts,
                                        offsets);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("richtext2", { 256, 64 }, [&](Rc<Image> image) {
        FontAndColor fonts[3]{ { latolight20 }, { mono20 }, { latolight20 } };
        uint32_t offsets[2]{ 6, 11 };

        auto run = fontManager->prepare(
            TextWithOptions{ utf8ToUtf32("Press Enter to continue"), TextOptions::Default }, fonts, offsets);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("richtext-liga", { 192, 64 }, [&](Rc<Image> image) {
        FontAndColor fonts[2]{ { lato24 }, { lato26 } };
        uint32_t offsets[1]{ 4 };

        auto run = fontManager->prepare(TextWithOptions{ utf8ToUtf32("fi fi fi"), TextOptions::Default },
                                        fonts, offsets);
        fontManager->testRender(image, run, { 5, 42 });
    });
    visualTestMono("richtext-ml", { 256, 256 }, [&](Rc<Image> image) {
        FontAndColor fonts[3]{ { lato24 }, { lato48 }, { lato24 } };
        uint32_t offsets[2]{ 24, 25 };

        auto run = fontManager->prepare(
            TextWithOptions{ utf8ToUtf32("Lorem ipsum dolor sit amet, consectetur adipiscing elit"),
                             TextOptions::Default },
            fonts, offsets, 170);
        fontManager->testRender(image, run, { 8, 32 });
    });

    fontManager.reset();
}

} // namespace Brisk
