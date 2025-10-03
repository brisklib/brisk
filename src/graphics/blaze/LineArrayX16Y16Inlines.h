#pragma once

#include "LineArrayX16Y16.h"
#include "ThreadMemory.h"


namespace Blaze {

inline void LineArrayX16Y16::Construct(LineArrayX16Y16 *placement,
    const TileIndex rowCount, const TileIndex columnCount,
    ThreadMemory &memory) {
    BLAZE_ASSERT(placement != nullptr);
    BLAZE_ASSERT(rowCount > 0);
    BLAZE_ASSERT(columnCount > 0);

    for (TileIndex i = 0; i < rowCount; i++) {
        new (placement + i) LineArrayX16Y16();
    }
}


inline LineArrayX16Y16Block *LineArrayX16Y16::GetFrontBlock() const {
    return mCurrent;
}


inline int LineArrayX16Y16::GetFrontBlockLineCount() const {
    return mCount;
}


inline void LineArrayX16Y16::AppendVerticalLine(
    ThreadMemory &memory, const F24Dot8 x, const F24Dot8 y0, const F24Dot8 y1) {
    AppendLine(memory, x, y0, x, y1);
}


inline void LineArrayX16Y16::AppendLineDownR_V(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLineUpR_V(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLineDownL_V(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLineUpL_V(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLineDownRL(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLineUpRL(ThreadMemory &memory,
    const F24Dot8 x0, const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    AppendLine(memory, x0, y0, x1, y1);
}


inline void LineArrayX16Y16::AppendLine(
    ThreadMemory &memory, const F8Dot8x2 y0y1, const F8Dot8x2 x0x1) {
    LineArrayX16Y16Block *current = mCurrent;
    const int count = mCount;

    if (count < LineArrayX16Y16Block::LinesPerBlock) {
        // Most common.
        current->Y0Y1[count] = y0y1;
        current->X0X1[count] = x0x1;

        mCount = count + 1;
    } else {
        LineArrayX16Y16Block *b = memory.FrameNewX16Y16Block(current);

        b->Y0Y1[0] = y0y1;
        b->X0X1[0] = x0x1;

        // Set count to 1 for segment being added.
        mCount = 1;

        mCurrent = b;
    }
}


inline void LineArrayX16Y16::AppendLine(ThreadMemory &memory, const F24Dot8 x0,
    const F24Dot8 y0, const F24Dot8 x1, const F24Dot8 y1) {
    if (y0 != y1) {
        AppendLine(memory, PackF24Dot8ToF8Dot8x2(y0, y1),
            PackF24Dot8ToF8Dot8x2(x0, x1));
    }
}

} // namespace Blaze