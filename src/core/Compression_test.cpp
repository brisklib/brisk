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
#include <brisk/core/Compression.hpp>
#include <brisk/core/Reflection.hpp>
#include "test/LoremIpsum.hpp"
#include <brisk/core/Utilities.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

void testCompression(CompressionMethod method, CompressionLevel level, const std::string& ext,
                     size_t readSize, size_t writeSize, size_t batchSize) {
    INFO("method=" << fmt::to_string(method));
    INFO("readSize=" << readSize);
    INFO("writeSize=" << writeSize);
    INFO("batchSize=" << batchSize);
    INFO("level=" << static_cast<int>(level));

    fs::path loremIpsumText   = tempFilePath("compression????????.txt");
    fs::path loremIpsumComp   = tempFilePath("compression????????." + ext);
    fs::path loremIpsumUncomp = tempFilePath("compression????????_r.txt");
    SCOPE_EXIT {
        std::error_code ec;
        fs::remove(loremIpsumText, ec);
        fs::remove(loremIpsumComp, ec);
        fs::remove(loremIpsumUncomp, ec);
    };

    Internal::compressionBatchSize = batchSize;

    CHECK(toStringView(compressionDecode(
              method, compressionEncode(method, toBytesView(loremIpsum), level))) == loremIpsum);

    REQUIRE(writeUtf8(loremIpsumText, loremIpsum).has_value());

    // Compression
    if (auto rd = openFileForReading(loremIpsumText)) {
        if (auto wr = openFileForWriting(loremIpsumComp)) {
            auto gz     = compressionEncoder(method, *wr, level);
            std::ignore = writeFromReader(gz, *rd, writeSize);
        } else {
            FAIL("Cannot open compressed file for writing");
        }
    } else {
        FAIL("Cannot open file for reading");
    }

    // Decompression
    if (auto rd = openFileForReading(loremIpsumComp)) {
        fmt::println("Compression ratio [{:4s}:{}]: {:5.1f}% {} -> {}", ext, static_cast<int>(level),
                     100.0 * (*rd)->size() / toBytesView(loremIpsum).size_bytes(),
                     toBytesView(loremIpsum).size_bytes(), (*rd)->size());
        if (auto wr = openFileForWriting(loremIpsumUncomp)) {
            auto gz     = compressionDecoder(method, *rd);
            std::ignore = writeFromReader(*wr, gz, readSize);
        } else {
            FAIL("Cannot open file for writing");
        }
    } else {
        FAIL("Cannot open compressed file for reading");
    }

    CHECK(readUtf8(loremIpsumUncomp) == loremIpsum);
}

void testCompression(CompressionMethod method, const std::string& ext) {
    for (CompressionLevel level : {
             CompressionLevel::Lowest,
             CompressionLevel::Normal,
             CompressionLevel::Highest,
         }) {
        testCompression(method, level, ext, 1, 1, 256);
        testCompression(method, level, ext, 17, 11, 256);
        testCompression(method, level, ext, 125, 277, 4096);
        testCompression(method, level, ext, 277, 125, 4096);
        testCompression(method, level, ext, 1, 125, 4096);
        testCompression(method, level, ext, 125, 1, 4096);
        testCompression(method, level, ext, 4096, 2048, 65536);
        testCompression(method, level, ext, 2048, 4096, 65536);
        testCompression(method, level, ext, 1, 2048, 65536);
        testCompression(method, level, ext, 2048, 1, 65536);
        testCompression(method, level, ext, 65536, 65536, 65536);
    }
}

TEST_CASE("gzip") {
    testCompression(CompressionMethod::GZip, "gz");
}

TEST_CASE("zlib") {
    testCompression(CompressionMethod::ZLib, "zlib");
}

TEST_CASE("lz4") {
    testCompression(CompressionMethod::LZ4, "lz4");
}

#ifdef BRISK_HAVE_BROTLI
TEST_CASE("brotli") {
    testCompression(CompressionMethod::Brotli, "br");
}
#endif
} // namespace Brisk
