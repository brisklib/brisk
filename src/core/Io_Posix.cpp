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
#include <array>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <brisk/core/Io.hpp>
#include <brisk/core/Utilities.hpp>

namespace Brisk {

namespace {

IoError nativeToResult(int code) {
    switch (code) {
    case ENODEV:
    case ENOENT:
    case ENXIO:
        return IoError::NotFound;
    case EPERM:
    case EACCES:
        return IoError::AccessDenied;
    case ENOSPC:
        return IoError::NoSpace;
    default:
        return IoError::UnknownError;
    }
}

using StrmCap = StreamCapabilities;

constexpr std::array<StreamCapabilities, 5> file_caps{
    StrmCap::CanRead | StrmCap::CanSeek | StrmCap::HasSize,
    StrmCap::CanRead | StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::CanTruncate |
        StrmCap::HasSize,
    StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::CanTruncate | StrmCap::HasSize,
    StrmCap::CanRead | StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::CanTruncate |
        StrmCap::HasSize,
    StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::CanTruncate | StrmCap::HasSize,
};

constexpr std::array<const char*, 5> file_modes{ "rb", "r+b", "wb", "w+b", "ab" };

#define IO_SEEK_64 fseeko
#define IO_TELL_64 ftello

class FileStream final : public Stream {
public:
    StreamCapabilities caps() const noexcept final {
        return m_caps;
    }

    uint64_t size() const final {
        if (!m_file)
            return invalidSize;
        const auto saved = IO_TELL_64(m_file);
        if (saved < 0 || IO_SEEK_64(m_file, 0, SEEK_END) != 0) {
            clearerr(m_file);
            return invalidSize;
        }
        const auto result   = IO_TELL_64(m_file);
        const bool restored = IO_SEEK_64(m_file, saved, SEEK_SET) == 0;
        if (result < 0 || !restored) {
            clearerr(m_file);
            return invalidSize;
        }
        return static_cast<uint64_t>(result);
    }

    bool truncate() final {
        if (!m_file)
            return false;
        const auto position = IO_TELL_64(m_file);
        return position >= 0 && ftruncate(fileno(m_file), position) == 0;
    }

    ~FileStream() {
        if (m_owns)
            std::fclose(m_file);
    }

    explicit FileStream(std::FILE* file, bool owns, StreamCapabilities caps)
        : m_file(file), m_owns(owns), m_caps(caps) {}

    bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) final {
        if (!m_file)
            return false;
        return IO_SEEK_64(m_file, position,
                          staticMap(origin, SeekOrigin::Beginning, SEEK_SET, SeekOrigin::Current, SEEK_CUR,
                                    SeekOrigin::End, SEEK_END, SEEK_SET)) == 0;
    }

    uint64_t tell() const final {
        if (!m_file)
            return invalidPosition;
        const auto position = IO_TELL_64(m_file);
        return position < 0 ? invalidPosition : static_cast<uint64_t>(position);
    }

    Transferred read(std::byte* data, size_t size) final {
        if (size == 0)
            return 0;
        if (!m_file || ferror(m_file))
            return Transferred::Error;
        if (feof(m_file))
            return Transferred::Eof;
        return fread(data, 1, size, m_file);
    }

    Transferred write(const std::byte* data, size_t size) final {
        if (size == 0)
            return 0;
        if (!m_file || ferror(m_file))
            return Transferred::Error;
        return fwrite(data, 1, size, m_file);
    }

    bool flush() final {
        return m_file && fflush(m_file) == 0;
    }

private:
    std::FILE* m_file;
    bool m_owns;
    StreamCapabilities m_caps;
};

StreamCapabilities fileCapabilities(std::FILE* file) {
    const int descriptor = fileno(file);
    if (descriptor < 0)
        throwException(EArgument("openFile requires a valid FILE*"));

    const int flags = fcntl(descriptor, F_GETFL);
    if (flags < 0)
        throwException(EArgument("openFile cannot inspect the FILE* access mode"));

    StreamCapabilities caps = StreamCapabilities{};
    const int access        = flags & O_ACCMODE;
    if (access == O_RDONLY || access == O_RDWR)
        caps |= StreamCapabilities::CanRead;
    if (access == O_WRONLY || access == O_RDWR)
        caps |= StreamCapabilities::CanWrite | StreamCapabilities::CanFlush;

    struct stat info;
    const bool regular = fstat(descriptor, &info) == 0 && S_ISREG(info.st_mode);
    if (regular)
        caps |= StreamCapabilities::CanSeek | StreamCapabilities::HasSize;
    if (regular && (caps && StreamCapabilities::CanWrite))
        caps |= StreamCapabilities::CanTruncate;
    return caps;
}

} // namespace

expected<std::FILE*, IoError> fopen_native(const fs::path& file_name, OpenFileMode mode) {
    const size_t index = static_cast<size_t>(mode);
    if (index >= file_modes.size())
        throwException(EArgument("invalid OpenFileMode"));
    std::FILE* f = fopen(file_name.string().c_str(), file_modes[index]);
    if (f)
        return f;
    return unexpected(nativeToResult(errno));
}

Rc<Stream> openFile(std::FILE* file, bool owns) {
    if (!file)
        throwException(EArgument("openFile requires a non-null FILE*"));
    return rcnew FileStream(file, owns, fileCapabilities(file));
}

Rc<Stream> stdoutStream() {
    return rcnew FileStream(stdout, false, StreamCapabilities::CanWrite | StreamCapabilities::CanFlush);
}

Rc<Stream> stderrStream() {
    return rcnew FileStream(stderr, false, StreamCapabilities::CanWrite | StreamCapabilities::CanFlush);
}

Rc<Stream> stdinStream() {
    return rcnew FileStream(stdin, false, StreamCapabilities::CanRead);
}

expected<Rc<Stream>, IoError> openFile(const fs::path& filePath, OpenFileMode mode) {
    const size_t index = static_cast<size_t>(mode);
    if (index >= file_caps.size())
        throwException(EArgument("invalid OpenFileMode"));
    return fopen_native(filePath, mode).map([index](std::FILE* f) {
        return rcnew FileStream(f, true, file_caps[index]);
    });
}

} // namespace Brisk
