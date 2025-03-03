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
#include <brisk/core/IO.hpp>

#ifdef BRISK_WINDOWS
#include <share.h>
#include <shlobj.h>
#include <io.h>
#endif

#ifdef BRISK_APPLE
#include <mach-o/dyld.h>
#endif
#ifdef BRISK_POSIX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <brisk/core/Text.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/App.hpp>
#include <fmt/format.h>
#include <mutex>

#include <array>
#include <random>

namespace Brisk {

size_t maxBytes = SIZE_MAX;

static IOError posixToResult(int code) {
    switch (code) {
    case ENODEV:
    case ENOENT:
    case ENXIO:
        return IOError::NotFound;
    case EPERM:
    case EACCES:
        return IOError::AccessDenied;
    case ENOSPC:
        return IOError::NoSpace;
    default:
        return IOError::UnknownError;
    }
}

using StrmCap = StreamCapabilities;

[[maybe_unused]] static std::array<StreamCapabilities, 5> file_caps{
    StrmCap::CanRead | StrmCap::CanSeek | StrmCap::HasSize,
    StrmCap::CanRead | StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::HasSize,
    StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::HasSize,
    StrmCap::CanRead | StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::HasSize,
    StrmCap::CanWrite | StrmCap::CanFlush | StrmCap::CanSeek | StrmCap::HasSize,
};

[[maybe_unused]] static std::array<const char*, 5> file_modes{
    "rb", "r+b", "wb", "w+b", "ab",
};

#ifdef BRISK_WINDOWS
[[maybe_unused]] static std::array<const wchar_t*, 5> file_modes_w{
    L"rb", L"r+b", L"wb", L"w+b", L"ab",
};
#endif

expected<std::FILE*, IOError> fopen_native(const fs::path& file_name, OpenFileMode mode) {
    std::FILE* f = nullptr;
#ifdef BRISK_WINDOWS
    errno_t e = _wfopen_s(&f, file_name.wstring().c_str(), file_modes_w[+mode]);
#else
    f     = fopen(file_name.string().c_str(), file_modes[+mode]);
    int e = errno;
#endif
    if (f)
        return f;
    return unexpected(posixToResult(e));
}

#if defined _MSC_VER // MSVC
#define IO_SEEK_64 _fseeki64
#define IO_TELL_64 _ftelli64
#elif defined BRISK_WINDOWS // MinGW
#define IO_SEEK_64 fseeko64
#define IO_TELL_64 ftello64
#else // macOS, Linux
#define IO_SEEK_64 fseeko
#define IO_TELL_64 ftello
#endif

class FileStream final : public Stream {
public:
    StreamCapabilities caps() const noexcept final {
        return m_caps;
    }

    uint64_t size() const {
        uint64_t saved = IO_TELL_64(m_file);
        IO_SEEK_64(m_file, 0, SEEK_END);
        uint64_t size = IO_TELL_64(m_file);
        IO_SEEK_64(m_file, saved, SEEK_SET);
        return size;
    }

    bool truncate() {
        return false;
    }

    ~FileStream() {
        if (m_owns) {
            std::fclose(m_file);
        }
    }

    explicit FileStream(std::FILE* file, bool owns, StreamCapabilities caps)
        : m_file(std::move(file)), m_owns(owns), m_caps(caps) {}

    [[nodiscard]] bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) final {
        return IO_SEEK_64(m_file, position,
                          staticMap(origin, SeekOrigin::Beginning, SEEK_SET, SeekOrigin::Current, SEEK_CUR,
                                    SeekOrigin::End, SEEK_END, SEEK_SET)) == 0;
    }

    [[nodiscard]] uint64_t tell() const final {
        return IO_TELL_64(m_file);
    }

    [[nodiscard]] Transferred read(std::byte* data, size_t size) final {
        if (!m_file || ferror(m_file))
            return Transferred::Error;
        if (feof(m_file))
            return Transferred::Eof;
        return fread(data, 1, size, m_file);
    }

    [[nodiscard]] Transferred write(const std::byte* data, size_t size) final {
        if (!m_file || ferror(m_file))
            return Transferred::Error;
        return fwrite(data, 1, size, m_file);
    }

    [[nodiscard]] bool flush() final {
        fflush(m_file);
        return true;
    }

private:
    std::FILE* m_file;
    bool m_owns;
    StreamCapabilities m_caps;
};

RC<Stream> openFile(std::FILE* file, bool owns) {
    return rcnew FileStream(file, owns, StreamCapabilities::All);
}

RC<Stream> stdoutStream() {
    return rcnew FileStream(stdout, false, StreamCapabilities::CanWrite | StreamCapabilities::CanFlush);
}

RC<Stream> stderrStream() {
    return rcnew FileStream(stderr, false, StreamCapabilities::CanWrite | StreamCapabilities::CanFlush);
}

RC<Stream> stdinStream() {
    return rcnew FileStream(stdin, false, StreamCapabilities::CanRead);
}

expected<RC<Stream>, IOError> openFile(const fs::path& filePath, OpenFileMode mode) {
    return fopen_native(filePath, mode).map([mode](std::FILE* f) {
        return rcnew FileStream(f, true, file_caps[+mode]);
    });
}

expected<RC<Stream>, IOError> openFileForReading(const fs::path& filePath) {
    return openFile(filePath, OpenFileMode::ReadExisting);
}

expected<RC<Stream>, IOError> openFileForWriting(const fs::path& filePath, bool appending) {
    return openFile(filePath, appending ? OpenFileMode::AppendOrCreate : OpenFileMode::RewriteOrCreate);
}

std::optional<uint64_t> writeFromReader(RC<Stream> dest, RC<Stream> src, size_t bufSize) {
    uint64_t transferred = 0;
    auto buf             = std::unique_ptr<std::byte[]>(new std::byte[bufSize]);
    Transferred rd;
    while ((rd = src->read(buf.get(), bufSize))) {
        if (dest->write(buf.get(), rd.bytes()) != rd.bytes())
            return transferred;
        transferred += rd.bytes();
    }
    if (!dest->flush())
        return std::nullopt;
    if (rd.isError())
        return std::nullopt;
    return transferred;
}

expected<Bytes, IOError> readBytes(const fs::path& file_name) {
    return openFileForReading(file_name).and_then([](const RC<Stream>& r) -> expected<Bytes, IOError> {
        auto rd = r->readUntilEnd();
        if (rd)
            return *rd;
        else
            return unexpected(IOError::CantRead);
    });
}

expected<std::string, IOError> readUtf8(const fs::path& file_name, bool removeBOM) {
    return readBytes(file_name).map([removeBOM](const Bytes& b) {
        if (removeBOM)
            return std::string(utf8SkipBom(std::string((const char*)b.data(), b.size())));
        else
            return std::string((const char*)b.data(), b.size());
    });
}

expected<Json, IOError> readJson(const fs::path& file_name) {
    return readUtf8(file_name).map([](const std::string& b) {
        return Json::fromJson(b).value_or(JsonNull{});
    });
}

expected<Json, IOError> readMsgpack(const fs::path& file_name) {
    return readBytes(file_name).map([](const Bytes& b) {
        return Json::fromMsgPack(b).value_or(JsonNull{});
    });
}

expected<std::vector<std::string>, IOError> readLines(const fs::path& file_name) {
    return readUtf8(file_name).map([](const std::string& b) {
        auto sv = split(b, "\n");
        std::vector<std::string> result(sv.begin(), sv.end());
        return result;
    });
}

status<IOError> writeBytes(const fs::path& file_name, const BytesView& b) {
    return openFileForWriting(file_name).and_then([b](const RC<Stream>& w) -> status<IOError> {
        return unexpected_if(w->writeAll(b), IOError::CantWrite);
    });
}

status<IOError> writeUtf8(const fs::path& file_name, std::string_view str, bool useBOM) {
    BytesView bv = toBytesView(str);
    if (useBOM) {
        return openFileForWriting(file_name).and_then([bv](const RC<Stream>& w) -> status<IOError> {
            return unexpected_if(w->writeAll(toBytesView(utf8_bom)) && w->writeAll(bv), IOError::CantWrite);
        });
    } else {
        return writeBytes(file_name, bv);
    }
}

status<IOError> writeJson(const fs::path& file_name, const Json& j, int indent) {
    return writeUtf8(file_name, j.toJson(indent));
}

status<IOError> writeMsgpack(const fs::path& file_name, const Json& j) {
    return writeBytes(file_name, j.toMsgPack());
}

fs::path executableOrBundlePath() {
    fs::path p = executablePath();
    if (lowerCase(p.parent_path().filename().string()) == "macos" &&
        lowerCase(p.parent_path().parent_path().filename().string()) == "contents") {
        return p.parent_path().parent_path().parent_path();
    }
    return p;
}

fs::path uniqueFileName(std::string_view base, std::string_view numbered, int i) {
    if (!fs::exists(base))
        return base;
    while (fs::exists(fmt::format(fmt::runtime(numbered), i))) {
        i++;
    }
    return fmt::format(fmt::runtime(numbered), i);
}

static std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
static std::mutex rnd_mutex;
static const std::string_view characters = "abcdefghijklmnopqrstuvwxyz0123456789";

fs::path tempFilePath(std::string pattern) {
    std::lock_guard lk(rnd_mutex);
    fs::path tmp = fs::temp_directory_path();
    for (int i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '?')
            pattern[i] = characters[rnd() % characters.size()];
        else if (pattern[i] == '*') {
            pattern.erase(i, 1);
            for (int j = 0; j < 16; j++) {
                pattern.insert(pattern.begin() + i, characters[rnd() % characters.size()]);
            }
            break;
        }
    }
    return tmp / pattern;
}

std::optional<fs::path> findDirNextToExe(std::string_view dirName) {
    fs::path path = executablePath();
    for (;;) {
        path = path.parent_path();
        if (!path.has_relative_path())
            return std::nullopt;
        if (fs::path dirPath = path / dirName; fs::is_directory(dirPath))
            return dirPath;
    }
}

fs::path platformDefaultFolder(DefaultFolder folder);

static std::string strOr(std::string a, std::string_view b) {
    return a.empty() ? std::string(b) : std::move(a);
}

constexpr static std::string_view defaultVendor = "Brisk";
constexpr static std::string_view defaultName   = "App";

fs::path defaultFolder(DefaultFolder folder) {
    switch (folder) {
    case DefaultFolder::Documents:
    case DefaultFolder::Pictures:
    case DefaultFolder::Music:
    case DefaultFolder::UserData:
    case DefaultFolder::SystemData:
    case DefaultFolder::Home:
        return platformDefaultFolder(folder);
    case DefaultFolder::VendorUserData:
    case DefaultFolder::VendorSystemData:
    case DefaultFolder::VendorHome:
        return platformDefaultFolder(static_cast<DefaultFolder>(+folder - +DefaultFolder::VendorUserData +
                                                                +DefaultFolder::UserData)) /
               strOr(appMetadata.vendor, defaultVendor);
    case DefaultFolder::AppUserData:
    case DefaultFolder::AppSystemData:
    case DefaultFolder::AppHome:
        return platformDefaultFolder(static_cast<DefaultFolder>(+folder - +DefaultFolder::AppUserData +
                                                                +DefaultFolder::UserData)) /
               strOr(appMetadata.vendor, defaultVendor) / strOr(appMetadata.name, defaultName);
    default:
        BRISK_UNREACHABLE();
    }
}

} // namespace Brisk
