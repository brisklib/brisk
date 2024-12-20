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
#include <zlib.h>

namespace Brisk {

#define WRITE_GZIP_HEADER 16
#define ACCEPT_GZIP_OR_ZLIB_HEADER 32

using Internal::compressionBatchSize;

class ZLibDecoder : public SequentialReader {
public:
    explicit ZLibDecoder(RC<Stream> reader) : reader(std::move(reader)) {
        buffer.reset(new uint8_t[compressionBatchSize]);
        bufferUsed  = 0;

        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;

        inflateInit2(&strm, MAX_WBITS | ACCEPT_GZIP_OR_ZLIB_HEADER);
    }

    [[nodiscard]] Transferred read(uint8_t* data, size_t size) final {
        if (finished)
            return Transferred::Eof;
        strm.avail_out = size;
        strm.next_out  = (Bytef*)data;
        do {
            strm.next_in = buffer.get();
            if (bufferUsed < compressionBatchSize) {
                Transferred sz = reader->read(buffer.get() + bufferUsed, compressionBatchSize - bufferUsed);
                if (sz.isError())
                    return sz;
                bufferUsed += sz.bytes();
                strm.avail_in = bufferUsed;
            } else {
                strm.avail_in = bufferUsed;
            }
            if (strm.avail_in == 0) {
                if (finished)
                    break;
                else
                    return Transferred::Error;
            }

            int e = inflate(&strm, Z_NO_FLUSH);
            if (e != Z_OK && e != Z_STREAM_END) {
                return Transferred::Error;
            } else if (e == Z_STREAM_END) {
                finished = true;
            }
            if (strm.avail_out != 0) {
                BRISK_ASSERT(strm.avail_in == 0);
            }
            bufferUsed = 0;
        } while (strm.avail_out);
        // keep unused part of input
        if (strm.avail_in > 0) {
            memmove(buffer.get(), strm.next_in, strm.avail_in);
            bufferUsed = strm.avail_in;
        }
        if (size == strm.avail_out && finished)
            return Transferred::Eof;
        else
            return size - strm.avail_out;
    }

    ~ZLibDecoder() {
        inflateEnd(&strm);
    }

private:
    RC<Stream> reader;
    z_stream strm{};
    std::unique_ptr<uint8_t[]> buffer;
    size_t bufferUsed = 0;
    bool finished     = false;
};

class ZLibEncoder final : public SequentialWriter {
public:
    explicit ZLibEncoder(RC<Stream> writer, CompressionLevel level, bool gzip = false)
        : writer(std::move(writer)) {
        buffer.reset(new uint8_t[compressionBatchSize]);

        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;
        deflateInit2(&strm, static_cast<int>(level), Z_DEFLATED, MAX_WBITS | (gzip ? WRITE_GZIP_HEADER : 0),
                     MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    }

    Transferred loop(int flush_flag) {
        strm.avail_out = (uInt)compressionBatchSize;
        strm.next_out  = (Bytef*)buffer.get();
        int e          = deflate(&strm, flush_flag);
        if (e != Z_OK && e != Z_STREAM_END && e != Z_BUF_ERROR) {
            return Transferred::Error;
        }
        size_t flushSize = strm.next_out - (Bytef*)buffer.get();
        if (flushSize) {
            Transferred wr = writer->write(buffer.get(), flushSize);
            if (wr.bytes() != flushSize) {
                return Transferred::Error;
            }
        }

        if (e == Z_STREAM_END) {
            return Transferred::Eof;
        }
        return compressionBatchSize;
    }

    [[nodiscard]] Transferred write(const uint8_t* data, size_t size) final {
        if (size == 0)
            return 0;
        strm.avail_in = (uInt)size;
        strm.next_in  = (Bytef*)data;

        while (strm.avail_in) {
            if (Transferred wr = loop(Z_NO_FLUSH); !wr) {
                return wr;
            }
        }
        return size;
    }

    [[nodiscard]] bool flush() final {
        for (;;) {
            if (Transferred wr = loop(Z_FINISH); wr.isError()) {
                return false;
            } else if (wr.isEOF()) {
                return writer->flush();
            }
        }
    }

    ~ZLibEncoder() final {
        deflateEnd(&strm);
    }

private:
    RC<Stream> writer;
    z_stream strm{};
    std::unique_ptr<uint8_t[]> buffer;
};

RC<Stream> gzipDecoder(RC<Stream> reader) {
    return RC<Stream>(new ZLibDecoder(std::move(reader)));
}

RC<Stream> gzipEncoder(RC<Stream> writer, CompressionLevel level) {
    return RC<Stream>(new ZLibEncoder(std::move(writer), level, true));
}

RC<Stream> zlibDecoder(RC<Stream> reader) {
    return RC<Stream>(new ZLibDecoder(std::move(reader)));
}

RC<Stream> zlibEncoder(RC<Stream> writer, CompressionLevel level) {
    return RC<Stream>(new ZLibEncoder(std::move(writer), level, false));
}

int zlibCompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen, int level, bool gzip) {
    z_stream strm{};
    const uInt max = (uInt)-1;
    uLong left     = *destLen;
    *destLen       = 0;
    strm.zalloc    = nullptr;
    strm.zfree     = nullptr;
    strm.opaque    = nullptr;

    int err        = deflateInit2(&strm, level, Z_DEFLATED, MAX_WBITS | (gzip ? WRITE_GZIP_HEADER : 0),
                                  MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
        return err;

    strm.next_out  = dest;
    strm.avail_out = 0;
    strm.next_in   = (z_const Bytef*)source;
    strm.avail_in  = 0;

    do {
        if (strm.avail_out == 0) {
            strm.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= strm.avail_out;
        }
        if (strm.avail_in == 0) {
            strm.avail_in = sourceLen > (uLong)max ? max : (uInt)sourceLen;
            sourceLen -= strm.avail_in;
        }
        err = deflate(&strm, sourceLen ? Z_NO_FLUSH : Z_FINISH);
    } while (err == Z_OK);

    *destLen = strm.total_out;
    deflateEnd(&strm);
    return err == Z_STREAM_END ? Z_OK : err;
}

int zlibUncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen) {
    z_stream stream{};
    int err;
    const uInt max = (uInt)-1;
    uLong len, left;
    Byte buf[1]; /* for detection of incomplete stream when *destLen == 0 */

    len = sourceLen;
    if (*destLen) {
        left     = *destLen;
        *destLen = 0;
    } else {
        left = 1;
        dest = buf;
    }

    stream.next_in  = (Bytef*)source;
    stream.avail_in = 0;
    stream.zalloc   = nullptr;
    stream.zfree    = nullptr;
    stream.opaque   = nullptr;

    err             = inflateInit2(&stream, MAX_WBITS | ACCEPT_GZIP_OR_ZLIB_HEADER);
    if (err != Z_OK)
        return err;

    stream.next_out  = dest;
    stream.avail_out = 0;

    do {
        if (stream.avail_out == 0) {
            stream.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= stream.avail_out;
        }
        if (stream.avail_in == 0) {
            stream.avail_in = len > (uLong)max ? max : (uInt)len;
            len -= stream.avail_in;
        }
        err = inflate(&stream, Z_NO_FLUSH);
    } while (err == Z_OK);

    if (dest != buf)
        *destLen = stream.total_out;
    else if (stream.total_out && err == Z_BUF_ERROR)
        left = 1;

    inflateEnd(&stream);
    return err == Z_STREAM_END                             ? Z_OK
           : err == Z_NEED_DICT                            ? Z_DATA_ERROR
           : err == Z_BUF_ERROR && left + stream.avail_out ? Z_DATA_ERROR
                                                           : err;
}

bytes zlibEncode2(bytes_view data, CompressionLevel level, bool gzip) {
    bytes result;
    size_t sz = compressBound(data.size());
    result.resize(sz);
    unsigned long destLen = result.size();
    while (int e = zlibCompress(result.data(), &destLen, data.data(), data.size(), static_cast<int>(level),
                                gzip)) {
        if (e != Z_BUF_ERROR)
            return {};
        result.resize(result.size() * 2);
        destLen = result.size();
    }
    result.resize(destLen);
    return result;
}

bytes gzipEncode(bytes_view data, CompressionLevel level) {
    return zlibEncode2(data, level, true);
}

bytes zlibEncode(bytes_view data, CompressionLevel level) {
    return zlibEncode2(data, level, false);
}

bytes zlibDecode(bytes_view data) {
    bytes result;
    size_t sz = data.size() * 4;
    result.resize(sz);
    unsigned long destLen = result.size();
    while (int e = zlibUncompress(result.data(), &destLen, data.data(), data.size())) {
        if (e != Z_BUF_ERROR)
            return {};
        result.resize(result.size() * 2);
        destLen = result.size();
    }
    result.resize(destLen);
    return result;
}

bytes gzipDecode(bytes_view data) {
    return zlibDecode(data);
}

} // namespace Brisk
