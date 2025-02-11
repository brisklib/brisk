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
#include <brisk/core/Stream.hpp>

namespace Brisk {

Transferred& Transferred::operator+=(Transferred other) noexcept {
    if (operator bool() && other.operator bool()) {
        m_bytes += other.m_bytes;
    } else {
        m_bytes = std::max(m_bytes, other.m_bytes);
    }
    return *this;
}

optional<std::vector<std::byte>> Stream::readUntilEnd(bool incompleteOk) {
    constexpr size_t SIZE = 16384;
    std::vector<std::byte> data;
    std::byte buf[SIZE];
    Transferred r;

    while ((r = read(buf, SIZE))) {
        data.insert(data.end(), buf, buf + r.bytes());
    }
    if (r.isError() && !incompleteOk)
        return nullopt;
    return data;
}

Transferred Stream::write(std::string_view data) {
    return write(reinterpret_cast<const std::byte*>(data.data()), data.size());
}

bool Stream::writeAll(std::span<const std::byte> data) {
    return write(data.data(), data.size()) == data.size();
}

bool SequentialReader::truncate() {
    throwException(ENotImplemented("truncate called for SequentialReader"));
}

bool SequentialReader::flush() {
    throwException(ENotImplemented("flush called for SequentialReader"));
}

uintmax_t SequentialReader::size() const {
    throwException(ENotImplemented("size called for SequentialReader"));
}

uintmax_t SequentialReader::tell() const {
    throwException(ENotImplemented("tell called for SequentialReader"));
}

bool SequentialReader::seek(intmax_t position, SeekOrigin origin) {
    throwException(ENotImplemented("seek called for SequentialReader"));
}

Transferred SequentialReader::write(const std::byte* data, size_t size) {
    throwException(ENotImplemented("write called for SequentialReader"));
}

StreamCapabilities SequentialReader::caps() const noexcept {
    return StreamCapabilities::CanRead;
}

StreamCapabilities MemoryStream::caps() const noexcept {
    return StreamCapabilities::CanRead | StreamCapabilities::CanWrite | StreamCapabilities::CanSeek |
           StreamCapabilities::CanFlush | StreamCapabilities::CanTruncate | StreamCapabilities::HasSize;
}

Transferred MemoryStream::read(std::byte* data, size_t size) {
    if (size == 0)
        return Transferred::Error;
    size_t trSize = std::min(size, static_cast<size_t>(m_data.size() - m_position));
    if (trSize == 0) {
        return Transferred::Eof;
    }
    std::copy(m_data.data() + m_position, m_data.data() + m_position + trSize, data);
    m_position += trSize;
    return trSize;
}

Transferred MemoryStream::write(const std::byte* data, size_t size) {
    if (size == 0)
        return Transferred::Error;
    if (m_data.size() < m_position + size)
        m_data.resize(m_position + size);
    std::copy(data, data + size, m_data.data() + m_position);
    m_position += size;
    return size;
}

bool MemoryStream::flush() {
    return true;
}

std::vector<std::byte>& MemoryStream::data() {
    return m_data;
}

const std::vector<std::byte>& MemoryStream::data() const {
    return m_data;
}

bool MemoryStream::truncate() {
    m_data.resize(m_position, std::byte{});
    return true;
}

uintmax_t MemoryStream::size() const {
    return m_data.size();
}

uintmax_t MemoryStream::tell() const {
    return m_position;
}

bool MemoryStream::seek(intmax_t position, SeekOrigin origin) {
    intmax_t newPosition;
    switch (origin) {
    case SeekOrigin::End:
        newPosition = m_data.size() + position;
        break;
    case SeekOrigin::Current:
        newPosition = m_position + position;
        break;
    case SeekOrigin::Beginning:
    default:
        newPosition = position;
        break;
    }
    if (newPosition < 0)
        return false;
    m_position = newPosition;
    return true;
}

bool SequentialWriter::truncate() {
    throwException(ENotImplemented("truncate called for SequentialWriter"));
}

uintmax_t SequentialWriter::size() const {
    throwException(ENotImplemented("size called for SequentialWriter"));
}

uintmax_t SequentialWriter::tell() const {
    throwException(ENotImplemented("tell called for SequentialWriter"));
}

bool SequentialWriter::seek(intmax_t position, SeekOrigin origin) {
    throwException(ENotImplemented("seek called for SequentialWriter"));
}

Transferred SequentialWriter::read(std::byte* data, size_t size) {
    throwException(ENotImplemented("read called for SequentialWriter"));
}

StreamCapabilities SequentialWriter::caps() const noexcept {
    return StreamCapabilities::CanWrite | StreamCapabilities::CanFlush;
}

MemoryStream::MemoryStream(std::vector<std::byte> data) : m_data(std::move(data)) {}

MemoryStream::MemoryStream() {}

StreamCapabilities Writer::caps() const noexcept {
    return SequentialWriter::caps() | StreamCapabilities::CanSeek;
}

StreamCapabilities Reader::caps() const noexcept {
    return SequentialReader::caps() | StreamCapabilities::CanSeek;
}
} // namespace Brisk
