
#pragma once


#include "Utils.h"

namespace Blaze {

/**
 * Descriptor for linearization into 8×32 pixel tiles.
 */
struct TileDescriptor_8x32 final {

    /**
     * Tile width in pixels.
     */
    static constexpr int TileW = 8;


    /**
     * Tile height in pixels.
     */
    static constexpr int TileH = 32;


    /**
     * Tile width in 24.8 fixed point format.
     */
    static constexpr F24Dot8 TileWF24Dot8 = 1 << 11;


    /**
     * Tile height in 24.8 fixed point format.
     */
    static constexpr F24Dot8 TileHF24Dot8 = 1 << 13;


    /**
     * Converts X value expressed as 24.8 fixed point number to horizontal tile
     * index.
     */
    static constexpr TileIndex F24Dot8ToTileColumnIndex(const F24Dot8 x) {
        return TileIndex(x >> 11);
    }


    /**
     * Converts Y value expressed as 24.8 fixed point number to vertical tile
     * index.
     */
    static constexpr TileIndex F24Dot8ToTileRowIndex(const F24Dot8 y) {
        return TileIndex(y >> 13);
    }


    /**
     * Converts X value to horizontal tile index.
     */
    static constexpr TileIndex PointsToTileColumnIndex(const int x) {
        return TileIndex(x >> 3);
    }


    /**
     * Converts Y value to vertical tile index.
     */
    static constexpr TileIndex PointsToTileRowIndex(const int y) {
        return (y >> 5);
    }


    /**
     * Converts horizontal tile index to X value.
     */
    static constexpr int TileColumnIndexToPoints(const TileIndex x) {
        return int(x) << 3;
    }


    /**
     * Converts vertical tile index to Y value.
     */
    static constexpr int TileRowIndexToPoints(const TileIndex y) {
        return int(y) << 5;
    }


    /**
     * Returns given vertical tile index to position in 24.8 format.
     */
    static constexpr F24Dot8 TileColumnIndexToF24Dot8(const TileIndex x) {
        return F24Dot8(x) << 11;
    }


    /**
     * Returns given horizontal tile index to position in 24.8 format.
     */
    static constexpr F24Dot8 TileRowIndexToF24Dot8(const TileIndex y) {
        return F24Dot8(y) << 13;
    }


    static constexpr bool CoverArrayContainsOnlyZeroes(const int32_t *t) {
        BLAZE_ASSERT(t != nullptr);

        // Combine all 32 values.
        const int32_t v = t[0] | t[1] | t[2] | t[3] | t[4] | t[5] | t[6] |
                          t[7] | t[8] | t[9] | t[10] | t[11] | t[12] | t[13] |
                          t[14] | t[15] | t[16] | t[17] | t[18] | t[19] |
                          t[20] | t[21] | t[22] | t[23] | t[24] | t[25] |
                          t[26] | t[27] | t[28] | t[29] | t[30] | t[31];

        // Zero means there are no non-zero values there.
        return v == 0;
    }


    static void FillStartCovers(int32_t *p, const int32_t value) {
        BLAZE_ASSERT(p != nullptr);

        p[0] = value;
        p[1] = value;
        p[2] = value;
        p[3] = value;
        p[4] = value;
        p[5] = value;
        p[6] = value;
        p[7] = value;
        p[8] = value;
        p[9] = value;
        p[10] = value;
        p[11] = value;
        p[12] = value;
        p[13] = value;
        p[14] = value;
        p[15] = value;
        p[16] = value;
        p[17] = value;
        p[18] = value;
        p[19] = value;
        p[20] = value;
        p[21] = value;
        p[22] = value;
        p[23] = value;
        p[24] = value;
        p[25] = value;
        p[26] = value;
        p[27] = value;
        p[28] = value;
        p[29] = value;
        p[30] = value;
        p[31] = value;
    }


    static void AccumulateStartCovers(int32_t *p, const int32_t value) {
        const int32_t p0 = p[0];
        const int32_t p1 = p[1];
        const int32_t p2 = p[2];
        const int32_t p3 = p[3];

        p[0] = value + p0;
        p[1] = value + p1;
        p[2] = value + p2;
        p[3] = value + p3;

        const int32_t p4 = p[4];
        const int32_t p5 = p[5];
        const int32_t p6 = p[6];
        const int32_t p7 = p[7];

        p[4] = value + p4;
        p[5] = value + p5;
        p[6] = value + p6;
        p[7] = value + p7;

        const int32_t p8 = p[8];
        const int32_t p9 = p[9];
        const int32_t p10 = p[10];
        const int32_t p11 = p[11];

        p[8] = value + p8;
        p[9] = value + p9;
        p[10] = value + p10;
        p[11] = value + p11;

        const int32_t p12 = p[12];
        const int32_t p13 = p[13];
        const int32_t p14 = p[14];
        const int32_t p15 = p[15];

        p[12] = value + p12;
        p[13] = value + p13;
        p[14] = value + p14;
        p[15] = value + p15;

        const int32_t p16 = p[16];
        const int32_t p17 = p[17];
        const int32_t p18 = p[18];
        const int32_t p19 = p[19];

        p[16] = value + p16;
        p[17] = value + p17;
        p[18] = value + p18;
        p[19] = value + p19;

        const int32_t p20 = p[20];
        const int32_t p21 = p[21];
        const int32_t p22 = p[22];
        const int32_t p23 = p[23];

        p[20] = value + p20;
        p[21] = value + p21;
        p[22] = value + p22;
        p[23] = value + p23;

        const int32_t p24 = p[24];
        const int32_t p25 = p[25];
        const int32_t p26 = p[26];
        const int32_t p27 = p[27];

        p[24] = value + p24;
        p[25] = value + p25;
        p[26] = value + p26;
        p[27] = value + p27;

        const int32_t p28 = p[28];
        const int32_t p29 = p[29];
        const int32_t p30 = p[30];
        const int32_t p31 = p[31];

        p[28] = value + p28;
        p[29] = value + p29;
        p[30] = value + p30;
        p[31] = value + p31;
    }


    alignas(64) static constexpr int32_t ZeroCovers[32] = { 0 };

private:
    TileDescriptor_8x32() = delete;
};

}