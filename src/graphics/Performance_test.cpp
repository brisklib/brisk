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
 * Brisk is dual-licensed under the GNU General Public License version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <catch2/catch_all.hpp>

#include <brisk/graphics/Fonts.hpp>
#include <brisk/graphics/Html.hpp>
#include <brisk/graphics/Path.hpp>

#include "Mask.hpp"

namespace Brisk {

namespace {

void registerPerformanceFonts() {
    static bool registered = false;
    if (registered) {
        return;
    }

    REQUIRE(fonts.has_value());
    REQUIRE(fonts
                ->addFontFromFile(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" / "Lato-Medium.ttf",
                                  "PerformanceLato")
                .has_value());
    REQUIRE(fonts
                ->addFontFromFile(fs::path(PROJECT_SOURCE_DIR) / "resources" / "fonts" /
                                      "GoNotoKurrent-Regular.ttf",
                                  "PerformanceNoto")
                .has_value());
    registered = true;
}

Path makeSmallRasterPath() {
    Path path;
    path.addCircle(32.f, 32.f, 24.f);
    return path;
}

Path makeLargeRasterPath() {
    Path path;
    path.addPolygon(1024.f, 900.f, 0.f, 0.f, 900.f, 900.f);
    return path;
}

Path makeComplexRasterPath() {
    Path path;
    for (int i = 0; i < 48; ++i) {
        const float x = 8.f + i * 18.f;
        path.moveTo(x, 8.f);
        path.cubicTo(x + 4.f, 64.f, x + 14.f, -48.f, x + 18.f, 8.f);
        path.cubicTo(x + 14.f, 64.f, x + 4.f, -48.f, x, 8.f);
        path.close();
    }
    return path;
}

} // namespace

TEST_CASE("Performance: text shaping", "[performance][cpu][text]") {
    registerPerformanceFonts();

    const Font latinFont{ "PerformanceLato", 24.f };
    const Font multilingualFont{ "PerformanceNoto", 24.f };

    const TextWithOptions shortText{ U"The quick brown fox" };

    std::u32string longSource;
    for (int i = 0; i < 32; ++i) {
        longSource += U"The quick brown fox jumps over the lazy dog. ";
    }
    const TextWithOptions longText{ std::move(longSource) };

    const TextWithOptions multilingualText{
        U"English Ελληνικά Кириллица العربية עברית हिन्दी 中文 日本語 한국어"
    };

    BENCHMARK("shape short text") {
        return fonts->shapeText(latinFont, shortText);
    };

    BENCHMARK("shape long text") {
        return fonts->shapeText(latinFont, longText);
    };

    BENCHMARK("shape multilingual text") {
        return fonts->shapeText(multilingualFont, multilingualText);
    };
}

TEST_CASE("Performance: text layout", "[performance][cpu][text-layout]") {
    registerPerformanceFonts();

    const Font font{ "PerformanceLato", 24.f };
    std::u32string source;
    for (int i = 0; i < 20; ++i) {
        source += U"One two three four five six seven eight nine ten. ";
    }

    const ShapedText shapedText = fonts->shapeText(font, TextWithOptions{ std::move(source) });
    TextLayoutOptions options;
    options.maxLineWidth = 320.f;
    options.alignment    = TextLayoutAlignment::Left;

    BENCHMARK("layout wrapped text") {
        return shapedText.layout(options);
    };
}

TEST_CASE("Performance: path rasterization", "[performance][cpu][path]") {
    const Path smallPath   = makeSmallRasterPath();
    const Path largePath   = makeLargeRasterPath();
    const Path complexPath = makeComplexRasterPath();

    BENCHMARK("rasterize small path") {
        return PreparedPath(smallPath, FillParams{}, noClipRect, false);
    };

    BENCHMARK("rasterize large path") {
        return PreparedPath(largePath, FillParams{}, noClipRect, false);
    };

    BENCHMARK("rasterize complex path") {
        return PreparedPath(complexPath, FillParams{}, noClipRect, false);
    };
}

TEST_CASE("Performance: path stroking", "[performance][cpu][path][stroker]") {
    const Path smallPath   = makeSmallRasterPath();
    const Path largePath   = makeLargeRasterPath();
    const Path complexPath = makeComplexRasterPath();

    const StrokeParams smallStroke{ .joinStyle = JoinStyle::Round,
                                    .capStyle = CapStyle::Round,
                                    .strokeWidth = 4.f };
    const StrokeParams largeStroke{ .joinStyle = JoinStyle::Miter,
                                    .capStyle = CapStyle::Flat,
                                    .strokeWidth = 12.f };
    const StrokeParams complexStroke{ .joinStyle = JoinStyle::Bevel,
                                      .capStyle = CapStyle::Square,
                                      .strokeWidth = 6.f };

    BENCHMARK("stroke small path") {
        return smallPath.stroke(smallStroke);
    };

    BENCHMARK("stroke large path") {
        return largePath.stroke(largeStroke);
    };

    BENCHMARK("stroke complex path") {
        return complexPath.stroke(complexStroke);
    };
}

TEST_CASE("Performance: HTML parsing", "[performance][cpu][html]") {
    constexpr std::string_view html = "The <b>quick</b> <font color=\"brown\">brown</font> "
                                      "<u>fox<br/>jumps</u> over the <small>lazy</small> dog. "
                                      "<p>Entities: &amp; &lt; &gt; &#x1F600;</p>";

    BENCHMARK("parse HTML rich text") {
        return Internal::RichText::fromHtml(html);
    };

    struct NoopSax final : HtmlSax {
    } sax;

    BENCHMARK("parse HTML SAX") {
        return parseHtml(html, &sax);
    };
}

} // namespace Brisk
