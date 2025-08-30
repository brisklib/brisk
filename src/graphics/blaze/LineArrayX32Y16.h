
#pragma once


#include "Utils.h"


namespace Blaze {

class ThreadMemory;


struct LineArrayX32Y16Block final {
    explicit LineArrayX32Y16Block(LineArrayX32Y16Block *next) : Next(next) {}


    static constexpr int LinesPerBlock = 32;


    // Y0 and Y1 encoded as two 8.8 fixed point numbers packed into one 32 bit
    // integer.
    F8Dot8x2 Y0Y1[LinesPerBlock];
    F24Dot8 X0[LinesPerBlock];
    F24Dot8 X1[LinesPerBlock];

    // Pointer to the next block of lines in the same row.
    LineArrayX32Y16Block *Next = nullptr;

private:
    LineArrayX32Y16Block() = delete;

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(LineArrayX32Y16Block);
};


struct LineArrayX32Y16 final {
    LineArrayX32Y16() {}

public:
    static void Construct(LineArrayX32Y16 *placement, const TileIndex rowCount,
        const TileIndex columnCount, ThreadMemory &memory);

    LineArrayX32Y16Block *GetFrontBlock() const;
    int GetFrontBlockLineCount() const;

public:
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
    void AppendLine(ThreadMemory &memory, const F8Dot8x2 y0y1, const F24Dot8 x0,
        const F24Dot8 x1);
    void AppendLine(ThreadMemory &memory, const F24Dot8 x0, const F24Dot8 y0,
        const F24Dot8 x1, const F24Dot8 y1);

private:
    LineArrayX32Y16Block *mCurrent = nullptr;
    int mCount = LineArrayX32Y16Block::LinesPerBlock;

private:
    BLAZE_DISABLE_COPY_AND_ASSIGN(LineArrayX32Y16);
};

} // namespace Blaze