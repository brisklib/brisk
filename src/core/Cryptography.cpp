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
#include <brisk/core/Cryptography.hpp>

#include "hash/hash.h"

namespace Brisk {

static_assert(sizeof(decltype(Hasher::state)) == sizeof(hash_state));

void cryptoRandomInplace(bytes_mutable_view data) {
    size_t result = cryptoRandomInplaceSafe(data);
    if (result != data.size()) {
        throwException(ECrypto("Not enough randomness for cryptoRandomInplace"));
    }
}

bytes cryptoRandom(size_t size) {
    bytes result(size);
    cryptoRandomInplace(result);
    return result;
}

class RandomReader : public SequentialReader {
public:
    RandomReader() = default;

    Transferred read(uint8_t* data, size_t size) final {
        return cryptoRandomInplaceSafe(bytes_mutable_view{ data, size });
    }
};

RC<Stream> cryptoRandomReader() {
    return RC<Stream>(new RandomReader());
}

struct HashFunctions {
    size_t hashsize;
    hash_init init;
    hash_process process;
    hash_done done;
};

static const HashFunctions& hashFunctions(HashMethod method) {
    static const std::array<HashFunctions, 1 + +HashMethod::Last> list{ {
        /*MD5     */ { 128 / 8, PUB_NAME(md5_init), PUB_NAME(md5_process), PUB_NAME(md5_done) },
        /*SHA1    */ { 160 / 8, PUB_NAME(sha1_init), PUB_NAME(sha1_process), PUB_NAME(sha1_done) },
        /*SHA256  */ { 256 / 8, PUB_NAME(sha256_init), PUB_NAME(sha256_process), PUB_NAME(sha256_done) },
        /*SHA512  */ { 512 / 8, PUB_NAME(sha512_init), PUB_NAME(sha512_process), PUB_NAME(sha512_done) },
        /*SHA3_256*/ { 256 / 8, PUB_NAME(sha3_256_init), PUB_NAME(sha3_process), PUB_NAME(sha3_done) },
        /*SHA3_512*/ { 512 / 8, PUB_NAME(sha3_512_init), PUB_NAME(sha3_process), PUB_NAME(sha3_done) },
    } };
    BRISK_ASSERT(static_cast<uint32_t>(+method) < list.size());

    return list[static_cast<uint32_t>(+method)];
}

static void hashTo(HashMethod method, bytes_view data, bytes_mutable_view hash) {
    const HashFunctions& desc = hashFunctions(method);
    hash_state state;
    desc.init(&state);
    desc.process(&state, data.data(), data.size());
    desc.done(&state, hash.data());
}

template <HashMethod method>
static FixedBits<hashBitSize(method)> hash(bytes_view data) {
    FixedBits<hashBitSize(method)> result;
    hashTo(method, data, result);
    return result;
}

MD5Hash md5(bytes_view data) {
    return hash<HashMethod::MD5>(data);
}

SHA1Hash sha1(bytes_view data) {
    return hash<HashMethod::SHA1>(data);
}

SHA256Hash sha256(bytes_view data) {
    return hash<HashMethod::SHA256>(data);
}

SHA512Hash sha512(bytes_view data) {
    return hash<HashMethod::SHA512>(data);
}

SHA3_256Hash sha3_256(bytes_view data) {
    return hash<HashMethod::SHA3_256>(data);
}

SHA3_512Hash sha3_512(bytes_view data) {
    return hash<HashMethod::SHA3_512>(data);
}

Bytes hash(HashMethod method, bytes_view data) {
    Bytes result(hashBitSize(method));
    hashTo(method, data, result);
    return result;
}

MD5Hash md5(std::string_view data) {
    return hash<HashMethod::MD5>(toBytesView(data));
}

SHA1Hash sha1(std::string_view data) {
    return hash<HashMethod::SHA1>(toBytesView(data));
}

SHA256Hash sha256(std::string_view data) {
    return hash<HashMethod::SHA256>(toBytesView(data));
}

SHA512Hash sha512(std::string_view data) {
    return hash<HashMethod::SHA512>(toBytesView(data));
}

SHA3_256Hash sha3_256(std::string_view data) {
    return hash<HashMethod::SHA3_256>(toBytesView(data));
}

SHA3_512Hash sha3_512(std::string_view data) {
    return hash<HashMethod::SHA3_512>(toBytesView(data));
}

Bytes hash(HashMethod method, std::string_view data) {
    Bytes result(hashBitSize(method));
    hashTo(method, toBytesView(data), result);
    return result;
}

class HashStream final : public SequentialWriter {
public:
    HashStream(const HashFunctions& desc, bytes_mutable_view hash) : desc(desc), hash(hash) {
        if (desc.init(&state) != CRYPT_OK) {
            failed = true;
        }
    }

    ~HashStream() final {
        if (!flushed) {
            std::ignore = flush();
        }
    }

    Transferred write(const uint8_t* data, size_t size) final {
        if (failed || flushed)
            return Transferred::Error;

        if (desc.process(&state, data, size) != CRYPT_OK) {
            failed = true;
            return Transferred::Error;
        }

        return size;
    }

    bool flush() final {
        return getHash(hash);
    }

    bool getHash(bytes_mutable_view hash) {
        if (flushed)
            return false;

        if (desc.hashsize != hash.size()) {
            return false;
        }

        if (failed) {
            std::fill(hash.begin(), hash.end(), 0);
        } else {
            desc.done(&state, hash.data());
        }

        flushed = true;
        return true;
    }

private:
    const HashFunctions& desc;
    bytes_mutable_view hash;
    hash_state state;
    bool failed  = false;
    bool flushed = false;
};

RC<Stream> hashStream(HashMethod method, bytes_mutable_view hash) {
    return RC<Stream>(new HashStream(hashFunctions(method), hash));
}

RC<Stream> md5HashStream(MD5Hash& hash) {
    return hashStream(HashMethod::MD5, toBytesMutableView(hash));
}

RC<Stream> sha1HashStream(SHA1Hash& hash) {
    return hashStream(HashMethod::SHA1, toBytesMutableView(hash));
}

RC<Stream> sha256HashStream(SHA256Hash& hash) {
    return hashStream(HashMethod::SHA256, toBytesMutableView(hash));
}

RC<Stream> sha512HashStream(SHA512Hash& hash) {
    return hashStream(HashMethod::SHA512, toBytesMutableView(hash));
}

RC<Stream> sha3_256HashStream(SHA3_256Hash& hash) {
    return hashStream(HashMethod::SHA3_256, toBytesMutableView(hash));
}

RC<Stream> sha3_512HashStream(SHA3_512Hash& hash) {
    return hashStream(HashMethod::SHA3_512, toBytesMutableView(hash));
}

Hasher::Hasher() noexcept
    : method(static_cast<HashMethod>(-1)) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
{}

Hasher::Hasher(HashMethod method) noexcept : method(method) {
    hashFunctions(method).init(reinterpret_cast<hash_state*>(state.data()));
}

bool Hasher::finish(bytes_mutable_view bytes) {
    auto desc = hashFunctions(method);
    BRISK_ASSERT(bytes.size() == desc.hashsize);
    return desc.done(reinterpret_cast<hash_state*>(state.data()), bytes.data()) == CRYPT_OK;
}

bool Hasher::write(bytes_view data) {
    auto desc = hashFunctions(method);
    return desc.process(reinterpret_cast<hash_state*>(state.data()), data.data(), data.size()) == CRYPT_OK;
}
} // namespace Brisk
