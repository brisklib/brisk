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

#include <brisk/core/Brisk.h>
#include "Stream.hpp"
#include "internal/Expected.hpp"
#include "internal/Filesystem.hpp"
#include "Reflection.hpp"

namespace Brisk {

enum class IOError {
    NotFound,
    AccessDenied,
    NoSpace,
    UnknownError,
    CantRead,
    CantWrite,
    UnsupportedFormat,
};

template <>
inline constexpr std::initializer_list<NameValuePair<IOError>> defaultNames<IOError>{
    { "NotFound", IOError::NotFound },
    { "AccessDenied", IOError::AccessDenied },
    { "NoSpace", IOError::NoSpace },
    { "UnknownError", IOError::UnknownError },
    { "CantRead", IOError::CantRead },
    { "CantWrite", IOError::CantWrite },
    { "UnsupportedFormat", IOError::UnsupportedFormat },
};
/**
 * @enum OpenFileMode
 * @brief Enum class representing file opening modes.
 */
enum class OpenFileMode {
    /**
     * @brief Opens a file for reading.
     *
     * The file must exist; if it does not, the open fails.
     */
    ReadExisting        = 0,

    /**
     * @brief Opens a file for reading and writing.
     *
     * The file must exist.
     */
    ReadWriteExisting   = 1,

    /**
     * @brief Opens a file for writing.
     *
     * If the file exists, it is truncated. If it does not exist,
     * a new file is created.
     */
    RewriteOrCreate     = 2,

    /**
     * @brief Opens a file for reading and writing.
     *
     * If the file exists, it is truncated; if not, a new file is created.
     */
    ReadRewriteOrCreate = 3,

    /**
     * @brief Opens a file for appending.
     *
     * If the file exists, data is appended at the end. If it does
     * not exist, a new file is created.
     */
    AppendOrCreate      = 3,

    /**
     * @brief Alias for ReadExisting.
     */
    r                   = ReadExisting,

    /**
     * @brief Alias for ReadWriteExisting.
     */
    r_plus              = ReadWriteExisting,

    /**
     * @brief Alias for RewriteOrCreate.
     */
    w                   = RewriteOrCreate,

    /**
     * @brief Alias for ReadRewriteOrCreate.
     */
    w_plus              = ReadRewriteOrCreate,

    /**
     * @brief Alias for AppendOrCreate.
     */
    a                   = AppendOrCreate,
};

template <>
inline constexpr std::initializer_list<NameValuePair<OpenFileMode>> defaultNames<OpenFileMode>{
    { "ReadExisting", OpenFileMode::ReadExisting },
    { "ReadWriteExisting", OpenFileMode::ReadWriteExisting },
    { "RewriteOrCreate", OpenFileMode::RewriteOrCreate },
    { "AppendOrCreate", OpenFileMode::AppendOrCreate },
};

template <>
constexpr inline bool isBitFlags<OpenFileMode> = true;

/**
 * @brief Opens a file with the specified path and mode.
 *
 * This function attempts to open a file using the provided path
 * and file mode. It returns an `expected` object containing either
 * a reference-counted stream on success or an I/O error on failure.
 *
 * @param filePath The path to the file to be opened.
 * @param mode The mode in which to open the file (default is
 *             `OpenFileMode::ReadWriteExisting`).
 * @return An `expected` object containing a reference-counted
 *         stream or an I/O error.
 */
[[nodiscard]] expected<RC<Stream>, IOError> openFile(const fs::path& filePath,
                                                     OpenFileMode mode = OpenFileMode::ReadWriteExisting);

/**
 * @brief Opens a file for reading.
 *
 * This function opens the specified file for reading. It returns
 * an `expected` object containing either a reference-counted
 * stream on success or an I/O error on failure.
 *
 * @param filePath The path to the file to be opened for reading.
 * @return An `expected` object containing a reference-counted
 *         stream or an I/O error.
 */
[[nodiscard]] expected<RC<Stream>, IOError> openFileForReading(const fs::path& filePath);

/**
 * @brief Opens a file for writing.
 *
 * This function opens the specified file for writing. If the
 * `appending` parameter is set to `true`, data will be appended
 * to the end of the file. It returns an `expected` object
 * containing either a reference-counted stream on success or
 * an I/O error on failure.
 *
 * @param filePath The path to the file to be opened for writing.
 * @param appending A flag indicating whether to append data
 *                  to the end of the file (default is `false`).
 * @return An `expected` object containing a reference-counted
 *         stream or an I/O error.
 */
[[nodiscard]] expected<RC<Stream>, IOError> openFileForWriting(const fs::path& filePath,
                                                               bool appending = false);

/**
 * @brief Opens a file for appending.
 *
 * This function is a convenience wrapper that opens the specified
 * file for writing in append mode. It returns an `expected`
 * object containing either a reference-counted stream on success
 * or an I/O error on failure.
 *
 * @param filePath The path to the file to be opened for appending.
 * @return An `expected` object containing a reference-counted
 *         stream or an I/O error.
 */
[[nodiscard]] inline expected<RC<Stream>, IOError> openFileForAppending(const fs::path& filePath) {
    return openFileForWriting(filePath, true);
}

/**
 * @brief Opens a file using a native file pointer.
 *
 * This function opens a file given a native `FILE*` pointer.
 * It allows specifying whether the function should take ownership
 * of the file pointer. It returns a reference-counted stream.
 *
 * @param file A pointer to the native file to be opened.
 * @param owns A flag indicating whether to take ownership of
 *             the file pointer (default is `false`).
 * @return A reference-counted stream.
 */
[[nodiscard]] RC<Stream> openFile(std::FILE* file, bool owns = false);

/**
 * @brief Retrieves the standard output stream.
 *
 * This function returns a reference-counted stream that points
 * to the standard output.
 *
 * @return A reference-counted stream for standard output.
 */
[[nodiscard]] RC<Stream> stdoutStream();

/**
 * @brief Retrieves the standard error stream.
 *
 * This function returns a reference-counted stream that points
 * to the standard error output.
 *
 * @return A reference-counted stream for standard error.
 */
[[nodiscard]] RC<Stream> stderrStream();

/**
 * @brief Retrieves the standard input stream.
 *
 * This function returns a reference-counted stream that points
 * to the standard input.
 *
 * @return A reference-counted stream for standard input.
 */
[[nodiscard]] RC<Stream> stdinStream();

/**
 * @brief Opens a file using the fopen function. This function handles file name encoding across all
 * platforms.
 *
 * This function wraps the `fopen` call and returns an
 * `expected` object containing either a file pointer or
 * an I/O error.
 *
 * @param file_name The path to the file to be opened.
 * @param mode The mode in which to open the file.
 * @return An `expected` object containing a file pointer
 *         or an I/O error.
 */
[[nodiscard]] expected<std::FILE*, IOError> fopen_native(const fs::path& file_name, OpenFileMode mode);

/**
 * @brief Reads the entire file as a vector of bytes.
 *
 * This function reads all the contents of the specified file
 * and returns them as a vector of bytes. It handles any I/O
 * errors that may occur during the read operation, returning
 * an `expected` object that contains either the byte vector
 * or an I/O error.
 *
 * @param file_name The path to the file to be read.
 * @return An `expected` object containing a vector of bytes if
 *         the read operation is successful, or an I/O error if
 *         it fails.
 */
[[nodiscard]] expected<Bytes, IOError> readBytes(const fs::path& file_name);

/**
 * @brief Reads the entire file as a UTF-8 encoded string.
 *
 * This function reads a UTF-8 encoded string from the specified
 * file. It can optionally remove the Byte Order Mark (BOM).
 * It returns an `expected` object containing either the string
 * or an I/O error.
 *
 * @param file_name The path to the file to be read.
 * @param removeBOM A flag indicating whether to remove the BOM
 *                   (default is `true`).
 * @return An `expected` object containing the UTF-8 string or
 *         an I/O error.
 */
[[nodiscard]] expected<std::string, IOError> readUtf8(const fs::path& file_name, bool removeBOM = true);

/**
 * @brief Reads a JSON object from a file.
 *
 * This function reads a JSON object from the specified file and
 * returns an `expected` object containing either the JSON object
 * or an I/O error.
 *
 * @param file_name The path to the file to be read.
 * @return An `expected` object containing the JSON object or an
 *         I/O error.
 */
[[nodiscard]] expected<Json, IOError> readJson(const fs::path& file_name);

/**
 * @brief Reads a JSON object in MessagePack format from a file.
 *
 * This function reads a JSON object in MessagePack format from the specified
 * file and returns an `expected` object containing either the
 * JSON object or an I/O error.
 *
 * @param file_name The path to the file to be read.
 * @return An `expected` object containing the MessagePack object
 *         or an I/O error.
 */
[[nodiscard]] expected<Json, IOError> readMsgpack(const fs::path& file_name);

/**
 * @brief Reads the entire file as a vector of UTF8-encoded strings.
 *
 * This function reads lines from the specified file and returns
 * an `expected` object containing either a vector of strings
 * (each representing a line) or an I/O error.
 *
 * @param file_name The path to the file to be read.
 * @return An `expected` object containing a vector of lines or
 *         an I/O error.
 */
[[nodiscard]] expected<std::vector<std::string>, IOError> readLines(const fs::path& file_name);

/**
 * @brief Writes a byte span to a file.
 *
 * This function writes bytes to the specified file and returns
 * a status indicating success or an I/O error.
 *
 * @param file_name The path to the file to be written.
 * @param b The bytes to write to the file.
 * @return A status indicating success or an I/O error.
 */
[[nodiscard]] status<IOError> writeBytes(const fs::path& file_name, const BytesView& b);

/**
 * @brief Writes a UTF-8 encoded string to a file.
 *
 * This function writes a UTF-8 encoded string to the specified
 * file. It can optionally include a Byte Order Mark (BOM)
 * at the beginning of the file.
 *
 * @param file_name The path to the file to be written.
 * @param str The UTF-8 string to write to the file.
 * @param useBOM A flag indicating whether to include a BOM
 *                (default is `false`).
 * @return A status indicating success or an I/O error.
 */
[[nodiscard]] status<IOError> writeUtf8(const fs::path& file_name, std::string_view str, bool useBOM = false);

/**
 * @brief Writes a JSON object to a file.
 *
 * This function writes a JSON object to the specified file,
 * optionally formatting it with the specified indentation level.
 * If the indentation level is negative, tabs will be used instead
 * of spaces for formatting.
 *
 * @param file_name The path to the file to be written.
 * @param j The JSON object to write to the file.
 * @param indent The number of spaces to use for indentation
 *               (default is `0`).
 * @return A status indicating success or an I/O error.
 */
[[nodiscard]] status<IOError> writeJson(const fs::path& file_name, const Json& j, int indent = 0);

/**
 * @brief Writes a MessagePack object to a file.
 *
 * This function writes a MessagePack object to the specified file.
 *
 * @param file_name The path to the file to be written.
 * @param j The JSON object to write as MessagePack to the file.
 * @return A status indicating success or an I/O error.
 */
[[nodiscard]] status<IOError> writeMsgpack(const fs::path& file_name, const Json& j);

/**
 * @brief Writes data from a reader stream to a destination stream.
 *
 * This function reads data from the source stream and writes it
 * to the destination stream, returning the number of bytes written
 * or the nullopt value if an error occurs.
 *
 * @param dest The destination stream to write to.
 * @param src The source stream to read from.
 * @param bufSize The buffer size to use for reading (default is
 *                `65536` bytes).
 * @return An optional indicating the number of bytes written, or
 *         an empty optional if an error occurs.
 */
[[nodiscard]] std::optional<uint64_t> writeFromReader(RC<Stream> dest, RC<Stream> src,
                                                      size_t bufSize = 65536);

/**
 * @enum DefaultFolder
 * @brief Enum class representing default folder types.
 *
 * This enum defines various types of default folders that can be
 * accessed in the file system. These folders are commonly used
 * for storing user-related data.
 */
enum class DefaultFolder {
    Documents,  ///< The Documents folder.
    Pictures,   ///< The Pictures folder.
    Music,      ///< The Music folder.
    UserData,   ///< The User Data folder.
    SystemData, ///< The System Data folder.
    Home,       ///< User's Home folder.

    VendorUserData,   ///< Vendor-specific folder inside the User Data folder.
    VendorSystemData, ///< Vendor-specific folder inside the System Data folder.
    VendorHome,       ///< Vendor-specific folder inside user's Home folder.

    AppUserData,   ///< App-specific folder inside the User Data folder.
    AppSystemData, ///< App-specific folder inside the System Data folder.
    AppHome,       ///< App-specific folder inside user's Home folder.
};

constexpr auto operator+(DefaultFolder value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

/**
 * @brief Returns the path to a specified default folder.
 *
 * This function takes a `DefaultFolder` enumeration value and
 * returns the corresponding file system path to that folder.
 *
 * @param folder The default folder to retrieve.
 * @return A `fs::path` representing the path to the specified
 *         default folder.
 */
fs::path defaultFolder(DefaultFolder folder);

/**
 * @brief Retrieves the paths of available font folders.
 *
 * This function returns a vector containing the paths of
 * directories where fonts are stored. It can be used to
 * locate font files on the system.
 *
 * @return A vector of `fs::path` objects representing the font
 *         folder paths.
 */
std::vector<fs::path> fontFolders();

/**
 * @brief Retrieves the path to the executable file.
 *
 * This function returns the file system path of the currently
 * running executable.
 *
 * @return A `fs::path` representing the path to the executable file.
 */
fs::path executablePath();

/**
 * @brief Retrieves the path to the executable or bundle.
 *
 * This function returns the file system path of the current
 * executable or its associated bundle, if applicable. This is
 * useful for obtaining the location of the application package
 * on certain platforms.
 *
 * @return A `fs::path` representing the path to the executable
 *         or bundle.
 */
fs::path executableOrBundlePath();

/**
 * @brief Generates a unique file name based on a base name and a numbering pattern.
 *
 * This function checks if a file with the specified `base` name exists.
 * If it does, it appends an incrementing number to the `numbered` pattern
 * until a unique file name is found that does not already exist.
 *
 * The `numbered` pattern is formatted using the given integer `i` to create
 * the file name. The function ensures that the resulting file name is unique
 * by incrementing `i` until a non-existing name is generated.
 *
 * @param base The base name of the file to check for existence ("screenshot.png").
 * @param numbered A format string representing the naming pattern, which
 *                 can include placeholders for the incrementing number ("screenshot ({}).png").
 * @param i The starting number to be used in the naming pattern.
 *
 * @return A `fs::path` representing a unique file name based on the input
 *         parameters.
 */
fs::path uniqueFileName(std::string_view base, std::string_view numbered, int i = 1);

/**
 * @brief Generates a temporary file path based on a specified pattern.
 *
 * This function creates a path for a temporary file by replacing
 * placeholders in the given pattern. It replaces `?` with a random
 * character from the set of lowercase letters and digits, and `*`
 * with 16 random characters from the same set.
 *
 * The function is thread-safe, using a mutex to ensure that the random
 * number generator is accessed in a synchronized manner.
 *
 * @param pattern A string representing the desired pattern for the
 *                temporary file name. The pattern can include `?`
 *                and `*` as placeholders for random characters.
 *
 * @return A `fs::path` representing the full path to the generated
 *         temporary file.
 */
fs::path tempFilePath(std::string pattern);

/**
 * @brief Finds a directory next to the executable.
 *
 * This function searches for a directory specified by `dirName`
 * adjacent to the current executable's path. It traverses up the
 * directory hierarchy until it finds the specified directory or
 * reaches the root of the file system.
 *
 * @param dirName A string view representing the name of the directory
 *                to find.
 *
 * @return An optional containing the path to the found directory if it exists;
 *         otherwise, `std::nullopt` is returned if the directory is not found.
 */
std::optional<fs::path> findDirNextToExe(std::string_view dirName);

} // namespace Brisk

