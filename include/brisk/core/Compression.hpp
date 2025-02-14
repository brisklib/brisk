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
#pragma once
#include "Bytes.hpp"
#include "IO.hpp"
#include "internal/Span.hpp"

namespace Brisk {

/** @brief Represents different levels of compression.
 *  Each level corresponds to a trade-off between speed and compression ratio.
 */
enum class CompressionLevel {
    Lowest  = 1, ///< Lowest compression, fastest speed.
    Low     = 3, ///< Low compression.
    Normal  = 5, ///< Standard compression level.
    High    = 7, ///< High compression, slower speed.
    Highest = 9, ///< Highest compression, slowest speed.
};

/** @brief Represents different compression methods available. */
enum class CompressionMethod {
    None = 0, ///< No compression.
    GZip = 1, ///< GZip compression.
    ZLib = 2, ///< ZLib compression.
    LZ4  = 3, ///< LZ4 compression.
#ifdef BRISK_HAVE_BROTLI
    Brotli = 4, ///< Brotli compression (enabled by BRISK_BROTLI cmake option).
#endif
};

/** @brief Default mapping between compression method names and their enum values. */
template <>
inline constexpr std::initializer_list<NameValuePair<CompressionMethod>> defaultNames<CompressionMethod>{
    { "None", CompressionMethod::None },     { "GZip", CompressionMethod::GZip },
    { "ZLib", CompressionMethod::ZLib },     { "LZ4", CompressionMethod::LZ4 },
#ifdef BRISK_HAVE_BROTLI
    { "Brotli", CompressionMethod::Brotli },
#endif
};

/** @brief Creates a Stream that reads GZip-compressed data from a source Stream.
 *  @param reader The source Stream containing compressed data.
 *  @return A Stream for reading the decompressed data, which can be processed on-the-fly in a streaming
 * manner.
 */
[[nodiscard]] RC<Stream> gzipDecoder(RC<Stream> reader);

/** @brief Creates a Stream that writes GZip-compressed data to a target Stream.
 *  @param writer The target Stream for writing compressed data.
 *  @param level The level of compression to apply.
 *  @return A Stream for writing the compressed data, which can be processed on-the-fly in a streaming manner.
 */
[[nodiscard]] RC<Stream> gzipEncoder(RC<Stream> writer, CompressionLevel level);

/** @brief Creates a Stream that reads ZLib-compressed data from a source Stream.
 *  @param reader The source Stream containing compressed data.
 *  @return A Stream for reading the decompressed data, which can be processed on-the-fly in a streaming
 * manner.
 */
[[nodiscard]] RC<Stream> zlibDecoder(RC<Stream> reader);

/** @brief Creates a Stream that writes ZLib-compressed data to a target Stream.
 *  @param writer The target Stream for writing compressed data.
 *  @param level The level of compression to apply.
 *  @return A Stream for writing the compressed data, which can be processed on-the-fly in a streaming manner.
 */
[[nodiscard]] RC<Stream> zlibEncoder(RC<Stream> writer, CompressionLevel level);

/** @brief Creates a Stream that reads LZ4-compressed data from a source Stream.
 *  @param reader The source Stream containing compressed data.
 *  @return A Stream for reading the decompressed data, which can be processed on-the-fly in a streaming
 * manner.
 */
[[nodiscard]] RC<Stream> lz4Decoder(RC<Stream> reader);

/** @brief Creates a Stream that writes LZ4-compressed data to a target Stream.
 *  @param writer The target Stream for writing compressed data.
 *  @param level The level of compression to apply.
 *  @return A Stream for writing the compressed data, which can be processed on-the-fly in a streaming manner.
 */
[[nodiscard]] RC<Stream> lz4Encoder(RC<Stream> writer, CompressionLevel level);

#ifdef BRISK_HAVE_BROTLI
/** @brief Creates a Stream that reads Brotli-compressed data from a source Stream.
 *  @param reader The source Stream containing compressed data.
 *  @return A Stream for reading the decompressed data, which can be processed on-the-fly in a streaming
 * manner.
 *  @note Available only if compiled with BRISK_BROTLI option enabled.
 */
[[nodiscard]] RC<Stream> brotliDecoder(RC<Stream> reader);

/** @brief Creates a Stream that writes Brotli-compressed data to a target Stream.
 *  @param writer The target Stream for writing compressed data.
 *  @param level The level of compression to apply.
 *  @return A Stream for writing the compressed data, which can be processed on-the-fly in a streaming manner.
 *  @note Available only if compiled with BRISK_BROTLI option enabled.
 */
[[nodiscard]] RC<Stream> brotliEncoder(RC<Stream> writer, CompressionLevel level);
#endif

/** @brief Compresses data using GZip.
 *  @param data The input data to compress.
 *  @param level The level of compression to apply.
 *  @return A vector of bytes containing the compressed data.
 */
[[nodiscard]] Bytes gzipEncode(BytesView data, CompressionLevel level);

/** @brief Decompresses GZip-compressed data.
 *  @param data The GZip-compressed input data.
 *  @return A vector of bytes containing the decompressed data.
 */
[[nodiscard]] Bytes gzipDecode(BytesView data);

/** @brief Compresses data using ZLib.
 *  @param data The input data to compress.
 *  @param level The level of compression to apply.
 *  @return A vector of bytes containing the compressed data.
 */
[[nodiscard]] Bytes zlibEncode(BytesView data, CompressionLevel level);

/** @brief Decompresses ZLib-compressed data.
 *  @param data The ZLib-compressed input data.
 *  @return A vector of bytes containing the decompressed data.
 */
[[nodiscard]] Bytes zlibDecode(BytesView data);

/** @brief Compresses data using LZ4.
 *  @param data The input data to compress.
 *  @param level The level of compression to apply.
 *  @return A vector of bytes containing the compressed data.
 */
[[nodiscard]] Bytes lz4Encode(BytesView data, CompressionLevel level);

/** @brief Decompresses LZ4-compressed data.
 *  @param data The LZ4-compressed input data.
 *  @return A vector of bytes containing the decompressed data.
 */
[[nodiscard]] Bytes lz4Decode(BytesView data);

#ifdef BRISK_HAVE_BROTLI
/** @brief Compresses data using Brotli.
 *  @param data The input data to compress.
 *  @param level The level of compression to apply.
 *  @return A vector of bytes containing the compressed data.
 *  @note Available only if compiled with BRISK_BROTLI option enabled.
 */
[[nodiscard]] Bytes brotliEncode(BytesView data, CompressionLevel level);

/** @brief Decompresses Brotli-compressed data.
 *  @param data The Brotli-compressed input data.
 *  @return A vector of bytes containing the decompressed data.
 *  @note Available only if compiled with BRISK_BROTLI option enabled.
 */
[[nodiscard]] Bytes brotliDecode(BytesView data);
#endif

/** @brief Creates a Stream that reads data compressed with a specified compression method.
 *  @param method The compression method to use for decompression.
 *  @param reader The source Stream containing compressed data.
 *  @return A Stream for reading the decompressed data.
 *  @note If CompressionMethod::None is used, no decompression is applied.
 */
[[nodiscard]] RC<Stream> compressionDecoder(CompressionMethod method, RC<Stream> reader);

/** @brief Creates a Stream that writes data using a specified compression method.
 *  @param method The compression method to use for compression.
 *  @param writer The target Stream for writing compressed data.
 *  @param level The level of compression to apply.
 *  @return A Stream for writing the compressed data.
 *  @note If CompressionMethod::None is used, no compression is applied.
 */
[[nodiscard]] RC<Stream> compressionEncoder(CompressionMethod method, RC<Stream> writer,
                                            CompressionLevel level);

/** @brief Compresses data using the specified compression method.
 *  @param method The compression method to apply.
 *  @param data The input data to compress.
 *  @param level The level of compression to apply.
 *  @return A vector of bytes containing the compressed data.
 *  @note If CompressionMethod::None is used, no compression is applied.
 */
[[nodiscard]] Bytes compressionEncode(CompressionMethod method, BytesView data, CompressionLevel level);

/** @brief Decompresses data using the specified compression method.
 *  @param method The compression method to use for decompression.
 *  @param data The compressed input data.
 *  @return A vector of bytes containing the decompressed data.
 *  @note If CompressionMethod::None is used, no decompression is applied.
 */
[[nodiscard]] Bytes compressionDecode(CompressionMethod method, BytesView data);

namespace Internal {
extern size_t compressionBatchSize; //< Must not be less than 256
}

} // namespace Brisk
