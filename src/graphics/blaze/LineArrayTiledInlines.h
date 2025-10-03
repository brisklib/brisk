
#pragma once

#include "LineArrayTiled.h"
#include "ThreadMemory.h"


namespace Blaze {

template <typename T>
inline void LineArrayTiled<T>::Construct(LineArrayTiled<T> *placement,
    const TileIndex rowCount, const TileIndex columnCount,
    ThreadMemory &memory) {
    BLAZE_ASSERT(placement != nullptr);
    BLAZE_ASSERT(rowCount > 0);
    BLAZE_ASSERT(columnCount > 0);

    const int bitVectorsPerRow = BitVectorsForMaxBitCount(columnCount);
    const int bitVectorCount = bitVectorsPerRow * rowCount;

    BitVector *bitVectors =
        memory.FrameMallocArrayZeroFill<BitVector>(bitVectorCount);

    LineArrayTiledBlock **blocks =
        static_cast<LineArrayTiledBlock **>(memory.TaskMalloc(
            sizeof(LineArrayTiledBlock *) * columnCount * rowCount));

    int32_t **covers = static_cast<int32_t **>(
        memory.TaskMalloc(sizeof(int32_t *) * columnCount * rowCount));

    int *counts = static_cast<int *>(
        memory.TaskMalloc(sizeof(int) * columnCount * rowCount));

    for (TileIndex i = 0; i < rowCount; i++) {
        new (placement + i)
            LineArrayTiled<T>(bitVectors, blocks, covers, counts);

        bitVectors += bitVectorsPerRow;
        blocks += columnCount;
        covers += columnCount;
        counts += columnCount;
    }
}


template <typename T>
inline const BitVector *LineArrayTiled<T>::GetTileAllocationBitVectors() const {
    return mBitVectors;
}


template <typename T>
inline const LineArrayTiledBlock *LineArrayTiled<T>::GetFrontBlockForColumn(
    const TileIndex columnIndex) const {
    return mBlocks[columnIndex];
}


template <typename T>
inline const int32_t *LineArrayTiled<T>::GetCoversForColumn(
    const TileIndex columnIndex) const {
    return mCovers[columnIndex];
}


template <typename T>
inline int LineArrayTiled<T>::GetTotalLineCountForColumn(
    const TileIndex columnIndex) const {
    return mCounts[columnIndex];
}


template <typename T>
inline void LineArrayTiled<T>::AppendVerticalLine(
    ThreadMemory &memory, const F24Dot8 x, const F24Dot8 y0, const F24Dot8 y1) {
    const TileIndex columnIndex =
        T::F24Dot8ToTileColumnIndex(x - FindTileColumnAdjustment(x));
    const F24Dot8 ex = x - T::TileColumnIndexToF24Dot8(columnIndex);

    Push(memory, columnIndex, ex, y0, ex, y1);
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineDownR_V(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x <= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y <= p1y);

    if (p0x < p1x) [[likely]] {
        AppendLineDownR(memory, p0x, p0y, p1x, p1y);
    } else {
        AppendVerticalLine(memory, p0x, p0y, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineUpR_V(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x <= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y >= p1y);

    if (p0x < p1x) [[likely]] {
        AppendLineUpR(memory, p0x, p0y, p1x, p1y);
    } else {
        AppendVerticalLine(memory, p0x, p0y, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineDownL_V(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x >= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y <= p1y);

    if (p0x > p1x) [[likely]] {
        AppendLineDownL(memory, p0x, p0y, p1x, p1y);
    } else {
        AppendVerticalLine(memory, p0x, p0y, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineUpL_V(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x >= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y >= p1y);

    if (p0x > p1x) [[likely]] {
        AppendLineUpL(memory, p0x, p0y, p1x, p1y);
    } else {
        AppendVerticalLine(memory, p0x, p0y, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineDownRL(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    BLAZE_ASSERT(x0 != x1);

    if (x0 < x1) {
        AppendLineDownR(memory, x0, y0, x1, y1);
    } else {
        AppendLineDownL(memory, x0, y0, x1, y1);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineUpRL(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    BLAZE_ASSERT(x0 != x1);

    if (x0 < x1) {
        AppendLineUpR(memory, x0, y0, x1, y1);
    } else {
        AppendLineUpL(memory, x0, y0, x1, y1);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineDownR(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x <= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y <= p1y);

    const TileIndex columnIndex0 = T::F24Dot8ToTileColumnIndex(p0x);
    const TileIndex columnIndex1 = T::F24Dot8ToTileColumnIndex(p1x - 1);

    BLAZE_ASSERT(columnIndex0 <= columnIndex1);

    if (columnIndex0 == columnIndex1) {
        Append(memory, columnIndex0, p0x, p0y, p1x, p1y);
    } else {
        // Number of pixels + 24.8 fraction for start and end points in
        // coordinate system of tiles these points belong to.
        const F24Dot8 fx = p0x - T::TileColumnIndexToF24Dot8(columnIndex0);

        BLAZE_ASSERT(fx >= 0);
        BLAZE_ASSERT(fx <= T::TileWF24Dot8);

        // Horizontal and vertical deltas.
        const F24Dot8 dx = p1x - p0x;
        const F24Dot8 dy = p1y - p0y;

        const F24Dot8 pp = (T::TileWF24Dot8 - fx) * dy;

        F24Dot8 cy = p0y + (pp / dx);

        TileIndex idx = columnIndex0 + 1;

        F24Dot8 cursor = T::TileColumnIndexToF24Dot8(idx);

        Append(memory, columnIndex0, p0x, p0y, cursor, cy);

        if (idx != columnIndex1) {
            F24Dot8 mod = (pp % dx) - dx;

            const F24Dot8 p = T::TileWF24Dot8 * dy;
            const F24Dot8 lift = p / dx;
            const F24Dot8 rem = p % dx;

            for (; idx != columnIndex1; idx++) {
                F24Dot8 delta = lift;

                mod += rem;

                if (mod >= 0) {
                    mod -= dx;
                    delta++;
                }

                const F24Dot8 ny = cy + delta;
                const F24Dot8 nx = cursor + T::TileWF24Dot8;

                Append(memory, idx, cursor, cy, nx, ny);

                cy = ny;
                cursor = nx;
            }
        }

        Append(memory, columnIndex1, cursor, cy, p1x, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineUpR(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x <= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y >= p1y);

    const TileIndex columnIndex0 = T::F24Dot8ToTileColumnIndex(p0x);
    const TileIndex columnIndex1 = T::F24Dot8ToTileColumnIndex(p1x - 1);

    BLAZE_ASSERT(columnIndex0 <= columnIndex1);

    if (columnIndex0 == columnIndex1) {
        Append(memory, columnIndex0, p0x, p0y, p1x, p1y);
    } else {
        // Number of pixels + 24.8 fraction for start and end points in
        // coordinate system of tiles these points belong to.
        const F24Dot8 fx = p0x - T::TileColumnIndexToF24Dot8(columnIndex0);

        BLAZE_ASSERT(fx >= 0);
        BLAZE_ASSERT(fx <= T::TileWF24Dot8);

        // Horizontal and vertical deltas.
        const F24Dot8 dx = p1x - p0x;
        const F24Dot8 dy = p0y - p1y;

        const F24Dot8 pp = (T::TileWF24Dot8 - fx) * dy;

        F24Dot8 cy = p0y - (pp / dx);

        TileIndex idx = columnIndex0 + 1;

        F24Dot8 cursor = T::TileColumnIndexToF24Dot8(idx);

        Append(memory, columnIndex0, p0x, p0y, cursor, cy);

        if (idx != columnIndex1) {
            F24Dot8 mod = (pp % dx) - dx;

            const F24Dot8 p = T::TileWF24Dot8 * dy;
            const F24Dot8 lift = p / dx;
            const F24Dot8 rem = p % dx;

            for (; idx != columnIndex1; idx++) {
                F24Dot8 delta = lift;

                mod += rem;

                if (mod >= 0) {
                    mod -= dx;
                    delta++;
                }

                const F24Dot8 ny = cy - delta;
                const F24Dot8 nx = cursor + T::TileWF24Dot8;

                Append(memory, idx, cursor, cy, nx, ny);

                cy = ny;
                cursor = nx;
            }
        }

        Append(memory, columnIndex1, cursor, cy, p1x, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineDownL(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x >= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y <= p1y);

    const TileIndex columnIndex0 = T::F24Dot8ToTileColumnIndex(p0x - 1);
    const TileIndex columnIndex1 = T::F24Dot8ToTileColumnIndex(p1x);

    BLAZE_ASSERT(columnIndex1 <= columnIndex0);

    if (columnIndex0 == columnIndex1) {
        Append(memory, columnIndex0, p0x, p0y, p1x, p1y);
    } else {
        // Number of pixels + 24.8 fraction for start and end points in
        // coordinate system of tiles these points belong to.
        const F24Dot8 fx = p0x - T::TileColumnIndexToF24Dot8(columnIndex0);

        BLAZE_ASSERT(fx >= 0);
        BLAZE_ASSERT(fx <= T::TileWF24Dot8);

        // Horizontal and vertical deltas.
        const F24Dot8 dx = p0x - p1x;
        const F24Dot8 dy = p1y - p0y;

        const F24Dot8 pp = fx * dy;

        F24Dot8 cy = p0y + (pp / dx);

        TileIndex idx = columnIndex0 - 1;

        F24Dot8 cursor = T::TileColumnIndexToF24Dot8(columnIndex0);

        Append(memory, columnIndex0, p0x, p0y, cursor, cy);

        if (idx != columnIndex1) {
            F24Dot8 mod = (pp % dx) - dx;

            const F24Dot8 p = T::TileWF24Dot8 * dy;
            const F24Dot8 lift = p / dx;
            const F24Dot8 rem = p % dx;

            for (; idx != columnIndex1; idx--) {
                F24Dot8 delta = lift;

                mod += rem;

                if (mod >= 0) {
                    mod -= dx;
                    delta++;
                }

                const F24Dot8 ny = cy + delta;
                const F24Dot8 nx = cursor - T::TileWF24Dot8;

                Append(memory, idx, cursor, cy, nx, ny);

                cy = ny;
                cursor = nx;
            }
        }

        Append(memory, columnIndex1, cursor, cy, p1x, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::AppendLineUpL(ThreadMemory &memory,
    const F24Dot8 p0x, const F24Dot8 p0y, const F24Dot8 p1x,
    const F24Dot8 p1y) {
    BLAZE_ASSERT(p0x >= p1x);
    BLAZE_ASSERT(p0y >= 0);
    BLAZE_ASSERT(p0y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p1y >= 0);
    BLAZE_ASSERT(p1y <= T::TileHF24Dot8);
    BLAZE_ASSERT(p0y >= p1y);

    const TileIndex columnIndex0 = T::F24Dot8ToTileColumnIndex(p0x - 1);
    const TileIndex columnIndex1 = T::F24Dot8ToTileColumnIndex(p1x);

    BLAZE_ASSERT(columnIndex1 <= columnIndex0);

    if (columnIndex0 == columnIndex1) {
        Append(memory, columnIndex0, p0x, p0y, p1x, p1y);
    } else {
        // Number of pixels + 24.8 fraction for start and end points in
        // coordinate system of tiles these points belong to.
        const F24Dot8 fx = p0x - T::TileColumnIndexToF24Dot8(columnIndex0);

        BLAZE_ASSERT(fx >= 0);
        BLAZE_ASSERT(fx <= T::TileWF24Dot8);

        // Horizontal and vertical deltas.
        const F24Dot8 dx = p0x - p1x;
        const F24Dot8 dy = p0y - p1y;

        const F24Dot8 pp = fx * dy;

        F24Dot8 cy = p0y - (pp / dx);

        TileIndex idx = columnIndex0 - 1;

        F24Dot8 cursor = T::TileColumnIndexToF24Dot8(columnIndex0);

        Append(memory, columnIndex0, p0x, p0y, cursor, cy);

        if (idx != columnIndex1) {
            F24Dot8 mod = (pp % dx) - dx;

            const F24Dot8 p = T::TileWF24Dot8 * dy;
            const F24Dot8 lift = p / dx;
            const F24Dot8 rem = p % dx;

            for (; idx != columnIndex1; idx--) {
                F24Dot8 delta = lift;

                mod += rem;

                if (mod >= 0) {
                    mod -= dx;
                    delta++;
                }

                const F24Dot8 ny = cy - delta;
                const F24Dot8 nx = cursor - T::TileWF24Dot8;

                Append(memory, idx, cursor, cy, nx, ny);

                cy = ny;
                cursor = nx;
            }
        }

        Append(memory, columnIndex1, cursor, cy, p1x, p1y);
    }
}


template <typename T>
inline void LineArrayTiled<T>::Append(ThreadMemory &memory,
    const TileIndex columnIndex, const F24Dot8 x0, const F24Dot8 y0,
    const F24Dot8 x1, const F24Dot8 y1) {
    if (y0 != y1) {
        const F24Dot8 cx = T::TileColumnIndexToF24Dot8(columnIndex);
        const F24Dot8 ex0 = x0 - cx;
        const F24Dot8 ex1 = x1 - cx;

        Push(memory, columnIndex, ex0, y0, ex1, y1);
    }
}


template <typename T>
inline void LineArrayTiled<T>::Push(ThreadMemory &memory,
    const TileIndex columnIndex, const F24Dot8 x0, const F24Dot8 y0,
    const F24Dot8 x1, const F24Dot8 y1) {
    if (ConditionalSetBit(mBitVectors, columnIndex)) {
        // First time line is inserted into this column.
        LineArrayTiledBlock *b = memory.FrameNewTiledBlock(nullptr);
        int32_t *covers = memory.FrameMallocArrayZeroFill<int32_t>(T::TileH);

        UpdateCoverTable(covers, y0, y1);

        b->P0P1[0] = PackF24Dot8ToF8Dot8x4(x0, y0, x1, y1);

        // First line sets count to 1 for line being inserted right now.
        mBlocks[columnIndex] = b;
        mCovers[columnIndex] = covers;
        mCounts[columnIndex] = 1;
    } else {
        LineArrayTiledBlock *current = mBlocks[columnIndex];

        // Count is total number of lines in all blocks. This makes things
        // easier later, at GPU data preparation stage.
        const int count = mCounts[columnIndex];

        BLAZE_ASSERT(count > 0);

        static constexpr int Mask = LineArrayTiledBlock::LinesPerBlock - 1;

        // Find out line count in current block. Assuming count will always be
        // at least 1 (first block allocation is handled as a special case and
        // sets count to 1). This value will be from 1 to maximum line count
        // for block.
        const int countInCurrentBlock = ((count - 1) & Mask) + 1;

        if (countInCurrentBlock < LineArrayTiledBlock::LinesPerBlock)
            [[likely]] {
            current->P0P1[countInCurrentBlock] =
                PackF24Dot8ToF8Dot8x4(x0, y0, x1, y1);

            UpdateCoverTable(mCovers[columnIndex], y0, y1);
        } else {
            LineArrayTiledBlock *b = memory.FrameNewTiledBlock(current);

            b->P0P1[0] = PackF24Dot8ToF8Dot8x4(x0, y0, x1, y1);

            UpdateCoverTable(mCovers[columnIndex], y0, y1);

            mBlocks[columnIndex] = b;
        }

        mCounts[columnIndex] = count + 1;
    }
}

} // namespace Blaze