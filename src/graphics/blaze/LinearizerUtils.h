
#pragma once

#include "CurveUtils.h"

namespace Blaze {

static inline void UpdateCoverTable_Down(
    int32_t *covers, const F24Dot8 y0, const F24Dot8 y1) {
    BLAZE_ASSERT(covers != nullptr);
    BLAZE_ASSERT(y0 < y1);

    // Integer parts for top and bottom.
    const int rowIndex0 = y0 >> 8;
    const int rowIndex1 = (y1 - 1) >> 8;

    BLAZE_ASSERT(rowIndex0 >= 0);
    // BLAZE_ASSERT(rowIndex0 < T::TileH);
    BLAZE_ASSERT(rowIndex1 >= 0);
    // BLAZE_ASSERT(rowIndex1 < T::TileH);

    const int fy0 = y0 - (rowIndex0 << 8);
    const int fy1 = y1 - (rowIndex1 << 8);

    if (rowIndex0 == rowIndex1) {
        covers[rowIndex0] -= fy1 - fy0;
    } else {
        covers[rowIndex0] -= 256 - fy0;

        for (int i = rowIndex0 + 1; i < rowIndex1; i++) {
            covers[i] -= 256;
        }

        covers[rowIndex1] -= fy1;
    }
}

static inline void UpdateCoverTable_Up(
    int32_t *covers, const F24Dot8 y0, const F24Dot8 y1) {
    BLAZE_ASSERT(covers != nullptr);
    BLAZE_ASSERT(y0 > y1);

    // Integer parts for top and bottom.
    const int rowIndex0 = (y0 - 1) >> 8;
    const int rowIndex1 = y1 >> 8;

    BLAZE_ASSERT(rowIndex0 >= 0);
    // BLAZE_ASSERT(rowIndex0 < T::TileH);
    BLAZE_ASSERT(rowIndex1 >= 0);
    // BLAZE_ASSERT(rowIndex1 < T::TileH);

    const int fy0 = y0 - (rowIndex0 << 8);
    const int fy1 = y1 - (rowIndex1 << 8);

    if (rowIndex0 == rowIndex1) {
        covers[rowIndex0] += fy0 - fy1;
    } else {
        covers[rowIndex0] += fy0;

        for (int i = rowIndex0 - 1; i > rowIndex1; i--) {
            covers[i] += 256;
        }

        covers[rowIndex1] += 256 - fy1;
    }
}

static inline void UpdateCoverTable(
    int32_t *covers, const F24Dot8 y0, const F24Dot8 y1) {
    if (y0 < y1) {
        UpdateCoverTable_Down(covers, y0, y1);
    } else {
        UpdateCoverTable_Up(covers, y0, y1);
    }
}

/**
 * Split quadratic curve in half.
 *
 * @param r Resulting curves. First curve will be represented as elements at
 * indices 0, 1 and 2. Second curve will be represented as elements at indices
 * 2, 3 and 4.
 *
 * @param s Source curve defined by three points.
 */
static inline void SplitQuadratic(F24Dot8Point r[5], const F24Dot8Point s[3]) {
    BLAZE_ASSERT(r != nullptr);
    BLAZE_ASSERT(s != nullptr);

    const F24Dot8 m0x = (s[0].X + s[1].X) >> 1;
    const F24Dot8 m0y = (s[0].Y + s[1].Y) >> 1;
    const F24Dot8 m1x = (s[1].X + s[2].X) >> 1;
    const F24Dot8 m1y = (s[1].Y + s[2].Y) >> 1;
    const F24Dot8 mx = (m0x + m1x) >> 1;
    const F24Dot8 my = (m0y + m1y) >> 1;

    r[0] = s[0];
    r[1].X = m0x;
    r[1].Y = m0y;
    r[2].X = mx;
    r[2].Y = my;
    r[3].X = m1x;
    r[3].Y = m1y;
    r[4] = s[2];
}

/**
 * Split cubic curve in half.
 *
 * @param r Resulting curves. First curve will be represented as elements at
 * indices 0, 1, 2 and 3. Second curve will be represented as elements at
 * indices 3, 4, 5 and 6.
 *
 * @param s Source curve defined by four points.
 */
static inline void SplitCubic(F24Dot8Point r[7], const F24Dot8Point s[4]) {
    BLAZE_ASSERT(r != nullptr);
    BLAZE_ASSERT(s != nullptr);

    const F24Dot8 m0x = (s[0].X + s[1].X) >> 1;
    const F24Dot8 m0y = (s[0].Y + s[1].Y) >> 1;
    const F24Dot8 m1x = (s[1].X + s[2].X) >> 1;
    const F24Dot8 m1y = (s[1].Y + s[2].Y) >> 1;
    const F24Dot8 m2x = (s[2].X + s[3].X) >> 1;
    const F24Dot8 m2y = (s[2].Y + s[3].Y) >> 1;
    const F24Dot8 m3x = (m0x + m1x) >> 1;
    const F24Dot8 m3y = (m0y + m1y) >> 1;
    const F24Dot8 m4x = (m1x + m2x) >> 1;
    const F24Dot8 m4y = (m1y + m2y) >> 1;
    const F24Dot8 mx = (m3x + m4x) >> 1;
    const F24Dot8 my = (m3y + m4y) >> 1;

    r[0] = s[0];
    r[1].X = m0x;
    r[1].Y = m0y;
    r[2].X = m3x;
    r[2].Y = m3y;
    r[3].X = mx;
    r[3].Y = my;
    r[4].X = m4x;
    r[4].Y = m4y;
    r[5].X = m2x;
    r[5].Y = m2y;
    r[6] = s[3];
}

static inline bool CutMonotonicQuadraticAt(const Float c0, const Float c1,
    const Float c2, const Float target, Float &t) {
    const Float A = c0 - c1 - c1 + c2;
    const Float B = 2 * (c1 - c0);
    const Float C = c0 - target;

    Float roots[2];

    const int count = FindQuadraticRoots(A, B, C, roots);

    if (count > 0) {
        t = roots[0];
        return true;
    }

    return false;
}

static inline bool CutMonotonicQuadraticAtX(
    const FloatPoint quadratic[3], const Float x, Float &t) {
    BLAZE_ASSERT(quadratic != nullptr);

    return CutMonotonicQuadraticAt(
        quadratic[0].X, quadratic[1].X, quadratic[2].X, x, t);
}

static inline bool CutMonotonicQuadraticAtY(
    const FloatPoint quadratic[3], const Float y, Float &t) {
    BLAZE_ASSERT(quadratic != nullptr);

    return CutMonotonicQuadraticAt(
        quadratic[0].Y, quadratic[1].Y, quadratic[2].Y, y, t);
}

static inline bool CutMonotonicCubicAt(Float &t, const Float pts[4]) {
    static constexpr Float Tolerance = 1e-7;

    Float negative = 0;
    Float positive = 0;

    if (pts[0] < 0) {
        if (pts[3] < 0) {
            return false;
        }

        negative = 0;
        positive = 1.0;
    } else if (pts[0] > 0) {
        if (pts[3] > 0) {
            return false;
        }

        negative = 1.0;
        positive = 0;
    } else {
        t = 0;
        return true;
    }

    do {
        const Float m = (positive + negative) / 2.0;
        const Float y01 = InterpolateLinear(pts[0], pts[1], m);
        const Float y12 = InterpolateLinear(pts[1], pts[2], m);
        const Float y23 = InterpolateLinear(pts[2], pts[3], m);
        const Float y012 = InterpolateLinear(y01, y12, m);
        const Float y123 = InterpolateLinear(y12, y23, m);
        const Float y0123 = InterpolateLinear(y012, y123, m);

        if (y0123 == 0.0) {
            t = m;
            return true;
        }

        if (y0123 < 0.0) {
            negative = m;
        } else {
            positive = m;
        }
    } while (Abs(positive - negative) > Tolerance);

    t = (negative + positive) / 2.0;

    return true;
}

static inline bool CutMonotonicCubicAtY(
    const FloatPoint pts[4], const Float y, Float &t) {
    Float c[4] = { pts[0].Y - y, pts[1].Y - y, pts[2].Y - y, pts[3].Y - y };

    return CutMonotonicCubicAt(t, c);
}

static inline bool CutMonotonicCubicAtX(
    const FloatPoint pts[4], const Float x, Float &t) {
    Float c[4] = { pts[0].X - x, pts[1].X - x, pts[2].X - x, pts[3].X - x };

    return CutMonotonicCubicAt(t, c);
}

/**
 * Returns true if a given quadratic curve is flat enough to be interpreted as
 * line for rasterizer.
 */
static inline bool IsQuadraticFlatEnough(const F24Dot8Point q[3]) {
    BLAZE_ASSERT(q != nullptr);

    if (q[0].X == q[2].X && q[0].Y == q[2].Y) {
        return true;
    }

    // Find middle point between start and end point.
    const F24Dot8 mx = (q[0].X + q[2].X) >> 1;
    const F24Dot8 my = (q[0].Y + q[2].Y) >> 1;

    // Calculate cheap distance between middle point and control point.
    const F24Dot8 dx = F24Dot8Abs(mx - q[1].X);
    const F24Dot8 dy = F24Dot8Abs(my - q[1].Y);

    // Add both distances together and compare with allowed error.
    const F24Dot8 dc = dx + dy;

    // 32 in 24.8 fixed point format is equal to 0.125.
    return dc <= 32;
}

static inline bool IsCubicFlatEnough(const F24Dot8Point c[4]) {
    BLAZE_ASSERT(c != nullptr);

    static constexpr F24Dot8 Tolerance = F24Dot8_1 >> 1;

    return F24Dot8Abs(2 * c[0].X - 3 * c[1].X + c[3].X) <= Tolerance &&
           F24Dot8Abs(2 * c[0].Y - 3 * c[1].Y + c[3].Y) <= Tolerance &&
           F24Dot8Abs(c[0].X - 3 * c[2].X + 2 * c[3].X) <= Tolerance &&
           F24Dot8Abs(c[0].Y - 3 * c[2].Y + 2 * c[3].Y) <= Tolerance;
}

} // namespace Blaze