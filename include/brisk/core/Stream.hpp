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

#include "Encoding.hpp"
#include "BasicTypes.hpp"
#include "Rc.hpp"
#include "internal/Span.hpp"
#include "Exceptions.hpp"

namespace Brisk {

/**
 * @struct Transferred
 * @brief Represents the result of a read or write operation, indicating
 *        success, failure, or end-of-file (EOF), along with the number of
 *        bytes transferred.
 *
 * This struct is used to encapsulate the result of I/O operations in a way
 * that clearly distinguishes between successful data transfers, errors, and
 * EOF conditions.
 */
struct Transferred {
    /**
     * @brief Default constructor.
     */
    constexpr Transferred() noexcept                   = default;

    /**
     * @brief Copy constructor.
     * @param other Another Transferred object to copy from.
     */
    constexpr Transferred(const Transferred&) noexcept = default;

    /**
     * @brief Constructs a Transferred object with a specified number of bytes.
     * @param bytes Number of bytes transferred.
     */
    constexpr Transferred(size_t bytes) noexcept : m_bytes(bytes) {}

    /**
     * @brief Special values for indicating EOF and error conditions.
     */
    enum : size_t {
        SpecialValues = 2,            /**< Number of special values used. */
        Eof           = SIZE_MAX - 1, /**< Value indicating End-Of-File. */
        Error         = SIZE_MAX,     /**< Value indicating an error. */
    };

    /**
     * @brief Checks if the result indicates an EOF condition.
     * @return True if the result is EOF, false otherwise.
     */
    [[nodiscard]] bool isEOF() const noexcept {
        return m_bytes == Eof;
    }

    /**
     * @brief Checks if the result indicates an error.
     * @return True if the result is an error, false otherwise.
     */
    [[nodiscard]] bool isError() const noexcept {
        return m_bytes == Error;
    }

    /**
     * @brief Retrieves the number of bytes successfully transferred.
     * @return Number of bytes transferred. If the result indicates EOF or error,
     *         the returned value is 0.
     */
    [[nodiscard]] size_t bytes() const noexcept {
        return std::max(m_bytes + SpecialValues, size_t(SpecialValues)) - SpecialValues;
    }

    /**
     * @brief Converts the Transferred object to a boolean value indicating
     *        whether at least one byte was transferred.
     * @return True if at least one byte was transferred, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_bytes + SpecialValues > SpecialValues;
    }

    /**
     * @brief Adds the number of bytes from another Transferred object to this one.
     *        If either Transferred object represents an error or EOF, the result
     *        will be the error or EOF accordingly.
     * @param other Another Transferred object to add.
     * @return Reference to this Transferred object with the updated byte count.
     */
    Transferred& operator+=(Transferred other) noexcept;

    /**
     * @brief Compares this Transferred object with another for equality.
     * @param t Another Transferred object to compare with.
     * @return True if both Transferred objects represent the same result (bytes count),
     *         false otherwise.
     */
    bool operator==(const Transferred& t) const noexcept = default;

    size_t m_bytes                                       = 0;
};

/**
 * @enum SeekOrigin
 * @brief Specifies the reference point for a seek operation in a stream.
 *
 * This enumeration is used to indicate the starting point from which a seek
 * operation should be performed.
 */
enum class SeekOrigin {
    Beginning, /**< Seek from the beginning of the stream. */
    End,       /**< Seek from the end of the stream. */
    Current,   /**< Seek from the current position in the stream. */
};

/**
 * @brief Constant representing an invalid position in a stream.
 *
 * This value is used to indicate an invalid or undefined position when working
 * with stream operations.
 */
constexpr inline uint64_t invalidPosition = UINTMAX_MAX;

/**
 * @brief Constant representing an invalid size for a stream.
 *
 * This value is used to indicate an invalid or undefined size when working
 * with stream operations.
 */
constexpr inline uint64_t invalidSize     = UINTMAX_MAX;

/**
 * @enum StreamCapabilities
 * @brief Describes the capabilities of a stream.
 *
 * This enumeration specifies various capabilities that a stream may support,
 * allowing for querying and conditional operations based on the stream's
 * features.
 */
enum class StreamCapabilities {
    CanRead     = 1,  /**< The stream supports reading operations. */
    CanWrite    = 2,  /**< The stream supports writing operations. */
    CanSeek     = 4,  /**< The stream supports seeking operations. */
    CanFlush    = 8,  /**< The stream supports flushing operations. */
    CanTruncate = 16, /**< The stream supports truncating operations. */
    HasSize     = 32, /**< The stream has a size property. */

    All         = CanRead | CanWrite | CanSeek | CanFlush | CanTruncate |
          HasSize, /**< All capabilities are supported. */
};

/**
 * @brief Macro to enable bitwise operations for StreamCapabilities.
 *
 * This macro allows for the use of bitwise operations on the StreamCapabilities
 * enumeration, such as bitwise OR, AND, and NOT operations.
 */
template <>
constexpr inline bool isBitFlags<StreamCapabilities> = true;

/**
 * @class Stream
 * @brief Abstract base class representing a stream with read, write, seek, and other operations.
 *
 * This class provides a common interface for stream operations such as reading, writing, seeking, and
 * querying stream properties. Concrete implementations should override the pure virtual functions to
 * provide specific behavior for different types of streams.
 */
class Stream {
public:
    /**
     * @brief Virtual destructor for the Stream class.
     */
    virtual ~Stream() {}

    /**
     * @brief Retrieves the capabilities of the stream.
     * @return StreamCapabilities A bitmask representing the capabilities of the stream.
     */
    [[nodiscard]] virtual StreamCapabilities caps() const noexcept       = 0;

    /**
     * @brief Reads up to `size` bytes from the stream into the provided buffer.
     *
     * The `read` function attempts to read `size` bytes of data from the stream and stores it in the
     * buffer pointed to by `data`.
     *
     * If the stream reaches the end of the file (EOF) before the requested number of bytes
     * are read, the function will return the number of bytes successfully read.
     *
     * @param data Pointer to the buffer where the read data will be stored.
     * @param size Number of bytes to attempt to read from the stream.
     * @return Transferred
     * - The total number of bytes read if successful.
     * - `Transferred::Eof` if EOF is encountered before any data is read.
     * - `Transferred::Error` if an error occurs during the read operation.
     *
     * @details
     * - If EOF is reached, the function returns the number of bytes read before EOF.
     * - If EOF is reached before any bytes are read, `Transferred::Eof` is returned.
     */
    [[nodiscard]] virtual Transferred read(std::byte* data, size_t size) = 0;

    [[nodiscard]] virtual Transferred read(uint8_t* data, size_t size) {
        return read(reinterpret_cast<std::byte*>(data), size);
    }

    /**
     * @brief Writes up to `size` bytes from the provided buffer to the stream.
     *
     * The `write` function attempts to write `size` bytes of data from the buffer pointed to by `data`
     * into the stream.
     *
     * @param data Pointer to the buffer containing the data to be written.
     * @param size Number of bytes to attempt to write to the stream.
     * @return Transferred
     * - The total number of bytes written if successful.
     * - `Transferred::Error` if an error occurs during the write operation.
     *
     * @details
     * - If any error occurs during the `WriteFile` call or no bytes are written, the function
     * returns `Transferred::Error`.
     */
    [[nodiscard]] virtual Transferred write(const std::byte* data, size_t size) = 0;

    [[nodiscard]] virtual Transferred write(uint8_t* data, size_t size) {
        return write(reinterpret_cast<const std::byte*>(data), size);
    }

    /**
     * @brief Flushes the stream, ensuring that all buffered data is written to the underlying storage.
     * @return True if the flush operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool flush()                                                           = 0;

    /**
     * @brief Seeks to a specific position in the stream.
     * @param position The position to seek to.
     * @param origin The reference point for the seek operation.
     * @return True if the seek operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) = 0;

    /**
     * @brief Retrieves the current position of the stream.
     * @return The current position of the stream or `invalidPosition` if an error occurs.
     */
    [[nodiscard]] virtual uint64_t tell() const                                                  = 0;

    /**
     * @brief Retrieves the size of the stream.
     * @return The size of the stream or `invalidSize` if an error occurs.
     */
    [[nodiscard]] virtual uint64_t size() const                                                  = 0;

    /**
     * @brief Truncates the stream to the current position.
     * @return True if the truncate operation was successful, false otherwise.
     */
    [[nodiscard]] virtual bool truncate()                                                        = 0;

public:
    /**
     * @brief Checks if the stream supports read operations.
     * @return True if the stream supports reading, false otherwise.
     */
    [[nodiscard]] bool canRead() const noexcept {
        return caps() && StreamCapabilities::CanRead;
    }

    /**
     * @brief Checks if the stream supports write operations.
     * @return True if the stream supports writing, false otherwise.
     */
    [[nodiscard]] bool canWrite() const noexcept {
        return caps() && StreamCapabilities::CanWrite;
    }

    /**
     * @brief Checks if the stream supports seek operations.
     * @return True if the stream supports seeking, false otherwise.
     */
    [[nodiscard]] bool canSeek() const noexcept {
        return caps() && StreamCapabilities::CanSeek;
    }

    /**
     * @brief Checks if the stream supports flush operations.
     * @return True if the stream supports flushing, false otherwise.
     */
    [[nodiscard]] bool canFlush() const noexcept {
        return caps() && StreamCapabilities::CanFlush;
    }

    /**
     * @brief Checks if the stream supports truncation operations.
     * @return True if the stream supports truncating, false otherwise.
     */
    [[nodiscard]] bool canTruncate() const noexcept {
        return caps() && StreamCapabilities::CanTruncate;
    }

    /**
     * @brief Checks if the stream has a size property.
     * @return True if the stream has a size property, false otherwise.
     */
    [[nodiscard]] bool hasSize() const noexcept {
        return caps() && StreamCapabilities::HasSize;
    }

    /**
     * @brief Reads data into a `std::span<std::byte>` buffer.
     * @param data A `std::span<std::byte>` representing the buffer to read into.
     * @return Transferred The result of the read operation.
     */
    [[nodiscard]] Transferred read(std::span<std::byte> data) {
        return read(data.data(), data.size());
    }

    /**
     * @brief Reads data from the stream until the end is reached or an error occurs.
     *
     * The `readUntilEnd` function reads data in chunks into a `std::vector<std::byte>` from the stream. It
     * continues reading until EOF is encountered or an error occurs. If an error occurs before all requested
     * data is read and `incompleteOk` is set to `false`, the function returns `std::nullopt` to indicate that
     * the read operation failed.
     *
     * @param incompleteOk If set to `true`, allows returning partial data if error occurs before the buffer
     * is completely filled.
     *
     * @return std::optional<std::vector<std::byte>>
     * - A `std::vector<std::byte>` containing the data read from the stream if successful.
     * - `std::nullopt` if an error occurs and `incompleteOk` is `false`.
     */
    std::optional<std::vector<std::byte>> readUntilEnd(bool incompleteOk = false);

    /**
     * @brief Writes all data from the given span to the stream.
     *
     * The `writeAll` function attempts to write the entire range of data specified by `data` to the
     * stream. It ensures that the number of bytes written matches the size of the `data` span.
     *
     * @param data A `std::span<const std::byte>` containing the data to be written.
     * @return bool True if the entire data was written successfully; otherwise, false.
     */
    [[nodiscard]] bool writeAll(std::span<const std::byte> data);

    /**
     * @brief Writes the contents of a `string_view` to the stream.
     *
     * The `write` function writes the data pointed to by `data`, interpreted as a sequence of bytes, to the
     * stream.
     *
     * @param data A `string_view` representing the data to be written.
     * @return Transferred The result of the write operation, indicating the number of bytes written or an
     * error.
     */
    [[nodiscard]] Transferred write(std::string_view data);
};

/**
 * @class SequentialReader
 * @brief Stream reader with restricted capabilities.
 *
 * Supports read operations only. Other operations throw ENotImplemented.
 */
class SequentialReader : public Stream {
public:
    using Stream::read;
    using Stream::readUntilEnd;

    StreamCapabilities caps() const noexcept override;

    Transferred write(const std::byte* data, size_t size) final;

    bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) override;

    uint64_t tell() const override;

    uint64_t size() const override;

    bool flush() final;

    bool truncate() final;
};

/**
 * @class SequentialWriter
 * @brief Stream writer with restricted capabilities.
 *
 * Supports write and flush operations only. Other operations throw ENotImplemented.
 */
class SequentialWriter : public Stream {
public:
    using Stream::flush;
    using Stream::write;

    StreamCapabilities caps() const noexcept override;

    Transferred read(std::byte* data, size_t size) final;

    bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) override;

    uint64_t tell() const override;

    uint64_t size() const override;

    bool truncate() override;
};

/**
 * @class Reader
 * @brief Stream reader with additional seek capabilities.
 *
 * Inherits from SequentialReader and supports seek operations.
 */
class Reader : public SequentialReader {
public:
    using Stream::hasSize;
    using Stream::seek;
    using Stream::size;
    using Stream::tell;

    StreamCapabilities caps() const noexcept override;
};

/**
 * @class Writer
 * @brief Stream writer with additional seek capabilities.
 *
 * Inherits from SequentialWriter and supports seek operations.
 */
class Writer : public SequentialWriter {
public:
    using Stream::hasSize;
    using Stream::seek;
    using Stream::size;
    using Stream::tell;

    StreamCapabilities caps() const noexcept override;
};

/**
 * @class MemoryStream
 * @brief In-memory stream with full read/write capabilities.
 * Internally stores data as a `Bytes` and grows as new data is written to the stream.
 *
 * Supports all stream operations, including read, write, seek, and size queries.
 */
class MemoryStream : public Stream {
public:
    /**
     * @brief Default constructor for `MemoryStream`.
     * Initializes an empty stream.
     */
    MemoryStream();

    /**
     * @brief Constructs a `MemoryStream` with the given initial data.
     * @param data A `std::vector<std::byte>` containing the initial data for the stream.
     */
    MemoryStream(std::vector<std::byte> data);

    [[nodiscard]] StreamCapabilities caps() const noexcept override;

    [[nodiscard]] Transferred read(std::byte* data, size_t size) override;

    [[nodiscard]] Transferred write(const std::byte* data, size_t size) override;

    [[nodiscard]] bool flush() override;

    [[nodiscard]] bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) override;

    [[nodiscard]] uint64_t tell() const override;

    [[nodiscard]] uint64_t size() const override;

    [[nodiscard]] bool truncate() override;

    /**
     * @brief Returns a const reference to the internal data buffer.
     * @return A constant reference to the `std::vector<std::byte>` holding the stream's data.
     */
    const std::vector<std::byte>& data() const;

    /**
     * @brief Returns a non-const reference to the internal data buffer.
     * @return A non-constant reference to the `std::vector<std::byte>` holding the stream's data.
     */
    std::vector<std::byte>& data();

private:
    std::vector<std::byte> m_data;
    uint64_t m_position = 0;
};

/**
 * @class SpanStream
 * @brief Stream based on a span of data with read/write capabilities.
 *
 * Supports read/write operations depending on whether the T is const-qualified.
 */
template <typename T>
class SpanStream : public Stream {
public:
    constexpr static bool readOnly = std::is_const_v<T>;
    using U                        = std::remove_const_t<T>;

    /**
     * @brief Constructs a `SpanStream` with the given data span.
     * @param data A `std::span<T>` representing the initial data for the stream.
     */
    SpanStream(std::span<T> data) : m_data(std::move(data)) {}

    [[nodiscard]] StreamCapabilities caps() const noexcept override {
        if constexpr (readOnly) {
            return StreamCapabilities::CanRead | StreamCapabilities::CanSeek | StreamCapabilities::HasSize;
        } else {
            return StreamCapabilities::CanRead | StreamCapabilities::CanWrite | StreamCapabilities::CanSeek |
                   StreamCapabilities::CanFlush | StreamCapabilities::HasSize;
        }
    }

    [[nodiscard]] Transferred read(std::byte* data, size_t size) override {
        if (size == 0)
            return Transferred::Error;
        size_t trSize = std::min(size, static_cast<size_t>(m_data.size_bytes() - m_bytePosition));
        if (trSize == 0) {
            return Transferred::Eof;
        }
        memcpy(data, reinterpret_cast<const std::byte*>(m_data.data()) + m_bytePosition, trSize);
        m_bytePosition += trSize;
        return trSize;
    }

    [[nodiscard]] Transferred write(const std::byte* data, size_t size) override {
        if constexpr (readOnly) {
            throwException(ENotImplemented("write called for read-only SpanStream"));
        } else {
            if (size == 0 || m_bytePosition >= m_data.size_bytes())
                return Transferred::Error;
            size = std::min(static_cast<size_t>(m_data.size_bytes() - m_bytePosition), size);
            if (size) {
                memcpy(reinterpret_cast<std::byte*>(m_data.data()) + m_bytePosition, data, size);
                m_bytePosition += size;
            }
            return size;
        }
    }

    [[nodiscard]] bool flush() override {
        if constexpr (readOnly) {
            throwException(ENotImplemented("flush called for read-only SpanStream"));
        } else {
            return true;
        }
    }

    [[nodiscard]] bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) override {
        int64_t newPosition;
        switch (origin) {
        case SeekOrigin::End:
            newPosition = m_data.size_bytes() + position;
            break;
        case SeekOrigin::Current:
            newPosition = m_bytePosition + position;
            break;
        case SeekOrigin::Beginning:
        default:
            newPosition = position;
            break;
        }
        if (newPosition < 0 || newPosition > static_cast<int64_t>(m_data.size_bytes()))
            return false;
        m_bytePosition = newPosition;
        return true;
    }

    [[nodiscard]] uint64_t tell() const override {
        return m_bytePosition;
    }

    [[nodiscard]] uint64_t size() const override {
        return m_data.size_bytes();
    }

    [[nodiscard]] bool truncate() override {
        if constexpr (readOnly) {
            throwException(ENotImplemented("truncate called for read-only SpanStream"));
        } else {
            return false;
        }
    }

    /**
     * @brief Returns a `std::span<T>` representing the data in the stream.
     * @return A `std::span<T>` containing the current data in the stream.
     */
    std::span<T> data() const {
        return m_data;
    }

private:
    std::span<T> m_data;
    uint64_t m_bytePosition = 0;
};

using ByteMutableViewStream = SpanStream<std::byte>;
using ByteViewStream        = SpanStream<const std::byte>;

} // namespace Brisk
