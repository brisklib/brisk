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
#include <mutex>
#include <random>

#include <fmt/format.h>

#include <brisk/core/App.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/Utilities.hpp>

namespace Brisk {

expected<Rc<Stream>, IoError> openFileForReading(const fs::path& filePath) {
    return openFile(filePath, OpenFileMode::ReadExisting);
}

expected<Rc<Stream>, IoError> openFileForWriting(const fs::path& filePath, bool appending) {
    return openFile(filePath, appending ? OpenFileMode::AppendOrCreate : OpenFileMode::RewriteOrCreate);
}

std::optional<uint64_t> writeFromReader(Rc<Stream> dest, Rc<Stream> src, size_t bufSize) {
    if (!dest || !src)
        throwException(EArgument("writeFromReader requires non-null streams"));
    if (bufSize == 0)
        throwException(EArgument("writeFromReader requires a non-zero buffer size"));
    if (!src->canRead() || !dest->canWrite() || !dest->canFlush())
        throwException(EArgument("writeFromReader streams lack required capabilities"));

    uint64_t transferred = 0;
    auto buf             = std::unique_ptr<std::byte[]>(new std::byte[bufSize]);
    Transferred rd;
    while ((rd = src->read(buf.get(), bufSize))) {
        size_t written = 0;
        while (written < rd.bytes()) {
            const Transferred wr = dest->write(buf.get() + written, rd.bytes() - written);
            if (wr.isError() || wr.bytes() == 0 || wr.bytes() > rd.bytes() - written)
                return std::nullopt;
            written += wr.bytes();
        }
        transferred += written;
    }
    if (!dest->flush())
        return std::nullopt;
    if (rd.isError())
        return std::nullopt;
    return transferred;
}

expected<Bytes, IoError> readBytes(const fs::path& file_name) {
    return openFileForReading(file_name).and_then([](const Rc<Stream>& r) -> expected<Bytes, IoError> {
        auto rd = r->readUntilEnd();
        if (rd)
            return *rd;
        else
            return unexpected(IoError::CantRead);
    });
}

expected<std::string, IoError> readUtf8(const fs::path& file_name, bool removeBOM) {
    return readBytes(file_name).map([removeBOM](const Bytes& b) {
        if (removeBOM)
            return std::string(utf8SkipBom(std::string((const char*)b.data(), b.size())));
        else
            return std::string((const char*)b.data(), b.size());
    });
}

expected<Json, IoError> readJson(const fs::path& file_name) {
    return readUtf8(file_name).map([](const std::string& b) {
        return Json::fromJson(b).value_or(JsonNull{});
    });
}

expected<Json, IoError> readMsgpack(const fs::path& file_name) {
    return readBytes(file_name).map([](const Bytes& b) {
        return Json::fromMsgPack(b).value_or(JsonNull{});
    });
}

expected<std::vector<std::string>, IoError> readLines(const fs::path& file_name) {
    return readUtf8(file_name).map([](const std::string& b) {
        auto sv = split(b, "\n");
        std::vector<std::string> result(sv.begin(), sv.end());
        return result;
    });
}

status<IoError> writeBytes(const fs::path& file_name, const BytesView& b) {
    return openFileForWriting(file_name).and_then([b](const Rc<Stream>& w) -> status<IoError> {
        return unexpected_if(w->writeAll(b), IoError::CantWrite);
    });
}

status<IoError> writeUtf8(const fs::path& file_name, std::string_view str, bool useBOM) {
    BytesView bv = toBytesView(str);
    if (useBOM) {
        return openFileForWriting(file_name).and_then([bv](const Rc<Stream>& w) -> status<IoError> {
            return unexpected_if(w->writeAll(toBytesView(utf8_bom)) && w->writeAll(bv), IoError::CantWrite);
        });
    } else {
        return writeBytes(file_name, bv);
    }
}

status<IoError> writeJson(const fs::path& file_name, const Json& j, int indent) {
    return writeUtf8(file_name, j.toJson(indent));
}

status<IoError> writeMsgpack(const fs::path& file_name, const Json& j) {
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
