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
 */

#include <catch2/catch_all.hpp>
#include "TextEngine/include/text_layout/Layout.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace TL = Brisk::TextLayout;

namespace Brisk::TextLayout {

constexpr TL::LayoutUnit k12 = TL::fromFloat(12.0f);
constexpr TL::LayoutUnit k16 = TL::fromFloat(16.0f);
constexpr TL::LayoutUnit k24 = TL::fromFloat(24.0f);

inline std::vector<ScriptTag> detectScripts(std::u32string_view paragraphText) {
    const DocumentSource document(paragraphText);
    std::vector<ScriptTag> result(document.graphemeCount());
    TL::detectScripts(document, CodepointRange{ 0, static_cast<CodepointIndex>(paragraphText.size()) },
                      result);
    return result;
}

} // namespace Brisk::TextLayout

namespace {

// Test-only PNG renderer.
// The implementation is included from the production source to keep this test helper
// identical to the historical renderer while ensuring it is not part of the library API.

const TL::FontDatabase* raw(const std::shared_ptr<const TL::FontDatabase>& database) {
    return database.get();
}

TL::FontDef makeFont() {
    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    return TL::FontDef{
        .familyNames   = families,
        .fontSize      = TL::k12,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };
}

uint32_t scriptTag(std::string_view tag) {
    REQUIRE(tag.size() == 4);
    return TL::fourCCToUint32(tag);
}

} // namespace

TEST_CASE("GraphemeMap::iterate empty text", "[layout2][grapheme]") {
    const TL::GraphemeMap map(U"");
    std::vector<TL::CodepointRange> ranges;
    map.iterate([&](TL::CodepointRange range) {
        ranges.push_back(range);
    });

    REQUIRE(ranges.empty());
}

TEST_CASE("GraphemeMap::iterate ascii and complex clusters", "[layout2][grapheme]") {
    // "abc"
    {
        const TL::GraphemeMap map(U"abc");
        std::vector<TL::CodepointRange> ranges;
        map.iterate([&](TL::CodepointRange range) {
            ranges.push_back(range);
        });
        REQUIRE(ranges.size() == 3);
        REQUIRE(ranges[0] == TL::CodepointRange{ 0, 1 });
        REQUIRE(ranges[1] == TL::CodepointRange{ 1, 2 });
        REQUIRE(ranges[2] == TL::CodepointRange{ 2, 3 });
    }

    // Combining character: 'e' (U+0065) + combining acute (U+0301) + 'b' (U+0062)
    {
        const std::u32string text = { 0x0065, 0x0301, 0x0062 };
        const TL::GraphemeMap map(text);
        std::vector<TL::CodepointRange> ranges;
        map.iterate([&](TL::CodepointRange range) {
            ranges.push_back(range);
        });
        REQUIRE(ranges.size() == 2);
        REQUIRE(ranges[0] == TL::CodepointRange{ 0, 2 });
        REQUIRE(ranges[1] == TL::CodepointRange{ 2, 3 });
    }

    // ZWJ sequence: \U0001F468 (U+1F468) + ZWJ (U+200D) + \U0001F4BB (U+1F4BB)
    {
        const std::u32string text = { 0x1F468, 0x200D, 0x1F4BB };
        const TL::GraphemeMap map(text);
        std::vector<TL::CodepointRange> ranges;
        map.iterate([&](TL::CodepointRange range) {
            ranges.push_back(range);
        });
        REQUIRE(ranges.size() == 1);
        REQUIRE(ranges[0] == TL::CodepointRange{ 0, 3 });
    }

    // CRLF sequence with surrounding characters: "a\r\nb"
    {
        const std::u32string text{ U'a', U'e', U'\u0301', U'\r', U'\n', U'b' };
        const TL::GraphemeMap map(text);
        std::vector<TL::CodepointRange> ranges;
        map.iterate([&](TL::CodepointRange range) {
            ranges.push_back(range);
        });
        REQUIRE(ranges.size() == 4);
        REQUIRE(ranges[0] == TL::CodepointRange{ 0, 1 }); // 'a'
        REQUIRE(ranges[1] == TL::CodepointRange{ 1, 3 }); // 'e' + acute
        REQUIRE(ranges[2] == TL::CodepointRange{ 3, 5 }); // "\r\n"
        REQUIRE(ranges[3] == TL::CodepointRange{ 5, 6 }); // 'b'
    }

    // Regional indicator flags (emoji flag pairing: \U0001F1FA\U0001F1F8 U+1F1FA U+1F1F8)
    {
        const std::u32string flagUS{ 0x1F1FA, 0x1F1F8 };
        const TL::GraphemeMap map(flagUS);
        std::vector<TL::CodepointRange> ranges;
        map.iterate([&](TL::CodepointRange range) {
            ranges.push_back(range);
        });
        REQUIRE(ranges.size() == 1);
        REQUIRE(ranges[0] == TL::CodepointRange{ 0, 2 });
    }
}

TEST_CASE("segmentParagraphs produces one paragraph for empty input", "[layout2][paragraph]") {
    std::vector<std::pair<TL::CodepointRange, TL::ParagraphSeparatorKind>> results;
    const TL::DocumentSource document(U"");
    TL::segmentParagraphs(document, TL::entireText,
                          [&](TL::CodepointRange range, TL::ParagraphSeparatorKind kind) {
                              results.emplace_back(range, kind);
                          });

    REQUIRE(results.size() == 1);
    REQUIRE(results[0].first == TL::CodepointRange{ 0, 0 });
    REQUIRE(results[0].second == TL::ParagraphSeparatorKind::None);
}

TEST_CASE("segmentParagraphs retains all separator ranges and empty paragraphs", "[layout2][paragraph]") {
    const std::u32string text{ U'a',      U'\n', U'\r',     U'\n', U'\r',     U'b', U'\u001C', U'c',
                               U'\u001D', U'd',  U'\u001E', U'e',  U'\u0085', U'f', U'\u2029' };

    std::vector<std::pair<TL::CodepointRange, TL::ParagraphSeparatorKind>> results;
    const TL::DocumentSource document(text);
    TL::segmentParagraphs(document, TL::entireText,
                          [&](TL::CodepointRange range, TL::ParagraphSeparatorKind kind) {
                              results.emplace_back(range, kind);
                          });

    REQUIRE(results.size() == 9);

    // Paragraph 0: "a\n" -> [0, 2)
    REQUIRE(results[0].first == TL::CodepointRange{ 0, 2 });
    REQUIRE(results[0].second == TL::ParagraphSeparatorKind::LineFeed);

    // Paragraph 1: "\r\n" -> [2, 4)
    REQUIRE(results[1].first == TL::CodepointRange{ 2, 4 });
    REQUIRE(results[1].second == TL::ParagraphSeparatorKind::CarriageReturnLineFeed);

    // Paragraph 2: "\r" -> [4, 5)
    REQUIRE(results[2].first == TL::CodepointRange{ 4, 5 });
    REQUIRE(results[2].second == TL::ParagraphSeparatorKind::CarriageReturn);

    // Paragraph 3: "b\x1c" -> [5, 7)
    REQUIRE(results[3].first == TL::CodepointRange{ 5, 7 });
    REQUIRE(results[3].second == TL::ParagraphSeparatorKind::InformationSeparator4);

    // Paragraph 4: "c\x1d" -> [7, 9)
    REQUIRE(results[4].first == TL::CodepointRange{ 7, 9 });
    REQUIRE(results[4].second == TL::ParagraphSeparatorKind::InformationSeparator3);

    // Paragraph 5: "d\x1e" -> [9, 11)
    REQUIRE(results[5].first == TL::CodepointRange{ 9, 11 });
    REQUIRE(results[5].second == TL::ParagraphSeparatorKind::InformationSeparator2);

    // Paragraph 6: "e\x85" -> [11, 13)
    REQUIRE(results[6].first == TL::CodepointRange{ 11, 13 });
    REQUIRE(results[6].second == TL::ParagraphSeparatorKind::NextLine);

    // Paragraph 7: "f\u2029" -> [13, 15)
    REQUIRE(results[7].first == TL::CodepointRange{ 13, 15 });
    REQUIRE(results[7].second == TL::ParagraphSeparatorKind::ParagraphSeparator);

    // Paragraph 8: trailing empty paragraph -> [15, 15)
    REQUIRE(results[8].first == TL::CodepointRange{ 15, 15 });
    REQUIRE(results[8].second == TL::ParagraphSeparatorKind::None);
}

TEST_CASE("segmentParagraphs keeps visible line separators in paragraph content", "[layout2][paragraph]") {
    const std::u32string text{ U'a', U'\v', U'b', U'\f', U'c', U'\u2028', U'd' };

    std::vector<std::pair<TL::CodepointRange, TL::ParagraphSeparatorKind>> results;
    const TL::DocumentSource document(text);
    TL::segmentParagraphs(document, TL::entireText,
                          [&](TL::CodepointRange range, TL::ParagraphSeparatorKind kind) {
                              results.emplace_back(range, kind);
                          });

    REQUIRE(results.size() == 1);
    REQUIRE(results[0].first == TL::CodepointRange{ 0, 7 });
    REQUIRE(results[0].second == TL::ParagraphSeparatorKind::None);
}

TEST_CASE("resolveBidi empty paragraph", "[layout2][bidi]") {
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> runs;
    const TL::DocumentSource emptyDocument(U"");
    const TL::Direction level = TL::resolveBidi(emptyDocument, TL::entireText, TL::BaseDirection::DefaultLTR,
                                                [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                                                    runs.emplace_back(range, runLevel);
                                                });

    REQUIRE(level == TL::Direction::LeftToRight);
    REQUIRE(runs.empty());

    runs.clear();
    const TL::Direction rtlLevel =
        TL::resolveBidi(emptyDocument, TL::entireText, TL::BaseDirection::RightToLeft,
                        [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                            runs.emplace_back(range, runLevel);
                        });
    REQUIRE(rtlLevel == TL::Direction::RightToLeft);
    REQUIRE(runs.empty());
}

TEST_CASE("resolveBidi pure LTR paragraph", "[layout2][bidi]") {
    const std::u32string_view text = U"Hello, world!";
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> runs;
    const TL::DocumentSource document(text);
    const TL::Direction level = TL::resolveBidi(document, TL::entireText, TL::BaseDirection::DefaultLTR,
                                                [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                                                    runs.emplace_back(range, runLevel);
                                                });

    REQUIRE(level == TL::Direction::LeftToRight);
    REQUIRE(runs.size() == 1);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 13 });
    REQUIRE(runs[0].second == 0);
}

TEST_CASE("resolveBidi pure RTL paragraph", "[layout2][bidi]") {
    // Hebrew: \u05E9\u05DC\u05D5\u05DD \u05E2\u05D5\u05DC\u05DD
    const std::u32string_view text = U"\u05E9\u05DC\u05D5\u05DD \u05E2\u05D5\u05DC\u05DD";
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> runs;
    const TL::DocumentSource document(text);
    const TL::Direction level = TL::resolveBidi(document, TL::entireText, TL::BaseDirection::DefaultLTR,
                                                [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                                                    runs.emplace_back(range, runLevel);
                                                });

    REQUIRE(level == TL::Direction::RightToLeft);
    REQUIRE(runs.size() == 1);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 9 });
    REQUIRE(runs[0].second == 1);
}

TEST_CASE("resolveBidi mixed LTR and RTL text", "[layout2][bidi]") {
    // "hello " (LTR, 6 chars) + "\u05E9\u05DC\u05D5\u05DD" (RTL, 4 chars) + " world" (LTR, 6 chars)
    const std::u32string_view text = U"hello \u05E9\u05DC\u05D5\u05DD world";
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> runs;
    const TL::DocumentSource document(text);
    const TL::Direction level = TL::resolveBidi(document, TL::entireText, TL::BaseDirection::DefaultLTR,
                                                [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                                                    runs.emplace_back(range, runLevel);
                                                });

    REQUIRE(level == TL::Direction::LeftToRight);
    REQUIRE(runs.size() == 3);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 6 });
    REQUIRE(runs[0].second == 0);
    REQUIRE(runs[1].first == TL::CodepointRange{ 6, 10 });
    REQUIRE(runs[1].second == 1);
    REQUIRE(runs[2].first == TL::CodepointRange{ 10, 16 });
    REQUIRE(runs[2].second == 0);
}

TEST_CASE("resolveBidi base direction policies", "[layout2][bidi]") {
    // Force RTL on ASCII
    const std::u32string_view ltrText = U"abc";
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> rtlForcedRuns;
    const TL::DocumentSource ltrDocument(ltrText);
    const TL::Direction rtlForcedLevel =
        TL::resolveBidi(ltrDocument, TL::entireText, TL::BaseDirection::RightToLeft,
                        [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                            rtlForcedRuns.emplace_back(range, runLevel);
                        });
    REQUIRE(rtlForcedLevel == TL::Direction::RightToLeft);
    REQUIRE(rtlForcedRuns.size() == 1);
    REQUIRE(rtlForcedRuns[0].first == TL::CodepointRange{ 0, 3 });
    REQUIRE(rtlForcedRuns[0].second == 2); // LTR text embedded in RTL paragraph gets level 2

    // Force LTR on Arabic
    const std::u32string arabic{ 0x0645, 0x0631 };
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> ltrForcedRuns;
    const TL::DocumentSource arabicDocument(arabic);
    const TL::Direction ltrForcedLevel =
        TL::resolveBidi(arabicDocument, TL::entireText, TL::BaseDirection::LeftToRight,
                        [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                            ltrForcedRuns.emplace_back(range, runLevel);
                        });
    REQUIRE(ltrForcedLevel == TL::Direction::LeftToRight);
    REQUIRE(ltrForcedRuns.size() == 1);
    REQUIRE(ltrForcedRuns[0].first == TL::CodepointRange{ 0, 2 });
    REQUIRE(ltrForcedRuns[0].second == 1); // RTL text in LTR paragraph gets level 1

    // Default direction on neutral-only text
    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> defaultLtrRuns;
    const TL::DocumentSource neutralDocument(U"!? 123");
    const TL::Direction defaultLtrLevel =
        TL::resolveBidi(neutralDocument, TL::entireText, TL::BaseDirection::DefaultLTR,
                        [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                            defaultLtrRuns.emplace_back(range, runLevel);
                        });
    REQUIRE(defaultLtrLevel == TL::Direction::LeftToRight);

    std::vector<std::pair<TL::CodepointRange, TL::BiDiLevel>> defaultRtlRuns;
    const TL::Direction defaultRtlLevel =
        TL::resolveBidi(neutralDocument, TL::entireText, TL::BaseDirection::DefaultRTL,
                        [&](TL::CodepointRange range, TL::BiDiLevel runLevel) {
                            defaultRtlRuns.emplace_back(range, runLevel);
                        });
    REQUIRE(defaultRtlLevel == TL::Direction::RightToLeft);
}

TEST_CASE("segmentParagraphs without trailing separator produces single paragraph with None kind",
          "[layout2][paragraph]") {
    const std::u32string text = U"Hello, world!";

    std::vector<std::pair<TL::CodepointRange, TL::ParagraphSeparatorKind>> results;
    const TL::DocumentSource document(text);
    TL::segmentParagraphs(document, TL::entireText,
                          [&](TL::CodepointRange range, TL::ParagraphSeparatorKind kind) {
                              results.emplace_back(range, kind);
                          });

    REQUIRE(results.size() == 1);
    REQUIRE(results[0].first == TL::CodepointRange{ 0, static_cast<TL::CodepointIndex>(text.size()) });
    REQUIRE(results[0].second == TL::ParagraphSeparatorKind::None);
}

TEST_CASE("itemizeScripts empty text", "[layout2][script]") {
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(U"");
    TL::itemizeScripts(document, TL::entireText, 0, {}, {},
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.empty());
}

TEST_CASE("itemizeScripts separates same-level Unicode scripts", "[layout2][script]") {
    const std::u32string_view text =
        U"Latin \u0395\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC \u0440\u0443\u0441\u0441\u043A\u0438\u0439 "
        U"\u0939\u093F\u0928\u094D\u0926\u0940";
    const auto scripts = TL::detectScripts(text);
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(text);
    TL::itemizeScripts(document, TL::entireText, 0, {}, scripts,
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 4);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 6 });
    REQUIRE(runs[0].second.script == scriptTag("Latn"));
    REQUIRE(runs[0].second.level == 0);

    REQUIRE(runs[1].first == TL::CodepointRange{ 6, 15 });
    REQUIRE(runs[1].second.script == scriptTag("Grek"));
    REQUIRE(runs[1].second.level == 0);

    REQUIRE(runs[2].first == TL::CodepointRange{ 15, 23 });
    REQUIRE(runs[2].second.script == scriptTag("Cyrl"));
    REQUIRE(runs[2].second.level == 0);

    REQUIRE(runs[3].first == TL::CodepointRange{ 23, 29 });
    REQUIRE(runs[3].second.script == scriptTag("Deva"));
    REQUIRE(runs[3].second.level == 0);
}

TEST_CASE("itemizeScripts resolves Common punctuation and matched pairs", "[layout2][script]") {
    const std::u32string_view text = U"abc (\u0440\u0443\u0441) xyz";
    const auto scripts             = TL::detectScripts(text);
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(text);
    TL::itemizeScripts(document, TL::entireText, 0, {}, scripts,
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 3);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 5 });
    REQUIRE(runs[0].second.script == scriptTag("Latn"));
    REQUIRE(runs[0].second.level == 0);

    REQUIRE(runs[1].first == TL::CodepointRange{ 5, 8 });
    REQUIRE(runs[1].second.script == scriptTag("Cyrl"));
    REQUIRE(runs[1].second.level == 0);

    REQUIRE(runs[2].first == TL::CodepointRange{ 8, 13 });
    REQUIRE(runs[2].second.script == scriptTag("Latn"));
    REQUIRE(runs[2].second.level == 0);
}

TEST_CASE("itemizeScripts preserves grapheme clusters while resolving inherited marks", "[layout2][script]") {
    const std::u32string text{ U'a', U'\u0301', U'\u092C', U'\u093F' };
    const auto scripts = TL::detectScripts(text);
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(text);
    TL::itemizeScripts(document, TL::entireText, 0, {}, scripts,
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 2);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 2 });
    REQUIRE(runs[0].second.script == scriptTag("Latn"));
    REQUIRE(runs[0].second.level == 0);

    REQUIRE(runs[1].first == TL::CodepointRange{ 2, 4 });
    REQUIRE(runs[1].second.script == scriptTag("Deva"));
    REQUIRE(runs[1].second.level == 0);
}

TEST_CASE("itemizeScripts honors the script override", "[layout2][script]") {
    const std::u32string_view text = U"abc \u0440\u0443\u0441\u0441\u043A\u0438\u0439";
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(text);
    TL::itemizeScripts(document, TL::entireText, 1, scriptTag("Grek"), {},
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 1);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, static_cast<TL::CodepointIndex>(text.size()) });
    REQUIRE(runs[0].second.script == scriptTag("Grek"));
    REQUIRE(runs[0].second.level == 1);
}

TEST_CASE("itemizeScripts preserves explicit script of nonspacing marks on common base",
          "[layout2][script]") {
    // U+25CC (DOTTED CIRCLE, Common) + U+05B0 (HEBREW POINT SHEVA, Hebrew Mn)
    const std::u32string text{ U'\u25CC', U'\u05B0' };
    const auto scripts = TL::detectScripts(text);
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(text);
    TL::itemizeScripts(document, TL::entireText, 1, {}, scripts,
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 1);
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 2 });
    REQUIRE(runs[0].second.script == scriptTag("Hebr"));
    REQUIRE(runs[0].second.level == 1);
}

TEST_CASE("itemizeScripts treats Unknown script as unresolved rather than strong", "[layout2][script]") {
    const std::u32string textWithPUA =
        std::u32string{ U"abc " } + char32_t(0xE000) + std::u32string{ U" (\u0440\u0443\u0441)" };
    const auto scripts = TL::detectScripts(textWithPUA);
    std::vector<std::pair<TL::CodepointRange, TL::ScriptRun>> runs;
    const TL::DocumentSource document(textWithPUA);
    TL::itemizeScripts(document, TL::entireText, 0, {}, scripts,
                       [&](TL::CodepointRange range, const TL::ScriptRun& run) {
                           runs.emplace_back(range, run);
                       });

    REQUIRE(runs.size() == 5);
    for (const auto& run : runs) {
        REQUIRE(!run.first.empty());
    }
    // [0, 4): "abc " -> Latn
    REQUIRE(runs[0].first == TL::CodepointRange{ 0, 4 });
    REQUIRE(runs[0].second.script == scriptTag("Latn"));

    // [4, 5): U+E000 -> Zzzz
    REQUIRE(runs[1].first == TL::CodepointRange{ 4, 5 });
    REQUIRE(runs[1].second.script == scriptTag("Zzzz"));

    // [5, 7): " (" -> Latn
    REQUIRE(runs[2].first == TL::CodepointRange{ 5, 7 });
    REQUIRE(runs[2].second.script == scriptTag("Latn"));

    // [7, 10): "\u0440\u0443\u0441" -> Cyrl
    REQUIRE(runs[3].first == TL::CodepointRange{ 7, 10 });
    REQUIRE(runs[3].second.script == scriptTag("Cyrl"));

    // [10, 11): ")" -> Latn (matching '(' on pairedStack resolved to Latn)
    REQUIRE(runs[4].first == TL::CodepointRange{ 10, 11 });
    REQUIRE(runs[4].second.script == scriptTag("Latn"));
}

TEST_CASE("detectScripts empty text", "[layout2][detectScripts]") {
    const auto scripts = TL::detectScripts(U"");
    REQUIRE(scripts.empty());
}

TEST_CASE("detectScripts basic script detection per grapheme", "[layout2][detectScripts]") {
    // "A \u03B1 \u0410" -> Latn, Latn (space resolved to Latn), Grek, Grek (space resolved to Grek), Cyrl
    const std::u32string_view text = U"A \u03B1 \u0410";
    const auto scripts             = TL::detectScripts(text);

    // Graphemes: 'A' (Latn), ' ' (resolved to Latn), '\u03B1' (Grek), ' ' (resolved to Grek), '\u0410' (Cyrl)
    REQUIRE(scripts.size() == 5);
    REQUIRE(scripts[0] == scriptTag("Latn"));
    REQUIRE(scripts[1] == scriptTag("Latn"));
    REQUIRE(scripts[2] == scriptTag("Grek"));
    REQUIRE(scripts[3] == scriptTag("Grek"));
    REQUIRE(scripts[4] == scriptTag("Cyrl"));
}

TEST_CASE("detectScripts resolves paired punctuation and inherited marks per grapheme",
          "[layout2][detectScripts]") {
    // "a\u0301 (\u0440\u0443\u0441)" ->
    // grapheme 0: 'a' + '\u0301' (Latn)
    // grapheme 1: ' ' (Latn)
    // grapheme 2: '(' (Latn, opening bracket associated with Latn)
    // grapheme 3: '\u0440' (Cyrl)
    // grapheme 4: '\u0443' (Cyrl)
    // grapheme 5: '\u0441' (Cyrl)
    // grapheme 6: ')' (Latn, paired with '(')
    const std::u32string text = { U'a', U'\u0301', U' ', U'(', 0x0440, 0x0443, 0x0441, U')' };
    const auto scripts        = TL::detectScripts(text);

    REQUIRE(scripts.size() == 7);
    REQUIRE(scripts[0] == scriptTag("Latn"));
    REQUIRE(scripts[1] == scriptTag("Latn"));
    REQUIRE(scripts[2] == scriptTag("Latn"));
    REQUIRE(scripts[3] == scriptTag("Cyrl"));
    REQUIRE(scripts[4] == scriptTag("Cyrl"));
    REQUIRE(scripts[5] == scriptTag("Cyrl"));
    REQUIRE(scripts[6] == scriptTag("Latn"));
}

TEST_CASE("detectScripts handles paired punctuation before first explicit script without corrupting stack",
          "[layout2][detectScripts]") {
    // "(Hi) [\u0440\u0443\u0441]"
    // Graphemes:
    // 0: '(' (open pair 0, before any explicit script; resolved to Latn when 'H' is reached)
    // 1: 'H' (Latn)
    // 2: 'i' (Latn)
    // 3: ')' (close pair 0, resolves to Latn)
    // 4: ' ' (Common -> Latn)
    // 5: '[' (open pair 4, resolves to Latn)
    // 6: '\u0440' (Cyrl)
    // 7: '\u0443' (Cyrl)
    // 8: '\u0441' (Cyrl)
    // 9: ']' (close pair 4, matching '[', must resolve to Latn, not Cyrl!)
    const std::u32string text = { U'(', U'H', U'i', U')', U' ', U'[', 0x0440, 0x0443, 0x0441, U']' };
    const auto scripts        = TL::detectScripts(text);

    REQUIRE(scripts.size() == 10);
    REQUIRE(scripts[0] == scriptTag("Latn"));
    REQUIRE(scripts[1] == scriptTag("Latn"));
    REQUIRE(scripts[2] == scriptTag("Latn"));
    REQUIRE(scripts[3] == scriptTag("Latn"));
    REQUIRE(scripts[4] == scriptTag("Latn"));
    REQUIRE(scripts[5] == scriptTag("Latn"));
    REQUIRE(scripts[6] == scriptTag("Cyrl"));
    REQUIRE(scripts[7] == scriptTag("Cyrl"));
    REQUIRE(scripts[8] == scriptTag("Cyrl"));
    REQUIRE(scripts[9] == scriptTag("Latn"));
}

TEST_CASE("resolveFonts empty script run", "[layout2][font_resolve]") {
    std::vector<std::pair<TL::CodepointRange, TL::ResolvedFont>> shapeableRuns;
    std::vector<std::tuple<TL::CodepointRange, char32_t, TL::ResolvedFont>> controlRuns;

    const TL::FontDef fontDef = makeFont();
    const std::array fontDefs{ &fontDef };
    const std::array<TL::CodepointIndex, 2> boundaries{ 0, 0 };
    const TL::BoundaryTable fontBoundaries{ boundaries };
    const TL::DocumentSource document(U"");

    TL::resolveFonts(
        document, TL::entireText, TL::ScriptRun{ 0, scriptTag("Latn") }, nullptr, fontDefs, fontBoundaries,
        [&](TL::CodepointRange range, const TL::ResolvedFont& run) {
            shapeableRuns.emplace_back(range, run);
        },
        [&](TL::CodepointRange range, char32_t codepoint, const TL::ResolvedFont& run) {
            controlRuns.emplace_back(range, codepoint, run);
        });

    REQUIRE(shapeableRuns.empty());
    REQUIRE(controlRuns.empty());
}

TEST_CASE("resolveFonts with default font database", "[layout2][font_resolve]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    const TL::FontDef fontDef{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };

    const std::u32string_view text = U"Hello\tWorld";
    const std::array fontDefs{ &fontDef };
    const std::array<TL::CodepointIndex, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::BoundaryTable fontBoundaries{ boundaries };

    std::vector<std::pair<TL::CodepointRange, TL::ResolvedFont>> shapeableRuns;
    std::vector<std::tuple<TL::CodepointRange, char32_t, TL::ResolvedFont>> controlRuns;

    const TL::DocumentSource document(text);
    TL::resolveFonts(
        document, TL::entireText, TL::ScriptRun{ 0, scriptTag("Latn") }, raw(database), fontDefs,
        fontBoundaries,
        [&](TL::CodepointRange range, const TL::ResolvedFont& run) {
            shapeableRuns.emplace_back(range, run);
        },
        [&](TL::CodepointRange range, char32_t codepoint, const TL::ResolvedFont& run) {
            controlRuns.emplace_back(range, codepoint, run);
        });

    REQUIRE(shapeableRuns.size() == 2);
    REQUIRE(controlRuns.size() == 1);

    // "Hello"
    REQUIRE(shapeableRuns[0].first == TL::CodepointRange{ 0, 5 });
    REQUIRE(shapeableRuns[0].second.fontHandle);
    REQUIRE(shapeableRuns[0].second.fontRunIndex == 0);

    // "\t"
    REQUIRE(std::get<0>(controlRuns[0]) == TL::CodepointRange{ 5, 6 });
    REQUIRE(std::get<1>(controlRuns[0]) == U'\t');
    REQUIRE(std::get<2>(controlRuns[0]).fontHandle);
    REQUIRE(std::get<2>(controlRuns[0]).fontRunIndex == 0);

    // "World"
    REQUIRE(shapeableRuns[1].first == TL::CodepointRange{ 6, 11 });
    REQUIRE(shapeableRuns[1].second.fontHandle);
    REQUIRE(shapeableRuns[1].second.fontRunIndex == 0);
}

TEST_CASE("resolveFonts falls back across family names", "[layout2][font_resolve]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 2> families{ "NoSuchFontHere", "Lato" };
    const TL::FontDef fontDef{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };

    const std::u32string_view text = U"Test";
    const std::array fontDefs{ &fontDef };
    const std::array<TL::CodepointIndex, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::BoundaryTable fontBoundaries{ boundaries };

    std::vector<std::pair<TL::CodepointRange, TL::ResolvedFont>> shapeableRuns;
    std::vector<std::tuple<TL::CodepointRange, char32_t, TL::ResolvedFont>> controlRuns;

    const TL::DocumentSource document(text);
    TL::resolveFonts(
        document, TL::entireText, TL::ScriptRun{ 0, scriptTag("Latn") }, raw(database), fontDefs,
        fontBoundaries,
        [&](TL::CodepointRange range, const TL::ResolvedFont& run) {
            shapeableRuns.emplace_back(range, run);
        },
        [&](TL::CodepointRange range, char32_t codepoint, const TL::ResolvedFont& run) {
            controlRuns.emplace_back(range, codepoint, run);
        });

    REQUIRE(shapeableRuns.size() == 1);
    REQUIRE(shapeableRuns[0].first == TL::CodepointRange{ 0, 4 });
    REQUIRE(shapeableRuns[0].second.fontHandle);
    REQUIRE(controlRuns.empty());
}

TEST_CASE("resolveFonts splits at font boundary", "[layout2][font_resolve]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families1{ "Lato" };
    static constexpr std::array<std::string_view, 1> families2{ "Lato" };
    const TL::FontDef fontDef1{
        .familyNames = families1,
        .fontSize    = TL::k12,
        .style       = TL::FontStyle::Normal,
        .weight      = TL::FontWeight::Regular,
    };
    const TL::FontDef fontDef2{
        .familyNames = families2,
        .fontSize    = TL::k24,
        .style       = TL::FontStyle::Normal,
        .weight      = TL::FontWeight::Bold,
    };

    const std::u32string_view text = U"HelloWorld";
    const std::array fontDefs{ &fontDef1, &fontDef2 };
    const std::array<TL::CodepointIndex, 3> boundaries{ 0, 5, 10 };
    const TL::BoundaryTable fontBoundaries{ boundaries };

    std::vector<std::pair<TL::CodepointRange, TL::ResolvedFont>> shapeableRuns;
    std::vector<std::tuple<TL::CodepointRange, char32_t, TL::ResolvedFont>> controlRuns;

    const TL::DocumentSource document(text);
    TL::resolveFonts(
        document, TL::CodepointRange{ 0, 10 }, TL::ScriptRun{ 0, scriptTag("Latn") }, raw(database), fontDefs,
        fontBoundaries,
        [&](TL::CodepointRange range, const TL::ResolvedFont& run) {
            shapeableRuns.emplace_back(range, run);
        },
        [&](TL::CodepointRange range, char32_t codepoint, const TL::ResolvedFont& run) {
            controlRuns.emplace_back(range, codepoint, run);
        });

    REQUIRE(shapeableRuns.size() == 2);
    REQUIRE(shapeableRuns[0].first == TL::CodepointRange{ 0, 5 });
    REQUIRE(shapeableRuns[0].second.fontRunIndex == 0);
    REQUIRE(shapeableRuns[1].first == TL::CodepointRange{ 5, 10 });
    REQUIRE(shapeableRuns[1].second.fontRunIndex == 1);
    REQUIRE(controlRuns.empty());

    // Also verify when resolving a subrange with document-relative coordinates
    std::vector<std::pair<TL::CodepointRange, TL::ResolvedFont>> subrangeRuns;
    TL::resolveFonts(
        document, TL::CodepointRange{ 5, 10 }, TL::ScriptRun{ 0, scriptTag("Latn") }, raw(database), fontDefs,
        fontBoundaries,
        [&](TL::CodepointRange range, const TL::ResolvedFont& run) {
            subrangeRuns.emplace_back(range, run);
        },
        [&](TL::CodepointRange, char32_t, const TL::ResolvedFont&) {});

    REQUIRE(subrangeRuns.size() == 1);
    REQUIRE(subrangeRuns[0].first == TL::CodepointRange{ 5, 10 });
    REQUIRE(subrangeRuns[0].second.fontRunIndex == 1);
    REQUIRE(shapeableRuns[1].second.fontRunIndex == 1);
    REQUIRE(controlRuns.empty());
}

TEST_CASE("getDefaultFontDatabase caches open fonts", "[layout2][fontdb]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    TL::FontHandle h1 =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
    TL::FontHandle h2 =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
    TL::FontHandle h3 =
        database->resolveFont("lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);

    REQUIRE(h1);
    REQUIRE(h1 == h2);
    REQUIRE(h1 == h3);

    TL::FontHandle hBold =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Bold, TL::k16);
    REQUIRE(hBold);
}

TEST_CASE("repeated font resolution does not reopen the face", "[layout2][fontdb][cache]") {
    auto database = TL::createFontDatabase();
    database->scanDirectory(TL::detail::findFontsDirectory());
    REQUIRE(database != nullptr);

    const TL::FontCacheStats before = database->cacheStats();
    const TL::FontHandle first =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
    REQUIRE(first);
    const TL::FontCacheStats afterFirst = database->cacheStats();
    REQUIRE(afterFirst.ftNewFaceCalls == before.ftNewFaceCalls + 1);

    for (int i = 0; i < 100; ++i) {
        REQUIRE(database->resolveFont("lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16) ==
                first);
    }
    const TL::FontCacheStats afterRepeated = database->cacheStats();
    REQUIRE(afterRepeated.ftNewFaceCalls == afterFirst.ftNewFaceCalls);
    REQUIRE(afterRepeated.ftDoneFaceCalls == afterFirst.ftDoneFaceCalls);
}

TEST_CASE("addAlias resolves aliases to existing families", "[layout2][fontdb][alias]") {
    auto database = TL::createFontDatabase();
    database->scanDirectory(TL::detail::findFontsDirectory());
    REQUIRE(database != nullptr);

    const TL::FontHandle direct =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
    REQUIRE(direct);

    // Unknown family fails before the alias exists.
    REQUIRE_FALSE(
        database->resolveFont("LatoAlias", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16));

    // Alias to a missing family is rejected.
    REQUIRE_FALSE(database->addAlias("NoSuchFamily", "LatoAlias"));

    // Alias registration succeeds and resolves to the same handle, case-insensitively.
    REQUIRE(database->addAlias("Lato", "LatoAlias"));
    REQUIRE(database->resolveFont("LatoAlias", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16) ==
            direct);
    REQUIRE(database->resolveFont("latoalias", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16) ==
            direct);

    // Rebinding an alias to another existing family works.
    REQUIRE(database->addAlias("lato", "LatoAlias"));
    REQUIRE(database->resolveFont("LatoAlias", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16) ==
            direct);
}

TEST_CASE("font instances cache canonical fractional sizes and remain stable", "[layout2][fontdb][cache]") {
    auto database = TL::createFontDatabase();
    database->scanDirectory(TL::detail::findFontsDirectory());
    REQUIRE(database != nullptr);

    const TL::FontHandle a =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::fromFloat(13.25f));
    const TL::FontCacheStats afterA = database->cacheStats();
    const TL::FontHandle same =
        database->resolveFont("lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::fromFloat(13.25f));
    const TL::FontHandle b =
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::fromFloat(19.75f));
    REQUIRE(a);
    REQUIRE(a == same);
    REQUIRE(a != b);
    REQUIRE(afterA.sizeInitializations == 1);
    REQUIRE(database->cacheStats().sizeInitializations == 2);

    const TL::VerticalMetrics aBefore  = TL::getVerticalMetrics(database.get(), a);
    const TL::VerticalMetrics bMetrics = TL::getVerticalMetrics(database.get(), b);
    const TL::VerticalMetrics aAfter   = TL::getVerticalMetrics(database.get(), a);
    REQUIRE(aBefore.ascent == aAfter.ascent);
    REQUIRE(aBefore.descent == aAfter.descent);
    REQUIRE(bMetrics.ascent >= aBefore.ascent);

    std::vector<TL::FontHandle> growth;
    for (int i = 8; i < 80; ++i) {
        growth.push_back(database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular,
                                               TL::fromFloat(i + 0.5f)));
    }
    REQUIRE(database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular,
                                  TL::fromFloat(13.25f)) == a);
    REQUIRE(TL::getVerticalMetrics(database.get(), a).ascent == aBefore.ascent);
}

TEST_CASE("invalid or unsupported variations are rejected", "[layout2][fontdb][variation]") {
    const auto database = TL::getDefaultFontDatabase();
    const std::array variations{ TL::FontVariation{ scriptTag("wght"), TL::fromFloat(500.0f) } };
    REQUIRE_FALSE(
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16, variations));
}

TEST_CASE("recent font ring reuses dropped instances and releases older ones", "[layout2][fontdb][cache]") {
    auto database = TL::createFontDatabase();
    database->scanDirectory(TL::detail::findFontsDirectory());

    {
        const TL::FontHandle first = database->resolveFont("Lato", TL::FontStyle::Normal,
                                                           TL::FontWeight::Regular, TL::fromFloat(11.25f));
        REQUIRE(first);
    }
    const size_t initialized = database->cacheStats().sizeInitializations;
    REQUIRE(
        database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::fromFloat(11.25f)));
    REQUIRE(database->cacheStats().sizeInitializations == initialized);

    for (int i = 20; i < 40; ++i) {
        REQUIRE(database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular,
                                      TL::fromFloat(static_cast<float>(i))));
    }
    REQUIRE(database->cacheStats().sizeCount <= 8);
}

TEST_CASE("prepared paragraph retains its font database and instances", "[layout2][fontdb][lifetime]") {
    auto database = TL::createFontDatabase();
    database->scanDirectory(TL::detail::findFontsDirectory());
    std::weak_ptr<TL::FontDatabase> weakDatabase = database;
    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font               = makeFont();
    font.familyNames               = families;
    const std::u32string_view text = U"persistent paragraph";
    const std::array fontDefs{ &font };
    // prepareDocument requires one boundary per fixture font. For an empty source, use a
    // non-empty boundary extent; no grapheme is resolved, and the synthetic buffers below are
    // authoritative for the test.
    const std::array<uint32_t, 2> boundaries{ 0, std::max<uint32_t>(1, text.size()) };

    auto paragraph =
        TL::prepareDocument(TL::DocumentSource(text), database, fontDefs, TL::BoundaryTable{ boundaries });
    REQUIRE_FALSE(paragraph.glyphRuns.empty());
    const TL::FontHandle retained = paragraph.glyphRuns.front().fontHandle;
    database.reset();
    REQUIRE_FALSE(weakDatabase.expired());
    REQUIRE(retained);
    REQUIRE(paragraph.fontDatabase->activate(retained).ftFace != nullptr);

    paragraph = {};
    REQUIRE(weakDatabase.expired());
}

TEST_CASE("shapeRun returns nothing for empty input", "[layout2][shape]") {
    const TL::FontDef fontDef = makeFont();
    const TL::ResolvedFont run{
        .fontHandle   = {},
        .fontRunIndex = 0,
    };

    bool glyphCalled = false;

    const TL::DocumentSource emptyDocument(U"");
    TL::shapeRun(emptyDocument, TL::CodepointRange{ 0, 0 }, TL::CodepointRange{ 0, 0 }, nullptr, run, 0,
                 scriptTag("Latn"), fontDef, {}, [&](const TL::Glyph&, bool) {
                     glyphCalled = true;
                 });

    REQUIRE_FALSE(glyphCalled);
}

TEST_CASE("shapeRun unrenderable run emits missing glyphs", "[layout2][shape]") {
    const TL::FontDef fontDef = makeFont();
    const TL::ResolvedFont run{
        .fontHandle   = {},
        .fontRunIndex = 0,
    };

    std::vector<TL::Glyph> glyphs;

    const std::u32string_view text = U"abc";
    const TL::DocumentSource document(text);
    TL::shapeRun(document, TL::CodepointRange{ 0, 3 }, TL::CodepointRange{ 0, 3 }, nullptr, run, 0,
                 scriptTag("Latn"), fontDef, {}, [&](const TL::Glyph& g, bool) {
                     glyphs.push_back(g);
                 });

    REQUIRE(glyphs.size() == 3);

    for (size_t i = 0; i < glyphs.size(); ++i) {
        REQUIRE(glyphs[i].glyphId == 0);
        REQUIRE(glyphs[i].graphemeIndex == i);
        REQUIRE(glyphs[i].xAdvance == TL::kZero);
    }
}

TEST_CASE("shapeRun emits a Lato notdef glyph for missing CJK text", "[layout2][shape][unicode]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    const TL::FontDef fontDef{
        .familyNames = families,
        .fontSize    = TL::k16,
        .style       = TL::FontStyle::Normal,
        .weight      = TL::FontWeight::Regular,
    };
    const TL::FontHandle handle = database->resolveFont(fontDef);
    REQUIRE(handle);

    const TL::ResolvedFont run{
        .fontHandle   = handle,
        .fontRunIndex = 0,
    };
    std::vector<TL::Glyph> glyphs;
    const std::u32string text{ 0x4E00 };
    const TL::DocumentSource document(text);
    // U+4E00 is absent from Lato and should use its .notdef glyph.
    TL::shapeRun(document, TL::CodepointRange{ 0, 1 }, TL::CodepointRange{ 0, 1 }, raw(database), run, 0,
                 scriptTag("Hani"), fontDef, {}, [&](const TL::Glyph& glyph, bool) {
                     glyphs.push_back(glyph);
                 });

    REQUIRE(glyphs.size() == 1);
    REQUIRE(glyphs[0].glyphId == 0);
    REQUIRE(glyphs[0].xAdvance > TL::kZero);
}

TEST_CASE("render missing CJK glyph from Lato to a grayscale PNG", "[layout2][render][unicode]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    font.fontSize    = TL::fromFloat(28.0f);
    const std::array fonts{ &font };
    const std::u32string_view text = U"Missing CJK: \u4E00";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };

    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(320.0f));

    const auto missing              = std::ranges::find_if(prepared.glyphs, [](const TL::Glyph& glyph) {
        return glyph.glyphId == 0;
    });
    REQUIRE(missing != prepared.glyphs.end());
    REQUIRE(missing->xAdvance > TL::kZero);
}

TEST_CASE("fourCC conversion preserves high bytes", "[layout2][unicode]") {
    const std::string_view tag{ "\x80\xFF"
                                "Az",
                                4 };
    REQUIRE(TL::fourCCToUint32(tag) == 0x80FF417A);
}

TEST_CASE("shapeRun basic Latin with default font database", "[layout2][shape]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    const TL::FontDef fontDef{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Medium,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };

    TL::FontHandle fontHandle = database->resolveFont(fontDef);
    REQUIRE(fontHandle);

    const TL::ResolvedFont run{
        .fontHandle   = fontHandle,
        .fontRunIndex = 0,
    };

    std::vector<TL::Glyph> glyphs;
    const std::u32string_view text = U"Hello";
    const TL::DocumentSource document(text);

    TL::shapeRun(document, TL::CodepointRange{ 0, 5 }, TL::CodepointRange{ 0, 5 }, raw(database), run, 0,
                 scriptTag("Latn"), fontDef, "en", [&](const TL::Glyph& g, bool) {
                     glyphs.push_back(g);
                 });

    REQUIRE(glyphs.size() >= 5);
    TL::LayoutUnit totalAdvance = TL::kZero;
    for (size_t i = 0; i < glyphs.size(); ++i) {
        REQUIRE(glyphs[i].glyphId != 0);
        REQUIRE(glyphs[i].xAdvance > TL::kZero);
        REQUIRE(glyphs[i].graphemeIndex == i);
        totalAdvance += glyphs[i].xAdvance;
    }
    REQUIRE(totalAdvance > TL::kZero);
}

TEST_CASE("shapeRun combining marks map to grapheme index", "[layout2][shape]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    const TL::FontDef fontDef{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Medium,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };

    TL::FontHandle fontHandle = database->resolveFont(fontDef);
    REQUIRE(fontHandle);

    const TL::ResolvedFont run{
        .fontHandle   = fontHandle,
        .fontRunIndex = 0,
    };

    // 'e' + combining acute accent U+0301 followed by 'b'
    // Grapheme 0: [0, 2) -> 'e' + mark, Grapheme 1: [2, 3) -> 'b'
    const std::u32string text{ U'e', U'\u0301', U'b' };

    std::vector<TL::Glyph> glyphs;

    const TL::DocumentSource document(text);
    TL::shapeRun(document, TL::CodepointRange{ 0, 3 }, TL::CodepointRange{ 0, 3 }, raw(database), run, 0,
                 scriptTag("Latn"), fontDef, {}, [&](const TL::Glyph& g, bool) {
                     glyphs.push_back(g);
                 });

    REQUIRE(!glyphs.empty());

    for (const auto& glyph : glyphs) {
        REQUIRE((glyph.graphemeIndex == 0 || glyph.graphemeIndex == 1));
    }
    REQUIRE(glyphs.back().graphemeIndex == 1);
}

TEST_CASE("shapeRun produces equivalent glyphs for precomposed and decomposed Hangul",
          "[layout2][shape][hangul]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Go Noto Kurrent-Regular" };
    TL::FontDef fontDef         = makeFont();
    fontDef.familyNames         = families;
    fontDef.fontSize            = TL::k24;
    const TL::FontHandle handle = database->resolveFont(fontDef);
    REQUIRE(handle);

    const TL::ResolvedFont run{ .fontHandle = handle, .fontRunIndex = 0 };
    std::vector<TL::Glyph> precomposed;
    std::vector<TL::Glyph> decomposed;
    const std::u32string_view hangulPrecomposed = U"\uD55C";
    const std::u32string_view hangulDecomposed  = U"\u1112\u1161\u11AB";
    const TL::DocumentSource precomposedDocument(hangulPrecomposed);
    const TL::DocumentSource decomposedDocument(hangulDecomposed);
    TL::shapeRun(precomposedDocument,
                 TL::CodepointRange{ 0, static_cast<uint32_t>(hangulPrecomposed.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(hangulPrecomposed.size()) }, raw(database), run,
                 0, scriptTag("Hang"), fontDef, "ko", [&](const TL::Glyph& glyph, bool) {
                     precomposed.push_back(glyph);
                 });
    TL::shapeRun(decomposedDocument, TL::CodepointRange{ 0, static_cast<uint32_t>(hangulDecomposed.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(hangulDecomposed.size()) }, raw(database), run,
                 0, scriptTag("Hang"), fontDef, "ko", [&](const TL::Glyph& glyph, bool) {
                     decomposed.push_back(glyph);
                 });

    REQUIRE(precomposed.size() == decomposed.size());
    REQUIRE(precomposed.size() == 1);
    for (size_t i = 0; i < precomposed.size(); ++i) {
        CHECK(precomposed[i].glyphId == decomposed[i].glyphId);
        CHECK(precomposed[i].xAdvance == decomposed[i].xAdvance);
        CHECK(precomposed[i].xOffset == decomposed[i].xOffset);
        CHECK(precomposed[i].yOffset == decomposed[i].yOffset);
        CHECK(precomposed[i].graphemeIndex == 0);
    }
}

TEST_CASE("shapeRun OpenType features (ligatures)", "[layout2][shape]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> monoFamilies{ "Source Code Pro" };
    static constexpr std::array<TL::OpenTypeFeatureFlag, 1> disableLiga{ TL::OpenTypeFeatureFlag{
        .feature = TL::fourCCToUint32("calt"), .enabled = false } };
    static constexpr std::array<TL::OpenTypeFeatureFlag, 1> enableLiga{ TL::OpenTypeFeatureFlag{
        .feature = TL::fourCCToUint32("calt"), .enabled = true } };

    const TL::FontDef fontDefDisabled{
        .familyNames   = monoFamilies,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Medium,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = disableLiga,
        .variations    = {},
    };

    const TL::FontDef fontDefEnabled{
        .familyNames   = monoFamilies,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Medium,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = enableLiga,
        .variations    = {},
    };

    TL::FontHandle fontDisabled = database->resolveFont(fontDefDisabled);
    TL::FontHandle fontEnabled  = database->resolveFont(fontDefEnabled);
    REQUIRE(fontDisabled);
    REQUIRE(fontEnabled);

    const TL::ResolvedFont runDisabled{
        .fontHandle   = fontDisabled,
        .fontRunIndex = 0,
    };

    const TL::ResolvedFont runEnabled{
        .fontHandle   = fontEnabled,
        .fontRunIndex = 0,
    };

    const std::u32string_view text = U"!==";

    std::vector<TL::Glyph> glyphsDisabled;
    const TL::DocumentSource document(text);
    TL::shapeRun(document, TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) }, raw(database), runDisabled, 0,
                 scriptTag("Latn"), fontDefDisabled, {}, [&](const TL::Glyph& g, bool) {
                     glyphsDisabled.push_back(g);
                 });

    std::vector<TL::Glyph> glyphsEnabled;
    TL::shapeRun(document, TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) }, raw(database), runEnabled, 0,
                 scriptTag("Latn"), fontDefEnabled, {}, [&](const TL::Glyph& g, bool) {
                     glyphsEnabled.push_back(g);
                 });

    REQUIRE(glyphsDisabled.size() == 3);
}

TEST_CASE("getVerticalMetrics returns correct metrics using FreeType", "[layout2][metrics]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    SECTION("null font handle returns zero metrics") {
        const TL::VerticalMetrics metrics = TL::getVerticalMetrics(nullptr, {});
        REQUIRE(metrics.ascent == TL::kZero);
        REQUIRE(metrics.descent == TL::kZero);
        REQUIRE(metrics.lineGap == TL::kZero);
    }

    SECTION("valid font handle returns non-zero vertical metrics") {
        TL::FontHandle fontHandle =
            database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
        REQUIRE(fontHandle);

        const TL::VerticalMetrics metrics = TL::getVerticalMetrics(raw(database), fontHandle);
        REQUIRE(metrics.ascent > TL::kZero);
        REQUIRE(metrics.descent > TL::kZero);
    }
}

TEST_CASE("getExtendedMetrics returns additional font metrics using FreeType", "[layout2][metrics]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    SECTION("null font handle returns zero metrics") {
        const TL::ExtendedMetrics metrics = TL::getExtendedMetrics(nullptr, {});
        REQUIRE(metrics.spaceAdvanceX == TL::kZero);
        REQUIRE(metrics.lineThickness == TL::kZero);
        REQUIRE(metrics.xHeight == TL::kZero);
        REQUIRE(metrics.capitalHeight == TL::kZero);
    }

    SECTION("valid font handle returns non-zero metrics") {
        TL::FontHandle fontHandle =
            database->resolveFont("Lato", TL::FontStyle::Normal, TL::FontWeight::Regular, TL::k16);
        REQUIRE(fontHandle);

        const TL::ExtendedMetrics metrics = TL::getExtendedMetrics(raw(database), fontHandle);
        REQUIRE(metrics.spaceAdvanceX > TL::kZero);
        REQUIRE(metrics.lineThickness > TL::kZero);
        REQUIRE(metrics.xHeight > TL::kZero);
        REQUIRE(metrics.capitalHeight > TL::kZero);
    }
}

TEST_CASE("full pipeline shapes multi-paragraph bidirectional text", "[layout2][pipeline]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    const TL::FontDef fontDef = makeFont();
    static constexpr std::array<std::string_view, 1> alternateFamilies{ "Source Code Pro" };
    const TL::FontDef alternateFontDef{
        .familyNames   = alternateFamilies,
        .fontSize      = TL::k12,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };
    const std::array fonts{ &fontDef, &alternateFontDef };
    const std::array partialFonts{ &alternateFontDef, &fontDef };

    // Paragraph 1 is LTR with an embedded Hebrew run. Paragraph 2 is RTL with
    // an embedded Latin run. Paragraph 3 mixes Greek and English text. All paragraphs
    // contain paired punctuation and combining marks where applicable.
    const std::u32string text{ U'E',      U'n',      U'g',      U'l',      U'i',      U's',      U'h',
                               U' ',      U'(',      U'e',      U'\u0301', U')',      U' ',      U'\u05E9',
                               U'\u05B8', U'\u05DC', U'\u05D5', U'\u05DD', U'\n',     U'(',      U'\u05E9',
                               U'\u05B8', U'\u05DC', U'\u05D5', U'\u05DD', U')',      U' ',      U'c',
                               U'a',      U'f',      U'\u00E9', U'\n',     U'\u0393', U'\u03B5', U'\u03B9',
                               U'\u03AC', U' ',      U'(',      U'\u03BA', U'\u03CC', U'\u03C3', U'\u03BC',
                               U'\u03B5', U')',      U' ',      U'E',      U'n',      U'g',      U'l',
                               U'i',      U's',      U'h' };

    struct ShapedRecord {
        TL::BiDiLevel level;
        TL::Direction direction;
        size_t glyphCount;
        size_t graphemeCount;
        TL::LayoutUnit advance;
    };

    std::vector<std::pair<TL::CodepointRange, TL::ParagraphSeparatorKind>> paragraphs;
    std::vector<TL::Direction> paragraphDirections;
    std::vector<ShapedRecord> shapedRuns;
    std::vector<uint32_t> shapedScripts;
    std::vector<TL::FontRunIndex> shapedFontRuns;
    std::vector<TL::CodepointRange> shapedRanges;

    const TL::DocumentSource document(text);
    TL::segmentParagraphs(
        document, TL::entireText, //
        [&](TL::CodepointRange paragraphRange, TL::ParagraphSeparatorKind separator) {
            const auto paragraphIndex = paragraphs.size();
            paragraphs.emplace_back(paragraphRange, separator);

            const auto separatorLength = TL::paragraphSeparatorKindCodepoints(separator);
            const TL::CodepointRange paragraphContent{ paragraphRange.min,
                                                       paragraphRange.max - separatorLength };
            const auto paragraphGraphemeRange = document.graphemes.toGraphemeRange(paragraphContent);
            std::vector<TL::ScriptTag> paragraphScripts(paragraphGraphemeRange.distance());
            TL::detectScripts(document, paragraphContent, paragraphScripts);
            const TL::GraphemeMap paragraphGraphemes(
                text.substr(paragraphContent.min, paragraphContent.distance()));
            const TL::Direction paragraphDirection = TL::resolveBidi(
                document, paragraphContent, TL::BaseDirection::DefaultLTR, //
                [&](TL::CodepointRange bidiRange, TL::BiDiLevel bidiLevel) {
                    const auto firstGrapheme =
                        paragraphGraphemes.toGrapheme(bidiRange.min - paragraphContent.min);
                    const auto lastGrapheme =
                        paragraphGraphemes.toGrapheme(bidiRange.max - 1 - paragraphContent.min) + 1;
                    const std::span<const uint32_t> bidiScripts{ paragraphScripts.data() + firstGrapheme,
                                                                 lastGrapheme - firstGrapheme };

                    TL::itemizeScripts(
                        document, bidiRange, bidiLevel, {}, bidiScripts, //
                        [&](TL::CodepointRange scriptRange, const TL::ScriptRun& scriptRun) {
                            const bool firstParagraphCodepoint =
                                paragraphIndex == 2 && scriptRange.min == paragraphRange.min;
                            const std::array<uint32_t, 3> fontBoundaryStorage{
                                0,
                                firstParagraphCodepoint ? scriptRange.min + 1u
                                                        : static_cast<uint32_t>(text.size()),
                                static_cast<uint32_t>(text.size())
                            };
                            const std::span<const TL::FontDef* const> scriptFonts =
                                firstParagraphCodepoint
                                    ? std::span<const TL::FontDef* const>{ partialFonts }
                                    : std::span<const TL::FontDef* const>{ fonts }.first(1);
                            const TL::BoundaryTable scriptFontBoundaries{
                                std::span<const uint32_t>{ fontBoundaryStorage }.first(
                                    firstParagraphCodepoint ? 3 : 2)
                            };

                            TL::resolveFonts(
                                document, scriptRange, scriptRun, raw(database), scriptFonts,
                                scriptFontBoundaries, //
                                [&](TL::CodepointRange fontRange, const TL::ResolvedFont& resolvedRun) {
                                    size_t glyphCount = 0;
                                    TL::shapeRun(document, paragraphContent, fontRange, raw(database),
                                                 resolvedRun, bidiLevel, scriptRun.script,
                                                 *scriptFonts[resolvedRun.fontRunIndex], {}, //
                                                 [&](const TL::Glyph&, bool) {
                                                     ++glyphCount;
                                                 });

                                    shapedRuns.push_back(ShapedRecord{
                                        .level      = bidiLevel,
                                        .direction  = TL::directionFromLevel(bidiLevel),
                                        .glyphCount = glyphCount,
                                        .graphemeCount =
                                            TL::GraphemeMap(text.substr(fontRange.min, fontRange.distance()))
                                                .graphemeCount(),
                                        .advance = TL::kZero,
                                    });
                                    shapedScripts.push_back(scriptRun.script);
                                    shapedFontRuns.push_back(firstParagraphCodepoint
                                                                 ? (resolvedRun.fontRunIndex == 0 ? 1u : 0u)
                                                                 : 0u);
                                    shapedRanges.push_back(fontRange);
                                },
                                [&](TL::CodepointRange, char32_t, const TL::ResolvedFont&) {});
                        });
                });
            paragraphDirections.push_back(paragraphDirection);
        });

    REQUIRE(paragraphs.size() == 3);
    REQUIRE(paragraphs[0].second == TL::ParagraphSeparatorKind::LineFeed);
    REQUIRE(paragraphs[1].second == TL::ParagraphSeparatorKind::LineFeed);
    REQUIRE(paragraphs[2].second == TL::ParagraphSeparatorKind::None);
    REQUIRE(paragraphDirections.size() == 3);
    REQUIRE(paragraphDirections[0] == TL::Direction::LeftToRight);
    REQUIRE(paragraphDirections[1] == TL::Direction::RightToLeft);
    REQUIRE(paragraphDirections[2] == TL::Direction::LeftToRight);

    REQUIRE(shapedRuns.size() >= 5);
    bool sawLtr               = false;
    bool sawRtl               = false;
    bool sawGreek             = false;
    bool sawDistinctGreekFont = false;
    for (const ShapedRecord& run : shapedRuns) {
        REQUIRE(run.glyphCount > 0);
        REQUIRE(run.graphemeCount > 0);
        const bool validLevel =
            run.level == (run.direction == TL::Direction::RightToLeft ? 1 : 0) || run.level > 1;
        REQUIRE(validLevel);
        if (run.direction == TL::Direction::LeftToRight) {
            sawLtr = true;
        } else {
            sawRtl = true;
        }
    }
    REQUIRE(shapedScripts.size() == shapedRuns.size());
    REQUIRE(shapedFontRuns.size() == shapedRuns.size());
    REQUIRE(shapedRanges.size() == shapedRuns.size());
    bool sawDefaultFontAfterAlternate = false;
    for (size_t i = 0; i < shapedRuns.size(); ++i) {
        if (shapedFontRuns[i] == 1 && shapedRanges[i].distance() == 1) {
            sawDistinctGreekFont = true;
        }
        if (shapedScripts[i] == scriptTag("Grek")) {
            sawGreek = true;
        }
        if (shapedFontRuns[i] == 0 && shapedRanges[i].min > paragraphs[2].first.min) {
            sawDefaultFontAfterAlternate = true;
        }
    }
    REQUIRE(sawLtr);
    REQUIRE(sawRtl);
    REQUIRE(sawGreek);
    REQUIRE(sawDistinctGreekFont);
    REQUIRE(sawDefaultFontAfterAlternate);
}

TEST_CASE("GraphemeMap empty text", "[layout2][grapheme]") {
    const TL::GraphemeMap map(U"");

    REQUIRE(map.graphemeCount() == 0);
    REQUIRE(map.codepointCount() == 0);
}

TEST_CASE("GraphemeMap ascii text", "[layout2][grapheme]") {
    const TL::GraphemeMap map(U"abc");

    REQUIRE(map.graphemeCount() == 3);
    REQUIRE(map.codepointCount() == 3);

    for (uint32_t i = 0; i < 3; ++i) {
        REQUIRE(map.toCodepoint(i) == i);
        REQUIRE(map.toGrapheme(i) == i);
        REQUIRE(map.codepointRangeForGrapheme(i) == TL::CodepointRange{ i, i + 1 });
    }
}

TEST_CASE("GraphemeMap combining cluster", "[layout2][grapheme]") {
    // 'e' (U+0065) + combining acute (U+0301) + 'b' (U+0062)
    const std::u32string text = { 0x0065, 0x0301, 0x0062 };
    const TL::GraphemeMap map(text);

    REQUIRE(map.graphemeCount() == 2);
    REQUIRE(map.codepointCount() == 3);

    REQUIRE(map.toCodepoint(0) == 0);
    REQUIRE(map.toCodepoint(1) == 2);
    REQUIRE(map.codepointRangeForGrapheme(0) == TL::CodepointRange{ 0, 2 });
    REQUIRE(map.codepointRangeForGrapheme(1) == TL::CodepointRange{ 2, 3 });

    REQUIRE(map.toGrapheme(0) == 0);
    REQUIRE(map.toGrapheme(1) == 0); // combining mark belongs to first cluster
    REQUIRE(map.toGrapheme(2) == 1);
}

TEST_CASE("GraphemeMap ZWJ sequence", "[layout2][grapheme]") {
    const std::u32string text = { 0x1F468, 0x200D, 0x1F4BB };
    const TL::GraphemeMap map(text);

    REQUIRE(map.graphemeCount() == 1);
    REQUIRE(map.codepointCount() == 3);
    REQUIRE(map.toCodepoint(0) == 0);
    REQUIRE(map.toGrapheme(0) == 0);
    REQUIRE(map.toGrapheme(1) == 0);
    REQUIRE(map.toGrapheme(2) == 0);
    REQUIRE(map.codepointRangeForGrapheme(0) == TL::CodepointRange{ 0, 3 });
}

TEST_CASE("GraphemeMap regional indicator sequence", "[layout2][grapheme]") {
    const std::u32string text{ 0x1F1FA, 0x1F1F8, U'a' };
    const TL::GraphemeMap map(text);

    REQUIRE(map.graphemeCount() == 2);
    REQUIRE(map.codepointCount() == 3);
    REQUIRE(map.toCodepoint(0) == 0);
    REQUIRE(map.toCodepoint(1) == 2);
    REQUIRE(map.toCodepoint(2) == 3);
    REQUIRE(map.toGrapheme(0) == 0);
    REQUIRE(map.toGrapheme(1) == 0);
    REQUIRE(map.toGrapheme(2) == 1);
    REQUIRE(map.toGrapheme(3) == 2);
    REQUIRE(map.codepointRangeForGrapheme(0) == TL::CodepointRange{ 0, 2 });
    REQUIRE(map.codepointRangeForGrapheme(1) == TL::CodepointRange{ 2, 3 });

    REQUIRE(map.toCodepointRange(TL::GraphemeRange{ 0, 0 }) == TL::CodepointRange{ 0, 0 });
    REQUIRE(map.toCodepointRange(TL::GraphemeRange{ 0, 1 }) == TL::CodepointRange{ 0, 2 });
    REQUIRE(map.toCodepointRange(TL::GraphemeRange{ 1, 2 }) == TL::CodepointRange{ 2, 3 });
    REQUIRE(map.toCodepointRange(TL::GraphemeRange{ 0, 2 }) == TL::CodepointRange{ 0, 3 });

    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 0, 0 }) == TL::GraphemeRange{ 0, 0 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 1, 1 }) == TL::GraphemeRange{ 0, 0 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 2, 2 }) == TL::GraphemeRange{ 1, 1 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 3, 3 }) == TL::GraphemeRange{ 2, 2 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 0, 1 }) == TL::GraphemeRange{ 0, 1 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 0, 2 }) == TL::GraphemeRange{ 0, 1 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 1, 2 }) == TL::GraphemeRange{ 0, 1 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 0, 3 }) == TL::GraphemeRange{ 0, 2 });
    REQUIRE(map.toGraphemeRange(TL::CodepointRange{ 2, 3 }) == TL::GraphemeRange{ 1, 2 });
}

TEST_CASE("GraphemeMap terminating entries", "[layout2][grapheme]") {
    // codepointToGrapheme has size == codepoint count + 1, with the last entry
    // equal to the grapheme count; graphemeToCodepoint's last entry equals the
    // total codepoint count. Both hold for the text constructor, the slice
    // constructor, and the empty map.
    const std::u32string text = { 0x0065, 0x0301, 0x0062 }; // "\u00E9b": 3 cps, 2 graphemes
    const TL::GraphemeMap map(text);

    REQUIRE(map.toGrapheme(map.codepointCount()) == map.graphemeCount());
    REQUIRE(map.toCodepoint(map.graphemeCount()) == map.codepointCount());

    const TL::GraphemeMap sliced(map, TL::GraphemeRange{ 1, 2 }); // single "b" cluster
    REQUIRE(sliced.graphemeCount() == 1);
    REQUIRE(sliced.codepointCount() == 1);
    REQUIRE(sliced.toGrapheme(sliced.codepointCount()) == sliced.graphemeCount());
    REQUIRE(sliced.toCodepoint(sliced.graphemeCount()) == sliced.codepointCount());

    const TL::GraphemeMap emptySlice(map, TL::GraphemeRange{ 1, 1 });
    REQUIRE(emptySlice.graphemeCount() == 0);
    REQUIRE(emptySlice.codepointCount() == 0);
    REQUIRE(emptySlice.toGrapheme(0) == 0);
    REQUIRE(emptySlice.toCodepoint(0) == 0);

    const TL::GraphemeMap empty;
    REQUIRE(empty.toGrapheme(0) == 0);
    REQUIRE(empty.toCodepoint(0) == 0);
}

TEST_CASE("DocumentSource basic accessors", "[layout2][document_source]") {
    const std::u32string text = U"Hello, \U0001F1FA\U0001F1F8 world!";
    const TL::DocumentSource doc(text);
    REQUIRE(doc.codepointCount() == text.size());
    REQUIRE(doc.graphemeCount() == doc.graphemes.graphemeCount());
    REQUIRE(doc.substr(TL::CodepointRange{ 0, 5 }) == U"Hello");
}

TEST_CASE("BoundaryTable validates and indexes boundaries", "[layout2][boundary]") {
    const std::array<uint32_t, 4> boundaries{ 0, 2, 5, 5 };
    const TL::BoundaryTable invalid{ boundaries };
    REQUIRE_FALSE(invalid.correct());

    const std::array<uint32_t, 4> validBoundaries{ 0, 2, 5, 9 };
    const TL::BoundaryTable table{ validBoundaries };
    REQUIRE(table.correct());
    REQUIRE(table.count() == 3);
    REQUIRE(table.total() == 9);
    REQUIRE_FALSE(table.empty());
    REQUIRE(table.begin(1) == 2);
    REQUIRE(table.end(1) == 5);
    REQUIRE(table.rangeOf(1) == Brisk::Range<uint32_t>{ 2, 5 });
    REQUIRE(table.indexOf(0) == 0);
    REQUIRE(table.indexOf(1) == 0);
    REQUIRE(table.indexOf(2) == 1);
    REQUIRE(table.indexOf(8) == 2);

    const std::array<uint32_t, 1> emptyBoundaries{ 0 };
    const TL::BoundaryTable emptyText{ emptyBoundaries };
    REQUIRE(emptyText.correct());
    REQUIRE(emptyText.count() == 0);
    REQUIRE(emptyText.total() == 0);
    REQUIRE(emptyText.empty());
}

TEST_CASE("paragraphSeparatorKindCodepoints covers every separator kind", "[layout2][paragraph]") {
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::None) == 0);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::LineFeed) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::CarriageReturn) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::CarriageReturnLineFeed) == 2);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::InformationSeparator4) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::InformationSeparator3) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::InformationSeparator2) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::NextLine) == 1);
    REQUIRE(TL::paragraphSeparatorKindCodepoints(TL::ParagraphSeparatorKind::ParagraphSeparator) == 1);
}

TEST_CASE("prepareDocument stores multiple paragraphs in document-relative flat buffers",
          "[layout2][document]") {
    const std::u32string text = U"one\ntwo\n";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };

    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });

    REQUIRE(document.paragraphs.size() == 3);
    CHECK(document.paragraphs[0].paragraphRange == TL::CodepointRange{ 0, 3 });
    CHECK(document.paragraphs[0].paragraphSeparatorKind == TL::ParagraphSeparatorKind::LineFeed);
    CHECK(document.paragraphs[1].paragraphRange == TL::CodepointRange{ 4, 7 });
    CHECK(document.paragraphs[2].paragraphRange == TL::CodepointRange{ 8, 8 });
    CHECK(document.graphemes.size() == document.graphemeMap.graphemeCount());
    CHECK(document.graphemeAdvancePrefix.size() == document.graphemes.size() + 1);
    CHECK(document.paragraphs[0].graphemeRange.max <= document.paragraphs[1].graphemeRange.min);
    CHECK(document.paragraphs[1].graphemeRange.max <= document.paragraphs[2].graphemeRange.min);
}

TEST_CASE("document layout and queries use document-relative offsets across paragraphs",
          "[layout2][document][queries]") {
    const std::u32string text = U"ab\ncd";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));

    REQUIRE(layout.lines.size() == 2);
    CHECK(layout.lines[0].graphemeRange.min == 0);
    CHECK(layout.lines[1].graphemeRange.min > layout.lines[0].graphemeRange.max);
    CHECK(layout.lines[1].baseDirection == TL::Direction::LeftToRight);

    const TL::CaretPosition first = TL::caretPosition(document, layout, { 0, TL::CaretAffinity::Downstream });
    const TL::CaretPosition second = TL::caretPosition(
        document, layout, { document.paragraphs[1].graphemeRange.min, TL::CaretAffinity::Downstream });
    CHECK(first.line == 0);
    CHECK(second.line == 1);

    const auto rects = [&] {
        std::vector<TL::SelectionRect> result;
        TL::selectionRects(document, layout, { 0, document.graphemeMap.graphemeCount() },
                           [&](const TL::SelectionRect& rect) {
                               result.push_back(rect);
                           });
        return result;
    }();
    REQUIRE(rects.size() == 2);
    CHECK(rects[0].line == 0);
    CHECK(rects[1].line == 1);
}

TEST_CASE("caret affinity selects the correct paragraph near separators",
          "[layout2][document][queries][caret][separator]") {
    const std::u32string text = U"a\nb";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));

    REQUIRE(document.paragraphs.size() == 2);
    REQUIRE(document.paragraphs[0].graphemeRange == TL::GraphemeRange{ 0, 1 });
    REQUIRE(document.paragraphs[1].graphemeRange == TL::GraphemeRange{ 2, 3 });

    // Grapheme 1 is the separator slot between the two paragraph content ranges.
    const TL::CaretPosition upstream =
        TL::caretPosition(document, layout, { 1, TL::CaretAffinity::Upstream });
    const TL::CaretPosition downstream =
        TL::caretPosition(document, layout, { 1, TL::CaretAffinity::Downstream });

    REQUIRE(upstream.line == 0);
    REQUIRE(downstream.line == 1);
    REQUIRE(upstream.x == layout.lines[0].originX + layout.lines[0].width);
    REQUIRE(downstream.x == layout.lines[1].originX);
}

TEST_CASE("document layout broadcasts paragraph settings and applies per-paragraph spans",
          "[layout2][document][alignment]") {
    const std::u32string text = U"a\nbb";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });

    const TL::LayoutUnit indent = TL::fromFloat(10.0f);
    const std::array indents{ indent };
    const TL::DocumentLayout broadcast =
        TL::layoutPreparedDocument(document, {}, TL::fromFloat(100.0f), {}, indents);
    REQUIRE(broadcast.lines.size() == 2);
    REQUIRE(broadcast.lines[0].originX == indent);
    REQUIRE(broadcast.lines[1].originX == indent);

    const std::array alignments{ TL::TextAlignment::Left, TL::TextAlignment::Right };
    const std::array noIndents{ TL::kZero, TL::kZero };
    const TL::DocumentLayout perParagraph =
        TL::layoutPreparedDocument(document, alignments, TL::fromFloat(100.0f), {}, noIndents);
    REQUIRE(perParagraph.lines.size() == 2);
    REQUIRE(perParagraph.lines[0].originX == TL::kZero);
    REQUIRE(perParagraph.lines[1].originX > TL::kZero);
}

TEST_CASE("document preparation resolves mixed paragraph directions", "[layout2][document][bidi]") {
    const std::u32string text = U"abc\n\u05D0\u05D1\u05D2";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const std::array directions{ TL::BaseDirection::DefaultLTR, TL::BaseDirection::RightToLeft };
    const TL::PreparedDocument document = TL::prepareDocument(TL::DocumentSource(text), database, fonts,
                                                              TL::BoundaryTable{ boundaries }, directions);
    REQUIRE(document.paragraphs.size() == 2);
    REQUIRE(document.paragraphs[0].baseDirection == TL::Direction::LeftToRight);
    REQUIRE(document.paragraphs[1].baseDirection == TL::Direction::RightToLeft);

    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));
    REQUIRE(layout.lines.size() == 2);
    REQUIRE(layout.lines[0].baseDirection == TL::Direction::LeftToRight);
    REQUIRE(layout.lines[1].baseDirection == TL::Direction::RightToLeft);
}

TEST_CASE("document layout preserves tabs and line-height across paragraphs",
          "[layout2][document][tabs][line-height]") {
    const std::u32string text = U"a\tb\nc\td";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    TL::FontDef font = makeFont();
    font.lineHeight  = TL::fromFloat(40.0f);
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::TabStops tabs{ .firstStop = TL::fromFloat(20.0f), .interval = TL::fromFloat(20.0f) };
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f), tabs);

    REQUIRE(layout.lines.size() == 2);
    REQUIRE(layout.lines[0].width == layout.lines[1].width);
    REQUIRE(layout.lines[1].baselineY - layout.lines[0].baselineY == TL::fromFloat(40.0f));
}

TEST_CASE("document queries handle separator gaps, empty paragraphs, and cross-paragraph selections",
          "[layout2][document][queries][edge]") {
    const std::u32string text = U"a\n\nb\n";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    TL::FontDef font = makeFont();
    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));

    REQUIRE(document.paragraphs.size() == 4);
    REQUIRE(layout.lines.size() == 4);
    REQUIRE(document.paragraphs[1].paragraphRange.empty());
    REQUIRE(document.paragraphs[3].paragraphRange.empty());

    const TL::GraphemeIndex firstEmpty = document.paragraphs[1].graphemeRange.min;
    const TL::CaretPosition emptyCaret =
        TL::caretPosition(document, layout, { firstEmpty, TL::CaretAffinity::Downstream });
    REQUIRE(emptyCaret.line == 1);
    REQUIRE(emptyCaret.x == TL::kZero);

    const TL::GraphemeIndex secondParagraph = document.paragraphs[2].graphemeRange.min;
    const TL::CaretPosition beforeSecond =
        TL::caretPosition(document, layout, { secondParagraph, TL::CaretAffinity::Downstream });
    REQUIRE(beforeSecond.line == 2);

    const TL::CaretIndex hit = TL::hitTestPoint(document, layout, TL::kZero, layout.lines[2].baselineY);
    CHECK(hit.grapheme == secondParagraph);

    std::vector<TL::SelectionRect> rects;
    TL::selectionRects(document, layout, { 0, document.graphemeMap.graphemeCount() },
                       [&](const TL::SelectionRect& rect) {
                           rects.push_back(rect);
                       });
    REQUIRE(rects.size() == 2);
    REQUIRE(rects[0].line == 0);
    REQUIRE(rects[1].line == 2);
}

TEST_CASE("selectionRects spans three paragraphs and skips a fully empty paragraph",
          "[layout2][document][queries][selection][edge]") {
    const std::u32string text = U"ab\n\ncd";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));

    REQUIRE(document.paragraphs.size() == 3);
    REQUIRE(document.paragraphs[0].paragraphRange == TL::CodepointRange{ 0, 2 }); // "ab"
    REQUIRE(document.paragraphs[1].paragraphRange.empty());                       // fully empty paragraph
    REQUIRE(document.paragraphs[2].paragraphRange == TL::CodepointRange{ 4, 6 }); // "cd"
    REQUIRE(layout.lines.size() == 3);

    // Select from the second grapheme of "ab" through the first grapheme of "cd", which spans the
    // separator gap and the fully empty middle paragraph.
    const TL::GraphemeIndex selectionStart = document.paragraphs[0].graphemeRange.min + 1;
    const TL::GraphemeIndex selectionEnd   = document.paragraphs[2].graphemeRange.min + 1;
    std::vector<TL::SelectionRect> rects;
    TL::selectionRects(document, layout, { selectionStart, selectionEnd },
                       [&](const TL::SelectionRect& rect) {
                           rects.push_back(rect);
                       });

    // The empty middle paragraph contributes no rectangle of its own.
    REQUIRE(rects.size() == 2);
    REQUIRE(rects[0].line == 0);
    REQUIRE(rects[1].line == 2);
    REQUIRE(rects[0].x1 > rects[0].x0);
    REQUIRE(rects[1].x1 > rects[1].x0);
    // The first rectangle covers only the second grapheme of "ab" ('b'), not the whole word.
    REQUIRE(rects[0].x0 > layout.lines[0].originX);
}

TEST_CASE("caretPosition and hitTestPoint agree at every paragraph separator in a multi-paragraph document",
          "[layout2][document][queries][caret][edge]") {
    const std::u32string text = U"a\nbb\nc";
    const auto database       = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);
    const TL::FontDef font = makeFont();
    const std::array fonts{ &font };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument document =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(document, {}, TL::fromFloat(1000.0f));

    REQUIRE(document.paragraphs.size() == 3);
    REQUIRE(layout.lines.size() == 3);
    REQUIRE(document.paragraphs[0].graphemeRange == TL::GraphemeRange{ 0, 1 }); // "a"
    REQUIRE(document.paragraphs[1].graphemeRange == TL::GraphemeRange{ 2, 4 }); // "bb"
    REQUIRE(document.paragraphs[2].graphemeRange == TL::GraphemeRange{ 5, 6 }); // "c"

    // Grapheme 1 is the separator between paragraphs 0 and 1; grapheme 4 is the separator between
    // paragraphs 1 and 2.
    const std::array<TL::GraphemeIndex, 2> separatorGraphemes{ 1, 4 };
    for (size_t i = 0; i < separatorGraphemes.size(); ++i) {
        const TL::GraphemeIndex separator = separatorGraphemes[i];
        const TL::CaretPosition upstream =
            TL::caretPosition(document, layout, { separator, TL::CaretAffinity::Upstream });
        const TL::CaretPosition downstream =
            TL::caretPosition(document, layout, { separator, TL::CaretAffinity::Downstream });

        REQUIRE(upstream.line == static_cast<TL::LineIndex>(i));
        REQUIRE(downstream.line == static_cast<TL::LineIndex>(i + 1));

        // Hit-testing the resolved caret position on its own line must reproduce the same caret.
        const TL::CaretIndex upstreamHit =
            TL::hitTestPoint(document, layout, upstream.x, layout.lines[upstream.line].baselineY);
        const TL::CaretIndex downstreamHit =
            TL::hitTestPoint(document, layout, downstream.x, layout.lines[downstream.line].baselineY);

        const TL::CaretPosition upstreamRoundTrip   = TL::caretPosition(document, layout, upstreamHit);
        const TL::CaretPosition downstreamRoundTrip = TL::caretPosition(document, layout, downstreamHit);
        REQUIRE(upstreamRoundTrip.x == upstream.x);
        REQUIRE(upstreamRoundTrip.line == upstream.line);
        REQUIRE(downstreamRoundTrip.x == downstream.x);
        REQUIRE(downstreamRoundTrip.line == downstream.line);
    }
}

// ---------------------------------------------------------------------------
// GreedyLineBreaker -- tested with hand-crafted PreparedDocument buffers.
// ---------------------------------------------------------------------------

namespace {

constexpr auto kNoBreak = TL::LineBreakKind::NoBreak;

struct TestGrapheme {
    TL::GraphemeFlags value;
    TL::LayoutUnit advance;
};

TestGrapheme advance(TL::LayoutUnit width, bool trimmableWhitespace = false) {
    return TestGrapheme{
        .value   = TL::GraphemeFlags{ .trimmableWhitespace = trimmableWhitespace },
        .advance = width,
    };
}

TestGrapheme tab() {
    return TestGrapheme{
        .value =
            TL::GraphemeFlags{
                .trimmableWhitespace = true,
                .isTab               = true,
            },
        .advance = TL::kZero,
    };
}

TL::PreparedDocument preparedDocument(std::u32string_view text, std::vector<TL::LineBreakKind> opportunities,
                                      std::vector<TestGrapheme> graphemes,
                                      std::vector<TL::GlyphRun> runs = {}) {
    const TL::FontDef fixtureFont = makeFont();
    const std::array fixtureFonts{ &fixtureFont };
    const std::array<uint32_t, 2> boundaries{ 0, std::max<uint32_t>(1, text.size()) };
    TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), TL::getDefaultFontDatabase(), fixtureFonts,
                            TL::BoundaryTable{ boundaries });
    prepared.paragraphs[0].paragraphRange = { 0, static_cast<TL::CodepointIndex>(text.size()) };
    prepared.paragraphs[0].graphemeRange  = { 0, prepared.graphemeMap.graphemeCount() };
    prepared.paragraphs[0].glyphRange     = { 0, 0 };
    prepared.paragraphs[0].glyphRunRange  = { 0, 0 };
    prepared.breakOpportunities           = std::move(opportunities);
    prepared.graphemes.clear();
    prepared.graphemes.reserve(graphemes.size());
    prepared.graphemeAdvancePrefix.clear();
    prepared.graphemeAdvancePrefix.reserve(graphemes.size() + 1);
    prepared.graphemeAdvancePrefix.push_back(TL::kZero);
    for (const TestGrapheme& grapheme : graphemes) {
        prepared.graphemes.push_back(grapheme.value);
        prepared.graphemeAdvancePrefix.push_back(prepared.graphemeAdvancePrefix.back() + grapheme.advance);
    }
    if (runs.empty() && !text.empty()) {
        runs.push_back(TL::GlyphRun{
            .codepointRange = { 0, static_cast<TL::CodepointIndex>(text.size()) },
            .graphemeRange  = { 0, prepared.graphemeMap.graphemeCount() },
            .glyphRange     = { 0, 0 },
        });
    }
    prepared.glyphRuns                   = std::move(runs);
    prepared.paragraphs[0].glyphRunRange = { 0, static_cast<TL::RunIndex>(prepared.glyphRuns.size()) };
    return prepared;
}

TL::LayoutUnit graphemeAdvance(const TL::PreparedDocument& prepared, TL::GraphemeIndex g) {
    return prepared.graphemeAdvancePrefix[g + 1] - prepared.graphemeAdvancePrefix[g];
}

std::vector<TL::GraphemeIndex> visualGraphemes(const TL::PreparedDocument& prepared,
                                               const TL::DocumentLayout& output, const TL::LayoutLine& line) {
    std::vector<TL::GraphemeIndex> result;
    for (TL::RunIndex layoutRunIndex = line.glyphRunRange.min; layoutRunIndex < line.glyphRunRange.max;
         ++layoutRunIndex) {
        const TL::LayoutGlyphRun& layoutRun = output.glyphRuns[layoutRunIndex];
        const auto preparedRun = std::ranges::find_if(prepared.glyphRuns, [&](const TL::GlyphRun& run) {
            return layoutRun.glyphRange.min >= run.glyphRange.min &&
                   layoutRun.glyphRange.max <= run.glyphRange.max;
        });
        REQUIRE(preparedRun != prepared.glyphRuns.end());
        for (TL::GlyphIndex glyphIndex = layoutRun.glyphRange.min; glyphIndex < layoutRun.glyphRange.max;
             ++glyphIndex) {
            result.push_back(preparedRun->graphemeRange.min + prepared.glyphs[glyphIndex].graphemeIndex);
        }
    }
    return result;
}

std::vector<TL::SelectionRect> selectionRects(const TL::PreparedDocument& prepared,
                                              const TL::DocumentLayout& layout, TL::GraphemeRange selection) {
    std::vector<TL::SelectionRect> result;
    TL::selectionRects(prepared, layout, selection, [&](const TL::SelectionRect& rect) {
        result.push_back(rect);
    });
    return result;
}

} // namespace

TEST_CASE("caretPosition resolves LTR boundaries and paragraph extremes", "[layout2][queries][caret]") {
    const auto prepared = preparedDocument(
        U"abc", std::vector<TL::LineBreakKind>(3, kNoBreak),
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(20.0f)), advance(TL::fromFloat(30.0f)) });
    REQUIRE(prepared.graphemes.size() == 3);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const TL::CaretPosition start   = TL::caretPosition(prepared, layout, { 0, TL::CaretAffinity::Upstream });
    const TL::CaretPosition middle =
        TL::caretPosition(prepared, layout, { 2, TL::CaretAffinity::Downstream });
    const TL::CaretPosition end = TL::caretPosition(prepared, layout, { 3, TL::CaretAffinity::Downstream });

    REQUIRE(start.x == TL::kZero);
    REQUIRE(start.line == 0);
    REQUIRE(start.level == 0);
    REQUIRE(middle.x == TL::fromFloat(30.0f));
    REQUIRE(middle.line == 0);
    REQUIRE(middle.level == 0);
    REQUIRE(end.x == TL::fromFloat(60.0f));
    REQUIRE(end.line == 0);
    REQUIRE(end.level == 0);
}

TEST_CASE("caretPosition uses affinity at a BiDi run boundary", "[layout2][queries][caret][bidi]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 2 }, .graphemeRange = { 0, 2 }, .level = 0 },
        TL::GlyphRun{ .codepointRange = { 2, 4 }, .graphemeRange = { 2, 4 }, .level = 1 },
    };
    const auto prepared = preparedDocument(U"abcd", std::vector<TL::LineBreakKind>(4, kNoBreak),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const TL::CaretPosition upstream =
        TL::caretPosition(prepared, layout, { 2, TL::CaretAffinity::Upstream });
    const TL::CaretPosition downstream =
        TL::caretPosition(prepared, layout, { 2, TL::CaretAffinity::Downstream });
    const TL::CaretPosition insideRtl =
        TL::caretPosition(prepared, layout, { 3, TL::CaretAffinity::Downstream });

    REQUIRE(upstream.x == TL::fromFloat(20.0f));
    REQUIRE(upstream.level == 0);
    REQUIRE(downstream.x == TL::fromFloat(40.0f));
    REQUIRE(downstream.level == 1);
    REQUIRE(insideRtl.x == TL::fromFloat(30.0f));
    REQUIRE(insideRtl.level == 1);
}

TEST_CASE("caretPosition uses affinity at a soft wrap", "[layout2][queries][caret][wrap]") {
    std::vector<TL::LineBreakKind> opportunities(5, kNoBreak);
    opportunities[2]    = TL::LineBreakKind::AllowBreak;
    const auto prepared = preparedDocument(U"ab cd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(5.0f), true),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(25.0f));

    const TL::CaretPosition upstream =
        TL::caretPosition(prepared, layout, { 3, TL::CaretAffinity::Upstream });
    const TL::CaretPosition downstream =
        TL::caretPosition(prepared, layout, { 3, TL::CaretAffinity::Downstream });

    REQUIRE(upstream.line == 0);
    REQUIRE(upstream.x == TL::fromFloat(25.0f));
    REQUIRE(downstream.line == 1);
    REQUIRE(downstream.x == TL::kZero);
}

TEST_CASE("caretPosition uses resolved tab width and handles an empty paragraph",
          "[layout2][queries][caret][tabs]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 1 }, .graphemeRange = { 0, 1 } },
        TL::GlyphRun{ .codepointRange = { 1, 2 }, .graphemeRange = { 1, 2 } },
        TL::GlyphRun{ .codepointRange = { 2, 3 }, .graphemeRange = { 2, 3 } },
    };
    const auto prepared =
        preparedDocument(U"a\tb", std::vector<TL::LineBreakKind>(3, kNoBreak),
                         { advance(TL::fromFloat(6.0f)), tab(), advance(TL::fromFloat(4.0f)) }, runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(
        prepared, {}, TL::fromFloat(100.0f),
        TL::TabStops{ .firstStop = TL::fromFloat(10.0f), .interval = TL::fromFloat(10.0f) });

    REQUIRE(TL::caretPosition(prepared, layout, { 1, TL::CaretAffinity::Downstream }).x ==
            TL::fromFloat(6.0f));
    REQUIRE(TL::caretPosition(prepared, layout, { 2, TL::CaretAffinity::Upstream }).x ==
            TL::fromFloat(10.0f));

    const auto empty                     = preparedDocument(U"", {}, {});
    const TL::DocumentLayout emptyLayout = TL::layoutPreparedDocument(empty, {}, TL::fromFloat(100.0f));
    const TL::CaretPosition emptyCaret =
        TL::caretPosition(empty, emptyLayout, { 0, TL::CaretAffinity::Downstream });
    REQUIRE(emptyCaret.x == TL::kZero);
    REQUIRE(emptyCaret.line == 0);
    REQUIRE(emptyCaret.level == 0);
}

TEST_CASE("hitTestPoint rounds to nearest LTR boundary and clamps horizontally",
          "[layout2][queries][hitTest]") {
    const auto prepared = preparedDocument(
        U"abc", std::vector<TL::LineBreakKind>(3, kNoBreak),
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(20.0f)), advance(TL::fromFloat(30.0f)) });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const TL::CaretIndex before     = TL::hitTestPoint(prepared, layout, TL::fromFloat(-10.0f), TL::kZero);
    const TL::CaretIndex leftOfMidpoint = TL::hitTestPoint(prepared, layout, TL::fromFloat(19.0f), TL::kZero);
    const TL::CaretIndex rightOfMidpoint =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(21.0f), TL::kZero);
    const TL::CaretIndex after = TL::hitTestPoint(prepared, layout, TL::fromFloat(100.0f), TL::kZero);

    REQUIRE(before.grapheme == 0);
    REQUIRE(before.affinity == TL::CaretAffinity::Downstream);
    REQUIRE(leftOfMidpoint.grapheme == 1);
    REQUIRE(rightOfMidpoint.grapheme == 2);
    REQUIRE(after.grapheme == 3);
    REQUIRE(after.affinity == TL::CaretAffinity::Upstream);
}

TEST_CASE("hitTestPoint returns the correct side of a BiDi boundary", "[layout2][queries][hitTest][bidi]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 2 }, .graphemeRange = { 0, 2 }, .level = 0 },
        TL::GlyphRun{ .codepointRange = { 2, 4 }, .graphemeRange = { 2, 4 }, .level = 1 },
    };
    const auto prepared = preparedDocument(U"abcd", std::vector<TL::LineBreakKind>(4, kNoBreak),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const TL::CaretIndex upstream   = TL::hitTestPoint(prepared, layout, TL::fromFloat(19.0f), TL::kZero);
    const TL::CaretIndex downstream = TL::hitTestPoint(prepared, layout, TL::fromFloat(39.0f), TL::kZero);

    REQUIRE(upstream.grapheme == 2);
    REQUIRE(upstream.affinity == TL::CaretAffinity::Upstream);
    REQUIRE(downstream.grapheme == 2);
    REQUIRE(downstream.affinity == TL::CaretAffinity::Downstream);
    REQUIRE(TL::caretPosition(prepared, layout, upstream).x == TL::fromFloat(20.0f));
    REQUIRE(TL::caretPosition(prepared, layout, downstream).x == TL::fromFloat(40.0f));
}

TEST_CASE("hitTestPoint chooses and clamps vertical lines including leading gaps",
          "[layout2][queries][hitTest][wrap]") {
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[1] = TL::LineBreakKind::MustBreak;
    std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 4 },
                      .graphemeRange  = { 0, 4 },
                      .metrics        = { TL::fromFloat(5.0f), TL::fromFloat(3.0f), TL::fromFloat(4.0f) } },
    };
    const auto prepared = preparedDocument(U"abcd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           std::move(runs));
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));
    REQUIRE(layout.lines.size() == 2);

    const TL::CaretIndex above        = TL::hitTestPoint(prepared, layout, TL::kZero, TL::fromFloat(-100.0f));
    const TL::CaretIndex gapNearFirst = TL::hitTestPoint(prepared, layout, TL::kZero, TL::fromFloat(9.0f));
    const TL::CaretIndex gapNearSecond = TL::hitTestPoint(prepared, layout, TL::kZero, TL::fromFloat(10.0f));
    const TL::CaretIndex below =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(100.0f), TL::fromFloat(100.0f));

    REQUIRE(above.grapheme == 0);
    REQUIRE(gapNearFirst.grapheme == 0);
    REQUIRE(gapNearSecond.grapheme == 0);
    REQUIRE(gapNearSecond.affinity == TL::CaretAffinity::Downstream);
    REQUIRE(below.grapheme == 4);
}

TEST_CASE("hitTestPoint handles tabs and an empty paragraph", "[layout2][queries][hitTest][tabs]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 1 }, .graphemeRange = { 0, 1 } },
        TL::GlyphRun{ .codepointRange = { 1, 2 }, .graphemeRange = { 1, 2 } },
        TL::GlyphRun{ .codepointRange = { 2, 3 }, .graphemeRange = { 2, 3 } },
    };
    const auto prepared =
        preparedDocument(U"a\tb", std::vector<TL::LineBreakKind>(3, kNoBreak),
                         { advance(TL::fromFloat(6.0f)), tab(), advance(TL::fromFloat(4.0f)) }, runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(
        prepared, {}, TL::fromFloat(100.0f),
        TL::TabStops{ .firstStop = TL::fromFloat(10.0f), .interval = TL::fromFloat(10.0f) });
    REQUIRE(TL::hitTestPoint(prepared, layout, TL::fromFloat(9.0f), TL::kZero).grapheme == 2);

    const auto empty                     = preparedDocument(U"", {}, {});
    const TL::DocumentLayout emptyLayout = TL::layoutPreparedDocument(empty, {}, TL::fromFloat(100.0f));
    const TL::CaretIndex emptyHit =
        TL::hitTestPoint(empty, emptyLayout, TL::fromFloat(50.0f), TL::fromFloat(50.0f));
    REQUIRE(emptyHit.grapheme == 0);
    REQUIRE(emptyHit.affinity == TL::CaretAffinity::Downstream);
}

TEST_CASE("selectionRects emits partial and multiline LTR rectangles", "[layout2][queries][selection]") {
    std::vector<TL::LineBreakKind> opportunities(5, kNoBreak);
    opportunities[2]    = TL::LineBreakKind::AllowBreak;
    const auto prepared = preparedDocument(U"ab cd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(5.0f), true),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(25.0f));

    const std::vector<TL::SelectionRect> rects = selectionRects(prepared, layout, { 1, 4 });
    REQUIRE(rects.size() == 2);
    REQUIRE(rects[0].line == 0);
    REQUIRE(rects[0].x0 == TL::fromFloat(10.0f));
    REQUIRE(rects[0].x1 == TL::fromFloat(25.0f));
    REQUIRE(rects[1].line == 1);
    REQUIRE(rects[1].x0 == TL::kZero);
    REQUIRE(rects[1].x1 == TL::fromFloat(10.0f));
}

TEST_CASE("selectionRects emits one visual-order rectangle per intersected BiDi run",
          "[layout2][queries][selection][bidi]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 2 }, .graphemeRange = { 0, 2 }, .level = 0 },
        TL::GlyphRun{ .codepointRange = { 2, 4 }, .graphemeRange = { 2, 4 }, .level = 1 },
        TL::GlyphRun{ .codepointRange = { 4, 6 }, .graphemeRange = { 4, 6 }, .level = 0 },
    };
    const auto prepared = preparedDocument(U"abcdef", std::vector<TL::LineBreakKind>(6, kNoBreak),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const std::vector<TL::SelectionRect> rects = selectionRects(prepared, layout, { 1, 5 });
    REQUIRE(rects.size() == 3);
    REQUIRE(rects[0].line == 0);
    REQUIRE(rects[0].x0 == TL::fromFloat(10.0f));
    REQUIRE(rects[0].x1 == TL::fromFloat(20.0f));
    REQUIRE(rects[1].x0 == TL::fromFloat(20.0f));
    REQUIRE(rects[1].x1 == TL::fromFloat(40.0f));
    REQUIRE(rects[2].x0 == TL::fromFloat(40.0f));
    REQUIRE(rects[2].x1 == TL::fromFloat(50.0f));
}

TEST_CASE("selectionRects follows reordered visual runs", "[layout2][queries][selection][bidi]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 1 }, .graphemeRange = { 0, 1 }, .level = 0 },
        TL::GlyphRun{ .codepointRange = { 1, 2 }, .graphemeRange = { 1, 2 }, .level = 1 },
        TL::GlyphRun{ .codepointRange = { 2, 3 }, .graphemeRange = { 2, 3 }, .level = 2 },
    };
    const auto prepared = preparedDocument(
        U"abc", std::vector<TL::LineBreakKind>(3, kNoBreak),
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(20.0f)), advance(TL::fromFloat(30.0f)) },
        runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    const std::vector<TL::SelectionRect> rects = selectionRects(prepared, layout, { 1, 3 });
    REQUIRE(rects.size() == 2);
    REQUIRE(rects[0].x0 == TL::fromFloat(10.0f));
    REQUIRE(rects[0].x1 == TL::fromFloat(40.0f));
    REQUIRE(rects[1].x0 == TL::fromFloat(40.0f));
    REQUIRE(rects[1].x1 == TL::fromFloat(60.0f));
}

TEST_CASE("selectionRects uses resolved tab geometry", "[layout2][queries][selection][tabs]") {
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 1 }, .graphemeRange = { 0, 1 } },
        TL::GlyphRun{ .codepointRange = { 1, 2 }, .graphemeRange = { 1, 2 } },
        TL::GlyphRun{ .codepointRange = { 2, 3 }, .graphemeRange = { 2, 3 } },
    };
    const auto prepared =
        preparedDocument(U"a\tb", std::vector<TL::LineBreakKind>(3, kNoBreak),
                         { advance(TL::fromFloat(6.0f)), tab(), advance(TL::fromFloat(4.0f)) }, runs);
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(
        prepared, {}, TL::fromFloat(100.0f),
        TL::TabStops{ .firstStop = TL::fromFloat(10.0f), .interval = TL::fromFloat(10.0f) });

    const std::vector<TL::SelectionRect> rects = selectionRects(prepared, layout, { 1, 2 });
    REQUIRE(rects.size() == 1);
    REQUIRE(rects[0].x0 == TL::fromFloat(6.0f));
    REQUIRE(rects[0].x1 == TL::fromFloat(10.0f));
}

TEST_CASE("selectionRects ignores empty and invalid selections", "[layout2][queries][selection][edge]") {
    const auto prepared = preparedDocument(
        U"abc", std::vector<TL::LineBreakKind>(3, kNoBreak),
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    REQUIRE(selectionRects(prepared, layout, { 1, 1 }).empty());
    REQUIRE(selectionRects(prepared, layout, { 2, 1 }).empty());
    REQUIRE(selectionRects(prepared, layout, { 0, 4 }).empty());
    REQUIRE(selectionRects(prepared, layout, { 4, 4 }).empty());

    const auto empty                     = preparedDocument(U"", {}, {});
    const TL::DocumentLayout emptyLayout = TL::layoutPreparedDocument(empty, {}, TL::fromFloat(100.0f));
    REQUIRE(selectionRects(empty, emptyLayout, { 0, 0 }).empty());
}

TEST_CASE("hitTestPoint clamps horizontally and vertically outside every line of a wrapped paragraph",
          "[layout2][queries][hitTest][edge]") {
    // Mandatory break after grapheme index 2 (before grapheme 3) splits "abcdef" into two lines of
    // three graphemes each, 10 units wide per grapheme (30 units per line).
    std::vector<TL::LineBreakKind> opportunities(6, kNoBreak);
    opportunities[2] = TL::LineBreakKind::MustBreak;
    std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 6 },
                      .graphemeRange  = { 0, 6 },
                      .metrics        = { TL::fromFloat(5.0f), TL::fromFloat(3.0f), TL::kZero } },
    };
    const auto prepared = preparedDocument(U"abcdef", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           std::move(runs));
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(1000.0f));
    REQUIRE(layout.lines.size() == 2);
    REQUIRE(layout.lines[1].baselineY > layout.lines[0].baselineY);

    // Far above and to the left of the whole document clamps to the very first boundary.
    const TL::CaretIndex topLeft =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(-1000.0f), TL::fromFloat(-1000.0f));
    REQUIRE(topLeft.grapheme == 0);
    REQUIRE(topLeft.affinity == TL::CaretAffinity::Downstream);

    // Far below and to the right of the whole document clamps to the very last boundary.
    const TL::CaretIndex bottomRight =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(1000.0f), TL::fromFloat(1000.0f));
    REQUIRE(bottomRight.grapheme == 6);
    REQUIRE(bottomRight.affinity == TL::CaretAffinity::Upstream);

    // Far to the right but vertically within the first line clamps horizontally to that line's own
    // last boundary rather than falling through to the second line.
    const TL::CaretIndex rightOfFirstLine =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(1000.0f), layout.lines[0].baselineY);
    REQUIRE(rightOfFirstLine.grapheme == 3);
    REQUIRE(rightOfFirstLine.affinity == TL::CaretAffinity::Upstream);

    // Far to the left but vertically within the second line clamps horizontally to that line's own
    // first boundary rather than snapping back to the first line.
    const TL::CaretIndex leftOfSecondLine =
        TL::hitTestPoint(prepared, layout, TL::fromFloat(-1000.0f), layout.lines[1].baselineY);
    REQUIRE(leftOfSecondLine.grapheme == 3);
    REQUIRE(leftOfSecondLine.affinity == TL::CaretAffinity::Downstream);
}

TEST_CASE("center alignment places every line with spaces at equal border distances",
          "[layout2][alignment]") {
    const auto prepared = preparedDocument(
        U"ab cdef",
        { kNoBreak, kNoBreak, TL::LineBreakKind::AllowBreak, kNoBreak, kNoBreak, kNoBreak, kNoBreak },
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(5.0f), true),
          advance(TL::fromFloat(15.0f)), advance(TL::fromFloat(15.0f)), advance(TL::fromFloat(15.0f)),
          advance(TL::fromFloat(15.0f)) });
    const TL::LayoutUnit maxLineWidth = TL::fromFloat(70.0f);
    const TL::DocumentLayout output =
        TL::layoutPreparedDocument(prepared, std::array{ TL::TextAlignment::Center }, maxLineWidth);

    REQUIRE(output.lines.size() == 2);
    for (const TL::LayoutLine& line : output.lines) {
        const TL::LayoutUnit leftBorderDistance  = line.originX;
        const TL::LayoutUnit rightBorderDistance = maxLineWidth - (line.originX + line.trimmedWidth);
        REQUIRE(leftBorderDistance == rightBorderDistance);
    }
    REQUIRE(output.lines[0].originX == TL::fromFloat(25.0f));
    REQUIRE(output.lines[1].originX == TL::fromFloat(5.0f));
}

TEST_CASE("layout applies the requested font line height between lines", "[layout2][line-height]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    font.lineHeight  = TL::fromFloat(40.0f);
    const std::array fonts{ &font };
    const std::u32string text = U"a b";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };

    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
    REQUIRE(prepared.graphemes.size() == 3);

    const TL::LayoutUnit firstLineWidth = graphemeAdvance(prepared, 0) + graphemeAdvance(prepared, 1);
    const TL::DocumentLayout output     = TL::layoutPreparedDocument(prepared, {}, firstLineWidth);

    REQUIRE(output.lines.size() == 2);
    REQUIRE(output.lines[1].baselineY - output.lines[0].baselineY == TL::fromFloat(40.0f));
}

TEST_CASE("GreedyLineBreaker single run that fits emits one TextEnd line", "[layout2][greedyLineBreaker]") {
    std::vector<TL::LineBreakKind> opportunities(3, kNoBreak);

    constexpr TL::LayoutUnit kMaxLineWidth = TL::fromFloat(100.0f);

    const auto prepared                    = preparedDocument(
        U"abc", std::move(opportunities),
        { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, kMaxLineWidth);
    const auto& lines               = output.lines;
    REQUIRE(output.lines.size() == 1);

    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[0].width == TL::fromFloat(30.0f));
    REQUIRE(lines[0].trimmedWidth == TL::fromFloat(30.0f));
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 3 });
    REQUIRE(lines[0].codepointRange == TL::CodepointRange{ 0, 3 });
}

TEST_CASE("GreedyLineBreaker soft-wraps at the last safe candidate and trims trailing whitespace",
          "[layout2][greedyLineBreaker]") {
    // "ab cd": a soft break opportunity sits right after the space.
    std::vector<TL::LineBreakKind> opportunities(5, kNoBreak);
    opportunities[2]    = TL::LineBreakKind::AllowBreak; // boundary before 'c' (grapheme index 3)

    const auto prepared = preparedDocument(U"ab cd", std::move(opportunities),
                                           {
                                               advance(TL::fromFloat(10.0f)),                    // 'a'
                                               advance(TL::fromFloat(10.0f)),                    // 'b'
                                               advance(TL::fromFloat(5.0f), /*trimmable=*/true), // ' '
                                               advance(TL::fromFloat(10.0f)),                    // 'c'
                                               advance(TL::fromFloat(10.0f)),                    // 'd'
                                           });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(25.0f));
    const auto& lines               = output.lines;

    REQUIRE(lines.size() == 2);

    REQUIRE(lines[0].endKind == TL::LineEndKind::SoftWrap);
    REQUIRE(lines[0].width == TL::fromFloat(25.0f));
    REQUIRE(lines[0].trimmedWidth == TL::fromFloat(20.0f)); // trailing space excluded
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 3 });

    REQUIRE(lines[1].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[1].width == TL::fromFloat(20.0f));
    REQUIRE(lines[1].trimmedWidth == TL::fromFloat(20.0f));
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 3, 5 });

    // Alignment is Start (LTR) -> originX is 0 for both lines.
    REQUIRE(output.horizontalTextBounds == TL::LayoutRange{ TL::kZero, TL::fromFloat(25.0f) });
    REQUIRE(output.horizontalTrimmedTextBounds == TL::LayoutRange{ TL::kZero, TL::fromFloat(20.0f) });
    REQUIRE(output.verticalTextBounds == TL::LayoutRange{ TL::kZero, TL::kZero });
}

TEST_CASE("GreedyLineBreaker expands tabs to repeating paragraph-relative stops",
          "[layout2][greedyLineBreaker][tabs]") {
    // "a\tb\t": tabs break after themselves (BA) and advance to stops 10, 20, ... .
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[1] = TL::LineBreakKind::AllowBreak;
    opportunities[3] = TL::LineBreakKind::AllowBreak;
    const auto prepared =
        preparedDocument(U"a\tb\t", std::move(opportunities),
                         { advance(TL::fromFloat(6.0f)), tab(), advance(TL::fromFloat(4.0f)), tab() });

    const TL::TabStops tabStops{
        .firstStop = TL::fromFloat(10.0f),
        .interval  = TL::fromFloat(10.0f),
    };
    const TL::DocumentLayout output =
        TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f), tabStops);

    REQUIRE(output.lines.size() == 1);
    REQUIRE(output.lines[0].width == TL::fromFloat(20.0f));
    REQUIRE(output.lines[0].trimmedWidth == TL::fromFloat(14.0f));
}

TEST_CASE("GreedyLineBreaker recalculates a carried tab after wrapping",
          "[layout2][greedyLineBreaker][tabs]") {
    // The tab overflows the first line, which is allowed to end after the preceding space.
    // On the next line it must be recomputed from the new line origin rather than retain its
    // original 5-unit advance.
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[1] = TL::LineBreakKind::AllowBreak;
    opportunities[2] = TL::LineBreakKind::NoBreak; // the tab may not become a later candidate
    const auto prepared =
        preparedDocument(U"a \tb", std::move(opportunities),
                         { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(5.0f), true), tab(),
                           advance(TL::fromFloat(5.0f)) });

    const TL::TabStops tabStops{
        .firstStop = TL::fromFloat(20.0f),
        .interval  = TL::fromFloat(20.0f),
    };
    const TL::DocumentLayout output =
        TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(19.0f), tabStops);

    REQUIRE(output.lines.size() == 2);
    REQUIRE(output.lines[0].graphemeRange == TL::GraphemeRange{ 0, 2 });
    REQUIRE(output.lines[0].width == TL::fromFloat(15.0f));
    REQUIRE(output.lines[0].trimmedWidth == TL::fromFloat(10.0f));
    REQUIRE(output.lines[1].graphemeRange == TL::GraphemeRange{ 2, 4 });
    REQUIRE(output.lines[1].width == TL::fromFloat(25.0f));
}

TEST_CASE("GreedyLineBreaker mandatory break splits the line unconditionally",
          "[layout2][greedyLineBreaker]") {
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[1]    = TL::LineBreakKind::MustBreak; // boundary before 'c' (grapheme index 2)
    const auto prepared = preparedDocument(U"abcd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(1000.0f));
    const auto& lines               = output.lines;
    REQUIRE(output.lines.size() == 2);
    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].endKind == TL::LineEndKind::MandatoryBreak);
    REQUIRE(lines[0].width == TL::fromFloat(20.0f));
    REQUIRE(lines[0].trimmedWidth == TL::fromFloat(20.0f));
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 2 });

    REQUIRE(lines[1].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[1].width == TL::fromFloat(20.0f));
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 2, 4 });
}

TEST_CASE("GreedyLineBreaker infinite width ignores optional breaks but honors mandatory breaks",
          "[layout2][greedyLineBreaker]") {
    std::vector<TL::LineBreakKind> opportunities(6, kNoBreak);
    opportunities[1]    = TL::LineBreakKind::AllowBreak;
    opportunities[3]    = TL::LineBreakKind::MustBreak;
    const auto prepared = preparedDocument(U"abcdef", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });

    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::kInfinity);

    REQUIRE(output.lines.size() == 2);
    REQUIRE(output.lines[0].endKind == TL::LineEndKind::MandatoryBreak);
    REQUIRE(output.lines[0].graphemeRange == TL::GraphemeRange{ 0, 4 });
    REQUIRE(output.lines[0].width == TL::fromFloat(40.0f));
    REQUIRE(output.lines[1].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(output.lines[1].graphemeRange == TL::GraphemeRange{ 4, 6 });
    REQUIRE(output.lines[1].width == TL::fromFloat(20.0f));
}

TEST_CASE("GreedyLineBreaker merges slices from multiple runs on the same line",
          "[layout2][greedyLineBreaker]") {
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    const std::vector<TL::GlyphRun> runs{
        TL::GlyphRun{ .codepointRange = { 0, 2 }, .graphemeRange = { 0, 2 }, .level = 0 },
        TL::GlyphRun{ .codepointRange = { 2, 4 }, .graphemeRange = { 2, 4 }, .level = 1 },
    };
    const auto prepared = preparedDocument(U"abcd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) },
                                           runs);
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(1000.0f));
    const auto& lines               = output.lines;

    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].width == TL::fromFloat(40.0f));
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 4 });
    REQUIRE(lines[0].glyphRunRange == TL::RunRange{ 0, 2 });
    REQUIRE(output.glyphRuns.size() == 2);
}

TEST_CASE("GreedyLineBreaker skips unsafe-to-break candidates and overflows until a safe one",
          "[layout2][greedyLineBreaker]") {
    // "abcd": an AllowBreak opportunity exists before 'c', but it is unsafe to break there
    // (as if HarfBuzz had reported it would require reshaping), so the line must keep growing
    // past maxLineWidth until the safe opportunity before 'd' is reached.
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[1]    = TL::LineBreakKind::NoBreak;    // boundary before 'c' -- HB unsafe
    opportunities[2]    = TL::LineBreakKind::AllowBreak; // boundary before 'd' -- safe
    const auto prepared = preparedDocument(U"abcd", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)),
                                             advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(15.0f));
    const auto& lines               = output.lines;
    REQUIRE(output.lines.size() == 2);

    REQUIRE(lines.size() == 2);
    // Overflows past maxLineWidth (15) up to 30 before the safe candidate before 'd' is used.
    REQUIRE(lines[0].endKind == TL::LineEndKind::SoftWrap);
    REQUIRE(lines[0].width == TL::fromFloat(30.0f));
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 3 }); // 'a','b','c'

    REQUIRE(lines[1].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[1].width == TL::fromFloat(10.0f));
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 3, 4 }); // 'd'
}

TEST_CASE("GreedyLineBreaker finish() splits off a trailing mandatory break as its own line",
          "[layout2][greedyLineBreaker]") {
    // The paragraph's very last codepoint is itself a mandatory break; addRun() never sees a
    // following grapheme to act on it, so finish() must detect and split it off before flushing
    // the (empty) remainder.
    std::vector<TL::LineBreakKind> opportunities(2, kNoBreak);
    opportunities[1]    = TL::LineBreakKind::MustBreak; // trailing mandatory break, after 'b'
    const auto prepared = preparedDocument(U"ab", std::move(opportunities),
                                           { advance(TL::fromFloat(10.0f)), advance(TL::fromFloat(10.0f)) });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(1000.0f));
    const auto& lines               = output.lines;
    REQUIRE(output.lines.size() == 2);

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].endKind == TL::LineEndKind::MandatoryBreak);
    REQUIRE(lines[0].width == TL::fromFloat(20.0f));
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 2 });

    REQUIRE(lines[1].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[1].width == TL::fromFloat(0.0f));
    REQUIRE(lines[1].trimmedWidth == TL::fromFloat(0.0f));
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 2, 2 });
}

TEST_CASE("GreedyLineBreaker recomputes trailing whitespace after a soft wrap",
          "[layout2][greedyLineBreaker]") {
    std::vector<TL::LineBreakKind> opportunities(5, kNoBreak);
    opportunities[1]                = TL::LineBreakKind::AllowBreak;
    const auto prepared             = preparedDocument(U"a  b ", std::move(opportunities),
                                                       {
                                                           advance(TL::fromFloat(20.0f)),
                                                           advance(TL::fromFloat(5.0f), true),
                                                           advance(TL::fromFloat(10.0f)),
                                                           advance(TL::fromFloat(10.0f)),
                                                           advance(TL::fromFloat(5.0f), true),
                                                       });
    const TL::DocumentLayout output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(30.0f));
    const auto& lines               = output.lines;
    REQUIRE(output.lines.size() == 2);

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 2 });
    REQUIRE(lines[0].trimmedWidth == TL::fromFloat(20.0f));
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 2, 5 });
    REQUIRE(lines[1].width == TL::fromFloat(25.0f));
    REQUIRE(lines[1].trimmedWidth == TL::fromFloat(20.0f));
}

TEST_CASE("layout places each U+2028 line separator segment on its own line",
          "[layout2][prepared][line-separator]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::u32string text = U"ab\u2028cd\u2028e";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };

    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
    REQUIRE(prepared.graphemes.size() == text.size());
    REQUIRE(prepared.breakOpportunities.size() == text.size());
    // U+2028 is a mandatory break after itself.
    REQUIRE(prepared.breakOpportunities[2] == TL::LineBreakKind::MustBreak);
    REQUIRE(prepared.breakOpportunities[5] == TL::LineBreakKind::MustBreak);

    const auto output = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(1000.0f));
    const auto& lines = output.lines;

    REQUIRE(lines.size() == 3);

    REQUIRE(lines[0].endKind == TL::LineEndKind::MandatoryBreak);
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 3 });
    REQUIRE(lines[0].codepointRange == TL::CodepointRange{ 0, 3 });
    REQUIRE(lines[0].trimmedWidth == prepared.graphemeAdvancePrefix[2] - prepared.graphemeAdvancePrefix[0]);

    REQUIRE(lines[1].endKind == TL::LineEndKind::MandatoryBreak);
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 3, 6 });
    REQUIRE(lines[1].codepointRange == TL::CodepointRange{ 3, 6 });
    REQUIRE(lines[1].trimmedWidth == prepared.graphemeAdvancePrefix[5] - prepared.graphemeAdvancePrefix[3]);

    REQUIRE(lines[2].endKind == TL::LineEndKind::ParagraphEnd);
    REQUIRE(lines[2].graphemeRange == TL::GraphemeRange{ 6, 7 });
    REQUIRE(lines[2].codepointRange == TL::CodepointRange{ 6, 7 });
    REQUIRE(lines[2].trimmedWidth == graphemeAdvance(prepared, 6));

    // All lines start at the left margin and baselines advance line by line.
    REQUIRE(lines[0].originX == TL::kZero);
    REQUIRE(lines[1].originX == TL::kZero);
    REQUIRE(lines[2].originX == TL::kZero);
    REQUIRE(lines[1].baselineY > lines[0].baselineY);
    REQUIRE(lines[2].baselineY > lines[1].baselineY);
}

TEST_CASE("prepareDocument produces cacheable glyph and run buffers", "[layout2][prepared]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::u32string text = U"Hello world";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };

    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, scriptTag("Latn"), "en");

    REQUIRE(prepared.graphemeMap.graphemeCount() == text.size());
    REQUIRE(prepared.graphemes.size() == text.size());
    REQUIRE(prepared.graphemeAdvancePrefix.size() == prepared.graphemes.size() + 1);
    REQUIRE(prepared.graphemeAdvancePrefix.front() == TL::kZero);
    REQUIRE(prepared.breakOpportunities.size() == text.size());
    REQUIRE_FALSE(prepared.glyphs.empty());
    REQUIRE_FALSE(prepared.glyphRuns.empty());

    TL::GlyphIndex nextGlyph       = 0;
    TL::GraphemeIndex nextGrapheme = 0;
    for (const TL::GlyphRun& run : prepared.glyphRuns) {
        REQUIRE(run.glyphRange.min == nextGlyph);
        REQUIRE(run.graphemeRange.min == nextGrapheme);
        REQUIRE(run.fontHandle);
        nextGlyph    = run.glyphRange.max;
        nextGrapheme = run.graphemeRange.max;
    }
    REQUIRE(nextGlyph == prepared.glyphs.size());
    REQUIRE(nextGrapheme == prepared.graphemes.size());

    for (TL::GraphemeIndex g = 0; g < prepared.graphemes.size(); ++g) {
        REQUIRE(graphemeAdvance(prepared, g) > TL::kZero);
    }
    REQUIRE(prepared.graphemes[5].trimmableWhitespace);
}

TEST_CASE("prepared paragraph layout exposes renderable glyph run slices", "[layout2][prepared]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::u32string text = U"ab cd";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, scriptTag("Latn"), "en");

    const TL::LayoutUnit firstWordWidth = graphemeAdvance(prepared, 0) + graphemeAdvance(prepared, 1);
    const TL::LayoutUnit firstLineWidth = firstWordWidth + graphemeAdvance(prepared, 2);
    const TL::DocumentLayout output     = TL::layoutPreparedDocument(prepared, {}, firstLineWidth);
    const auto& lines                   = output.lines;

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].glyphRunRange.distance() == 1);
    REQUIRE(lines[1].glyphRunRange.distance() == 1);
    REQUIRE(output.glyphRuns.size() == 2);

    const TL::LayoutGlyphRun& first  = output.glyphRuns[lines[0].glyphRunRange.min];
    const TL::LayoutGlyphRun& second = output.glyphRuns[lines[1].glyphRunRange.min];
    REQUIRE(first.preparedRunIndex == 0);
    REQUIRE(second.preparedRunIndex == 0);
    REQUIRE(prepared.glyphRuns[0].fontHandle);
    REQUIRE(first.glyphRange.min == prepared.glyphRuns[0].glyphRange.min);
    REQUIRE(first.glyphRange.max <= second.glyphRange.min);
    REQUIRE(second.glyphRange.max == prepared.glyphRuns[0].glyphRange.max);
    REQUIRE(first.xOffset == lines[0].originX);
    REQUIRE(first.yOffset == lines[0].baselineY);
    REQUIRE(second.xOffset < lines[1].originX);
    REQUIRE(second.yOffset == lines[1].baselineY);
}

TEST_CASE("prepared paragraph wraps mixed LTR and RTL text in visual glyph order",
          "[layout2][prepared][bidi]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::u32string text = U"abc \u05D0\u05D1\u05D2 def";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const auto prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");

    TL::LayoutUnit firstLineWidth = TL::kZero;
    for (TL::GraphemeIndex g = 0; g < 8; ++g) {
        firstLineWidth += graphemeAdvance(prepared, g);
    }
    const auto output = TL::layoutPreparedDocument(prepared, {}, firstLineWidth);
    const auto& lines = output.lines;

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 8 });
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 8, 11 });
    REQUIRE(visualGraphemes(prepared, output, lines[0]) ==
            std::vector<TL::GraphemeIndex>{ 0, 1, 2, 3, 6, 5, 4, 7 });
    REQUIRE(visualGraphemes(prepared, output, lines[1]) == std::vector<TL::GraphemeIndex>{ 8, 9, 10 });
}

TEST_CASE("prepared paragraph wraps one RTL run containing two words", "[layout2][prepared][bidi]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    const std::array fonts{ &font };
    const std::u32string text = U"\u05E9\u05DC\u05D5\u05DD \u05E2\u05D5\u05DC\u05DD";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const auto prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultRTL }, {}, "he");

    REQUIRE(prepared.glyphRuns.size() == 1);
    REQUIRE(prepared.glyphRuns[0].level % 2 == 1);
    TL::LayoutUnit firstWordWidth = TL::kZero;
    for (TL::GraphemeIndex g = 0; g < 5; ++g) {
        firstWordWidth += graphemeAdvance(prepared, g);
    }
    const auto output = TL::layoutPreparedDocument(prepared, {}, firstWordWidth);
    const auto& lines = output.lines;

    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0].graphemeRange == TL::GraphemeRange{ 0, 5 });
    REQUIRE(lines[1].graphemeRange == TL::GraphemeRange{ 5, 9 });
    REQUIRE(visualGraphemes(prepared, output, lines[0]) == std::vector<TL::GraphemeIndex>{ 4, 3, 2, 1, 0 });
    REQUIRE(visualGraphemes(prepared, output, lines[1]) == std::vector<TL::GraphemeIndex>{ 8, 7, 6, 5 });
}

TEST_CASE("shapeRun applies Latin ligatures with Lato", "[layout2][shape][ligatures]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    static constexpr std::array<TL::OpenTypeFeatureFlag, 1> disableLiga{ TL::OpenTypeFeatureFlag{
        .feature = TL::fourCCToUint32("liga"), .enabled = false } };

    const TL::FontDef enabledFont{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = {},
        .variations    = {},
    };
    const TL::FontDef disabledFont{
        .familyNames   = families,
        .fontSize      = TL::k16,
        .style         = TL::FontStyle::Normal,
        .weight        = TL::FontWeight::Regular,
        .lineHeight    = TL::kZero,
        .letterSpacing = TL::kZero,
        .wordSpacing   = TL::kZero,
        .verticalAlign = TL::kZero,
        .features      = disableLiga,
        .variations    = {},
    };

    const TL::FontHandle enabledHandle  = database->resolveFont(enabledFont);
    const TL::FontHandle disabledHandle = database->resolveFont(disabledFont);
    REQUIRE(enabledHandle);
    REQUIRE(disabledHandle);

    const std::u32string_view text = U"office affine efficient fi fl ffi ffl";
    std::vector<TL::Glyph> enabledGlyphs;
    std::vector<TL::Glyph> disabledGlyphs;
    const TL::DocumentSource document(text);

    TL::shapeRun(document, TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) }, raw(database),
                 { enabledHandle, 0 }, 0, scriptTag("Latn"), enabledFont, "en",
                 [&](const TL::Glyph& glyph, bool) {
                     enabledGlyphs.push_back(glyph);
                 });
    TL::shapeRun(document, TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) },
                 TL::CodepointRange{ 0, static_cast<uint32_t>(text.size()) }, raw(database),
                 { disabledHandle, 0 }, 0, scriptTag("Latn"), disabledFont, "en",
                 [&](const TL::Glyph& glyph, bool) {
                     disabledGlyphs.push_back(glyph);
                 });

    REQUIRE(enabledGlyphs.size() < disabledGlyphs.size());

    const std::array fonts{ &enabledFont };
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const auto prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
    const auto layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(560.0f));
}

TEST_CASE("caret and selection queries preserve logical boundaries inside a ligature",
          "[layout2][queries][ligatures]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    font.fontSize    = TL::k16;
    const std::array fonts{ &font };
    const std::u32string_view text = U"ffi";
    const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
    const TL::PreparedDocument prepared =
        TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                            std::array{ TL::BaseDirection::DefaultLTR }, scriptTag("Latn"), "en");
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(100.0f));

    REQUIRE(prepared.graphemeMap.graphemeCount() == 3);
    REQUIRE(prepared.glyphs.size() < prepared.graphemeMap.graphemeCount());
    REQUIRE(layout.lines.size() == 1);

    std::array<TL::CaretPosition, 4> carets;
    for (TL::GraphemeIndex boundary = 0; boundary <= 3; ++boundary) {
        carets[boundary] = TL::caretPosition(
            prepared, layout,
            { boundary, boundary == 3 ? TL::CaretAffinity::Upstream : TL::CaretAffinity::Downstream });
        REQUIRE(carets[boundary].line == 0);
        REQUIRE(carets[boundary].level == 0);
    }

    REQUIRE(carets[0].x == layout.lines[0].originX);
    REQUIRE(carets[0].x < carets[1].x);
    REQUIRE(carets[1].x < carets[2].x);
    REQUIRE(carets[2].x < carets[3].x);
    REQUIRE(carets[3].x == layout.lines[0].originX + layout.lines[0].width);

    SECTION("hit testing round-trips every ligature caret") {
        for (TL::GraphemeIndex boundary = 0; boundary <= 3; ++boundary) {
            const TL::CaretIndex hit =
                TL::hitTestPoint(prepared, layout, carets[boundary].x, layout.lines[0].baselineY);
            REQUIRE(hit.grapheme == boundary);
            REQUIRE(TL::caretPosition(prepared, layout, hit).x == carets[boundary].x);
        }
    }

    SECTION("selection rectangles use internal ligature caret boundaries") {
        const std::vector<TL::SelectionRect> middle = selectionRects(prepared, layout, { 1, 2 });
        REQUIRE(middle.size() == 1);
        REQUIRE(middle[0].line == 0);
        REQUIRE(middle[0].x0 == carets[1].x);
        REQUIRE(middle[0].x1 == carets[2].x);

        const std::vector<TL::SelectionRect> whole = selectionRects(prepared, layout, { 0, 3 });
        REQUIRE(whole.size() == 1);
        REQUIRE(whole[0].x0 == carets[0].x);
        REQUIRE(whole[0].x1 == carets[3].x);
    }
}

TEST_CASE("render complex multilingual layouts to grayscale PNG files", "[layout2][render][png]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Go Noto Kurrent-Regular" };
    TL::FontDef font = makeFont();
    font.familyNames = families;
    font.fontSize    = TL::fromFloat(28.0f);
    const std::array fonts{ &font };

    REQUIRE(database->resolveFont(font));

    const auto writeAndCheck = [&](const TL::PreparedDocument& prepared, const TL::DocumentLayout& layout,
                                   std::string_view filename) {
        const std::filesystem::path outputPath = std::filesystem::current_path() / filename;
        std::error_code error;
        std::filesystem::remove(outputPath, error);

        (void)prepared;
        (void)layout;
        (void)outputPath;
        // TODO: Render layout.
    };

    const auto render = [&](std::u32string_view text, TL::BaseDirection direction, std::string_view language,
                            std::string_view filename, TL::TextAlignment alignment = TL::TextAlignment::Start,
                            TL::TabStops tabStops = {}, bool allowBreakAnywhere = false,
                            size_t expectedLineCount = 0) {
        const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
        const TL::PreparedDocument prepared =
            TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                                std::array{ direction }, {}, language);
        const TL::DocumentLayout layout = TL::layoutPreparedDocument(
            prepared, std::array{ alignment }, TL::fromFloat(100.0f), tabStops, {}, allowBreakAnywhere);
        if (expectedLineCount != 0) {
            REQUIRE(layout.lines.size() == expectedLineCount);
        }
        writeAndCheck(prepared, layout, filename);
    };

    SECTION("allow breaking anywhere at HarfBuzz-safe boundaries") {
        // The narrow width forces breaks inside words; the generated PNG makes the visual result
        // easy to inspect while HarfBuzz still prevents breaks inside unsafe shaping sequences.
        render(U"extraordinary typography", TL::BaseDirection::LeftToRight, "en",
               "text-layout-break-anywhere.png", TL::TextAlignment::Start, {}, true, 4);
    }

    SECTION("Latin tab stops") {
        // The repeated labels make the columns and all successive tab-stop advances easy to inspect.
        render(U"Item\tQuantity\tUnit price\tTotal", TL::BaseDirection::LeftToRight, "en",
               "text-layout-tabs-latin.png", TL::TextAlignment::Start,
               {
                   .firstStop = TL::fromFloat(120.0f),
                   .interval  = TL::fromFloat(120.0f),
               });
    }

    SECTION("mixed BiDi tab stops") {
        // Tabs are measured in logical order from the RTL paragraph margin before visual reordering.
        render(U"\u05EA\u05D9\u05D0\u05D5\u05E8\tSKU A-104\t\u05DB\u05DE\u05D5\u05EA 12\tPrice $15.00",
               TL::BaseDirection::RightToLeft, "he", "text-layout-tabs-mixed-bidi.png",
               TL::TextAlignment::Start,
               {
                   .firstStop = TL::fromFloat(120.0f),
                   .interval  = TL::fromFloat(120.0f),
               });
    }

    SECTION("Latin Greek Cyrillic and combining marks") {
        render(U"Latin: caf\u00E9, na\u00EFve, A\u0308.  \u0395\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC: "
               U"\u039A\u03B1\u03BB\u03B7\u03BC\u03AD\u03C1\u03B1 \u03BA\u03CC\u03C3\u03BC\u03B5.  "
               U"\u041A\u0438\u0440\u0438\u043B\u043B\u0438\u0446\u0430: "
               U"\u041F\u0440\u0438\u0432\u0435\u0442, \u043C\u0438\u0440!",
               TL::BaseDirection::DefaultLTR, "en", "text-layout-latin-greek-cyrillic.png");
    }

    SECTION("letter and word spacing with ligatures") {
        static constexpr std::array<std::string_view, 1> latoFamilies{ "Lato" };
        static constexpr std::array<TL::OpenTypeFeatureFlag, 1> enableLiga{ TL::OpenTypeFeatureFlag{
            TL::fourCCToUint32("liga"), true } };

        TL::FontDef spacedFont   = makeFont();
        spacedFont.familyNames   = latoFamilies;
        spacedFont.fontSize      = TL::fromFloat(28.0f);
        spacedFont.letterSpacing = TL::fromFloat(3.0f);
        spacedFont.wordSpacing   = TL::fromFloat(9.0f);
        spacedFont.features      = enableLiga;
        const std::array spacedFonts{ &spacedFont };
        const std::u32string_view text = U"office affinity: ffi fi fl  spaced words";
        const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
        const TL::PreparedDocument prepared = TL::prepareDocument(
            TL::DocumentSource(text), database, spacedFonts, TL::BoundaryTable{ boundaries },
            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
        const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(760.0f));
        REQUIRE(prepared.glyphs.size() < text.size()); // Shaping produced at least one ligature.
        REQUIRE(layout.lines.size() >= 1);
        writeAndCheck(prepared, layout, "text-layout-spacing-ligatures.png");

        TL::FontDef letterOnlyFont = spacedFont;
        letterOnlyFont.wordSpacing = TL::kZero;
        const std::array letterOnlyFonts{ &letterOnlyFont };
        const TL::PreparedDocument letterOnly = TL::prepareDocument(
            TL::DocumentSource(text), database, letterOnlyFonts, TL::BoundaryTable{ boundaries },
            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
        const TL::DocumentLayout letterOnlyLayout =
            TL::layoutPreparedDocument(letterOnly, {}, TL::fromFloat(760.0f));
        REQUIRE(layout.horizontalTextBounds.distance() > letterOnlyLayout.horizontalTextBounds.distance());
        writeAndCheck(letterOnly, letterOnlyLayout, "text-layout-letter-spacing-ligatures.png");
    }

    SECTION("Arabic Hebrew and embedded Latin") {
        render(U"English 123 \u2014 \u0627\u0644\u0639\u0631\u0628\u064A\u0629: "
               U"\u0627\u0644\u0633\u064E\u0651\u0644\u064E\u0627\u0645\u064F "
               U"\u0639\u064E\u0644\u064E\u064A\u0652\u0643\u064F\u0645\u0652 \u2014 "
               U"\u05E2\u05D1\u05E8\u05D9\u05EA: \u05E9\u05B8\u05DC\u05D5\u05B9\u05DD "
               U"\u05E2\u05D5\u05B9\u05DC\u05B8\u05DD \u2014 ABC 456",
               TL::BaseDirection::RightToLeft, "ar", "text-layout-arabic-hebrew-bidi.png");
    }

    SECTION("Indic and Southeast Asian shaping") {
        render(
            U"\u0939\u093F\u0928\u094D\u0926\u0940: \u0928\u092E\u0938\u094D\u0924\u0947 "
            U"\u0926\u0941\u0928\u093F\u092F\u093E\u0964  \u09AC\u09BE\u0982\u09B2\u09BE: "
            U"\u09A8\u09AE\u09B8\u09CD\u0995\u09BE\u09B0 \u09AC\u09BF\u09B6\u09CD\u09AC\u0964  "
            U"\u0E44\u0E17\u0E22: \u0E2A\u0E27\u0E31\u0E2A\u0E14\u0E35\u0E0A\u0E32\u0E27\u0E42\u0E25\u0E01  "
            U"\u0DC3\u0D82\u0DBD\u0D82\u0DA4: \u0D86\u0DAF\u0DBA\u0D94\u0DBA\u0DB1\u0DCA",
            TL::BaseDirection::DefaultLTR, "hi", "text-layout-indic-southeast-asian.png");
    }

    SECTION("CJK and emoji sequences") {
        render(
            U"\u65E5\u672C\u8A9E\u306E\u7D44\u7248\u3001\u4E2D\u56FD\u8A9E\u6392\u7248\u3002  "
            U"Emoji: \U0001F468\u200D\U0001F4BB \U0001F469\u200D\U0001F469\u200D\U0001F467\u200D\U0001F466 "
            U"\U0001F44D\U0001F3FD \U0001F1FA\U0001F1F8 \u2615\uFE0F",
            TL::BaseDirection::DefaultLTR, "ja", "text-layout-cjk-emoji.png");
    }

    SECTION("Hangul shaping") {
        // Go Noto Kurrent stores modern Hangul as conjoining Jamo and relies on
        // Hangul-specific OpenType features. A Japanese language tag selects the
        // font's JAN language system, which intentionally omits those features.
        render(U"\uD55C\uAE00 \uD14D\uC2A4\uD2B8.  \uB300\uD55C\uBBFC\uAD6D \uD55C\uAD6D\uC5B4 \uC870\uD310.",
               TL::BaseDirection::DefaultLTR, "ko", "text-layout-hangul.png");
    }

    SECTION("paired brackets across directional runs") {
        render(U"LTR outer: [English (\u05E2\u05D1\u05E8\u05D9\u05EA) "
               U"\u0627\u0644\u0639\u0631\u0628\u064A\u0629] {123 (ABC)} \u2014 "
               U"RTL outer: [\u05E9\u05DC\u05D5\u05DD (English) \u0645\u0631\u062D\u0628\u0627]",
               TL::BaseDirection::DefaultLTR, "en", "text-layout-paired-brackets.png");
    }

    SECTION("DefaultRTL with neutral and numeric prefixes") {
        render(U"123 (ABC) \u2014 \u05E9\u05B8\u05DC\u05D5\u05B9\u05DD [English 456] \u2014 "
               U"\u0627\u0644\u0639\u0631\u0628\u064A\u0629 (test) \u2014 !!!",
               TL::BaseDirection::DefaultRTL, "he", "text-layout-default-rtl.png");
    }

    SECTION("center-aligned wrapped text") {
        const std::u32string_view text =
            U"Centered multilingual text \u2014 \u0395\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC, "
            U"\u0627\u0644\u0639\u0631\u0628\u064A\u0629, \u05E2\u05D1\u05E8\u05D9\u05EA, \u65E5\u672C\u8A9E "
            U"\u2014 "
            U"with lines of deliberately different widths.";
        const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
        const TL::PreparedDocument prepared =
            TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                                std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
        const TL::DocumentLayout layout = TL::layoutPreparedDocument(
            prepared, std::array{ TL::TextAlignment::Center }, TL::fromFloat(420.0f));
        REQUIRE(layout.lines.size() >= 2);
        REQUIRE(std::ranges::any_of(layout.lines, [](const TL::LayoutLine& line) {
            return line.originX > TL::kZero;
        }));
        writeAndCheck(prepared, layout, "text-layout-center-aligned.png");
    }

    SECTION("multiple font families and sizes") {
        static constexpr std::array<std::string_view, 1> notoFamily{ "Go Noto Kurrent-Regular" };
        static constexpr std::array<std::string_view, 1> latoFamily{ "Lato" };
        static constexpr std::array<std::string_view, 1> codeFamily{ "Source Code Pro" };

        TL::FontDef smallNoto  = makeFont();
        smallNoto.familyNames  = notoFamily;
        smallNoto.fontSize     = TL::fromFloat(18.0f);
        TL::FontDef largeLato  = makeFont();
        largeLato.familyNames  = latoFamily;
        largeLato.fontSize     = TL::fromFloat(38.0f);
        largeLato.weight       = TL::FontWeight::Medium;
        TL::FontDef mediumCode = makeFont();
        mediumCode.familyNames = codeFamily;
        mediumCode.fontSize    = TL::fromFloat(25.0f);
        mediumCode.weight      = TL::FontWeight::Medium;
        TL::FontDef largeNoto  = smallNoto;
        largeNoto.fontSize     = TL::fromFloat(32.0f);

        const std::array styledFonts{ &smallNoto, &largeLato, &mediumCode, &largeNoto };
        const std::u32string part1 = U"Small multilingual: \u0395\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC "
                                     U"\u0627\u0644\u0639\u0631\u0628\u064A\u0629  ";
        const std::u32string part2 = U"LARGE LATO  ";
        const std::u32string part3 = U"Code: != -> ffi  ";
        const std::u32string part4 =
            U"\u65E5\u672C\u8A9E \u0939\u093F\u0928\u094D\u0926\u0940 \u05E2\u05D1\u05E8\u05D9\u05EA";
        const std::u32string text = part1 + part2 + part3 + part4;
        const std::array<uint32_t, 5> boundaries{
            0,
            static_cast<uint32_t>(part1.size()),
            static_cast<uint32_t>(part1.size() + part2.size()),
            static_cast<uint32_t>(part1.size() + part2.size() + part3.size()),
            static_cast<uint32_t>(text.size()),
        };
        const TL::PreparedDocument prepared = TL::prepareDocument(
            TL::DocumentSource(text), database, styledFonts, TL::BoundaryTable{ boundaries },
            std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");
        const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(760.0f));

        REQUIRE(prepared.glyphRuns.size() >= styledFonts.size());
        REQUIRE(std::ranges::any_of(prepared.glyphRuns, [&](const TL::GlyphRun& run) {
            return run.fontRunIndex == 1 && run.fontHandle;
        }));
        writeAndCheck(prepared, layout, "text-layout-various-fonts-sizes.png");
    }

    SECTION("positive and hanging textIndent visual rendering") {
        const std::u32string_view text =
            U"Typography indent test: the first line of this paragraph is visibly indented, "
            U"while subsequent wrapped lines align to the left margin correctly.";
        const std::array<uint32_t, 2> boundaries{ 0, static_cast<uint32_t>(text.size()) };
        const TL::PreparedDocument prepared =
            TL::prepareDocument(TL::DocumentSource(text), database, fonts, TL::BoundaryTable{ boundaries },
                                std::array{ TL::BaseDirection::DefaultLTR }, {}, "en");

        // Positive text-indent (first-line indent)
        const TL::DocumentLayout positiveIndentLayout = TL::layoutPreparedDocument(
            prepared, {}, TL::fromFloat(400.0f), {}, std::array{ TL::fromFloat(40.0f) });
        REQUIRE(positiveIndentLayout.lines.size() >= 2);
        CHECK(positiveIndentLayout.lines[0].originX == TL::fromFloat(40.0f));
        CHECK(positiveIndentLayout.lines[1].originX == TL::fromFloat(0.0f));
        writeAndCheck(prepared, positiveIndentLayout, "text-layout-indent-positive.png");

        // Negative text-indent (hanging indent)
        const TL::DocumentLayout hangingIndentLayout = TL::layoutPreparedDocument(
            prepared, {}, TL::fromFloat(400.0f), {}, std::array{ TL::fromFloat(-30.0f) });
        REQUIRE(hangingIndentLayout.lines.size() >= 2);
        CHECK(hangingIndentLayout.lines[0].originX == TL::fromFloat(-30.0f));
        CHECK(hangingIndentLayout.lines[1].originX == TL::fromFloat(0.0f));
        writeAndCheck(prepared, hangingIndentLayout, "text-layout-indent-hanging.png");
    }
}

TEST_CASE("render English alphabet with hinting enabled and disabled", "[layout2][render][png][hinting]") {
    const auto database = TL::getDefaultFontDatabase();
    REQUIRE(database != nullptr);

    static constexpr std::array<std::string_view, 1> families{ "Lato" };

    TL::FontDef enabledFont        = makeFont();
    enabledFont.familyNames        = families;
    enabledFont.fontSize           = TL::fromFloat(18.0f);
    enabledFont.hinting            = TL::Hinting::Enable;
    TL::FontDef disabledFont       = enabledFont;
    disabledFont.hinting           = TL::Hinting::Disable;

    const std::u32string_view text = U"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const std::array fonts{ &enabledFont, &disabledFont };
    const std::u32string doubled = std::u32string(text) + U"\n" + std::u32string(text);
    const std::array<uint32_t, 3> boundaries{
        0,
        static_cast<uint32_t>(text.size()) + 1,
        static_cast<uint32_t>(doubled.size()),
    };

    const TL::PreparedDocument prepared = TL::prepareDocument(
        TL::DocumentSource(doubled), database, fonts, TL::BoundaryTable{ boundaries },
        std::array{ TL::BaseDirection::DefaultLTR, TL::BaseDirection::DefaultLTR }, {}, "en");
    const TL::DocumentLayout layout = TL::layoutPreparedDocument(prepared, {}, TL::fromFloat(600.0f));

    REQUIRE(layout.lines.size() == 2);
    // The two lines use different font runs (one per hinting policy).
    REQUIRE(prepared.glyphRuns.size() >= 2);
    REQUIRE(std::ranges::any_of(prepared.glyphRuns, [](const TL::GlyphRun& run) {
        return run.fontRunIndex == 0 && run.fontHandle;
    }));
    REQUIRE(std::ranges::any_of(prepared.glyphRuns, [](const TL::GlyphRun& run) {
        return run.fontRunIndex == 1 && run.fontHandle;
    }));

    // TODO: Render layout.
}

TEST_CASE("GreedyLineBreaker textIndent indents first line and adjusts line wrap", "[layout2][textIndent]") {
    // 4 words of 20px advance each: "aa bb cc dd"
    // Grapheme advances: 20, 20, 20, 20
    std::vector<TL::LineBreakKind> opportunities(4, kNoBreak);
    opportunities[0]    = TL::LineBreakKind::AllowBreak;
    opportunities[1]    = TL::LineBreakKind::AllowBreak;
    opportunities[2]    = TL::LineBreakKind::AllowBreak;

    const auto prepared = preparedDocument(U"abcd", std::move(opportunities),
                                           { advance(TL::fromFloat(20.0f)), advance(TL::fromFloat(20.0f)),
                                             advance(TL::fromFloat(20.0f)), advance(TL::fromFloat(20.0f)) });

    SECTION("positive textIndent in LTR shifts first line origin and wraps earlier") {
        // maxLineWidth = 50. First line available width = 50 - 15 = 35.
        // First line fits 1 word (20px <= 35px), wraps at 2nd word (40px > 35px).
        // Remaining lines have maxLineWidth = 50, fitting 2 words (40px <= 50px).
        // Total 3 lines: [0..1] (20px), [1..3] (40px), [3..4] (20px).
        const TL::LayoutUnit maxLineWidth = TL::fromFloat(50.0f);
        const TL::LayoutUnit textIndent   = TL::fromFloat(15.0f);

        const auto layout =
            TL::layoutPreparedDocument(prepared, {}, maxLineWidth, {}, std::array{ textIndent });

        REQUIRE(layout.lines.size() == 3);
        CHECK(layout.lines[0].originX == TL::fromFloat(15.0f));
        CHECK(layout.lines[0].graphemeRange == TL::GraphemeRange{ 0, 1 });
        CHECK(layout.lines[1].originX == TL::fromFloat(0.0f));
        CHECK(layout.lines[1].graphemeRange == TL::GraphemeRange{ 1, 3 });
        CHECK(layout.lines[2].originX == TL::fromFloat(0.0f));
        CHECK(layout.lines[2].graphemeRange == TL::GraphemeRange{ 3, 4 });
    }

    SECTION(
        "negative textIndent (hanging indent) outdents first line and allows more content on first line") {
        // maxLineWidth = 30. First line available width = 30 - (-15) = 45.
        // First line fits 2 words (40px <= 45px).
        // Second line maxLineWidth = 30, fits 1 word (20px <= 30px).
        // Third line fits 1 word.
        const TL::LayoutUnit maxLineWidth = TL::fromFloat(30.0f);
        const TL::LayoutUnit textIndent   = TL::fromFloat(-15.0f);

        const auto layout =
            TL::layoutPreparedDocument(prepared, {}, maxLineWidth, {}, std::array{ textIndent });

        REQUIRE(layout.lines.size() == 3);
        CHECK(layout.lines[0].originX == TL::fromFloat(-15.0f));
        CHECK(layout.lines[0].graphemeRange == TL::GraphemeRange{ 0, 2 });
        CHECK(layout.lines[1].originX == TL::fromFloat(0.0f));
        CHECK(layout.lines[1].graphemeRange == TL::GraphemeRange{ 2, 3 });
        CHECK(layout.lines[2].originX == TL::fromFloat(0.0f));
        CHECK(layout.lines[2].graphemeRange == TL::GraphemeRange{ 3, 4 });
    }

    SECTION("textIndent with Right alignment") {
        const TL::LayoutUnit maxLineWidth = TL::fromFloat(100.0f);
        const TL::LayoutUnit textIndent   = TL::fromFloat(10.0f);

        const auto layout = TL::layoutPreparedDocument(prepared, std::array{ TL::TextAlignment::Right },
                                                       maxLineWidth, {}, std::array{ textIndent });

        REQUIRE(layout.lines.size() == 1);
        // remainingSpace = 100 - 80 = 20. originX = remainingSpace - textIndent = 20 - 10 = 10.
        CHECK(layout.lines[0].originX == TL::fromFloat(10.0f));
    }
}
