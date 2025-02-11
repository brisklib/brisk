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
#include <brisk/core/Stream.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

static std::string_view pangram = "The quick brown fox jumps over the lazy dog";

static std::byte buf[1024];

TEST_CASE("MemoryStream") {
    RC<Stream> m = rcnew MemoryStream();
    CHECK(m->size() == 0);
    CHECK(m->tell() == 0);

    SECTION("read") {
        CHECK(m->read(buf, 100) == Transferred::Eof);
    }
    SECTION("write & read") {
        CHECK(m->write("abcdef") == 6);
        CHECK(m->size() == 6);
        CHECK(m->tell() == 6);
        CHECK(m->seek(0) == true);
        CHECK(m->read(buf, 100) == 6);
        CHECK(std::string_view(reinterpret_cast<char*>(buf), 6) == "abcdef");
        CHECK(m->read(buf, 100) == Transferred::Eof);
        CHECK(m->readUntilEnd() == Bytes{});
        CHECK(m->seek(0) == true);
        CHECK(m->readUntilEnd(true) == toBytes(std::vector<char>{ 'a', 'b', 'c', 'd', 'e', 'f' }));
    }
}

TEST_CASE("SpanStream") {
    RC<Stream> m = rcnew ByteViewStream(toBytesView(pangram));
    CHECK(m->size() == 43);
    CHECK(m->tell() == 0);
    CHECK_THROWS_AS(m->write(buf, 1), ENotImplemented);

    SECTION("read") {
        CHECK(m->read(buf, 100) == 43);
        CHECK(BytesView(buf, 43) == toBytesView(pangram));
    }
}
} // namespace Brisk
