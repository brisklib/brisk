
#include "CurveUtils.h"


namespace Blaze {

static int AcceptRoot(Float *t, const Float root) {
    if (root < -BLAZE_EPSILON) {
        return 0;
    } else if (root > (1.0 + BLAZE_EPSILON)) {
        return 0;
    }

    t[0] = Clamp(root, Float(0.0), Float(1.0));

    return 1;
}


int FindQuadraticRoots(
    const Float a, const Float b, const Float c, Float roots[2]) {
    BLAZE_ASSERT(roots != nullptr);

    const Float delta = b * b - 4.0 * a * c;

    if (delta < 0.0) {
        return 0;
    }

    if (delta > 0.0) {
        const Float d = Sqrt(delta);
        const Float q = -0.5 * (b + (b < 0.0 ? -d : d));
        const Float rv0 = q / a;
        const Float rv1 = c / q;

        if (FuzzyIsEqual(rv0, rv1)) {
            return AcceptRoot(roots, rv0);
        }

        if (rv0 < rv1) {
            int n = AcceptRoot(roots, rv0);

            n += AcceptRoot(roots + n, rv1);

            return n;
        } else {
            int n = AcceptRoot(roots, rv1);

            n += AcceptRoot(roots + n, rv0);

            return n;
        }
    }

    if (a != 0) {
        return AcceptRoot(roots, -0.5 * b / a);
    }

    return 0;
}


static int AcceptRootWithin(Float *t, const Float root) {
    if (root <= BLAZE_EPSILON) {
        return 0;
    } else if (root >= (1.0 - BLAZE_EPSILON)) {
        return 0;
    }

    t[0] = root;

    return 1;
}


static int FindQuadraticRootsWithin(
    const Float a, const Float b, const Float c, Float roots[2]) {
    BLAZE_ASSERT(roots != nullptr);

    const Float delta = b * b - 4.0 * a * c;

    if (delta < 0.0) {
        return 0;
    }

    if (delta > 0.0) {
        const Float d = Sqrt(delta);
        const Float q = -0.5 * (b + (b < 0.0 ? -d : d));
        const Float rv0 = q / a;
        const Float rv1 = c / q;

        if (FuzzyIsEqual(rv0, rv1)) {
            return AcceptRootWithin(roots, rv0);
        }

        if (rv0 < rv1) {
            int n = AcceptRootWithin(roots, rv0);

            n += AcceptRootWithin(roots + n, rv1);

            return n;
        } else {
            int n = AcceptRootWithin(roots, rv1);

            n += AcceptRootWithin(roots + n, rv0);

            return n;
        }
    }

    if (a != 0) {
        return AcceptRootWithin(roots, -0.5 * b / a);
    }

    return 0;
}


bool FindQuadraticExtrema(
    const Float a, const Float b, const Float c, Float &t) {
    const Float aMinusB = a - b;
    const Float d = aMinusB - b + c;

    if (aMinusB == 0 || d == 0) {
        return false;
    }

    const Float tv = aMinusB / d;

    BLAZE_ASSERT(IsFinite(tv));

    if (tv <= 2 * BLAZE_EPSILON || tv >= (1.0 - 2 * BLAZE_EPSILON)) {
        return false;
    }

    t = tv;

    return true;
}


int FindCubicExtrema(
    const Float a, const Float b, const Float c, const Float d, Float t[2]) {
    const Float A = d - a + 3.0 * (b - c);
    const Float B = 2.0 * (a - b - b + c);
    const Float C = b - a;

    return FindQuadraticRootsWithin(A, B, C, t);
}


int CutCubicAtYExtrema(const FloatPoint src[4], FloatPoint dst[10]) {
    BLAZE_ASSERT(src != nullptr);
    BLAZE_ASSERT(dst != nullptr);

    Float t[2];

    const int n = FindCubicExtrema(src[0].Y, src[1].Y, src[2].Y, src[3].Y, t);

    if (n == 1) {
        // One root, two output curves.

        BLAZE_ASSERT(t[0] > 0.0);
        BLAZE_ASSERT(t[0] < 1.0);

        CutCubicAt(src, dst, t[0]);

        // Make sure curve tangents at extrema are horizontal.
        const Float y = dst[3].Y;

        dst[2].Y = y;
        dst[4].Y = y;

        return 2;
    }

    if (n == 2) {
        // Two roots, three output curves.

        // Expect sorted roots from FindCubicExtrema.
        BLAZE_ASSERT(t[0] < t[1]);
        BLAZE_ASSERT(t[0] > 0.0);
        BLAZE_ASSERT(t[0] < 1.0);
        BLAZE_ASSERT(t[1] > 0.0);
        BLAZE_ASSERT(t[1] < 1.0);

        FloatPoint tmp[7];

        CutCubicAt(src, tmp, t[0]);

        dst[0] = tmp[0];
        dst[1] = tmp[1];
        dst[2] = tmp[2];

        const Float d = 1.0 - t[0];

        BLAZE_ASSERT(IsFinite(d));

        // Clamp to make sure we don't go out of range due to limited
        // precision.
        const Float tt = Clamp((t[1] - t[0]) / d, Float(0.0), Float(1.0));

        CutCubicAt(tmp + 3, dst + 3, tt);

        // Make sure curve tangents at extremas are horizontal.
        const Float y0 = dst[3].Y;
        const Float y1 = dst[6].Y;

        dst[2].Y = y0;
        dst[4].Y = y0;
        dst[5].Y = y1;
        dst[7].Y = y1;

        return 3;
    }

    BLAZE_ASSERT(n == 0);

    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];

    return 1;
}


int CutCubicAtXExtrema(const FloatPoint src[4], FloatPoint dst[10]) {
    BLAZE_ASSERT(src != nullptr);
    BLAZE_ASSERT(dst != nullptr);

    Float t[2];

    const int n = FindCubicExtrema(src[0].X, src[1].X, src[2].X, src[3].X, t);

    if (n == 1) {
        // One root, two output curves.

        BLAZE_ASSERT(t[0] > 0.0);
        BLAZE_ASSERT(t[0] < 1.0);

        CutCubicAt(src, dst, t[0]);

        // Make sure curve tangents at extrema are horizontal.
        const Float x = dst[3].X;

        dst[2].X = x;
        dst[4].X = x;

        return 2;
    }

    if (n == 2) {
        // Two roots, three output curves.

        // Expect sorted roots from FindCubicExtrema.
        BLAZE_ASSERT(t[0] < t[1]);
        BLAZE_ASSERT(t[0] > 0.0);
        BLAZE_ASSERT(t[0] < 1.0);
        BLAZE_ASSERT(t[1] > 0.0);
        BLAZE_ASSERT(t[1] < 1.0);

        FloatPoint tmp[7];

        CutCubicAt(src, tmp, t[0]);

        dst[0] = tmp[0];
        dst[1] = tmp[1];
        dst[2] = tmp[2];

        const Float d = 1.0 - t[0];

        BLAZE_ASSERT(IsFinite(d));

        // Clamp to make sure we don't go out of range due to limited
        // precision.
        const Float tt = Clamp((t[1] - t[0]) / d, Float(0.0), Float(1.0));

        CutCubicAt(tmp + 3, dst + 3, tt);

        // Make sure curve tangents at extremas are horizontal.
        const Float x0 = dst[3].X;
        const Float x1 = dst[6].X;

        dst[2].X = x0;
        dst[4].X = x0;
        dst[5].X = x1;
        dst[7].X = x1;

        return 3;
    }

    BLAZE_ASSERT(n == 0);

    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];

    return 1;
}


static bool IsQuadraticMonotonic(const Float a, const Float b, const Float c) {
    const Float ab = a - b;
    Float bc = b - c;

    if (ab < 0) {
        bc = -bc;
    }

    return ab != 0 && bc >= 0;
}


int CutQuadraticAtYExtrema(const FloatPoint src[3], FloatPoint dst[5]) {
    const Float a = src[0].Y;
    const Float b = src[1].Y;
    const Float c = src[2].Y;

    if (IsQuadraticMonotonic(a, b, c)) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        return 1;
    }

    Float t = 0;

    if (FindQuadraticExtrema(a, b, c, t)) {
        CutQuadraticAt(src, dst, t);

        const Float y = dst[2].Y;

        dst[1].Y = y;
        dst[3].Y = y;

        return 2;
    }

    dst[0] = FloatPoint{ src[0].X, a };

    dst[1] = FloatPoint{ src[1].X, Abs(a - b) < Abs(b - c) ? a : c };

    dst[2] = FloatPoint{ src[2].X, c };

    return 1;
}


int CutQuadraticAtXExtrema(const FloatPoint src[3], FloatPoint dst[5]) {
    const Float a = src[0].X;
    const Float b = src[1].X;
    const Float c = src[2].X;

    if (IsQuadraticMonotonic(a, b, c)) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        return 1;
    }

    Float t = 0;

    if (FindQuadraticExtrema(a, b, c, t)) {
        CutQuadraticAt(src, dst, t);

        const Float x = dst[2].X;

        dst[1].X = x;
        dst[3].X = x;

        return 2;
    }

    dst[0] = FloatPoint{ a, src[0].Y };

    dst[1] = FloatPoint{ Abs(a - b) < Abs(b - c) ? a : c, src[1].Y };

    dst[2] = FloatPoint{ c, src[2].Y };

    return 1;
}

} // namespace Blaze