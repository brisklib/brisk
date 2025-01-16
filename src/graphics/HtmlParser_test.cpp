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
 */
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

#include <brisk/graphics/Fonts.hpp>

namespace Brisk {

TEST_CASE("HtmlParser") {
    Internal::RichText richText;
    auto html = Internal::parseHtml("abc");
    REQUIRE(html);
    richText = Internal::processHtml(html, Font{});
    CHECK(richText.text == U"abc");

    html = Internal::parseHtml("<math>x&gt;y</math>");
    REQUIRE(html);
    richText = Internal::processHtml(html, Font{});
    CHECK(richText.text == U"x>y");

    html = Internal::parseHtml("abc<br/>def &#40;&#x40;");
    REQUIRE(html);
    richText = Internal::processHtml(html, Font{});
    CHECK(richText.text == U"abc\ndef (@");

    html = Internal::parseHtml("<em>abcdef</em>");
    REQUIRE(html);
    richText = Internal::processHtml(html, Font{});
    CHECK(richText.text == U"abcdef");

    html = Internal::parseHtml("<b>bold <i>bold italic</i></b>");
    REQUIRE(html);
    richText = Internal::processHtml(html, Font{});
    CHECK(richText.text == U"bold bold italic");
    CHECK(richText.offsets == std::vector{ 5u });
    CHECK(richText.fonts[0].font.weight == FontWeight::Bold);
    CHECK(richText.fonts[0].font.style == FontStyle::Normal);
    CHECK(richText.fonts[1].font.weight == FontWeight::Bold);
    CHECK(richText.fonts[1].font.style == FontStyle::Italic);

    CHECK(Internal::parseHtmlColor("#F30") == 0xFF3300_rgb);
    CHECK(Internal::parseHtmlColor("#FE3811") == 0xFE3811_rgb);
    CHECK(Internal::parseHtmlColor("#F307") == 0xFF330077_rgba);
    CHECK(Internal::parseHtmlColor("#FE381176") == 0xFE381176_rgba);
    CHECK(Internal::parseHtmlColor("black") == 0x000000_rgb);
    CHECK(Internal::parseHtmlColor("transparent") == 0x00000000_rgba);
    CHECK(Internal::parseHtmlColor("peachpuff") == 0xffdab9_rgb);
}
} // namespace Brisk
