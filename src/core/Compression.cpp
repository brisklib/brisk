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
#include <brisk/core/Compression.hpp>

namespace Brisk {

[[nodiscard]] Rc<Stream> compressionDecoder(CompressionMethod method, Rc<Stream> reader) {
    switch (method) {
    case CompressionMethod::None:
        return reader;
    case CompressionMethod::GZip:
        return gzipDecoder(std::move(reader));
    case CompressionMethod::ZLib:
        return zlibDecoder(std::move(reader));
    case CompressionMethod::LZ4:
        return lz4Decoder(std::move(reader));
#ifdef BRISK_HAVE_BROTLI
    case CompressionMethod::Brotli:
        return brotliDecoder(std::move(reader));
#endif
    default:
        BRISK_UNREACHABLE();
    }
}

[[nodiscard]] Rc<Stream> compressionEncoder(CompressionMethod method, Rc<Stream> writer,
                                            CompressionLevel level) {
    switch (method) {
    case CompressionMethod::None:
        return writer;
    case CompressionMethod::GZip:
        return gzipEncoder(std::move(writer), level);
    case CompressionMethod::ZLib:
        return zlibEncoder(std::move(writer), level);
    case CompressionMethod::LZ4:
        return lz4Encoder(std::move(writer), level);
#ifdef BRISK_HAVE_BROTLI
    case CompressionMethod::Brotli:
        return brotliEncoder(std::move(writer), level);
#endif
    default:
        BRISK_UNREACHABLE();
    }
}

[[nodiscard]] Bytes compressionEncode(CompressionMethod method, BytesView data, CompressionLevel level) {
    switch (method) {
    case CompressionMethod::None:
        return Bytes(data.begin(), data.end());
    case CompressionMethod::GZip:
        return gzipEncode(data, level);
    case CompressionMethod::ZLib:
        return zlibEncode(data, level);
    case CompressionMethod::LZ4:
        return lz4Encode(data, level);
#ifdef BRISK_HAVE_BROTLI
    case CompressionMethod::Brotli:
        return brotliEncode(data, level);
#endif
    default:
        BRISK_UNREACHABLE();
    }
}

[[nodiscard]] Bytes compressionDecode(CompressionMethod method, BytesView data) {
    switch (method) {
    case CompressionMethod::None:
        return Bytes(data.begin(), data.end());
    case CompressionMethod::GZip:
        return gzipDecode(data);
    case CompressionMethod::ZLib:
        return zlibDecode(data);
    case CompressionMethod::LZ4:
        return lz4Decode(data);
#ifdef BRISK_HAVE_BROTLI
    case CompressionMethod::Brotli:
        return brotliDecode(data);
#endif
    default:
        BRISK_UNREACHABLE();
    }
}

size_t Internal::compressionBatchSize = 4096;

} // namespace Brisk
