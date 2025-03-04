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
#include <share.h>
#include <shlobj.h>
#include <windows.h>
#include "io.h"

#include <brisk/core/IO.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Text.hpp>
#include <sys/stat.h>

namespace Brisk {

struct Win32Handle {
    Win32Handle() noexcept : handle(INVALID_HANDLE_VALUE) {}

    Win32Handle(HANDLE handle) noexcept : handle(handle) {}

    Win32Handle(const Win32Handle& copy)            = delete;
    Win32Handle& operator=(const Win32Handle& copy) = delete;

    Win32Handle(Win32Handle&& move) noexcept : handle(move.handle) {
        move.handle = INVALID_HANDLE_VALUE;
    }

    Win32Handle& operator=(Win32Handle&& move) noexcept {
        if (this != &move) {
            CloseHandle(handle);
            handle      = move.handle;
            move.handle = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    ~Win32Handle() {
        CloseHandle(handle); // closing an INVALID_HANDLE_VALUE is no-op
    }

    HANDLE get() const noexcept {
        return handle;
    }

    operator bool() const noexcept {
        return handle != INVALID_HANDLE_VALUE;
    }

    HANDLE handle;
};

static LARGE_INTEGER toLarge(int64_t value) {
    LARGE_INTEGER result;
    result.QuadPart = value;
    return result;
}

constexpr static size_t batchSize = 1 << 30; // 1GiB

class Win32HandleStream final : public Stream {
public:
    [[nodiscard]] StreamCapabilities caps() const noexcept {
        return StreamCapabilities::CanRead | StreamCapabilities::CanWrite | StreamCapabilities::CanSeek |
               StreamCapabilities::CanFlush | StreamCapabilities::CanTruncate | StreamCapabilities::HasSize;
    }

    [[nodiscard]] uint64_t size() const final {
        LARGE_INTEGER fs;
        if (!GetFileSizeEx(m_handle.get(), &fs))
            return invalidSize;
        return fs.QuadPart;
    }

    [[nodiscard]] bool truncate() final {
        return SetEndOfFile(m_handle.get());
    }

    explicit Win32HandleStream(Win32Handle handle) noexcept : m_handle(std::move(handle)) {}

    [[nodiscard]] bool seek(int64_t position, SeekOrigin origin = SeekOrigin::Beginning) final {
        if (!m_handle)
            return false;
        if (!SetFilePointerEx(m_handle.get(), toLarge(position), nullptr,
                              staticMap(origin, SeekOrigin::Beginning, FILE_BEGIN, SeekOrigin::Current,
                                        FILE_CURRENT, SeekOrigin::End, FILE_END, FILE_BEGIN))) {
            return false;
        }
        return true;
    }

    [[nodiscard]] uint64_t tell() const final {
        if (!m_handle)
            return invalidPosition;
        LARGE_INTEGER result;
        if (!SetFilePointerEx(m_handle.get(), toLarge(0), &result, FILE_CURRENT)) {
            return invalidPosition;
        }
        return result.QuadPart;
    }

    [[nodiscard]] Transferred read(std::byte* data, size_t size) final {
        if (!m_handle)
            return Transferred::Error;

        size_t remaining = size;

        while (remaining > 0) {
            // Ensure we don't try to read more than 1 GiB in one call to ReadFile
            DWORD currentSize = (DWORD)std::min(batchSize, (size_t)remaining);
            DWORD bytesRead   = 0;
            if (!ReadFile(m_handle.get(), data, currentSize, &bytesRead, nullptr)) {
                return Transferred::Error; // If ReadFile fails
            }
            if (bytesRead == 0) {
                // End of file reached, return the total bytes read so far
                return remaining < size ? size - remaining : Transferred::Eof;
            }

            remaining -= bytesRead;
            data += bytesRead;
        }
        // If we reached here, all chunks were read, return full size
        return size;
    }

    [[nodiscard]] Transferred write(const std::byte* data, size_t size) final {
        if (!m_handle)
            return Transferred::Error;

        size_t remaining = size;

        while (remaining > 0) {
            // Ensure we don't try to write more than 1 GiB in one call to WriteFile
            DWORD currentSize  = (DWORD)std::min(batchSize, (size_t)remaining);
            DWORD bytesWritten = 0;

            if (!WriteFile(m_handle.get(), data, currentSize, &bytesWritten, nullptr) || bytesWritten == 0) {
                return Transferred::Error; // If WriteFile fails or no bytes written
            }

            remaining -= bytesWritten;
            data += bytesWritten;
        }
        return size - remaining;
    }

    [[nodiscard]] bool flush() final {
        if (!m_handle)
            return false;
        return FlushFileBuffers(m_handle.get());
    }

private:
    Win32Handle m_handle;
};

static REFKNOWNFOLDERID folderId(DefaultFolder folder) {
    switch (folder) {
    case DefaultFolder::Home:
        return FOLDERID_Profile;
    case DefaultFolder::Documents:
        return FOLDERID_Documents;
    case DefaultFolder::Music:
        return FOLDERID_Music;
    case DefaultFolder::Pictures:
        return FOLDERID_Pictures;
    case DefaultFolder::UserData:
        return FOLDERID_RoamingAppData;
    case DefaultFolder::SystemData:
        return FOLDERID_ProgramData;
    default:
        return FOLDERID_Documents;
    }
}

static fs::path platformDefaultFolder(REFKNOWNFOLDERID folder) {
    PWSTR pstr = nullptr;
    SHGetKnownFolderPath(folder, 0, NULL, &pstr);
    std::wstring str(pstr);
    CoTaskMemFree(pstr);
    return wcsToUtf8(str);
}

fs::path platformDefaultFolder(DefaultFolder folder) {
    return platformDefaultFolder(folderId(folder));
}

std::vector<fs::path> fontFolders() {
    return { platformDefaultFolder(FOLDERID_Fonts), // System font folder must be first
             platformDefaultFolder(FOLDERID_LocalAppData) / "Microsoft" / "Windows" / "Fonts" };
}

fs::path executablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(0), path, MAX_PATH);
    return wcsToUtf8(path);
}

} // namespace Brisk
