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

#include "Brisk.h"
#include <cstdlib>
#include <new>
#include <algorithm>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace Brisk {

/**
 * @brief Maximum alignment for SIMD instructions based on the platform.
 *
 * This constant defines the maximum alignment required for SIMD operations.
 */
#ifdef BRISK_X86
constexpr inline size_t maximumSimdAlignment = 32;
#elif defined BRISK_ARM
constexpr inline size_t maximumSimdAlignment = 16;
#else
constexpr inline size_t maximumSimdAlignment = 16;
#endif

/**
 * @brief Cache line alignment size.
 *
 * This constant defines the alignment size for cache lines, which is typically 64 bytes.
 */
constexpr inline size_t cacheAlignment         = 64;

/**
 * @brief Default memory alignment.
 *
 * This constant sets the default alignment for memory allocations, which is the same as the maximum SIMD
 * alignment.
 */
constexpr inline size_t defaultMemoryAlignment = maximumSimdAlignment;

namespace Internal {
template <typename T>
constinit inline size_t sizeOfSafe = sizeof(T);
template <>
constinit inline size_t sizeOfSafe<void> = 1;
} // namespace Internal

/**
 * @brief Aligns a value up to the nearest multiple of the specified alignment.
 *
 * @tparam T Type of the value to be aligned.
 * @param value The value to be aligned.
 * @param alignment The alignment requirement.
 * @return The aligned value.
 */
template <typename T>
constexpr T alignUp(T value, std::type_identity_t<T> alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

/**
 * @brief Allocates memory with specified alignment.
 *
 * This function allocates memory with the given size and alignment. If alignment is not specified, the
 * default memory alignment is used.
 *
 * @tparam T Type of the allocated memory. Defaults to void.
 * @param size The number of elements of type T to allocate.
 * @param alignment The alignment requirement (default is defaultMemoryAlignment).
 * @return Pointer to the allocated memory.
 */
template <typename T = void>
[[nodiscard]] inline BRISK_INLINE T* alignedAlloc(size_t size, size_t alignment = defaultMemoryAlignment) {
#ifdef _MSC_VER
    return reinterpret_cast<T*>(
        _aligned_malloc(alignUp(size * Internal::sizeOfSafe<T>, alignment), alignment));
#else
    return reinterpret_cast<T*>(
        std::aligned_alloc(alignment, alignUp(size * Internal::sizeOfSafe<T>, alignment)));
#endif
}

/**
 * @brief Frees memory allocated with alignedAlloc.
 *
 * This function frees memory that was previously allocated with alignedAlloc.
 *
 * @tparam T Type of the pointer to be freed. Defaults to void.
 * @param ptr Pointer to the memory to be freed.
 */
template <typename T = void>
inline BRISK_INLINE void alignedFree(T* ptr) {
#ifdef _MSC_VER
    return _aligned_free(ptr);
#else
    return std::free(ptr);
#endif
}

/**
 * @brief Provides custom `new` and `delete` operators for aligned memory allocation.
 *
 * This struct defines custom `new` and `delete` operators that use aligned memory allocation. By inheriting
 * from `AlignedAllocation`, a derived class will automatically use these aligned operators for dynamic memory
 * allocation. This ensures that instances of the derived class are always allocated with the appropriate
 * alignment.
 */
struct AlignedAllocation {

    /**
     * @brief Allocates memory with alignment using the `new` operator.
     *
     * This function allocates memory with default alignment using the `new` operator.
     *
     * @param size The size of the memory to allocate.
     * @return Pointer to the allocated memory.
     */
    BRISK_INLINE static void* operator new(size_t size) noexcept {
        return alignedAlloc(size);
    }

    /**
     * @brief Frees memory using the `delete` operator.
     *
     * This function frees memory that was previously allocated with the custom `new` operator.
     *
     * @param ptr Pointer to the memory to be freed.
     */
    BRISK_INLINE static void operator delete(void* ptr) noexcept {
        alignedFree(ptr);
    }

#ifdef __cpp_aligned_new
    /**
     * @brief Allocates memory with specified alignment using the `new` operator.
     *
     * This function allocates memory with the specified alignment using the `new` operator.
     *
     * @param size The size of the memory to allocate.
     * @param al The alignment requirement.
     * @return Pointer to the allocated memory.
     */
    BRISK_INLINE static void* operator new(size_t size, std::align_val_t al) noexcept {
        return alignedAlloc(size, std::max(size_t(defaultMemoryAlignment), static_cast<size_t>(al)));
    }

    /**
     * @brief Frees memory with specified alignment using the `delete` operator.
     *
     * This function frees memory that was previously allocated with the custom `new` operator with alignment.
     *
     * @param ptr Pointer to the memory to be freed.
     * @param al The alignment requirement.
     */
    BRISK_INLINE static void operator delete(void* ptr, std::align_val_t al) noexcept {
        return alignedFree(ptr);
    }
#endif
};
} // namespace Brisk
