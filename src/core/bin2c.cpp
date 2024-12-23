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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <brisk/core/Compression.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/Embed.hpp>

// bin2c output.c input.bin
// bin2c --br output.c input.bin
// bin2c --gz output.c input.bin
// bin2c --lz4 output.c input.bin

inline void shift(int& argc, const char**& argv) {
    ++argv;
    --argc;
}

namespace Brisk {

using namespace std::string_view_literals;

constexpr std::u32string_view validChars = U"abcdefghijklmnopqrstuvwxyz0123456789_";

std::string cIdent(const std::string& s) {
    std::string id = asciiTransform(lowerCase(s), [](char32_t ch) {
        return validChars.find(ch) == std::u32string_view::npos ? U'_' : ch;
    });
    if ((id.front() >= '0' && id.front() <= '9') || id.front() == '_')
        id = "rsrc_" + id;
    return id;
}

class CWriter final : public Writer {
public:
    CWriter(RC<Stream> dataWriter, RC<Stream> headerWriter, EmbeddedResourceFlags flags, std::string ident)
        : dataWriter(std::move(dataWriter)), headerWriter(std::move(headerWriter)), ident(std::move(ident)),
          flags(flags) {

        std::ignore = this->dataWriter->write(
            R"(/* Autogenerated by bin2c */
#include <stdint.h>

#ifndef __INTELLISENSE__
#ifdef __cplusplus
extern "C" {
#endif
)");

        std::ignore = this->dataWriter->write(fmt::format(R"(const uint8_t {0}[] = {{
)",
                                                          this->ident));
    }

    Transferred write(const uint8_t* data, size_t size) final {
        for (size_t i = 0; i < size; ++i) {
            bool firstOnLine = numWritten % 16 == 0;
            if (numWritten && firstOnLine) {
                if (this->dataWriter->write(",\n").isError())
                    return Transferred::Error;
            }
            if (this->dataWriter
                    ->write(fmt::format(fmt::runtime(firstOnLine ? "0x{:02X}" : ",0x{:02X}"), data[i]))
                    .isError()) {
                return Transferred::Error;
            }
            ++numWritten;
        }
        return size;
    }

    bool flush() final {
        std::ignore = this->dataWriter->write(fmt::format(R"(}};
#ifdef __cplusplus
}}
#endif
#endif
)"));

        std::ignore = this->headerWriter->write(fmt::format(R"(/* Autogenerated by bin2c */
#pragma once
#include <brisk/core/Embed.hpp>
namespace Brisk {{
namespace Internal {{
    extern "C" uint8_t {0}[];
}}
inline const Bytes& {0}() {{
    const size_t size = {1};
    constexpr EmbeddedResourceFlags flags = static_cast<EmbeddedResourceFlags>({2});
    static const Bytes cached = loadResource<flags>(bytes_view(Internal::{0}, size));
    return cached;
}}
}} // namespace Brisk

)",
                                                            ident, numWritten, +flags));

        fmt::println("Output size: {}", numWritten);
        return true;
    }

    RC<Stream> dataWriter;
    RC<Stream> headerWriter;
    std::string ident;
    EmbeddedResourceFlags flags;
    size_t numWritten = 0;
};

CompressionMethod method    = CompressionMethod::None;
CompressionLevel level      = CompressionLevel::High;
EmbeddedResourceFlags flags = EmbeddedResourceFlags::None;
std::string id;

int bin2c(int argc, const char** argv) {

    shift(argc, argv);
    if (argc < 3) {
        fprintf(stderr, "bin2c requires at least three arguments: <datafile> <headerfile> <input file>\n");
        return 1;
    }
    for (;;) {
        if (argv[0] == "--gz"sv) {
            method = CompressionMethod::GZip;
            flags |= EmbeddedResourceFlags::GZip;
            shift(argc, argv);
        } else if (argv[0] == "--br"sv) {
#ifdef BRISK_HAVE_BROTLI
            method = CompressionMethod::Brotli;
            flags |= EmbeddedResourceFlags::Brotli;
#else
            fprintf(stderr, "Brotli support is disabled during the build\n");
            return 1;
#endif
            shift(argc, argv);
        } else if (argv[0] == "--zlib"sv) {
            method = CompressionMethod::ZLib;
            flags |= EmbeddedResourceFlags::ZLib;
            shift(argc, argv);
        } else if (argv[0] == "--lz4"sv) {
            method = CompressionMethod::LZ4;
            flags |= EmbeddedResourceFlags::LZ4;
            shift(argc, argv);
        } else if (argv[0] == "--id"sv) {
            shift(argc, argv);
            if (argc < 2) {
                fprintf(stderr, "--id requires an argument\n");
                return 1;
            }
            id = argv[0];
            shift(argc, argv);
        } else if (argv[0][0] == '-' && "123456789"sv.find(argv[0][1]) != std::string_view::npos) {
            level = static_cast<CompressionLevel>(argv[0][1] - '0');
            shift(argc, argv);
        } else {
            break;
        }
    }

    if (argc != 3) {
        fprintf(stderr,
                "bin2c requires exactly three positional arguments: <datafile> <headerfile> <input file>\n");
        return 1;
    }

    fs::path datafile   = argv[0];
    fs::path headerfile = argv[1];
    fs::path input      = argv[2];

    if (id.empty()) {
        id = cIdent(input.stem().string());
    }

    if (auto rd = openFileForReading(input)) {
        if (auto wr = openFileForWriting(datafile)) {
            if (auto hdr = openFileForWriting(headerfile)) {
                fmt::println("Input size: {}", (*rd)->size());
                RC<Stream> out(new CWriter(std::move(*wr), std::move(*hdr), flags, id));
                switch (method) {
#ifdef BRISK_HAVE_BROTLI
                case CompressionMethod::Brotli:
                    out = brotliEncoder(std::move(out), level);
                    break;
#endif
                case CompressionMethod::GZip:
                    out = gzipEncoder(std::move(out), level);
                    break;
                case CompressionMethod::LZ4:
                    out = lz4Encoder(std::move(out), level);
                    break;
                default:
                    break;
                }
                if (!writeFromReader(out, *rd, 8192)) {
                    fprintf(stderr, "File writing incomplete\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Cannot open the header file for writing\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Cannot open the data file for writing\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Cannot open the input file for reading\n");
        return 1;
    }

    return 0;
}
} // namespace Brisk

int main(int argc, const char** argv) {
    return Brisk::bin2c(argc, argv);
}
