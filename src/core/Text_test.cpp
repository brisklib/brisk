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
#include <fmt/format.h>
#include <string_view>
#include <brisk/core/Text.hpp>
#include "test/HelloWorld.hpp"

using namespace Brisk;
using namespace std::string_view_literals;

TEST_CASE("case") {
    CHECK(lowerCase("") == "");
    CHECK(upperCase("") == "");
    CHECK(lowerCase("abcDeF") == "abcdef");
    CHECK(upperCase("abcDeF") == "ABCDEF");

    for (size_t i = 0; i < std::size(helloWorld); ++i) {
        CHECK(lowerCase(helloWorld[i]) == helloWorld_LC[i]);
        CHECK(upperCase(helloWorld[i]) == helloWorld_UC[i]);
    }
}

TEST_CASE("wordWrap") {
    CHECK(wordWrap("Hello, world!", 13) == "Hello, world!");
    CHECK(wordWrap("Hello, world!", 12) == "Hello,\nworld!");
    CHECK(wordWrap("Hello, world!", 5) == "Hello\n,\nworld\n!");
    CHECK(wordWrap("Hello, world!", 3) == "Hel\nlo,\nwor\nld!");

    CHECK(wordWrap("Hello, world!\nHello, world!", 13) == "Hello, world!\nHello, world!");
    CHECK(wordWrap("Hello, world!\nHello, world!", 12) == "Hello,\nworld!\nHello,\nworld!");
    CHECK(wordWrap("Hello, world!\nHello, world!", 5) == "Hello\n,\nworld\n!\nHello\n,\nworld\n!");
    CHECK(wordWrap("Hello, world!\nHello, world!", 3) == "Hel\nlo,\nwor\nld!\nHel\nlo,\nwor\nld!");
}

TEST_CASE("split") {
    CHECK(split("Hello, world!"sv, ", ") == std::vector<std::string_view>{ "Hello", "world!" });
    CHECK(split(", world!"s, ", ") == std::vector<std::string_view>{ "", "world!" });
    CHECK(split("Hello, ", ", ") == std::vector<std::string_view>{ "Hello", "" });
}

TEST_CASE("join") {
    CHECK(join(std::vector<std::string>{ "A", "B", "C" }, "/") == "A/B/C");
    CHECK(join(std::vector<std::string>{ "A", "B", "C" }, '/') == "A/B/C");
    CHECK(join(std::vector<std::string>{ "", "", "" }, "/") == "//");
}

TEST_CASE("replaceAll") {
    CHECK(replaceAll("Hello, world!", "world", "Martians") == "Hello, Martians!");
}

TEST_CASE("shorten") {
    CHECK(shorten("abcdefghijklmnopqrstuvwxyz", 10, 0.f, U"...") == "...tuvwxyz");
    CHECK(shorten("abcdefghijklmnopqrstuvwxyz", 10, 0.5f, U"...") == "abc...wxyz");
    CHECK(shorten("abcdefghijklmnopqrstuvwxyz", 10, 1.f, U"...") == "abcdefg...");

    CHECK(shorten("abcdefghijklmnopqrstuvwxyz", 3, 0.f, U"...") == "...");
    CHECK(shorten("abcdefghijklmnopqrstuvwxyz", 1, 0.f, U"...") == "...");
}
