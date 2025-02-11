#include <brisk/core/Compression.hpp>
#include <brisk/core/Log.hpp>
#include <lz4.h>
#include <lz4hc.h>
#include <lz4frame.h>
#include <memory>

namespace Brisk {

using Internal::compressionBatchSize;

class LZ4Decoder : public SequentialReader {
public:
    explicit LZ4Decoder(RC<Stream> reader) : reader(std::move(reader)) {
        buffer.reset(new uint8_t[compressionBatchSize]);
        bufferSize = bufferConsumed = 0;
        LZ4F_errorCode_t result     = LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);
        if (LZ4F_isError(result)) {
            LOG_ERROR(lz4, "LZ4F_createDecompressionContext failed: {}", LZ4F_getErrorName(result));
            dctx = nullptr;
        }
    }

    [[nodiscard]] Transferred read(uint8_t* data, size_t size) final {
        if (!dctx)
            return Transferred::Error;
        if (finished)
            return Transferred::Eof;

        size_t available_out = size;
        uint8_t* next_out    = data;
        size_t in_size;
        const uint8_t* next_in;

        do {
            if (bufferSize < compressionBatchSize) {
                Transferred sz = reader->read(buffer.get() + bufferSize, compressionBatchSize - bufferSize);
                if (sz.isError()) {
                    return sz;
                }
                bufferSize += sz.bytes();
            }

            in_size              = bufferSize - bufferConsumed;
            next_in              = buffer.get() + bufferConsumed;

            size_t next_out_size = available_out;
            size_t result = LZ4F_decompress(dctx, next_out, &next_out_size, next_in, &in_size, nullptr);
            if (LZ4F_isError(result)) {
                LOG_ERROR(lz4, "LZ4F_decompress failed: {}", LZ4F_getErrorName(result));
                return Transferred::Error;
            }

            if (bufferSize == bufferConsumed && next_out_size == 0) {
                finished = true;
                break;
            }

            bufferConsumed += in_size;

            if (bufferConsumed > 0) {
                if (bufferSize > bufferConsumed)
                    memmove(buffer.get(), buffer.get() + bufferConsumed, bufferSize - bufferConsumed);
                bufferSize -= bufferConsumed;
                bufferConsumed = 0;
            }

            available_out -= next_out_size;
            next_out += next_out_size;

        } while (available_out > 0);

        if (size == available_out && finished) {
            return Transferred::Eof;
        } else {
            return size - available_out;
        }
    }

    ~LZ4Decoder() {
        if (dctx) {
            LZ4F_freeDecompressionContext(dctx);
        }
    }

private:
    RC<Stream> reader;
    LZ4F_dctx* dctx = nullptr;
    std::unique_ptr<uint8_t[]> buffer;
    size_t bufferSize     = 0;
    size_t bufferConsumed = 0;
    bool finished         = false;
};

constexpr int lz4Level(CompressionLevel level) noexcept {
    return (static_cast<int>(level) - 1) * (LZ4HC_CLEVEL_MAX - LZ4HC_CLEVEL_MIN) / 8 + LZ4HC_CLEVEL_MIN;
}

static_assert(lz4Level(CompressionLevel::Lowest) == LZ4HC_CLEVEL_MIN);
static_assert(lz4Level(CompressionLevel::Highest) == LZ4HC_CLEVEL_MAX);

class LZ4Encoder final : public SequentialWriter {
public:
    explicit LZ4Encoder(RC<Stream> writer, CompressionLevel level) : writer(std::move(writer)) {

        LZ4F_errorCode_t result = LZ4F_createCompressionContext(&cctx, LZ4F_VERSION);
        if (LZ4F_isError(result)) {
            LOG_ERROR(lz4, "LZ4F_createCompressionContext failed: {}", LZ4F_getErrorName(result));
            cctx = nullptr;
        }
        preferences.frameInfo.blockSizeID = LZ4F_max4MB;
        preferences.compressionLevel      = lz4Level(level);

        bufferSize                        = LZ4F_compressBound(compressionBatchSize, &preferences);
        buffer.reset(new uint8_t[bufferSize]);
    }

    bool writeHeader() {
        uint8_t headerData[LZ4F_HEADER_SIZE_MAX];
        size_t headerSize = LZ4F_compressBegin(cctx, headerData, std::size(headerData), &preferences);
        if (LZ4F_isError(headerSize)) {
            LOG_ERROR(lz4, "LZ4F_compressBegin failed: {}", LZ4F_getErrorName(headerSize));
            return false;
        }
        return this->writer->write(headerData, headerSize) == headerSize;
    }

    [[nodiscard]] Transferred write(const uint8_t* data, size_t size) final {
        if (!headerWritten) {
            if (!writeHeader())
                return Transferred::Error;
            headerWritten = true;
        }

        if (!cctx || size == 0) {
            return Transferred::Error;
        }

        const uint8_t* next_in = data;
        size_t available_in    = size;

        while (available_in > 0) {
            size_t available_out = bufferSize;
            uint8_t* next_out    = buffer.get();

            size_t result        = LZ4F_compressUpdate(cctx, next_out, available_out, next_in,
                                                       std::min(available_in, compressionBatchSize), nullptr);
            if (LZ4F_isError(result)) {
                LOG_ERROR(lz4, "LZ4F_compressUpdate failed: {}", LZ4F_getErrorName(result));
                return Transferred::Error;
            }

            size_t flushSize = result;
            if (flushSize > 0) {
                Transferred wr = writer->write(buffer.get(), flushSize);
                if (wr.bytes() != flushSize) {
                    return wr;
                }
            }
            available_in -= std::min(available_in, compressionBatchSize);
        }
        return size;
    }

    [[nodiscard]] bool flush() final {
        if (!cctx) {
            return false;
        }
        if (!headerWritten) {
            if (!writeHeader())
                return Transferred::Error;
            headerWritten = true;
        }

        size_t available_out = bufferSize;
        uint8_t* next_out    = buffer.get();

        size_t result        = LZ4F_compressEnd(cctx, next_out, available_out, nullptr);
        if (LZ4F_isError(result)) {
            LOG_ERROR(lz4, "LZ4F_compressEnd failed: {}", LZ4F_getErrorName(result));
            return false;
        }

        size_t flushSize = result;
        if (flushSize > 0) {
            Transferred wr = writer->write(buffer.get(), flushSize);
            if (wr.bytes() != flushSize) {
                return false;
            }
        }

        return writer->flush();
    }

    ~LZ4Encoder() {
        if (cctx) {
            LZ4F_freeCompressionContext(cctx);
        }
    }

private:
    RC<Stream> writer;
    bool headerWritten = false;
    LZ4F_cctx* cctx    = nullptr;
    LZ4F_preferences_t preferences{};
    std::unique_ptr<uint8_t[]> buffer;
    size_t bufferSize{};
};

RC<Stream> lz4Decoder(RC<Stream> reader) {
    return RC<Stream>(new LZ4Decoder(std::move(reader)));
}

RC<Stream> lz4Encoder(RC<Stream> writer, CompressionLevel level) {
    return RC<Stream>(new LZ4Encoder(std::move(writer), level));
}

Bytes lz4Encode(BytesView data, CompressionLevel level) {
    Bytes result;
    size_t max_compressed_size = LZ4F_compressFrameBound(data.size(), nullptr);
    result.resize(max_compressed_size);

    size_t compressed_size =
        LZ4F_compressFrame(result.data(), max_compressed_size, data.data(), data.size(), nullptr);
    if (LZ4F_isError(compressed_size)) {
        LOG_ERROR(lz4, "LZ4F_compressFrame failed: {}", LZ4F_getErrorName(compressed_size));
        return {};
    }

    result.resize(compressed_size);
    return result;
}

Bytes lz4Decode(BytesView data) {
    Bytes result;
    result.resize(data.size() * 3);

    LZ4F_dctx* dctx;
    LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION);

    size_t decoded_size  = result.size();
    size_t consumed_size = data.size();

    size_t result_size =
        LZ4F_decompress(dctx, result.data(), &decoded_size, data.data(), &consumed_size, nullptr);
    if (LZ4F_isError(result_size)) {
        LOG_ERROR(lz4, "LZ4F_decompress failed: {}", LZ4F_getErrorName(result_size));
        LZ4F_freeDecompressionContext(dctx);
        return {};
    }

    result.resize(decoded_size);
    LZ4F_freeDecompressionContext(dctx);
    return result;
}

} // namespace Brisk
