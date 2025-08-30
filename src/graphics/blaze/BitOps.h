
#pragma once


#include "Utils.h"

#if defined _MSC_VER
#include <intrin.h>
#endif

namespace Blaze {

// BitVector is a fixed size bit array that fits into one register.
//
// Bit vector type should be either uint32_t or uint64_t, depending on target
// CPU. It is tempting to just use an alias for something like uintptr_t, but it
// is easier for compiler-specific implementation to choose correct functions
// for builtins. See CountBits for GCC, for example. It has two implementations,
// for uint32_t and uint64_t, calling either __builtin_popcount or
// __builtin_popcountl. And the rest of the API can use these functions
// without worrying that compiler will get confused which version to call.


#ifdef BLAZE_MACHINE_64
using BitVector = uint64_t;
#elif defined BLAZE_MACHINE_32
using BitVector = uint32_t;
#else
#error Unknown architecture
#endif


/**
 * Returns the number of bits set to 1 in a given value.
 *
 * @param v Value to count bits for. Must not be 0.
 */
template <typename T>
static constexpr int CountBits(const T v) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    BLAZE_ASSERT(v != 0);

    int num = 0;

    for (int i = 0; i < BLAZE_BIT_SIZE_OF(T); i++) {
        const T bit = v >> i;

        num += static_cast<int>(bit & 1);
    }

    return num;
}


/**
 * Returns the number of trailing zero bits in a given value, starting at the
 * least significant bit position.
 *
 * @param v Value to count trailing zeroes for. Must not be 0.
 */
template <typename T>
static constexpr int CountTrailingZeroes(const T v) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    BLAZE_ASSERT(v != 0);

    int i = 0;

    for (; i < BLAZE_BIT_SIZE_OF(T); i++) {
        const T bit = v >> i;
        const int m = static_cast<int>(bit & 1);

        if (m != 0) {
            return i;
        }
    }

    return i;
}


// Include compiler-specific bit ops.


#if defined __GNUC__ || defined __clang__
template <>
constexpr int CountBits<uint32_t>(const uint32_t v) {
    BLAZE_ASSERT(v != 0);

    return __builtin_popcount(v);
}


template <>
constexpr int CountBits<uint64_t>(const uint64_t v) {
    BLAZE_ASSERT(v != 0);

    return __builtin_popcountll(v);
}


template <>
constexpr int CountTrailingZeroes<uint32_t>(const uint32_t v) {
    BLAZE_ASSERT(v != 0);

    return __builtin_ctz(v);
}


template <>
constexpr int CountTrailingZeroes<uint64_t>(const uint64_t v) {
    BLAZE_ASSERT(v != 0);

    return __builtin_ctzll(v);
}

#elif defined _MSC_VER

// CountBits for uint32_t
template <>
int CountBits<uint32_t>(const uint32_t v) {
    BLAZE_ASSERT(v != 0);

#if defined(__AVX2__) || defined(__SSE4_2__)
    return static_cast<int>(_mm_popcnt_u32(v));
#else
    // fallback if POPCNT is not available
    unsigned int x = v;
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    return static_cast<int>((((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24);
#endif
}

// CountBits for uint64_t
template <>
int CountBits<uint64_t>(const uint64_t v) {
    BLAZE_ASSERT(v != 0);

#if defined(_M_X64) && (defined(__AVX2__) || defined(__SSE4_2__))
    return static_cast<int>(_mm_popcnt_u64(v));
#else
    // fallback: split into 32-bit halves
    return CountBits<uint32_t>(static_cast<uint32_t>(v)) +
           CountBits<uint32_t>(static_cast<uint32_t>(v >> 32));
#endif
}

// CountTrailingZeroes for uint32_t
template <>
int CountTrailingZeroes<uint32_t>(const uint32_t v) {
    BLAZE_ASSERT(v != 0);

    unsigned long index;
    _BitScanForward(&index, v);
    return static_cast<int>(index);
}

// CountTrailingZeroes for uint64_t
template <>
int CountTrailingZeroes<uint64_t>(const uint64_t v) {
    BLAZE_ASSERT(v != 0);

#if defined(_M_X64)
    unsigned long index;
    _BitScanForward64(&index, v);
    return static_cast<int>(index);
#else
    // fallback: handle high/low 32-bits manually
    unsigned long index;
    if (_BitScanForward(&index, static_cast<uint32_t>(v)))
        return static_cast<int>(index);
    _BitScanForward(&index, static_cast<uint32_t>(v >> 32));
    return static_cast<int>(index + 32);
#endif
}

#endif


/**
 * Returns the amount of BitVector values needed to contain at least a given
 * amount of bits.
 *
 * @param maxBitCount Maximum number of bits for which storage is needed. Must
 * be at least 1.
 */
static constexpr int BitVectorsForMaxBitCount(const int maxBitCount) {
    BLAZE_ASSERT(maxBitCount);

    const int x = BLAZE_BIT_SIZE_OF(BitVector);

    return (maxBitCount + x - 1) / x;
}


/**
 * Calculates how many bits are set to 1 in bitmap.
 *
 * @param vec An array of BitVector values containing bits.
 *
 * @param count A number of values in vec. Note that this is not maximum
 * amount of bits to scan, but the amount of BitVector numbers vec contains.
 */
static constexpr int CountBitsInVector(const BitVector *vec, const int count) {
    int num = 0;

    for (int i = 0; i < count; i++) {
        const BitVector value = vec[i];

        if (value != 0) {
            num += CountBits(value);
        }
    }

    return num;
}


/**
 * Finds if bit at a given index is set to 1. If it is, this function returns
 * false. Otherwise, it sets bit at this index and returns true.
 *
 * @param vec Array of bit vectors. Must not be nullptr and must contain at
 * least (index + 1) amount of bits.
 *
 * @param index Bit index to test and set. Must be at least 0.
 */
template <typename T>
static BLAZE_FORCE_INLINE bool ConditionalSetBit(T *vec, const int index) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned type");

    BLAZE_ASSERT(vec != nullptr);
    BLAZE_ASSERT(index >= 0);

    const int vecIndex = index / BLAZE_BIT_SIZE_OF(T);

    T *v = vec + vecIndex;

    const int localIndex = index % BLAZE_BIT_SIZE_OF(T);
    const T current = *v;
    const T bit = T(1) << localIndex;

    if ((current & bit) == 0) {
        v[0] = current | bit;
        return true;
    }

    return false;
}


/**
 * Returns index to the first bit vector value which contains at least one bit
 * set to 1. If the entire array contains only zero bit vectors, an index to
 * the last bit vector will be returned.
 *
 * @param vec Bit vector array. Must not be nullptr.
 *
 * @param maxBitVectorCount A number of items in bit vector array. This
 * function always returns value less than this.
 */
static constexpr int FindFirstNonZeroBitVector(
    const BitVector *vec, const int maxBitVectorCount) {
    BLAZE_ASSERT(vec != nullptr);
    BLAZE_ASSERT(maxBitVectorCount > 0);

    int i = 0;

    for (; i < maxBitVectorCount; i++) {
        if (vec[i] != 0) {
            return i;
        }
    }

    return i;
}

} // namespace Blaze