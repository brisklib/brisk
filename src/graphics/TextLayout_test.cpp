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

#include <brisk/graphics/Fonts.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Utilities.hpp>
#include <catch2/catch_all.hpp>
#include "VisualTests.hpp"
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

static void registerTextLayoutTestFont() {
    static bool registered = false;
    if (registered) {
        return;
    }

    REQUIRE(fonts.has_value());
    auto fontData = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf");
    REQUIRE(fontData.has_value());
    Internal::registerTextLayoutFont(*fonts, *fontData, "TextLayoutTest");
    registered = true;
}

static void registerTextLayoutVisualFonts() {
    static bool registered = false;
    if (registered) {
        return;
    }

    REQUIRE(fonts.has_value());
    const auto registerFont = [](std::string_view fileName, std::string_view alias) {
        auto fontData = readBytes(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / fileName);
        REQUIRE(fontData.has_value());
        Internal::registerTextLayoutFont(*fonts, *fontData, alias);
    };
    registerFont("GoNotoKurrent-Regular.ttf", "TextLayoutNoto");
    registerFont("SourceCodePro-Medium.ttf", "TextLayoutMono");
    registerFont("Lato-Medium.ttf", "TextLayoutTest");
    registered = true;
}

TEST_CASE("TextLayout prepared document exposes document data", "[text-layout]") {
    registerTextLayoutTestFont();

    const Font font{ "TextLayoutTest", 24.f };
    const TextWithOptions text{ U"Hello world" };
    const PreparedDocument prepared = fonts->prepareDocument(font, text);

    CHECK_FALSE(prepared.empty());
    CHECK(prepared.characterCount() == 11);
    CHECK(prepared.graphemeCount() == 11);
    REQUIRE(prepared.graphemeBoundaries().size() == 12);
    CHECK(prepared.graphemeBoundaries().front() == 0);
    CHECK(prepared.graphemeBoundaries().back() == prepared.characterCount());

    for (uint32_t character = 0; character <= prepared.characterCount(); ++character) {
        const uint32_t grapheme = prepared.characterToGrapheme(character);
        REQUIRE(grapheme <= prepared.graphemeCount());
        CHECK(prepared.graphemeToCharacter(grapheme) <= character);
    }

    REQUIRE(prepared.paragraphCount() == 1);
    const PreparedParagraph paragraph = prepared.paragraph(0);
    CHECK(paragraph.characterRange == Range<uint32_t>{ 0, 11 });
    CHECK(paragraph.graphemeRange == Range<uint32_t>{ 0, 11 });
    CHECK(paragraph.direction == TextDirection::LTR);
}

TEST_CASE("TextLayout wraps and exposes line data", "[text-layout]") {
    registerTextLayoutTestFont();

    const Font font{ "TextLayoutTest", 24.f };
    const TextWithOptions text{ U"One two three four five" };
    const PreparedDocument prepared = fonts->prepareDocument(font, text);

    TextLayoutOptions options;
    options.maxLineWidth        = 50.f;
    options.alignment           = TextLayoutAlignment::Left;
    const DocumentLayout layout = prepared.layout(options);

    CHECK_FALSE(layout.empty());
    CHECK(layout.lineCount() > 1);
    CHECK(layout.bounds().width() > 0.f);
    CHECK(layout.trimmedBounds().width() > 0.f);

    for (size_t index = 0; index < layout.lineCount(); ++index) {
        const DocumentLine line = layout.line(index);
        CHECK(line.characterRange.min <= line.characterRange.max);
        CHECK(line.graphemeRange.min <= line.graphemeRange.max);
        CHECK(line.width >= 0.f);
        CHECK(line.trimmedWidth >= 0.f);
        CHECK(line.trimmedWidth <= line.width);
        CHECK(line.ascender >= 0.f);
        CHECK(line.descender >= 0.f);
    }

    const CaretPosition first = layout.caretPosition({ 0, CaretAffinity::Downstream });
    const CaretPosition last  = layout.caretPosition({ prepared.graphemeCount(), CaretAffinity::Upstream });
    CHECK(first.line < layout.lineCount());
    CHECK(last.line < layout.lineCount());
    CHECK((last.x >= first.x || last.line > first.line));

    const CaretIndex hit = layout.hitTest({ first.x, first.x });
    CHECK(hit.grapheme <= prepared.graphemeCount());

    size_t selectionCount = 0;
    layout.selectionRects({ 0, prepared.graphemeCount() }, [&](const TextSelectionRect& rect) {
        ++selectionCount;
        CHECK(rect.line < layout.lineCount());
        CHECK(rect.x0 <= rect.x1);
    });
    CHECK(selectionCount > 0);
}

TEST_CASE("TextLayout identifies paragraph separator graphemes", "[text-layout]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 20.f }, TextWithOptions{ U"abc\ndef" });

    CHECK(prepared.graphemeCount() == 7);
    for (uint32_t grapheme = 0; grapheme < prepared.graphemeCount(); ++grapheme) {
        CHECK(prepared.isParagraphSeparator(grapheme) == (grapheme == 3));
    }
    CHECK_FALSE(prepared.isParagraphSeparator(prepared.graphemeCount()));
}

TEST_CASE("TextLayout gives a trailing empty line visible caret metrics", "[text-layout]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 20.f }, TextWithOptions{ U"abc\n" });
    const DocumentLayout layout = prepared.layout();

    REQUIRE(layout.lineCount() == 2);
    const DocumentLine emptyLine = layout.line(1);
    CHECK(emptyLine.characterRange.empty());
    CHECK(emptyLine.ascender > 0.f);
    CHECK(emptyLine.descender > 0.f);
    CHECK_FALSE(layout.caretRect(layout.lineBeginning(1)).empty());
}

TEST_CASE("TextLayout indexed access handles invalid indices", "[text-layout]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 20.f }, TextWithOptions{ U"x" });
    const DocumentLayout layout = prepared.layout();

    CHECK(prepared.paragraph(100).characterRange.empty());
    CHECK(layout.line(100).characterRange.empty());
    CHECK(prepared.graphemeToCharacter(100) == prepared.characterCount());
}

TEST_CASE("TextLayout interaction APIs expose character and line queries", "[text-layout][interaction]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 20.f }, TextWithOptions{ U"first second third" });
    TextLayoutOptions options;
    options.maxLineWidth            = 55.f;
    const DocumentLayout layout     = prepared.layout(options);

    const CaretIndex characterCaret = prepared.caretFromCharacter(6, CaretAffinity::Downstream);
    CHECK(prepared.characterFromCaret(characterCaret) == 6);
    CHECK(layout.lineForCharacter(6) == layout.lineForCaret(characterCaret));

    for (size_t index = 0; index < layout.lineCount(); ++index) {
        const CaretIndex beginning = layout.lineBeginning(index);
        const CaretIndex ending    = layout.lineEnd(index);
        CHECK(beginning.affinity == CaretAffinity::Downstream);
        CHECK(ending.affinity == CaretAffinity::Upstream);
        CHECK(layout.lineForCaret(beginning) == index);
        CHECK(layout.lineForCaret(ending) == index);

        const RectangleF caret  = layout.caretRect(beginning);
        const DocumentLine line = layout.line(index);
        const float halfLeading = line.leading * 0.5f;
        CHECK(caret.y1 == line.baseline - line.ascender - halfLeading);
        CHECK(caret.y2 == line.baseline + line.descender + halfLeading);
    }

    const CaretPosition current = layout.caretPosition(layout.lineBeginning(0));
    const CaretIndex next       = layout.moveCaretVertically(layout.lineBeginning(0), current.x, 1);
    CHECK(layout.lineForCaret(next) == (layout.lineCount() > 1 ? 1 : 0));

    size_t characterRectangles = 0;
    layout.selectionRectsByCharacter({ 2, 14 }, [&](const TextSelectionRect& rect) {
        ++characterRectangles;
        CHECK(rect.line < layout.lineCount());
        CHECK(rect.x0 <= rect.x1);
    });
    CHECK(characterRectangles > 0);
}

TEST_CASE("TextLayout preparation validates font input", "[text-layout]") {
    registerTextLayoutTestFont();

    REQUIRE_THROWS_AS(fonts->prepareDocument(TextWithOptions{ U"text" }, {}, {}), EArgument);

    const FontAndColor styles[2]{ { Font{ "TextLayoutTest", 20.f } }, { Font{ "TextLayoutTest", 20.f } } };
    const uint32_t invalidOffsets[1]{ 0 };
    REQUIRE_THROWS_AS(fonts->prepareDocument(TextWithOptions{ U"text" }, styles, invalidOffsets), EArgument);

    const uint32_t outOfRangeOffsets[1]{ 4 };
    REQUIRE_THROWS_AS(fonts->prepareDocument(TextWithOptions{ U"text" }, styles, outOfRangeOffsets),
                      EArgument);

    const uint32_t duplicateOffsets[2]{ 1, 1 };
    const FontAndColor threeStyles[3]{ { Font{ "TextLayoutTest", 20.f } },
                                       { Font{ "TextLayoutTest", 20.f } },
                                       { Font{ "TextLayoutTest", 20.f } } };
    REQUIRE_THROWS_AS(fonts->prepareDocument(TextWithOptions{ U"text" }, threeStyles, duplicateOffsets),
                      EArgument);
}

TEST_CASE("TextLayout honors text layout options", "[text-layout]") {
    registerTextLayoutTestFont();

    const Font font{ "TextLayoutTest", 20.f };
    const PreparedDocument wrapped = fonts->prepareDocument(font, TextWithOptions{ U"one two three" });
    TextLayoutOptions options;
    options.maxLineWidth = 30.f;
    CHECK(wrapped.layout(options).lineCount() > 1);

    const PreparedDocument singleLine =
        fonts->prepareDocument(font, TextWithOptions{ U"one two three", TextOptions::SingleLine });
    CHECK(singleLine.layout(options).lineCount() == 1);

    const PreparedDocument wrapAnywhere =
        fonts->prepareDocument(font, TextWithOptions{ U"onetwothree", TextOptions::WrapAnywhere });
    CHECK(wrapAnywhere.layout(options).lineCount() > 1);
}

TEST_CASE("TextLayout CPU rasterization supports oversampling", "[text-layout]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 24.f }, TextWithOptions{ U"A" });
    const DocumentLayout layout = prepared.layout();
    CHECK(layout.lineCount() == 1);

    std::optional<Internal::TextLayoutGlyphBitmap> firstGlyph;
    Internal::loadTextLayoutGlyphRun(prepared, 0, nullptr,
                                     [&](uint32_t, const Internal::TextLayoutGlyphBitmap& glyph) {
                                         if (!firstGlyph) {
                                             firstGlyph = glyph;
                                         }
                                     });
    REQUIRE(firstGlyph.has_value());
    CHECK_FALSE(firstGlyph->color);
    CHECK(firstGlyph->horizontalScale == fonts->hscale());
    REQUIRE(firstGlyph->sprite);
    CHECK(firstGlyph->sprite->size.width > 0);
    CHECK(firstGlyph->sprite->size.height > 0);
}

TEST_CASE("TextLayout CPU rasterization supports SVG glyphs", "[text-layout]") {
    REQUIRE(fonts.has_value());
    const auto registered = fonts->addFontFromFile(
        fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "NotoColorEmoji-SVG.otf", "TextLayoutEmoji");
    REQUIRE(registered);

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutEmoji", 48.f }, TextWithOptions{ U"😀" });
    const DocumentLayout layout = prepared.layout();
    CHECK(layout.lineCount() == 1);

    std::optional<Internal::TextLayoutGlyphBitmap> glyph;
    Internal::loadTextLayoutGlyphRun(prepared, 0, nullptr,
                                     [&](uint32_t, const Internal::TextLayoutGlyphBitmap& loadedGlyph) {
                                         if (!glyph) {
                                             glyph = loadedGlyph;
                                         }
                                     });
    REQUIRE(glyph.has_value());
    CHECK(glyph->color);
    CHECK(glyph->horizontalScale == 1);
    REQUIRE(glyph->sprite);
    CHECK(glyph->sprite->size.width > 0);
    CHECK(glyph->sprite->size.height > 0);
    bool hasPixels = false;
    for (const auto value : glyph->sprite->bytes()) {
        hasPixels |= value != std::byte{};
    }
    CHECK(hasPixels);
}

TEST_CASE("TextLayout CPU renderer handles positioned glyphs", "[text-layout]") {
    registerTextLayoutTestFont();

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutTest", 24.f }, TextWithOptions{ U"A" });
    const DocumentLayout layout = prepared.layout();
    Rc<Image> image             = rcnew Image(Size{ 128, 64 }, ImageFormat::Greyscale_U8Gamma);
    {
        auto pixels = image->mapWrite<ImageFormat::Greyscale_U8Gamma>();
        pixels.clear(Color(30, 34, 38));
    }

    Internal::renderPreparedDocument(image, Point{ 8, 32 }, prepared, layout);

    const auto pixels = image->mapRead<ImageFormat::Greyscale_U8Gamma>();
    size_t nonzero    = 0;
    for (int y = 0; y < pixels.height(); ++y) {
        for (int x = 0; x < pixels.width(); ++x) {
            nonzero += pixels(x, y).grey != 0;
        }
    }
    CHECK(nonzero > 0);

    auto png = pngEncode(image);
    CHECK(writeBytes(PROJECT_BINARY_DIR "/visualTest/text-layout-cpu.png", png));
}

TEST_CASE("TextLayout CPU renderer saves emoji", "[text-layout]") {
    REQUIRE(fonts.has_value());
    const auto registered = fonts->addFontFromFile(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" /
                                                       "NotoColorEmoji-SVG.otf",
                                                   "TextLayoutEmojiPng");
    REQUIRE(registered);

    const PreparedDocument prepared =
        fonts->prepareDocument(Font{ "TextLayoutEmojiPng", 48.f }, TextWithOptions{ U"😀 👑 🌟" });
    const DocumentLayout layout = prepared.layout();
    Rc<Image> image = rcnew Image(Size{ 320, 100 }, ImageFormat::RGBA_U8Gamma, ColorW(Palette::white));
    Internal::renderPreparedDocument(image, Point{ 8, 32 }, prepared, layout);

    CHECK(writeBytes(PROJECT_BINARY_DIR "/visualTest/text-layout-emoji.png", pngEncode(image)));
}

TEST_CASE("TextLayout Canvas renderer", "[text-layout][visual]") {
    registerTextLayoutTestFont();

    const Font font{ "TextLayoutTest", 24.f };
    const PreparedDocument prepared = fonts->prepareDocument(
        font, TextWithOptions{ U"New document layout\nwraps and renders through Canvas" });

    TextLayoutOptions options;
    options.maxLineWidth        = 270.f;
    const DocumentLayout layout = prepared.layout(options);

    renderTest(
        "text-layout-canvas", Size{ 360, 180 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(245, 247, 250));
            canvas.fillRect({ 0, 0, 360, 180 });
            canvas.setFillColor(Palette::black);
            canvas.fillText({ 32, 24 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer styles and selection", "[text-layout][visual]") {
    registerTextLayoutTestFont();

    Font decorated           = Font{ "TextLayoutTest", 24.f };
    decorated.textDecoration = TextDecoration::Underline | TextDecoration::Overline;
    const FontAndColor styles[]{
        { Font{ "TextLayoutTest", 24.f }, Palette::Standard::blue },
        { decorated, Palette::Standard::red },
    };
    const TextWithOptions source{ U"Blue text, decorated red text" };
    const uint32_t styleOffset[]{ 15 };
    const PreparedDocument prepared = fonts->prepareDocument(source, styles, styleOffset);
    TextLayoutOptions options;
    options.maxLineWidth        = 320.f;
    const DocumentLayout layout = prepared.layout(options);

    renderTest(
        "text-layout-canvas-styles", Size{ 400, 140 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(255, 255, 255));
            canvas.fillRect({ 0, 0, 400, 140 });
            canvas.setFillColor(Color(225, 230, 240));
            canvas.fillTextSelection({ 32, 24 }, layout, { 5, 24 });
            canvas.fillText({ 32, 24 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer bidirectional text", "[text-layout][visual][bidi]") {
    registerTextLayoutVisualFonts();

    const Font font{ "TextLayoutNoto", 26.f };
    const PreparedDocument prepared = fonts->prepareDocument(
        font, TextWithOptions{ U"English שלום עולם — العربية مرحبًا بالعالم\nLTR 123 אבג 456 RTL" });
    TextLayoutOptions options;
    options.maxLineWidth        = 520.f;
    const DocumentLayout layout = prepared.layout(options);

    renderTest(
        "text-layout-bidi", Size{ 600, 150 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(248, 249, 252));
            canvas.fillRect({ 0, 0, 600, 150 });
            canvas.setFillColor(Palette::black);
            canvas.fillText({ 28, 26 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer wraps bidirectional text", "[text-layout][visual][bidi]") {
    registerTextLayoutVisualFonts();

    const Font font{ "TextLayoutNoto", 24.f };
    const PreparedDocument prepared = fonts->prepareDocument(
        font, TextWithOptions{ U"هذه جملة عربية طويلة للاختبار مع English words بين النصوص العربية" });
    TextLayoutOptions options;
    options.maxLineWidth        = 270.f;
    const DocumentLayout layout = prepared.layout(options);

    renderTest(
        "text-layout-bidi-wrap", Size{ 340, 260 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(255, 255, 255));
            canvas.fillRect({ 0, 0, 340, 260 });
            canvas.setFillColor(Palette::black);
            canvas.fillText({ 24, 24 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer supports monospace", "[text-layout][visual][fonts]") {
    registerTextLayoutVisualFonts();
    const FontAndColor styles[]{
        { Font{ "TextLayoutMono", 23.f }, Palette::black },
    };
    const TextWithOptions source{
        U" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
    };
    const PreparedDocument prepared = fonts->prepareDocument(source, std::span{ styles }, {});
    const TextLayoutOptions options{ .maxLineWidth = 600.f - 48.f, .allowBreakAnywhere = true };
    const DocumentLayout layout = prepared.layout(options);
    REQUIRE(layout.lineCount() == 3);
    REQUIRE(layout.bounds().area() > 500.f);

    renderTest(
        "text-layout-monospace", Size{ 600, 120 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(250, 250, 250));
            canvas.fillRect({ 0, 0, 600, 120 });
            canvas.fillText({ 24, 4 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer supports multiple fonts", "[text-layout][visual][fonts]") {
    registerTextLayoutVisualFonts();
    const auto registered = fonts->addFontFromFile(
        fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "NotoColorEmoji-SVG.otf", "TextLayoutEmoji");
    REQUIRE(registered);

    const FontAndColor styles[]{
        { Font{ "TextLayoutTest", 25.f }, Palette::Standard::blue },
        { Font{ "TextLayoutMono", 23.f }, Palette::Standard::red },
        { Font{ "TextLayoutNoto", 25.f }, Palette::Standard::green },
        { Font{ "TextLayoutEmoji", 25.f }, Palette::Standard::green },
    };
    const TextWithOptions source{ U"Lato text | monospaced text | שלום עולם | 😀 👑 🌟" };
    const uint32_t offsets[]{ 11, 30, 42 };
    const PreparedDocument prepared = fonts->prepareDocument(source, std::span{ styles }, offsets);
    const DocumentLayout layout     = prepared.layout();

    renderTest(
        "text-layout-multiple-fonts", Size{ 720, 120 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(250, 250, 250));
            canvas.fillRect({ 0, 0, 720, 120 });
            canvas.fillText({ 24, 28 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer draws multiline selection", "[text-layout][visual][selection]") {
    registerTextLayoutTestFont();

    const Font font{ "TextLayoutTest", 24.f };
    const TextWithOptions source{ U"First line of selectable text\nSecond line is selected\nThird line" };
    const PreparedDocument prepared = fonts->prepareDocument(font, source);
    TextLayoutOptions options;
    options.maxLineWidth        = 300.f;
    const DocumentLayout layout = prepared.layout(options);

    renderTest(
        "text-layout-selection-multiline", Size{ 380, 180 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(255, 255, 255));
            canvas.fillRect({ 0, 0, 380, 180 });
            canvas.setFillColor(Color(180, 210, 255));
            canvas.fillTextSelection({ 24, 24 }, layout, { 6, 56 });
            canvas.setFillColor(Palette::black);
            canvas.fillText({ 24, 24 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

TEST_CASE("TextLayout Canvas renderer draws ligatures", "[text-layout][visual][ligatures]") {
    registerTextLayoutVisualFonts();

    Font font{ "TextLayoutTest", 30.f };
    font.features = { OpenTypeFeatureFlag{ OpenTypeFeature::liga, true } };
    const PreparedDocument prepared =
        fonts->prepareDocument(font, TextWithOptions{ U"fi  fl  ffi  ffl  office  affine  official" });
    const DocumentLayout layout = prepared.layout();

    renderTest(
        "text-layout-ligatures", Size{ 620, 100 },
        [&](RenderContext& context) {
            Canvas canvas(context);
            canvas.setFillColor(Color(245, 247, 250));
            canvas.fillRect({ 0, 0, 620, 100 });
            canvas.setFillColor(Palette::black);
            canvas.fillText({ 20, 28 }, layout);
        },
        ColorF{ 1.f, 1.f });
}

} // namespace Brisk
