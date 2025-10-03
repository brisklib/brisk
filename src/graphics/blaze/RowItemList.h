
#pragma once


#include "ThreadMemory.h"
#include "Utils.h"


namespace Blaze {

template <typename T>
struct RowItemList final {
    RowItemList() {}


    struct Block final {
        Block() {}

        static constexpr int ItemsPerBlock = 32;

        T Items[ItemsPerBlock];
        Block *Previous = nullptr;
        Block *Next = nullptr;

        // Always start with one. Blocks never sit allocated, but without
        // items.
        int Count = 1;

    private:
        BLAZE_DISABLE_COPY_AND_ASSIGN(Block);
    };

    Block *First = nullptr;

    // While inserting, new items are added to last block.
    Block *Last = nullptr;

    template <typename... Args>
    void Append(ThreadMemory &memory, Args &&...args);

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(RowItemList);
};


template <typename T>
template <typename... Args>
inline void RowItemList<T>::Append(ThreadMemory &memory, Args &&...args) {
    if (Last == nullptr) {
        // Adding first item.
        Block *b = memory.FrameNew<Block>();

        BLAZE_ASSERT(First == nullptr);

        new (b->Items) T(std::forward<Args>(args)...);

        First = b;
        Last = b;
    } else {
        // Inserting n-th item.
        Block *current = Last;
        const int count = current->Count;

        if (count < Block::ItemsPerBlock) {
            new (current->Items + count) T(std::forward<Args>(args)...);

            current->Count = count + 1;
        } else {
            Block *b = memory.FrameNew<Block>();

            new (b->Items) T(std::forward<Args>(args)...);

            // Insert to doubly-linked list.
            current->Next = b;
            b->Previous = current;

            Last = b;
        }
    }
}

} // namespace Blaze