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
#include "Bytes.hpp"
#include "BasicTypes.hpp"
#include "internal/Span.hpp"
#include "IO.hpp"
#include "Exceptions.hpp"

namespace Brisk {

/**
 * @brief Exception class for cryptographic errors.
 */
class ECrypto : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception;
};

/**
 * @brief Retrieves cryptographically secure random bytes.
 *
 * @param data The buffer to store the random bytes.
 * @return The number of bytes received.
 */
[[nodiscard]] size_t cryptoRandomInplaceSafe(BytesMutableView data);

/**
 * @brief Retrieves cryptographically secure random bytes.
 *
 * @param data The buffer to store the random bytes.
 * @exception CryptoException Thrown if there are not enough random bytes.
 */
void cryptoRandomInplace(BytesMutableView data);

/**
 * @brief Retrieves cryptographically secure random bytes.
 *
 * @param size The number of bytes to return.
 * @return The random bytes as a `Bytes` object.
 */
[[nodiscard]] Bytes cryptoRandom(size_t size);

/**
 * @brief Retrieves cryptographically secure random bytes as a fixed-size array.
 *
 * @tparam Size The size of the fixed byte array.
 * @param size_constant The size of the fixed byte array (unused).
 * @return The random bytes as a `FixedBytes<Size>` object.
 */
template <size_t Size>
[[nodiscard]] inline FixedBytes<Size> cryptoRandomFixed(size_constant<Size> = {}) {
    FixedBytes<Size> result;
    cryptoRandomInplace(result);
    return result;
}

/**
 * @brief Creates a RC<Stream> for cryptographically secure random bytes.
 *
 * @return A RC<Stream> for random bytes.
 */
[[nodiscard]] RC<Stream> cryptoRandomReader();

/**
 * @brief Enum class representing various hash methods.
 */
enum class HashMethod {
    MD5,            ///< MD5 hashing method
    SHA1,           ///< SHA-1 hashing method
    SHA256,         ///< SHA-256 hashing method
    SHA512,         ///< SHA-512 hashing method
    SHA3_256,       ///< SHA3-256 hashing method
    SHA3_512,       ///< SHA3-512 hashing method
    Last = SHA3_512 ///< Sentinel value for the last hash method
};

constexpr auto operator+(HashMethod x) noexcept {
    return static_cast<std::underlying_type_t<decltype(x)>>(x);
}

/**
 * @brief Returns the output bit size of the specified hash method.
 *
 * @param method The hash method for which to retrieve the bit size.
 * @return The bit size of the hash output for the given method.
 *         Returns 0 if the method is unrecognized.
 */
constexpr size_t hashBitSize(HashMethod method) {
    switch (method) {
    case HashMethod::MD5:
        return 128;
    case HashMethod::SHA1:
        return 160;
    case HashMethod::SHA256:
    case HashMethod::SHA3_256:
        return 256;
    case HashMethod::SHA512:
    case HashMethod::SHA3_512:
        return 512;
    default:
        return 0;
    }
}

/**
 * @brief Provides names for hash methods.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<HashMethod>> defaultNames<HashMethod>{
    { "MD5", HashMethod::MD5 },           { "SHA1", HashMethod::SHA1 },
    { "SHA256", HashMethod::SHA256 },     { "SHA512", HashMethod::SHA512 },
    { "SHA3_256", HashMethod::SHA3_256 }, { "SHA3_512", HashMethod::SHA3_512 },
};

using MD5Hash      = FixedBits<128>; ///< Alias for MD5 hash (128 bits)
using SHA1Hash     = FixedBits<160>; ///< Alias for SHA-1 hash (160 bits)
using SHA256Hash   = FixedBits<256>; ///< Alias for SHA-256 hash (256 bits)
using SHA512Hash   = FixedBits<512>; ///< Alias for SHA-512 hash (512 bits)
using SHA3_256Hash = FixedBits<256>; ///< Alias for SHA3-256 hash (256 bits)
using SHA3_512Hash = FixedBits<512>; ///< Alias for SHA3-512 hash (512 bits)

/**
 * @brief Hashes a sequence of bytes using the specified hashing method.
 *
 * @param method The hashing method to use.
 * @param data The sequence of bytes to hash.
 * @return The resulting hash as a `Bytes` object.
 */
[[nodiscard]] Bytes hash(HashMethod method, BytesView data);

/**
 * @brief Computes the MD5 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting MD5 hash as an `MD5Hash` object.
 */
[[nodiscard]] MD5Hash md5(BytesView data);

/**
 * @brief Computes the SHA-1 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting SHA-1 hash as a `SHA1Hash` object.
 */
[[nodiscard]] SHA1Hash sha1(BytesView data);

/**
 * @brief Computes the SHA-256 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting SHA-256 hash as a `SHA256Hash` object.
 */
[[nodiscard]] SHA256Hash sha256(BytesView data);

/**
 * @brief Computes the SHA-512 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting SHA-512 hash as a `SHA512Hash` object.
 */
[[nodiscard]] SHA512Hash sha512(BytesView data);

/**
 * @brief Computes the SHA3-256 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting SHA3-256 hash as a `SHA3_256Hash` object.
 */
[[nodiscard]] SHA3_256Hash sha3_256(BytesView data);

/**
 * @brief Computes the SHA3-512 hash of a sequence of bytes.
 *
 * @param data The sequence of bytes to hash.
 * @return The resulting SHA3-512 hash as a `SHA3_512Hash` object.
 */
[[nodiscard]] SHA3_512Hash sha3_512(BytesView data);

/**
 * @brief Hashes a string using the specified hashing method.
 *
 * @param method The hashing method to use.
 * @param data The string to hash.
 * @return The resulting hash as a `Bytes` object.
 */
[[nodiscard]] Bytes hash(HashMethod method, std::string_view data);

/**
 * @brief Computes the MD5 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting MD5 hash as an `MD5Hash` object.
 */
[[nodiscard]] MD5Hash md5(std::string_view data);

/**
 * @brief Computes the SHA-1 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting SHA-1 hash as a `SHA1Hash` object.
 */
[[nodiscard]] SHA1Hash sha1(std::string_view data);

/**
 * @brief Computes the SHA-256 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting SHA-256 hash as a `SHA256Hash` object.
 */
[[nodiscard]] SHA256Hash sha256(std::string_view data);

/**
 * @brief Computes the SHA-512 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting SHA-512 hash as a `SHA512Hash` object.
 */
[[nodiscard]] SHA512Hash sha512(std::string_view data);

/**
 * @brief Computes the SHA3-256 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting SHA3-256 hash as a `SHA3_256Hash` object.
 */
[[nodiscard]] SHA3_256Hash sha3_256(std::string_view data);

/**
 * @brief Computes the SHA3-512 hash of a string.
 *
 * @param data The string to hash.
 * @return The resulting SHA3-512 hash as a `SHA3_512Hash` object.
 */
[[nodiscard]] SHA3_512Hash sha3_512(std::string_view data);

/**
 * @brief Provides a common interface for hashers, enabling generic hashing functionality.
 */
struct Hasher {
    /**
     * @brief Default constructor.
     * Initializes the hasher with the default hashing method.
     */
    Hasher() noexcept;

    /**
     * @brief Constructs a hasher with the specified hashing method.
     *
     * @param method The hashing method to use.
     */
    explicit Hasher(HashMethod method) noexcept;

    /**
     * @brief Finalizes the hashing process and writes the result to the provided buffer.
     *
     * @param hashOutput The buffer where the final hash result will be written.
     * @return True if the finalization was successful, otherwise false.
     */
    bool finish(BytesMutableView hashOutput);

    /**
     * @brief Writes a sequence of bytes to the hasher for hashing.
     *
     * @param data The sequence of bytes to hash.
     * @return True if the write operation was successful, otherwise false.
     */
    bool write(BytesView data);

    /**
     * @brief Writes a sequence of bytes to the hasher for hashing.
     *
     * @param data Pointer to the byte data.
     * @param size The number of bytes to write.
     * @return True if the write operation was successful, otherwise false.
     */
    bool write(const std::byte* data, size_t size) {
        return write({ data, size });
    }

    bool write(const uint8_t* data, size_t size) {
        return write(reinterpret_cast<const std::byte*>(data), size);
    }

    /** The hashing method used by this hasher. */
    HashMethod method;

    /** Internal state of the hasher. */
    alignas(8) FixedBytes<416> state;
};

/**
 * @brief Provides a SHA-256 specific hasher with SHA-256 as the hashing method.
 */
struct SHA256Hasher : public Hasher {
    /**
     * @brief Constructs a SHA256Hasher with SHA-256 as the hashing method.
     */
    SHA256Hasher() noexcept : Hasher(HashMethod::SHA256) {}

    /**
     * @brief Finalizes the hashing process and writes the result to the provided SHA256Hash object.
     *
     * @param hash The SHA256Hash object where the final hash result will be written.
     * @return True if the finalization was successful, otherwise false.
     */
    bool finish(SHA256Hash& hash);
};

/**
 * @brief Provides a SHA-512 specific hasher with SHA-512 as the hashing method.
 */
struct SHA512Hasher : public Hasher {
    /**
     * @brief Constructs a SHA512Hasher with SHA-512 as the hashing method.
     */
    SHA512Hasher() noexcept : Hasher(HashMethod::SHA512) {}

    /**
     * @brief Finalizes the hashing process and writes the result to the provided SHA512Hash object.
     *
     * @param hashOutput The SHA512Hash object where the final hash result will be written.
     * @return True if the finalization was successful, otherwise false.
     */
    bool finish(SHA512Hash& hashOutput);
};

/**
 * @brief Creates a RC<Stream> for hashing using the specified hash method.
 *
 * @param method The hashing method to use.
 * @param hashOutput The buffer where the final hash result will be written.
 * @return A RC<Stream> for hashing.
 */
[[nodiscard]] RC<Stream> hashStream(HashMethod method, BytesMutableView hashOutput);

/**
 * @brief Creates a RC<Stream> for MD5 hashing.
 *
 * @param hashOutput The MD5Hash object where the final hash result will be written.
 * @return A RC<Stream> for MD5 hashing.
 */
[[nodiscard]] RC<Stream> md5HashStream(MD5Hash& hashOutput);

/**
 * @brief Creates a RC<Stream> for SHA-1 hashing.
 *
 * @param hashOutput The SHA1Hash object where the final hash result will be written.
 * @return A RC<Stream> for SHA-1 hashing.
 */
[[nodiscard]] RC<Stream> sha1HashStream(SHA1Hash& hashOutput);

/**
 * @brief Creates a RC<Stream> for SHA-256 hashing.
 *
 * @param hashOutput The SHA256Hash object where the final hash result will be written.
 * @return A RC<Stream> for SHA-256 hashing.
 */
[[nodiscard]] RC<Stream> sha256HashStream(SHA256Hash& hashOutput);

/**
 * @brief Creates a RC<Stream> for SHA-512 hashing.
 *
 * @param hashOutput The SHA512Hash object where the final hash result will be written.
 * @return A RC<Stream> for SHA-512 hashing.
 */
[[nodiscard]] RC<Stream> sha512HashStream(SHA512Hash& hashOutput);

/**
 * @brief Creates a RC<Stream> for SHA3-256 hashing.
 *
 * @param hashOutput The SHA3_256Hash object where the final hash result will be written.
 * @return A RC<Stream> for SHA3-256 hashing.
 */
[[nodiscard]] RC<Stream> sha3_256HashStream(SHA3_256Hash& hashOutput);

/**
 * @brief Creates a RC<Stream> for SHA3-512 hashing.
 *
 * @param hashOutput The SHA3_512Hash object where the final hash result will be written.
 * @return A RC<Stream> for SHA3-512 hashing.
 */
[[nodiscard]] RC<Stream> sha3_512HashStream(SHA3_512Hash& hashOutput);

} // namespace Brisk
