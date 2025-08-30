
#pragma once


#include "Utils.h"


namespace Blaze {

/*
 * Roots must not be nullptr. Returns 0, 1 or 2.
 */
int FindQuadraticRoots(
    const Float a, const Float b, const Float c, Float roots[2]);


/**
 * Finds extrema on X axis of a quadratic curve and splits it at this point,
 * producing up to 2 output curves. Returns the number of output curves. If
 * extrema was not found, this function returns 1 and destination array
 * contains points of the original input curve. Always returns 1 or 2.
 *
 * Curves are stored in output as follows.
 *
 * 1. dst[0], dst[1], dst[2]
 * 2. dst[2], dst[3], dst[4]
 *
 * @param src Input quadratic curve as 3 points.
 *
 * @param dst Pointer to memory for destination curves. Must be large enough
 * to keep 5 FloatPoint values.
 */
int CutQuadraticAtXExtrema(const FloatPoint src[3], FloatPoint dst[5]);


/**
 * Finds extremas on X axis of a cubic curve and splits it at these points,
 * producing up to 3 output curves. Returns the number of output curves. If no
 * extremas are found, this function returns 1 and destination array contains
 * points of the original input curve. Always returns 1, 2 or 3.
 *
 * Curves are stored in output as follows.
 *
 * 1. dst[0], dst[1], dst[2], dst[3]
 * 2. dst[3], dst[4], dst[5], dst[6]
 * 3. dst[6], dst[7], dst[8], dst[9]
 *
 * @param src Input cubic curve as 4 points.
 *
 * @param dst Pointer to memory for destination curves. Must be large enough
 * to keep 10 FloatPoint values.
 */
int CutCubicAtXExtrema(const FloatPoint src[4], FloatPoint dst[10]);


/**
 * Finds extrema on Y axis of a quadratic curve and splits it at this point,
 * producing up to 2 output curves. Returns the number of output curves. If
 * extrema was not found, this function returns 1 and destination array
 * contains points of the original input curve. Always returns 1 or 2.
 *
 * Curves are stored in output as follows.
 *
 * 1. dst[0], dst[1], dst[2]
 * 2. dst[2], dst[3], dst[4]
 *
 * @param src Input quadratic curve as 3 points.
 *
 * @param dst Pointer to memory for destination curves. Must be large enough
 * to keep 5 FloatPoint values.
 */
int CutQuadraticAtYExtrema(const FloatPoint src[3], FloatPoint dst[5]);


/**
 * Finds extremas on Y axis of a cubic curve and splits it at these points,
 * producing up to 3 output curves. Returns the number of output curves. If no
 * extremas are found, this function returns 1 and destination array contains
 * points of the original input curve. Always returns 1, 2 or 3.
 *
 * Curves are stored in output as follows.
 *
 * 1. dst[0], dst[1], dst[2], dst[3]
 * 2. dst[3], dst[4], dst[5], dst[6]
 * 3. dst[6], dst[7], dst[8], dst[9]
 *
 * @param src Input cubic curve as 4 points.
 *
 * @param dst Pointer to memory for destination curves. Must be large enough
 * to keep 10 FloatPoint values.
 */
int CutCubicAtYExtrema(const FloatPoint src[4], FloatPoint dst[10]);



/**
 * Returns true if a given value is between a and b.
 */
static inline bool IsValueBetweenAAndB(
    const Float a, const Float value, const Float b) {
    if (a <= b) {
        return a <= value && value <= b;
    } else {
        return a >= value && value >= b;
    }
}


/**
 * Returns true if given cubic curve is monotonic in X. This function only
 * checks if cubic control points are between end points. This means that this
 * function can return false when in fact curve does not change direction in X.
 *
 * Use this function for fast monotonicity checks.
 */
static inline bool CubicControlPointsBetweenEndPointsX(
    const FloatPoint pts[4]) {
    return IsValueBetweenAAndB(pts[0].X, pts[1].X, pts[3].X) &&
           IsValueBetweenAAndB(pts[0].X, pts[2].X, pts[3].X);
}


static inline bool QuadraticControlPointBetweenEndPointsX(
    const FloatPoint pts[3]) {
    return IsValueBetweenAAndB(pts[0].X, pts[1].X, pts[2].X);
}


/**
 * Returns true if given cubic curve is monotonic in Y. This function only
 * checks if cubic control points are between end points. This means that this
 * function can return false when in fact curve does not change direction in Y.
 *
 * Use this function for fast monotonicity checks.
 */
static inline bool CubicControlPointsBetweenEndPointsY(
    const FloatPoint pts[4]) {
    return IsValueBetweenAAndB(pts[0].Y, pts[1].Y, pts[3].Y) &&
           IsValueBetweenAAndB(pts[0].Y, pts[2].Y, pts[3].Y);
}


static inline bool QuadraticControlPointBetweenEndPointsY(
    const FloatPoint pts[3]) {
    return IsValueBetweenAAndB(pts[0].Y, pts[1].Y, pts[2].Y);
}


static inline void InterpolateQuadraticCoordinates(
    const Float *src, Float *dst, const Float t) {
    BLAZE_ASSERT(t >= 0.0);
    BLAZE_ASSERT(t <= 1.0);

    const Float ab = InterpolateLinear(src[0], src[2], t);
    const Float bc = InterpolateLinear(src[2], src[4], t);

    dst[0] = src[0];
    dst[2] = ab;
    dst[4] = InterpolateLinear(ab, bc, t);
    dst[6] = bc;
    dst[8] = src[4];
}


static inline void CutQuadraticAt(
    const FloatPoint src[3], FloatPoint dst[5], const Float t) {
    BLAZE_ASSERT(t >= 0.0);
    BLAZE_ASSERT(t <= 1.0);

    InterpolateQuadraticCoordinates(&src[0].X, &dst[0].X, t);
    InterpolateQuadraticCoordinates(&src[0].Y, &dst[0].Y, t);
}


static inline void InterpolateCubicCoordinates(
    const Float *src, Float *dst, const Float t) {
    BLAZE_ASSERT(t >= 0.0);
    BLAZE_ASSERT(t <= 1.0);

    const Float ab = InterpolateLinear(src[0], src[2], t);
    const Float bc = InterpolateLinear(src[2], src[4], t);
    const Float cd = InterpolateLinear(src[4], src[6], t);
    const Float abc = InterpolateLinear(ab, bc, t);
    const Float bcd = InterpolateLinear(bc, cd, t);
    const Float abcd = InterpolateLinear(abc, bcd, t);

    dst[0] = src[0];
    dst[2] = ab;
    dst[4] = abc;
    dst[6] = abcd;
    dst[8] = bcd;
    dst[10] = cd;
    dst[12] = src[6];
}


static inline void CutCubicAt(
    const FloatPoint src[4], FloatPoint dst[7], const Float t) {
    BLAZE_ASSERT(t >= 0.0);
    BLAZE_ASSERT(t <= 1.0);

    InterpolateCubicCoordinates(&src[0].X, &dst[0].X, t);
    InterpolateCubicCoordinates(&src[0].Y, &dst[0].Y, t);
}

} // namespace Blaze