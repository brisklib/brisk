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
#include <catch2/catch_all.hpp>
#include <brisk/core/Rc.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Stream.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {

struct Slice {
    BytesView bytes;
    size_t offset = 0;

    Transferred read(uint8_t* data, size_t size) {
        return {};
    }

    Transferred write(const uint8_t* data, size_t size) {
        return {};
    }

    bool flush() {
        return true;
    }
};

class PartialWriter final : public Stream {
public:
    explicit PartialWriter(size_t maximumWrite, bool flushResult = true)
        : m_maximumWrite(maximumWrite), m_flushResult(flushResult) {}

    StreamCapabilities caps() const noexcept final {
        return StreamCapabilities::CanWrite | StreamCapabilities::CanFlush;
    }

    Transferred read(std::byte*, size_t) final {
        return Transferred::Error;
    }

    Transferred write(const std::byte* data, size_t size) final {
        if (size == 0)
            return 0;
        const size_t count = std::min(size, m_maximumWrite);
        m_data.insert(m_data.end(), data, data + count);
        return count;
    }

    bool flush() final {
        return m_flushResult;
    }

    bool seek(int64_t, SeekOrigin = SeekOrigin::Beginning) final {
        return false;
    }

    uint64_t tell() const final {
        return invalidPosition;
    }

    uint64_t size() const final {
        return invalidSize;
    }

    bool truncate() final {
        return false;
    }

    const Bytes& data() const {
        return m_data;
    }

private:
    Bytes m_data;
    size_t m_maximumWrite;
    bool m_flushResult;
};

TEST_CASE("stdoutStream") {
    CHECK(stdoutStream()->canWrite());
    CHECK(!stdoutStream()->canRead());
    CHECK(stdoutStream()->write("stdout\n\n") == 8);
    CHECK(stderrStream()->write("stderr\n\n") == 8);
    CHECK(!stdinStream()->canWrite());
    CHECK(stdinStream()->canRead());
}

TEST_CASE("defaultPaths") {
    fmt::println("DefaultFolder::Documents = {}", defaultFolder(DefaultFolder::Documents).string());
    fmt::println("DefaultFolder::Pictures = {}", defaultFolder(DefaultFolder::Pictures).string());
    fmt::println("DefaultFolder::Music = {}", defaultFolder(DefaultFolder::Music).string());
    fmt::println("DefaultFolder::UserData = {}", defaultFolder(DefaultFolder::UserData).string());
    fmt::println("DefaultFolder::SystemData = {}", defaultFolder(DefaultFolder::SystemData).string());
    fmt::println("DefaultFolder::Home = {}", defaultFolder(DefaultFolder::Home).string());
    fmt::println("DefaultFolder::VendorUserData = {}", defaultFolder(DefaultFolder::VendorUserData).string());
    fmt::println("DefaultFolder::VendorSystemData = {}",
                 defaultFolder(DefaultFolder::VendorSystemData).string());
    fmt::println("DefaultFolder::VendorHome = {}", defaultFolder(DefaultFolder::VendorHome).string());
    fmt::println("DefaultFolder::AppUserData = {}", defaultFolder(DefaultFolder::AppUserData).string());
    fmt::println("DefaultFolder::AppSystemData = {}", defaultFolder(DefaultFolder::AppSystemData).string());
    fmt::println("DefaultFolder::AppHome = {}", defaultFolder(DefaultFolder::AppHome).string());
}

TEST_CASE("executablePath") {
    fmt::println("executablePath = {}", executablePath().string());
}

TEST_CASE("fileModesAndStreamOperations") {
    const fs::path path = tempFilePath("io-test-????????.bin");
    REQUIRE(writeBytes(path, toBytesView("abc")));

    auto append = openFileForAppending(path);
    REQUIRE(append.has_value());
    CHECK((*append)->write("de") == 2);
    CHECK((*append)->flush());
    append->reset();

    auto bytes = readBytes(path);
    REQUIRE(bytes);
    CHECK(std::string(reinterpret_cast<const char*>(bytes->data()), bytes->size()) == "abcde");

    auto rewrite = openFile(path, OpenFileMode::ReadRewriteOrCreate);
    REQUIRE(rewrite.has_value());
    REQUIRE((*rewrite)->write("abcdef") == 6);
    REQUIRE((*rewrite)->seek(3));
    CHECK((*rewrite)->truncate());
    CHECK((*rewrite)->size() == 3);
    rewrite->reset();

    std::error_code ec;
    fs::remove(path, ec);
}

TEST_CASE("fileArgumentValidation") {
    CHECK_THROWS_AS(openFile(static_cast<std::FILE*>(nullptr)), EArgument);
    CHECK_THROWS_AS(writeFromReader({}, rcnew MemoryStream()), EArgument);
    CHECK_THROWS_AS(writeFromReader(rcnew MemoryStream(), {}, 1), EArgument);
    CHECK_THROWS_AS(writeFromReader(rcnew MemoryStream(), rcnew MemoryStream(), 0), EArgument);
}

TEST_CASE("fileModeCapabilities") {
    const fs::path path = tempFilePath("io-modes-????????.bin");
    REQUIRE(writeBytes(path, toBytesView("abc")));

    auto read = openFile(path, OpenFileMode::ReadExisting);
    REQUIRE(read.has_value());
    CHECK((*read)->canRead());
    CHECK(!(*read)->canWrite());
    CHECK((*read)->canSeek());
    CHECK((*read)->hasSize());
    read->reset();

    auto readWrite = openFile(path, OpenFileMode::ReadWriteExisting);
    REQUIRE(readWrite.has_value());
    CHECK((*readWrite)->canRead());
    CHECK((*readWrite)->canWrite());
    CHECK((*readWrite)->canTruncate());
    readWrite->reset();

    auto rewrite = openFile(path, OpenFileMode::RewriteOrCreate);
    REQUIRE(rewrite.has_value());
    CHECK(!(*rewrite)->canRead());
    CHECK((*rewrite)->canWrite());
    CHECK((*rewrite)->canTruncate());
    rewrite->reset();

    auto readRewrite = openFile(path, OpenFileMode::ReadRewriteOrCreate);
    REQUIRE(readRewrite.has_value());
    CHECK((*readRewrite)->canRead());
    CHECK((*readRewrite)->canWrite());
    CHECK((*readRewrite)->canTruncate());
    readRewrite->reset();

    auto append = openFile(path, OpenFileMode::AppendOrCreate);
    REQUIRE(append.has_value());
    CHECK(!(*append)->canRead());
    CHECK((*append)->canWrite());
    CHECK((*append)->canTruncate());
    append->reset();

    CHECK_THROWS_AS(openFile(path, static_cast<OpenFileMode>(99)), EArgument);

    std::error_code ec;
    fs::remove(path, ec);
}

TEST_CASE("fileStreamZeroLengthOperations") {
    const fs::path path = tempFilePath("io-zero-????????.bin");
    REQUIRE(writeBytes(path, toBytesView("abc")));

    auto file = openFile(path, OpenFileMode::ReadWriteExisting);
    REQUIRE(file.has_value());
    CHECK((*file)->read(static_cast<std::byte*>(nullptr), 0) == 0);
    CHECK((*file)->write(static_cast<const std::byte*>(nullptr), 0) == 0);
    CHECK((*file)->tell() == 0);
    CHECK((*file)->size() == 3);
    file->reset();

    std::error_code ec;
    fs::remove(path, ec);
}

TEST_CASE("partialWritesAreRetried") {
    Rc<Stream> writer = rcnew PartialWriter(2);
    CHECK(writer->writeAll(toBytesView("abcdef")));
    CHECK(std::string(reinterpret_cast<const char*>(std::static_pointer_cast<PartialWriter>(writer)->data().data()),
                      6) == "abcdef");

    Rc<Stream> source = rcnew MemoryStream(toBytes("abcdef"));
    auto partial = rcnew PartialWriter(2);
    auto result = writeFromReader(partial, source, 3);
    REQUIRE(result.has_value());
    CHECK(*result == 6);
    CHECK(std::string(reinterpret_cast<const char*>(partial->data().data()), partial->data().size()) == "abcdef");
}

TEST_CASE("transferFailuresAreReported") {
    Rc<Stream> source = rcnew MemoryStream(toBytes("abcdef"));

    auto writeFailure = rcnew PartialWriter(0);
    CHECK(!writeFromReader(writeFailure, source, 3).has_value());

    source = rcnew MemoryStream(toBytes("abcdef"));
    auto flushFailure = rcnew PartialWriter(8, false);
    CHECK(!writeFromReader(flushFailure, source, 3).has_value());
}

TEST_CASE("tempFilePathGeneratesCandidates") {
    const fs::path question = tempFilePath("io-temp-????.tmp");
    const fs::path tempDirectory = fs::temp_directory_path();
    CHECK(question.string().starts_with(tempDirectory.string()));
    CHECK(question.filename().string().size() == std::string("io-temp-????.tmp").size());
    CHECK(question.filename().string().find('?') == std::string::npos);

    const fs::path star = tempFilePath("io-temp-*.tmp");
    CHECK(star.string().starts_with(tempDirectory.string()));
    CHECK(star.filename().string().size() == std::string("io-temp-????????????????.tmp").size());
    CHECK(star.filename().string().find('*') == std::string::npos);
    CHECK(!fs::exists(star));
}

} // namespace Brisk
