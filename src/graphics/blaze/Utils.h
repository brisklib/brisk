
#pragma once

#include <cstdint>
#include <cstring>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <type_traits>


namespace Blaze {

#if defined(__GNUC__) || defined(__clang__)
#define BLAZE_FORCE_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define BLAZE_FORCE_INLINE __forceinline
#else
#define BLAZE_FORCE_INLINE
#endif

#ifdef DEBUG
#define BLAZE_ASSERT(p) assert(p)
#else
#define BLAZE_ASSERT(p)
#endif

#if INTPTR_MAX == INT64_MAX
#define BLAZE_MACHINE_64
#undef BLAZE_MACHINE_32
#elif INTPTR_MAX == INT32_MAX
#define BLAZE_MACHINE_32
#undef BLAZE_MACHINE_64
#else
#error Weird pointer size!
#endif

#define BLAZE_BIT_SIZE_OF(p) (sizeof(p) << 3)

#ifdef BLAZE_FLOAT64
using Float = double;
#define BLAZE_EPSILON DBL_EPSILON
#else
using Float = float;
#define BLAZE_EPSILON FLT_EPSILON
#endif

static_assert(sizeof(int8_t) == 1);
static_assert(sizeof(uint8_t) == 1);
static_assert(sizeof(int16_t) == 2);
static_assert(sizeof(uint16_t) == 2);
static_assert(sizeof(int32_t) == 4);
static_assert(sizeof(uint32_t) == 4);
static_assert(sizeof(int64_t) == 8);
static_assert(sizeof(uint64_t) == 8);

#define BLAZE_DISABLE_COPY_AND_ASSIGN(c)                                       \
    c(const c &a) = delete;                                                    \
    void operator=(const c &a);

BLAZE_FORCE_INLINE double Round(const double v) {
    return round(v);
}

BLAZE_FORCE_INLINE float Round(const float v) {
    return roundf(v);
}

template <typename T>
BLAZE_FORCE_INLINE T Min(const T a, const T b) {
    return a < b ? a : b;
}

template <typename T>
BLAZE_FORCE_INLINE T Max(const T a, const T b) {
    return a > b ? a : b;
}

/**
 * Finds the smallest of the three values.
 */
template <typename T>
BLAZE_FORCE_INLINE T Min3(const T a, const T b, const T c) {
    return Min(a, Min(b, c));
}

/**
 * Finds the greatest of the three values.
 */
template <typename T>
BLAZE_FORCE_INLINE T Max3(const T a, const T b, const T c) {
    return Max(a, Max(b, c));
}

/**
 * Rounds-up a given number.
 */
BLAZE_FORCE_INLINE float Ceil(const float v) {
    return ceilf(v);
}

/**
 * Rounds-up a given number.
 */
BLAZE_FORCE_INLINE double Ceil(const double v) {
    return ceil(v);
}

/**
 * Rounds-down a given number.
 */
BLAZE_FORCE_INLINE float Floor(const float v) {
    return floorf(v);
}

/**
 * Rounds-down a given number.
 */
BLAZE_FORCE_INLINE double Floor(const double v) {
    return floor(v);
}

/**
 * Returns square root of a given number.
 */
BLAZE_FORCE_INLINE float Sqrt(const float v) {
    return sqrtf(v);
}

/**
 * Returns square root of a given number.
 */
BLAZE_FORCE_INLINE double Sqrt(const double v) {
    return sqrt(v);
}

/**
 * Returns value clamped to range between minimum and maximum values.
 */
template <typename T>
BLAZE_FORCE_INLINE T Clamp(const T val, const T min, const T max) {
    return val > max ? max : val < min ? min : val;
}

/**
 * Returns absolute of a given value.
 */
template <typename T>
BLAZE_FORCE_INLINE T Abs(const T t) {
    return t >= 0 ? t : -t;
}

/**
 * Returns true if a given floating point value is not a number.
 */
BLAZE_FORCE_INLINE bool IsNaN(const float x) {
    return x != x;
}

/**
 * Returns true if a given double precision floating point value is not a
 * number.
 */
BLAZE_FORCE_INLINE bool IsNaN(const double x) {
    return x != x;
}

/**
 * Returns true if a given floating point number is finite.
 */
BLAZE_FORCE_INLINE bool IsFinite(const Float x) {
    // 0 × finite → 0
    // 0 × infinity → NaN
    // 0 × NaN → NaN
    const Float p = x * 0;

    return !IsNaN(p);
}

/**
 * Linearly interpolate between A and B.
 * If t is 0, returns A.
 * If t is 1, returns B.
 * If t is something else, returns value linearly interpolated between A and B.
 */
template <typename T, typename V>
BLAZE_FORCE_INLINE T InterpolateLinear(const T A, const T B, const V t) {
    BLAZE_ASSERT(t >= 0);
    BLAZE_ASSERT(t <= 1);

    return A + ((B - A) * t);
}

/**
 * Returns true if two given numbers are considered equal.
 */
BLAZE_FORCE_INLINE bool FuzzyIsEqual(const double a, const double b) {
    return (fabs(a - b) < DBL_EPSILON);
}

/**
 * Returns true if two given numbers are considered equal.
 */
BLAZE_FORCE_INLINE bool FuzzyIsEqual(const float a, const float b) {
    return (fabsf(a - b) < FLT_EPSILON);
}

/**
 * Returns true if a number can be considered being equal to zero.
 */
BLAZE_FORCE_INLINE bool FuzzyIsZero(const double d) {
    return fabs(d) < DBL_EPSILON;
}

/**
 * Returns true if two given numbers are not considered equal.
 */
BLAZE_FORCE_INLINE bool FuzzyNotEqual(const double a, const double b) {
    return (fabs(a - b) >= DBL_EPSILON);
}

/**
 * Returns true if two given numbers are not considered equal.
 */
BLAZE_FORCE_INLINE bool FuzzyNotEqual(const float a, const float b) {
    return (fabsf(a - b) >= FLT_EPSILON);
}

/**
 * Returns true if a number can be considered being equal to zero.
 */
BLAZE_FORCE_INLINE bool FuzzyIsZero(const float f) {
    return fabsf(f) < FLT_EPSILON;
}

/**
 * Returns true if a number can not be considered being equal to zero.
 */
BLAZE_FORCE_INLINE bool FuzzyNotZero(const double d) {
    return fabs(d) >= DBL_EPSILON;
}

/**
 * Returns true if a number can not be considered being equal to zero.
 */
BLAZE_FORCE_INLINE bool FuzzyNotZero(const float f) {
    return fabsf(f) >= FLT_EPSILON;
}

/**
 * Finds the greatest of the four values.
 */
template <typename T>
BLAZE_FORCE_INLINE T Max4(const T a, const T b, const T c, const T d) {
    return Max(a, Max(b, Max(c, d)));
}

/**
 * Finds the smallest of the four values.
 */
template <typename T>
BLAZE_FORCE_INLINE T Min4(const T a, const T b, const T c, const T d) {
    return Min(a, Min(b, Min(c, d)));
}

/**
 * Convert degrees to radians.
 */
BLAZE_FORCE_INLINE Float Deg2Rad(const Float x) {
    // pi / 180
    return x * 0.01745329251994329576923690768489;
}

/**
 * Convert radians to degrees.
 */
BLAZE_FORCE_INLINE Float Rad2Deg(const Float x) {
    // 180 / pi.
    return x * 57.295779513082320876798154814105;
}

/**
 * Calculates sine of a given number.
 */
BLAZE_FORCE_INLINE float Sin(const float v) {
    return sinf(v);
}

/**
 * Calculates sine of a given number.
 */
BLAZE_FORCE_INLINE double Sin(const double v) {
    return sin(v);
}

/**
 * Calculate cosine of a given number.
 */
BLAZE_FORCE_INLINE float Cos(const float v) {
    return cosf(v);
}

/**
 * Calculate cosine of a given number.
 */
BLAZE_FORCE_INLINE double Cos(const double v) {
    return cos(v);
}

/**
 * Returns tangent of a given number.
 */
BLAZE_FORCE_INLINE float Tan(const float v) {
    return tanf(v);
}

/**
 * Returns tangent of a given number.
 */
BLAZE_FORCE_INLINE double Tan(const double v) {
    return tan(v);
}

/**
 * Fill rule for filling a Bézier path.
 */
enum class FillRule : uint8_t { NonZero = 0, EvenOdd };

struct FloatPoint final {
    Float X = 0;
    Float Y = 0;
};


BLAZE_FORCE_INLINE FloatPoint operator-(
    const FloatPoint a, const FloatPoint b) {
    return FloatPoint{ a.X - b.X, a.Y - b.Y };
}


BLAZE_FORCE_INLINE FloatPoint operator+(
    const FloatPoint a, const FloatPoint b) {
    return FloatPoint{ a.X + b.X, a.Y + b.Y };
}


BLAZE_FORCE_INLINE FloatPoint operator*(
    const FloatPoint a, const FloatPoint b) {
    return FloatPoint{ a.X * b.X, a.Y * b.Y };
}

struct IntRect final {
    BLAZE_FORCE_INLINE IntRect(
        const int x, const int y, const int width, const int height)
        : MinX(x), MinY(y), MaxX(x + width), MaxY(y + height) {}

    int MinX = 0;
    int MinY = 0;
    int MaxX = 0;
    int MaxY = 0;
};


struct IntSize final {
    int Width = 0;
    int Height = 0;
};

/**
 * Bézier path command.
 */
enum class PathTag : uint8_t { Move = 0, Line, Quadratic, Cubic, Close };


using TileIndex = uint32_t;


/**
 * Represents a rectangle in destination image coordinates, measured in tiles.
 */
struct TileBounds final {

    BLAZE_FORCE_INLINE TileBounds(const TileIndex x, const TileIndex y,
        const TileIndex horizontalCount, const TileIndex verticalCount)
        : X(x), Y(y), ColumnCount(horizontalCount), RowCount(verticalCount) {
        BLAZE_ASSERT(ColumnCount > 0);
        BLAZE_ASSERT(RowCount > 0);
    }

    // Minimum horizontal and vertical tile indices.
    TileIndex X = 0;
    TileIndex Y = 0;

    // Horizontal and vertical tile counts. Total number of tiles covered
    // by a geometry can be calculated by multiplying these two values.
    TileIndex ColumnCount = 0;
    TileIndex RowCount = 0;
};


struct FloatRect final {
    BLAZE_FORCE_INLINE FloatRect(
        const Float x, const Float y, const Float width, const Float height)
        : MinX(x), MinY(y), MaxX(x + width), MaxY(y + height) {}


    BLAZE_FORCE_INLINE FloatRect(const IntRect &r)
        : MinX(r.MinX), MinY(r.MinY), MaxX(r.MaxX), MaxY(r.MaxY) {}


    IntRect ToExpandedIntRect() const {
        const int minx = int(Floor(MinX));
        const int miny = int(Floor(MinY));
        const int maxx = int(Ceil(MaxX));
        const int maxy = int(Ceil(MaxY));

        return IntRect(minx, miny, maxx - minx, maxy - miny);
    }


    Float MinX = 0;
    Float MinY = 0;
    Float MaxX = 0;
    Float MaxY = 0;
};


/**
 * 24.8 fixed point number.
 */
using F24Dot8 = int32_t;


static_assert(sizeof(F24Dot8) == 4);


/**
 * Value equivalent to one in 24.8 fixed point format.
 */
static constexpr F24Dot8 F24Dot8_1 = 1 << 8;


/**
 * Value equivalent to two in 24.8 fixed point format.
 */
static constexpr F24Dot8 F24Dot8_2 = 2 << 8;


/**
 * Converts floating point number to 24.8 fixed point number. Does not check if
 * a number is small enough to be represented as 24.8 number.
 */
BLAZE_FORCE_INLINE F24Dot8 ToF24Dot8(const Float v) {
    return F24Dot8(Round(v * Float(256.0)));
}


/**
 * Returns absolute value for a given 24.8 fixed point number.
 */
BLAZE_FORCE_INLINE F24Dot8 F24Dot8Abs(const F24Dot8 v) {
    static_assert(sizeof(F24Dot8) == 4);
    static_assert(sizeof(int) == 4);

    const int mask = v >> 31;

    return (v + mask) ^ mask;
}


/**
 * 8.8 fixed point number.
 */
using F8Dot8 = int16_t;

using F8Dot8x2 = uint32_t;
using F8Dot8x4 = uint64_t;


static_assert(sizeof(F8Dot8) == 2);
static_assert(sizeof(F8Dot8x2) == 4);
static_assert(sizeof(F8Dot8x4) == 8);


BLAZE_FORCE_INLINE F8Dot8x2 PackF24Dot8ToF8Dot8x2(
    const F24Dot8 a, const F24Dot8 b) {
    static_assert(sizeof(F24Dot8) == 4);

    // Values must be small enough.
    BLAZE_ASSERT((a & 0xffff0000) == 0);
    BLAZE_ASSERT((b & 0xffff0000) == 0);

    return F8Dot8x2(a) | (F8Dot8x2(b) << 16);
}


BLAZE_FORCE_INLINE F8Dot8x4 PackF24Dot8ToF8Dot8x4(
    const F24Dot8 a, const F24Dot8 b, const F24Dot8 c, const F24Dot8 d) {
    static_assert(sizeof(F24Dot8) == 4);

    // Values must be small enough.
    BLAZE_ASSERT((a & 0xffff0000) == 0);
    BLAZE_ASSERT((b & 0xffff0000) == 0);
    BLAZE_ASSERT((c & 0xffff0000) == 0);
    BLAZE_ASSERT((d & 0xffff0000) == 0);

    return F8Dot8x4(a) | (F8Dot8x4(b) << 16) | (F8Dot8x4(c) << 32) |
           (F8Dot8x4(d) << 48);
}


BLAZE_FORCE_INLINE F24Dot8 UnpackLoFromF8Dot8x2(const F8Dot8x2 a) {
    return F24Dot8(a & 0xffff);
}


BLAZE_FORCE_INLINE F24Dot8 UnpackHiFromF8Dot8x2(const F8Dot8x2 a) {
    static_assert(
        std::is_unsigned<F8Dot8x2>::value, "T must be an unsigned type");

    return F24Dot8(a >> 16);
}

struct F24Dot8Point final {
    F24Dot8 X;
    F24Dot8 Y;
};


BLAZE_FORCE_INLINE F24Dot8Point FloatPointToF24Dot8Point(const FloatPoint p) {
    return F24Dot8Point{ ToF24Dot8(p.X), ToF24Dot8(p.Y) };
}


BLAZE_FORCE_INLINE F24Dot8Point FloatPointToF24Dot8Point(
    const Float x, const Float y) {
    return F24Dot8Point{ ToF24Dot8(x), ToF24Dot8(y) };
}


static_assert(sizeof(F24Dot8Point) == 8);


/**
 * Keeps maximum point for clipping.
 */
struct ClipBounds final {

    constexpr ClipBounds(const int maxx, const int maxy)
        : MaxX(maxx), MaxY(maxy), FMax(F24Dot8Point{ maxx << 8, maxy << 8 }) {
        BLAZE_ASSERT(maxx > 0);
        BLAZE_ASSERT(maxy > 0);
    }


    const Float MaxX = 0;
    const Float MaxY = 0;
    const F24Dot8Point FMax = { 0, 0 };

private:
    // Prevent creating this with empty bounds as this is most likely not an
    // intentional situation.
    ClipBounds() = delete;
};



using FillRuleFn = int32_t (*)(const int32_t);


/**
 * Given area, calculate alpha in range 0-255 using non-zero fill rule.
 *
 * This function implements `min(abs(area), 1.0)`.
 */
BLAZE_FORCE_INLINE int32_t AreaToAlphaNonZero(const int32_t area) {
    static_assert(sizeof(int32_t) == 4);

    const int32_t aa = area >> 9;

    // Find absolute area value.
    const int32_t mask = aa >> 31;
    const int32_t aaabs = (aa + mask) ^ mask;

    // Clamp absolute area value to be 255 or less.
    return Min(aaabs, 255);
}


/**
 * Given area, calculate alpha in range 0-255 using even-odd fill rule.
 *
 * This function implements `abs(area - 2.0 × round(0.5 × area))`.
 */
BLAZE_FORCE_INLINE int32_t AreaToAlphaEvenOdd(const int32_t area) {
    static_assert(sizeof(int32_t) == 4);

    const int32_t aa = area >> 9;

    // Find absolute area value.
    const int32_t mask = aa >> 31;
    const int32_t aaabs = (aa + mask) ^ mask;

    const int32_t aac = aaabs & 511;

    if (aac > 256) {
        return 512 - aac;
    }

    return Min(aac, 255);
}


/**
 * This function returns 1 if value is greater than zero and it is divisible
 * by 256 (equal to one in 24.8 format) without a reminder.
 */
BLAZE_FORCE_INLINE int FindAdjustment(const F24Dot8 value) {
    static_assert(sizeof(F24Dot8) == 4);

    // Will be set to 0 is value is zero or less. Otherwise it will be 1.
    const int lte0 = ~((value - 1) >> 31) & 1;

    // Will be set to 1 if value is divisible by 256 without a reminder.
    // Otherwise it will be 0.
    const int db256 = (((value & 255) - 1) >> 31) & 1;

    // Return 1 if both bits (more than zero and disisible by 256) are set.
    return lte0 & db256;
}

} // namespace Blaze