
#pragma once


#include <new>
#include "Utils.h"


namespace Blaze {

class BumpAllocator final {
public:
    BumpAllocator() {}

public:
    ~BumpAllocator();

public:
    /**
     * Allocates memory for one element of type T. Does not zero-fill
     * allocated memory and does not call any constructors.
     */
    template <typename T>
    T *Malloc();


    /**
     * Allocates memory for a given amount of pointers of type T. Does not
     * zero-fill allocated memory. Note that this method does not allocate
     * any objects, only arrays of pointers.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T **MallocPointers(const int count);


    /**
     * Allocates memory for a given amount of pointers of type T and fills the
     * entire block of allocated memory with zeroes. Note that this method
     * does not allocate any objects, only arrays of pointers.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T **MallocPointersZeroFill(const int count);


    /**
     * Allocates memory for an array of values of type T. Does not zero-fill
     * allocated memory and does not call any constructors.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T *MallocArray(const int count);


    /**
     * Allocates memory for an array of values of type T. Fills allocated
     * memory with zeroes, but does not call any constructors.
     *
     * @param count A number of pointers to allocate. Must be at least 1.
     */
    template <typename T>
    T *MallocArrayZeroFill(const int count);


    /**
     * Allocates memory for one element of type T and calls constructor with
     * given parameters. Returns fully constructed object of type T.
     */
    template <typename T, typename... Args>
    T *New(Args &&...args);


    /**
     * Allocates given amount of bytes. Does not zero-fill allocated memory.
     *
     * @param size A number of bytes to allocate. Must be at least 1.
     */
    void *Malloc(const int size);


    /**
     * Resets this allocator to the initial state.
     */
    void Free();

private:
    /**
     * Represents a single block of raw memory in a linked list. One such
     * block manages relatively large amount of memory.
     */
    struct Block final {
        // Entire arena.
        uint8_t *Bytes = nullptr;

        // Next block in all block list.
        Block *Next = nullptr;

        int Position = 0;
        int BlockSize = 0;
    };

    Block *mMasterActiveList = nullptr;
    Block *mMasterFreeList = nullptr;

private:
    void *MallocFromNewBlock(const int size);

private:
    static void FreeBlockChain(Block *block);


    /**
     * Returns allocation size rounded up so that the next allocation from the
     * same block will be aligned oto 16 byte boundary.
     */
    static constexpr int RoundUpAllocationSizeForNextAllocation(
        const int size) {
        BLAZE_ASSERT(size > 0);

        const int m = size + 15;

        return m & ~15;
    }

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(BumpAllocator);
};


template <typename T>
inline T *BumpAllocator::Malloc() {
    return static_cast<T *>(Malloc(sizeof(T)));
}


template <typename T>
inline T **BumpAllocator::MallocPointers(const int count) {
    BLAZE_ASSERT(count > 0);

    return static_cast<T **>(Malloc(sizeof(T *) * count));
}


template <typename T>
inline T **BumpAllocator::MallocPointersZeroFill(const int count) {
    BLAZE_ASSERT(count > 0);

    const int b = sizeof(T *) * count;

    T **p = static_cast<T **>(Malloc(b));

    memset(p, 0, b);

    return p;
}


template <typename T>
inline T *BumpAllocator::MallocArray(const int count) {
    BLAZE_ASSERT(count > 0);

    return static_cast<T *>(Malloc(sizeof(T) * count));
}


template <typename T>
inline T *BumpAllocator::MallocArrayZeroFill(const int count) {
    BLAZE_ASSERT(count > 0);

    const int b = sizeof(T) * count;

    T *p = static_cast<T *>(Malloc(b));

    memset(p, 0, b);

    return p;
}


template <typename T, typename... Args>
inline T *BumpAllocator::New(Args &&...args) {
    return new (Malloc<T>()) T(std::forward<Args>(args)...);
}


inline void *BumpAllocator::Malloc(const int size) {
    Block *mal = mMasterActiveList;

    if (mal != nullptr) [[likely]] {
        const int remainingSize = mal->BlockSize - mal->Position;

        if (remainingSize >= size) [[likely]] {
            void *p = mal->Bytes + mal->Position;

            mal->Position += RoundUpAllocationSizeForNextAllocation(size);

            return p;
        }
    }

    return MallocFromNewBlock(size);
}

} // namespace Blaze