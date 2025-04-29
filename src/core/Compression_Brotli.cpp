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
#include <brotli/decode.h>
#include <brotli/encode.h>

namespace Brisk {

using Internal::compressionBatchSize;

struct BrotliDecoderDeleter {
    void operator()(BrotliDecoderState* s) {
        BrotliDecoderDestroyInstance(s);
    }
};

struct BrotliEncoderDeleter {
    void operator()(BrotliEncoderState* s) {
        BrotliEncoderDestroyInstance(s);
    }
};

class BrotliDecoder : public SequentialReader {
public:
    explicit BrotliDecoder(Rc<Stream> reader) : reader(std::move(reader)) {
        buffer.reset(new std::byte[compressionBatchSize]);
        bufferUsed = 0;
        state.reset(BrotliDecoderCreateInstance(nullptr, nullptr, nullptr));
    }

    [[nodiscard]] Transferred read(std::byte* data, size_t size) final {
        if (finished)
            return Transferred::Eof;
        size_t available_out = size;
        uint8_t* next_out    = reinterpret_cast<uint8_t*>(data);
        size_t available_in;
        const uint8_t* next_in;

        do {
            next_in = (const uint8_t*)buffer.get();
            if (bufferUsed < compressionBatchSize) {
                Transferred sz = reader->read(buffer.get() + bufferUsed, compressionBatchSize - bufferUsed);
                if (sz.isError()) {
                    return sz;
                }
                available_in = bufferUsed + sz.bytes();
            } else {
                available_in = bufferUsed;
            }

            BrotliDecoderResult result = BrotliDecoderDecompressStream(state.get(), &available_in, &next_in,
                                                                       &available_out, &next_out, nullptr);
            if (result == BROTLI_DECODER_RESULT_ERROR) {
                return Transferred::Error;
            }
            if (result == BROTLI_DECODER_RESULT_SUCCESS) {
                finished = true;
                break;
            }

            if (available_out != 0) {
                BRISK_ASSERT(available_in == 0);
            }
            bufferUsed = 0;
        } while (available_out);
        // keep unused part of input
        if (available_in > 0) {
            memmove(buffer.get(), next_in, available_in);
            bufferUsed = available_in;
        }
        if (size == available_out && finished)
            return Transferred::Eof;
        else
            return size - available_out;
    }

    ~BrotliDecoder() {}

private:
    Rc<Stream> reader;
    std::unique_ptr<BrotliDecoderState, BrotliDecoderDeleter> state;
    std::unique_ptr<std::byte[]> buffer;
    size_t bufferUsed = 0;
    bool finished     = false;
};

constexpr static int brotliLgWin = (BROTLI_MIN_WINDOW_BITS + BROTLI_MAX_WINDOW_BITS) / 2;

constexpr int brotliQuality(CompressionLevel level) {
    return (static_cast<int>(level) - 1) * (BROTLI_MAX_QUALITY - BROTLI_MIN_QUALITY) / 8 + BROTLI_MIN_QUALITY;
}

static_assert(brotliQuality(CompressionLevel::Lowest) == BROTLI_MIN_QUALITY);
static_assert(brotliQuality(CompressionLevel::Highest) == BROTLI_MAX_QUALITY);
static_assert(brotliQuality(CompressionLevel::Normal) == (BROTLI_MAX_QUALITY + BROTLI_MIN_QUALITY) / 2);

class BrotliEncoder final : public SequentialWriter {
public:
    explicit BrotliEncoder(Rc<Stream> writer, CompressionLevel level) : writer(std::move(writer)) {
        buffer.reset(new std::byte[compressionBatchSize]);

        state.reset(BrotliEncoderCreateInstance(nullptr, nullptr, nullptr));
        BrotliEncoderSetParameter(state.get(), BROTLI_PARAM_QUALITY, brotliQuality(level));
        BrotliEncoderSetParameter(state.get(), BROTLI_PARAM_LGWIN, brotliLgWin);
    }

    [[nodiscard]] Transferred write(const std::byte* data, size_t size) final {
        if (size == 0)
            return 0;
        const uint8_t* next_in = reinterpret_cast<const uint8_t*>(data);
        size_t available_in    = size;
        while (available_in > 0) {
            size_t available_out = compressionBatchSize;
            uint8_t* next_out    = (uint8_t*)buffer.get();
            if (!BrotliEncoderCompressStream(state.get(), BROTLI_OPERATION_PROCESS, &available_in, &next_in,
                                             &available_out, &next_out, nullptr)) {
                return Transferred::Error;
            }
            size_t flushSize = compressionBatchSize - available_out;
            if (flushSize) {
                Transferred wr = writer->write(buffer.get(), flushSize);
                if (wr.bytes() != flushSize) {
                    return wr;
                }
            }
        }
        return size;
    }

    [[nodiscard]] bool flush() final {
        const uint8_t* next_in = nullptr;
        size_t avail_in        = 0;
        while (!BrotliEncoderIsFinished(state.get())) {
            size_t avail_out  = compressionBatchSize;
            uint8_t* next_out = (uint8_t*)buffer.get();
            if (!BrotliEncoderCompressStream(state.get(), BROTLI_OPERATION_FINISH, &avail_in, &next_in,
                                             &avail_out, &next_out, nullptr)) {
                return false;
            }
            size_t flushSize = compressionBatchSize - avail_out;
            if (flushSize) {
                Transferred wr = writer->write(buffer.get(), flushSize);
                if (wr.bytes() != flushSize) {
                    return false;
                }
            }
        }
        return writer->flush();
    }

    ~BrotliEncoder() final {}

private:
    Rc<Stream> writer;
    std::unique_ptr<BrotliEncoderState, BrotliEncoderDeleter> state;
    std::unique_ptr<std::byte[]> buffer;
};

Rc<Stream> brotliDecoder(Rc<Stream> reader) {
    return Rc<Stream>(new BrotliDecoder(std::move(reader)));
}

Rc<Stream> brotliEncoder(Rc<Stream> writer, CompressionLevel level) {
    return Rc<Stream>(new BrotliEncoder(std::move(writer), level));
}

Bytes brotliEncode(BytesView data, CompressionLevel level) {
    Bytes result;
    int q     = brotliQuality(level);
    size_t sz = BrotliEncoderMaxCompressedSize(data.size());
    if (sz == 0)
        sz = data.size() / 2;
    result.resize(sz);
    size_t encoded_size = result.size();
    while (!BrotliEncoderCompress(q, brotliLgWin, BrotliEncoderMode::BROTLI_MODE_GENERIC, data.size(),
                                  (const uint8_t*)data.data(), &encoded_size, (uint8_t*)result.data())) {
        if (encoded_size == 0)
            return {};
        result.resize(result.size() * 2);
        encoded_size = result.size();
    }
    result.resize(encoded_size);
    return result;
}

Bytes brotliDecode(BytesView data) {
    Bytes result;
    result.resize(data.size() * 3);
    size_t decoded_size = result.size();
    while (!BrotliDecoderDecompress(data.size(), (const uint8_t*)data.data(), &decoded_size,
                                    (uint8_t*)result.data())) {
        if (decoded_size == 0)
            return {};
        result.resize(result.size() * 2);
        decoded_size = result.size();
    }
    result.resize(decoded_size);
    return result;
}

} // namespace Brisk
