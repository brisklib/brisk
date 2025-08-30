
#pragma once


#include <new>
#include "LineArrayTiled.h"
#include "LineArrayX16Y16.h"
#include "LineArrayX32Y16.h"
#include "Utils.h"


namespace Blaze {

struct LineArrayTiledBlock;
struct LineArrayX16Y16Block;
struct LineArrayX32Y16Block;

class LineBlockAllocator final {
public:
    LineBlockAllocator() {}


    ~LineBlockAllocator();


    /**
     * Returns new tiled line array block. Returned memory is not zero-filled.
     */
    LineArrayTiledBlock *NewTiledBlock(LineArrayTiledBlock *next);


    /**
     * Returns new narrow line array block. Returned memory is not
     * zero-filled.
     */
    LineArrayX16Y16Block *NewX16Y16Block(LineArrayX16Y16Block *next);


    /**
     * Returns new wide line array block. Returned memory is not zero-filled.
     */
    LineArrayX32Y16Block *NewX32Y16Block(LineArrayX32Y16Block *next);


    /**
     * Resets this allocator to initial state. Should be called
     * after frame ends.
     */
    void Clear();

private:
    // If these get bigger, there is probably too much wasted memory for most
    // input paths.
    static_assert(sizeof(LineArrayTiledBlock) <= 1024);
    static_assert(sizeof(LineArrayX16Y16Block) <= 1024);
    static_assert(sizeof(LineArrayX32Y16Block) <= 1024);

    // Points to the current arena.
    uint8_t *mCurrent = nullptr;
    uint8_t *mEnd = nullptr;

    struct Arena final {
        // Each arena is 32 kilobytes.
        static constexpr int Size = 1024 * 32;

        union {
            uint8_t Memory[Size];

            struct {
                // Points to the next item in free list.
                Arena *NextFree;

                // Points to the next item in all block list.
                Arena *NextAll;
            } Links;
        };
    };

    static_assert(sizeof(Arena) == Arena::Size);
    static_assert(sizeof(Arena::Links) == (sizeof(void *) * 2));

    Arena *mAllArenas = nullptr;
    Arena *mFreeArenas = nullptr;

private:
    template <typename T>
    T *NewBlock(T *next);

    template <typename T>
    T *NewBlockFromNewArena(T *next);

private:
    void NewArena();

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(LineBlockAllocator);
};


inline LineArrayTiledBlock *LineBlockAllocator::NewTiledBlock(
    LineArrayTiledBlock *next) {
    return NewBlock<LineArrayTiledBlock>(next);
}


inline LineArrayX16Y16Block *LineBlockAllocator::NewX16Y16Block(
    LineArrayX16Y16Block *next) {
    return NewBlock<LineArrayX16Y16Block>(next);
}


inline LineArrayX32Y16Block *LineBlockAllocator::NewX32Y16Block(
    LineArrayX32Y16Block *next) {
    return NewBlock<LineArrayX32Y16Block>(next);
}


template <typename T>
inline T *LineBlockAllocator::NewBlock(T *next) {
    uint8_t *current = mCurrent;

    if (current < mEnd) [[likely]] {
        T *b = reinterpret_cast<T *>(current);

        mCurrent = reinterpret_cast<uint8_t *>(b + 1);

        return new (b) T(next);
    }

    return NewBlockFromNewArena<T>(next);
}


template <typename T>
inline T *LineBlockAllocator::NewBlockFromNewArena(T *next) {
    NewArena();

    T *b = reinterpret_cast<T *>(mCurrent);

    mCurrent = reinterpret_cast<uint8_t *>(b + 1);

    return new (b) T(next);
}

} // namespace Blaze