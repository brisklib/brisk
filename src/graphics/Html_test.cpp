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
#include <catch2/catch_all.hpp>

#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/graphics/Html.hpp>

#include "Catch2Utils.hpp"

namespace Brisk {

TEST_CASE("HtmlParser") {
    using namespace Internal;
    std::optional<std::pair<std::u32string, RichText>> rich;

    rich = RichText::fromHtml("abc");
    REQUIRE(rich);
    CHECK(rich->first == U"abc");

    rich = RichText::fromHtml("");
    REQUIRE(rich);
    CHECK(rich->first.empty());
    CHECK(rich->second.fonts.empty());
    CHECK(rich->second.offsets.empty());
    CHECK(rich->second.flags.empty());

    CHECK(RichText::fromHtml("   ")->first == U"   ");
    CHECK(RichText::fromHtml("\t\n\r")->first == U"\t\n\r");

    rich = RichText::fromHtml("<br/>");
    REQUIRE(rich);

    rich = RichText::fromHtml("<b >B</b >");
    REQUIRE(rich);
    CHECK(rich->first == U"B");
    CHECK(rich->second.flags[0] == FontFormatFlags::Weight);

    rich = RichText::fromHtml("<br />");
    REQUIRE(rich);
    CHECK(rich->first == U"\n");

    CHECK_FALSE(RichText::fromHtml("< b >B< / b >"));
    CHECK_FALSE(RichText::fromHtml("<br / >"));

    rich = RichText::fromHtml("<math>x&gt;y</math>");
    REQUIRE(rich);
    CHECK(rich->first == U"x>y");

    rich = RichText::fromHtml("abc<br/>def &#40;&#x40;");
    REQUIRE(rich);
    CHECK(rich->first == U"abc\ndef (@");

    rich = RichText::fromHtml("&nbsp;");
    REQUIRE(rich);
    CHECK(rich->first == U"\u00a0");

    CHECK(RichText::fromHtml("&amp;&apos;&gt;&lt;&quot;")->first == U"&'><\"");
    CHECK(RichText::fromHtml("&#0;")->first == std::u32string{ U'\0' });
    CHECK(RichText::fromHtml("&#x10FFFF;")->first == U"\U0010FFFF");
    CHECK_FALSE(RichText::fromHtml("&unknown;"));
    CHECK_FALSE(RichText::fromHtml("&#x;"));
    CHECK_FALSE(RichText::fromHtml("&#12x;"));

    rich = RichText::fromHtml("<em>abcdef</em>");
    REQUIRE(rich);
    CHECK(rich->first == U"abcdef");

    rich = RichText::fromHtml("<font color=\"brown\">abcdef</font>");
    REQUIRE(rich);

    for (int value = 1; value <= 256; ++value) {
        rich = RichText::fromHtml("<font size=\"" + std::to_string(value) + "\">x</font>");
        REQUIRE(rich);
        REQUIRE(rich->second.fonts.size() == 1);
        CHECK(rich->second.fonts[0].font.fontSize == value);
        CHECK(rich->second.flags[0] == FontFormatFlags::Size);

        Font base;
        base.fontSize = 20.f;
        rich->second.setBaseFont(base);
        CHECK(rich->second.fonts[0].font.fontSize == value);
    }

    rich = RichText::fromHtml("<font size=\"0\">x</font>");
    REQUIRE(rich);
    CHECK(rich->second.flags[0] == FontFormatFlags::None);
    rich = RichText::fromHtml("<font size=\"257\">x</font>");
    REQUIRE(rich);
    CHECK(rich->second.flags[0] == FontFormatFlags::None);

    const float savedPixelRatio = pixelRatio();
    pixelRatio()                = 2.f;
    rich                        = RichText::fromHtml("<font size=\"24\">x</font>");
    REQUIRE(rich);
    CHECK(rich->second.fonts[0].font.fontSize == 24.f);
    pixelRatio() = savedPixelRatio;

    rich         = RichText::fromHtml("<tag attr=unquoted-value></tag>");
    REQUIRE(rich);
    rich = RichText::fromHtml("<tag attr='quoted-value'></tag>");
    REQUIRE(rich);
    rich = RichText::fromHtml("<tag attr=\"quoted-value\"></tag>");
    REQUIRE(rich);

    rich = RichText::fromHtml("<tag attr=abc&quot;def></tag>");
    REQUIRE(rich);
    rich = RichText::fromHtml("<tag attr='abc&quot;def'></tag>");
    REQUIRE(rich);
    rich = RichText::fromHtml("<tag attr=\"abc&quot;def\"></tag>");
    REQUIRE(rich);

    rich = RichText::fromHtml("<b>bold <i>bold italic</i></b>");
    REQUIRE(rich);
    CHECK(rich->first == U"bold bold italic");
    CHECK(rich->second.offsets == std::vector{ 5u });
    CHECK(rich->second.fonts[0].font.weight == FontWeight::Bold);
    CHECK(rich->second.fonts[0].font.style == FontStyle::Normal);
    CHECK(rich->second.flags[0] == FontFormatFlags::Weight);
    CHECK(rich->second.fonts[1].font.weight == FontWeight::Bold);
    CHECK(rich->second.fonts[1].font.style == FontStyle::Italic);
    CHECK(rich->second.flags[1] == (FontFormatFlags::Style | FontFormatFlags::Weight));

    CHECK_FALSE(RichText::fromHtml("<b>x</i>"));
    CHECK_FALSE(RichText::fromHtml("<b><i>x</b></i>"));
    CHECK_FALSE(RichText::fromHtml("<b>x"));
    CHECK_FALSE(RichText::fromHtml("</b>"));
    rich = RichText::fromHtml("<b></b>");
    REQUIRE(rich);
    CHECK(rich->first.empty());
    CHECK_FALSE(RichText::fromHtml("&#xD800;"));
    CHECK_FALSE(RichText::fromHtml("&#x110000;"));
    CHECK_FALSE(RichText::fromHtml("&#4294967296;"));

    std::string_view empty;
    CHECK(parseHtml(empty, nullptr) == false);

    struct EmptySax final : HtmlSax {
    } sax;

    CHECK(parseHtml(empty, &sax));

    rich = RichText::fromHtml("The <b>quick</b> <font color=\"brown\">brown</font> <u>fox<br/>jumps</u> over "
                              "the <small>lazy</small> dog");
    REQUIRE(rich);
    CHECK(rich->first == U"The quick brown fox\njumps over the lazy dog");
    CHECK(rich->second.offsets == std::vector{ 4u, 9u, 10u, 15u, 16u, 25u, 35u, 39u });
    CHECK(rich->second.flags[0] == FontFormatFlags::None);
    CHECK(rich->second.flags[1] == FontFormatFlags::Weight);
    CHECK(rich->second.flags[2] == FontFormatFlags::None);
    CHECK(rich->second.flags[3] == FontFormatFlags::None);
    CHECK(rich->second.flags[4] == FontFormatFlags::None);
    CHECK(rich->second.flags[5] == FontFormatFlags::TextDecoration);
    CHECK(rich->second.flags[6] == FontFormatFlags::None);
    CHECK(rich->second.flags[7] == (FontFormatFlags::Size | FontFormatFlags::SizeIsRelative));
    CHECK(rich->second.flags[8] == FontFormatFlags::None);

    rich = RichText::fromHtml(
        "<big>BIG</big><small>SMALL</small><big><big>BIGGER</big></big><big><small>NORMAL</small></big>");
    REQUIRE(rich);
    CHECK(rich->first == U"BIGSMALLBIGGERNORMAL");
    CHECK(rich->second.offsets == std::vector{ 3u, 8u, 14u });
    CHECK(rich->second.fonts[0].font.fontSize == 2);
    CHECK(rich->second.flags[0] == (FontFormatFlags::Size | FontFormatFlags::SizeIsRelative));
    CHECK(rich->second.fonts[1].font.fontSize == 0.5f);
    CHECK(rich->second.flags[1] == (FontFormatFlags::Size | FontFormatFlags::SizeIsRelative));
    CHECK(rich->second.fonts[2].font.fontSize == 4);
    CHECK(rich->second.flags[2] == (FontFormatFlags::Size | FontFormatFlags::SizeIsRelative));
    CHECK(rich->second.fonts[3].font.fontSize == 1);
    CHECK(rich->second.flags[3] == (FontFormatFlags::Size | FontFormatFlags::SizeIsRelative));

    CHECK(parseHtmlColor("#F30") == 0xFF3300_rgb);
    CHECK(parseHtmlColor("#FE3811") == 0xFE3811_rgb);
    CHECK(parseHtmlColor("#F307") == 0xFF330077_rgba);
    CHECK(parseHtmlColor("#FE381176") == 0xFE381176_rgba);
    CHECK(parseHtmlColor("black") == 0x000000_rgb);
    CHECK(parseHtmlColor("transparent") == 0x00000000_rgba);
    CHECK(parseHtmlColor("peachpuff") == 0xffdab9_rgb);
}
} // namespace Brisk
