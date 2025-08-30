
#include "BumpAllocator.h"


namespace Blaze {

static constexpr int kMinimumMasterBlockSize = 1024 * 128;


/**
 * Returns block allocation size aligned to 32 kilobyte boundary.
 */
static int RoundUpBlockSize(const int size) {
    BLAZE_ASSERT(size > 0);

    const int m = size + 32767;

    return m & ~32767;
}


BumpAllocator::~BumpAllocator() {
    FreeBlockChain(mMasterActiveList);
    FreeBlockChain(mMasterFreeList);
}


void BumpAllocator::FreeBlockChain(Block *block) {
    while (block != nullptr) {
        Block *next = block->Next;

        free(block->Bytes);
        free(block);

        block = next;
    }
}


void *BumpAllocator::MallocFromNewBlock(const int size) {
    BLAZE_ASSERT(size > 0);

    Block **ptr = &mMasterFreeList;

    while ((*ptr) != nullptr) {
        Block *b = (*ptr);

        BLAZE_ASSERT(b->Position == 0);

        if (b->BlockSize >= size) {
            (*ptr) = b->Next;

            // Block is large enough. Remove from free list and insert to
            // active block list.
            void *p = b->Bytes;

            b->Position = RoundUpAllocationSizeForNextAllocation(size);
            b->Next = mMasterActiveList;

            mMasterActiveList = b;

            return p;
        }

        ptr = &(b->Next);
    }

    // A new block is needed.
    Block *block = reinterpret_cast<Block *>(malloc(sizeof(Block)));

    block->BlockSize = Max(kMinimumMasterBlockSize, RoundUpBlockSize(size));

    block->Bytes = reinterpret_cast<uint8_t *>(malloc(block->BlockSize));

    BLAZE_ASSERT(block->Bytes != nullptr);

    // Assign position to allocation size because we will return base pointer
    // later without adjusting current position.

    block->Position = RoundUpAllocationSizeForNextAllocation(size);

    // Insert to main list.
    block->Next = mMasterActiveList;

    mMasterActiveList = block;

    return block->Bytes;
}


void BumpAllocator::Free() {
    Block *b = mMasterActiveList;

    while (b != nullptr) {
        Block *next = b->Next;

        b->Next = mMasterFreeList;
        b->Position = 0;

        mMasterFreeList = b;

        b = next;
    }

    mMasterActiveList = nullptr;
}

} // namespace Blaze