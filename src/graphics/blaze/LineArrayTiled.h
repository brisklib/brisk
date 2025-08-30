
#pragma once

#include "BitOps.h"
#include "Utils.h"
#include "LinearizerUtils.h"


namespace Blaze {

class ThreadMemory;

struct LineArrayTiledBlock final {
    explicit LineArrayTiledBlock(LineArrayTiledBlock *next) : Next(next) {}

    static constexpr int LinesPerBlock = 8;

    F8Dot8x4 P0P1[LinesPerBlock];

    LineArrayTiledBlock *Next = nullptr;

private:
    LineArrayTiledBlock() = delete;

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(LineArrayTiledBlock);
};

template <typename T>
struct LineArrayTiled final {

    static void Construct(LineArrayTiled<T> *placement,
        const TileIndex rowCount, const TileIndex columnCount,
        ThreadMemory &memory);

    const BitVector *GetTileAllocationBitVectors() const;
    const LineArrayTiledBlock *GetFrontBlockForColumn(
        const TileIndex columnIndex) const;
    const int32_t *GetCoversForColumn(const TileIndex columnIndex) const;
    int GetTotalLineCountForColumn(const TileIndex columnIndex) const;

    void AppendVerticalLine(ThreadMemory &memory, const F24Dot8 x,
        const F24Dot8 y0, const F24Dot8 y1);
    void AppendLineDownR_V(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineUpR_V(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineDownL_V(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineUpL_V(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineDownRL(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineUpRL(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);

private:
    void AppendLineDownR(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineUpR(ThreadMemory &memory, const F24Dot8 x0, const F24Dot8 y0,
        const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineDownL(ThreadMemory &memory, const F24Dot8 x0,
        const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void AppendLineUpL(ThreadMemory &memory, const F24Dot8 x0, const F24Dot8 y0,
        const F24Dot8 x1, const F24Dot8 y1);

private:
    void Append(ThreadMemory &memory, const TileIndex columnIndex,
        const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);
    void Push(ThreadMemory &memory, const TileIndex columnIndex,
        const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1);

private:
    static constexpr int AdjustmentMask = (F24Dot8_1 * T::TileW) - 1;

    static constexpr int FindTileColumnAdjustment(const F24Dot8 value) {
        static_assert(sizeof(F24Dot8) == 4);

        // Will be set to 0 is value is zero or less. Otherwise it will be 1.
        const int lte0 = ~((value - 1) >> 31) & 1;

        // Will be set to 1 if value is divisible by tile width (in 24.8
        // format) without a reminder. Otherwise it will be 0.
        const int db = (((value & AdjustmentMask) - 1) >> 31) & 1;

        // Return 1 if both bits (more than zero and disisible by 256) are set.
        return lte0 & db;
    }

private:
    constexpr LineArrayTiled(BitVector *bitVectors,
        LineArrayTiledBlock **blocks, int32_t **covers, int *counts)
        : mBitVectors(bitVectors), mBlocks(blocks), mCovers(covers),
          mCounts(counts) {}

private:
    // One bit for each tile column.
    BitVector *mBitVectors = nullptr;

    // One block pointer for each tile column. Not zero-filled at the
    // beginning, individual pointers initialized to newly allocated blocks
    // once the first line is inserted into particular column.
    LineArrayTiledBlock **mBlocks = nullptr;

    // One cover array for each tile column. Not zero-filled at the beginning,
    // individual cover arrays allocated and zero-filled once the first line
    // is inserted into particular column.
    int32_t **mCovers = nullptr;

    // One count for each tile column. Not zero-filled at the beginning,
    // individual counts initialized to one once the first line is inserted
    // into particular column.
    int *mCounts = nullptr;
};

} // namespace Blaze