
#pragma once

#include "Utils.h"

namespace Blaze {

/**
 * Descriptor for linearization into 8×8 pixel tiles.
 */
struct TileDescriptor_8x8 final {

    /**
     * Tile width in pixels.
     */
    static constexpr int TileW = 8;

    /**
     * Tile height in pixels.
     */
    static constexpr int TileH = 8;

    /**
     * Tile width in 24.8 fixed point format.
     */
    static constexpr F24Dot8 TileWF24Dot8 = 1 << 11;

    /**
     * Tile height in 24.8 fixed point format.
     */
    static constexpr F24Dot8 TileHF24Dot8 = 1 << 11;

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
        return TileIndex(y >> 11);
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
        return TileIndex(y >> 3);
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
        return int(y) << 3;
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
        return F24Dot8(y) << 11;
    }

    static constexpr bool CoverArrayContainsOnlyZeroes(const int32_t *t) {
        BLAZE_ASSERT(t != nullptr);

        // Combine all 8 values.
        const int32_t v = t[0] | t[1] | t[2] | t[3] | t[4] | t[5] | t[6] | t[7];

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
    }

    alignas(64) static constexpr int32_t ZeroCovers[8] = { 0 };

private:
    TileDescriptor_8x8() = delete;
};

} // namespace Blaze