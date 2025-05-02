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

#include <brisk/core/Hash.hpp>

using namespace Brisk;

TEST_CASE("CRC") {
    CHECK(crc32("") == 0x00000000);
    CHECK(crc32("a") == 0xe8b7be43);
    CHECK(crc32("ab") == 0x9e83486d);
    CHECK(crc32("abc") == 0x352441c2);
    CHECK(crc32("abcd") == 0xed82cd11);
    CHECK(crc32("abcde") == 0x8587d865);
    CHECK(crc32("abcdef") == 0x4b8e39ef);

    struct A {
        char a = 'a';
        char b = 'b';
        char c = 'c';
        char d = 'd';
    };

    CHECK(crc32(asBytesView(A{})) == 0xed82cd11);
}
