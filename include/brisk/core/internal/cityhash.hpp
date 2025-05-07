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
#pragma once

#include <cstdint>
#include <algorithm>
#include "../Simd.hpp" // For constexpr byteswap

// Constexpr implementation of Cityhash64 algorithm
// Based on CityHash, by Geoff Pike and Jyrki Alakuijala
// http://code.google.com/p/cityhash/

namespace Brisk {

namespace CityHash {

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef std::pair<uint64, uint64> uint128;

constexpr inline uint64_t bswap_64(uint64_t x) {
    return Internal::byteswap(x);
}

using std::make_pair;
using std::pair;

constexpr static uint32 Fetch32(const char* p) {
    uint32 result = static_cast<uint8_t>(p[3]);
    result <<= 8;
    result |= static_cast<uint8_t>(p[2]);
    result <<= 8;
    result |= static_cast<uint8_t>(p[1]);
    result <<= 8;
    result |= static_cast<uint8_t>(p[0]);
    return result;
}

constexpr static uint64 Fetch64(const char* p) {
    uint64 result = Fetch32(p + 4);
    result <<= 32;
    result |= Fetch32(p);
    return result;
}

// Some primes between 2^63 and 2^64 for various uses.
constexpr static uint64 k0 = 0xc3a5c85c97cb3127ULL;
constexpr static uint64 k1 = 0xb492b66fbe98f273ULL;
constexpr static uint64 k2 = 0x9ae16a3b2f90404fULL;

constexpr static uint32 Rotate32(uint32 val, int shift) {
    // Avoid shifting by 32: doing so yields an undefined result.
    return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

// Bitwise right rotate.  Normally this will compile to a single
// instruction, especially if the shift is a manifest constant.
constexpr static uint64 Rotate(uint64 val, int shift) {
    // Avoid shifting by 64: doing so yields an undefined result.
    return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

constexpr inline uint64 Uint128Low64(const uint128& x) {
    return x.first;
}

constexpr inline uint64 Uint128High64(const uint128& x) {
    return x.second;
}

constexpr inline uint64 Hash128to64(const uint128& x) {
    // Murmur-inspired hashing.
    const uint64 kMul = 0x9ddfea08eb382d69ULL;
    uint64 a          = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
    a ^= (a >> 47);
    uint64 b = (Uint128High64(x) ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

constexpr static uint64 ShiftMix(uint64 val) {
    return val ^ (val >> 47);
}

constexpr static uint64 HashLen16(uint64 u, uint64 v) {
    return Hash128to64(uint128(u, v));
}

constexpr static uint64 HashLen16(uint64 u, uint64 v, uint64 mul) {
    // Murmur-inspired hashing.
    uint64 a = (u ^ v) * mul;
    a ^= (a >> 47);
    uint64 b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

// Return a 16-byte hash for 48 bytes.  Quick and dirty.
// Callers do best to use "random-looking" values for a and b.
constexpr static pair<uint64, uint64> WeakHashLen32WithSeeds(uint64 w, uint64 x, uint64 y, uint64 z, uint64 a,
                                                             uint64 b) {
    a += w;
    b        = Rotate(b + a + z, 21);
    uint64 c = a;
    a += x;
    a += y;
    b += Rotate(a, 44);
    return make_pair(a + z, b + c);
}

// Return a 16-byte hash for s[0] ... s[31], a, and b.  Quick and dirty.
constexpr static pair<uint64, uint64> WeakHashLen32WithSeeds(const char* s, uint64 a, uint64 b) {
    return WeakHashLen32WithSeeds(Fetch64(s), Fetch64(s + 8), Fetch64(s + 16), Fetch64(s + 24), a, b);
}

constexpr static uint64 HashLen0to16(const char* s, size_t len) {
    if (len >= 8) {
        uint64 mul = k2 + len * 2;
        uint64 a   = Fetch64(s) + k2;
        uint64 b   = Fetch64(s + len - 8);
        uint64 c   = Rotate(b, 37) * mul + a;
        uint64 d   = (Rotate(a, 25) + b) * mul;
        return HashLen16(c, d, mul);
    }
    if (len >= 4) {
        uint64 mul = k2 + len * 2;
        uint64 a   = Fetch32(s);
        return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
    }
    if (len > 0) {
        uint8 a  = static_cast<uint8>(s[0]);
        uint8 b  = static_cast<uint8>(s[len >> 1]);
        uint8 c  = static_cast<uint8>(s[len - 1]);
        uint32 y = static_cast<uint32>(a) + (static_cast<uint32>(b) << 8);
        uint32 z = static_cast<uint32>(len) + (static_cast<uint32>(c) << 2);
        return ShiftMix(y * k2 ^ z * k0) * k2;
    }
    return k2;
}

// This probably works well for 16-byte strings as well, but it may be overkill
// in that case.
constexpr static uint64 HashLen17to32(const char* s, size_t len) {
    uint64 mul = k2 + len * 2;
    uint64 a   = Fetch64(s) * k1;
    uint64 b   = Fetch64(s + 8);
    uint64 c   = Fetch64(s + len - 8) * mul;
    uint64 d   = Fetch64(s + len - 16) * k2;
    return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d, a + Rotate(b + k2, 18) + c, mul);
}

// Return an 8-byte hash for 33 to 64 bytes.
constexpr static uint64 HashLen33to64(const char* s, size_t len) {
    uint64 mul = k2 + len * 2;
    uint64 a   = Fetch64(s) * k2;
    uint64 b   = Fetch64(s + 8);
    uint64 c   = Fetch64(s + len - 24);
    uint64 d   = Fetch64(s + len - 32);
    uint64 e   = Fetch64(s + 16) * k2;
    uint64 f   = Fetch64(s + 24) * 9;
    uint64 g   = Fetch64(s + len - 8);
    uint64 h   = Fetch64(s + len - 16) * mul;
    uint64 u   = Rotate(a + g, 43) + (Rotate(b, 30) + c) * 9;
    uint64 v   = ((a + g) ^ d) + f + 1;
    uint64 w   = bswap_64((u + v) * mul) + h;
    uint64 x   = Rotate(e + f, 42) + c;
    uint64 y   = (bswap_64((v + w) * mul) + g) * mul;
    uint64 z   = e + f + c;
    a          = bswap_64((x + z) * mul + y) + b;
    b          = ShiftMix((z + a) * mul + d + h) * mul;
    return b + x;
}

constexpr uint64 CityHash64(const char* s, size_t len) {
    if (len <= 32) {
        if (len <= 16) {
            return HashLen0to16(s, len);
        } else {
            return HashLen17to32(s, len);
        }
    } else if (len <= 64) {
        return HashLen33to64(s, len);
    }

    // For strings over 64 bytes we hash the end first, and then as we
    // loop we keep 56 bytes of state: v, w, x, y, and z.
    uint64 x               = Fetch64(s + len - 40);
    uint64 y               = Fetch64(s + len - 16) + Fetch64(s + len - 56);
    uint64 z               = HashLen16(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
    pair<uint64, uint64> v = WeakHashLen32WithSeeds(s + len - 64, len, z);
    pair<uint64, uint64> w = WeakHashLen32WithSeeds(s + len - 32, y + k1, x);
    x                      = x * k1 + Fetch64(s);

    // Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
    len                    = (len - 1) & ~static_cast<size_t>(63);
    do {
        x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        len -= 64;
    } while (len != 0);
    return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z, HashLen16(v.second, w.second) + x);
}

template <size_t N>
constexpr uint64 cityHash(const char (&s)[N]) {
    return cityHash(s, N - 1);
}

constexpr uint64 CityHash64WithSeeds(const char* s, size_t len, uint64 seed0, uint64 seed1) {
    return HashLen16(CityHash64(s, len) - seed0, seed1);
}

constexpr uint64 CityHash64WithSeed(const char* s, size_t len, uint64 seed) {
    return CityHash64WithSeeds(s, len, k2, seed);
}

template <size_t N>
constexpr uint64 CityHash64WithSeeds(const char (&s)[N], uint64 seed0, uint64 seed1) {
    return HashLen16(CityHash64(s, N - 1) - seed0, seed1);
}

template <size_t N>
constexpr uint64 CityHash64WithSeed(const char (&s)[N], uint64 seed) {
    return CityHash64WithSeeds(s, N - 1, k2, seed);
}

} // namespace CityHash

constexpr inline uint64_t operator""_hash(const char* str, size_t len) noexcept {
    return CityHash::CityHash64(str, len);
}

} // namespace Brisk
