
#pragma once


#include "BitOps.h"
#include "Utils.h"
#include "CurveUtils.h"
#include "Geometry.h"
#include "LinearizerUtils.h"
#include "SIMD.h"
#include "ThreadMemory.h"

namespace Blaze {


/**
 * Takes one geometry containing path as input and processes it as follows -
 *
 *   • Transforms path points by transformation matrix configured for
 *     geometry.
 *   • Clips all transformed segments (lines, quadratic curves and cubic
 *     curves) to destination image bounds.
 *   • Converts all segments, including curves, into series of lines in 24.8
 *     fixed point format.
 *   • Divides all lines that cross horizontal interval boundaries into two
 *     lines at points where they intersect intervals. Interval height is
 *     defined by T template parameter.
 *   • Inserts all lines to corresponding line arrays.
 *   • Lines that fall to the left of destination image bounds are processed
 *     as "start cover" contributors.
 *
 * Normally, only one Linearizer per path should be created, but it is not an
 * error to create more than one. For example, if only one path is being
 * rendered in the current pass. Or when there is one abnormally large path,
 * much larger than others being rendered in the same pass.
 *
 * To create a Linearizer, call Create static function. It will allocate a new
 * Linearizer object and process all segments. It returns already processed
 * geometry.
 *
 * Important notes about memory management -
 *
 *   • Linearizer itself is created in task memory. Thi means that once the
 *     current task completes, that memory will be discarded. You must
 *     retrieve all important information from Linearizer object before task
 *     completes.
 *   • All line segment blocks are allocated in frame memory. Thi means that
 *     line segments will persist until the frame is complete. You do not have
 *     to copy line segment blocks before task ends.
 *   • Start cover table is allocated in frame memory.
 *   • All start covers are allocated in frame memory.
 *   • You must not delete Linearizer returned by Create function.
 */
template <typename T, typename L>
struct Linearizer final {

    static Linearizer *Create(ThreadMemory &memory, const TileBounds &bounds,
        const bool contains, const Geometry *geometry);


    /**
     * Returns tile bounds occupied by content this linearizer processed.
     */
    TileBounds GetTileBounds() const;


    /**
     * Returns A table of start cover arrays. The first item in this table
     * contains start covers of the first tile row, second item contains start
     * covers for the second row, etc. The number of items in the returned
     * table is equal to tile row count. Tile rows that did not have any
     * geometry to the left of destination image, will contain nullptr in the
     * returned table.
     */
    int32_t **GetStartCoverTable() const;


    /**
     * Returns line array at a given index.
     *
     * @param index Line array index. Must be at least zero and less than a
     * number of tile rows.
     */
    const L *GetLineArrayAtIndex(const TileIndex index) const;

private:
    /**
     * Constructs Linearizer with bounds.
     */
    Linearizer(const TileBounds &bounds);


    /**
     * Processes geometry assuming that all points within be strictly
     * contained within tile bounds. It still clamps all coordinates to make
     * sure crash does not happen, but no clipping is performed and
     * out-of-bounds geometry will look incorrect in the result.
     *
     * @param geometry Geometry to use as a source of segments. Must not be
     * nullptr.
     *
     * @param memory Thread memory for allocations.
     */
    void ProcessContained(const Geometry *geometry, ThreadMemory &memory);


    /**
     * Processes geometry assuming that parts of it may be out of bounds and
     * clipping should be performed. This method is generally slower than
     * ProcessContained because of doing extra work of determining how
     * individual segments contribute to the result.
     */
    void ProcessUncontained(const Geometry *geometry, ThreadMemory &memory,
        const ClipBounds &clip, const Matrix &matrix);


    void AddUncontainedLine(ThreadMemory &memory, const ClipBounds &clip,
        const FloatPoint p0, const FloatPoint p1);


    void AddContainedLineF24Dot8(
        ThreadMemory &memory, const F24Dot8Point p0, const F24Dot8Point p1);


    /**
     * Adds quadratic curve which potentially is not completely within
     * clipping bounds. Curve does not have to be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Arbitrary quadratic curve.
     */
    void AddUncontainedQuadratic(
        ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[3]);


    /**
     * Adds quadratic curve which potentially is not completely within
     * clipping bounds. Curve must be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Monotonic quadratic curve.
     */
    void AddUncontainedMonotonicQuadratic(
        ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[3]);


    /**
     * Adds quadratic curve which potentially is not completely within
     * clipping bounds horizontally, but it is within top and bottom edges of
     * the clipping bounds. Curve must be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Monotonic quadratic curve.
     */
    void AddVerticallyContainedMonotonicQuadratic(
        ThreadMemory &memory, const ClipBounds &clip, FloatPoint p[3]);


    /**
     * Adds quadratic curve completely contained within tile bounds. Curve
     * points are in 24.8 format.
     */
    void AddContainedQuadraticF24Dot8(
        ThreadMemory &memory, const F24Dot8Point q[3]);


    /**
     * Adds cubic curve which potentially is not completely within clipping
     * bounds. Curve does not have to be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Arbitrary cubic curve.
     */
    void AddUncontainedCubic(
        ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[4]);


    /**
     * Adds cubic curve which potentially is not completely within clipping
     * bounds. Curve must be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Monotonic cubic curve.
     */
    void AddUncontainedMonotonicCubic(
        ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[4]);


    /**
     * Adds cubic curve which potentially is not completely within clipping
     * bounds horizontally, but it is within top and bottom edges of the
     * clipping bounds. Curve must be monotonic.
     *
     * @param memory Thread memory.
     *
     * @param clip Clipping bounds.
     *
     * @param p Monotonic cubic curve.
     */
    void AddVerticallyContainedMonotonicCubic(
        ThreadMemory &memory, const ClipBounds &clip, FloatPoint p[4]);


    void AddPotentiallyUncontainedCubicF24Dot8(
        ThreadMemory &memory, const F24Dot8Point max, const F24Dot8Point c[3]);


    /**
     * Adds cubic curve completely contained within tile bounds. Curve points
     * are in 24.8 format.
     */
    void AddContainedCubicF24Dot8(
        ThreadMemory &memory, const F24Dot8Point c[3]);


    /**
     * Inserts vertical line to line array at a given index.
     */
    void AppendVerticalLine(ThreadMemory &memory, const TileIndex rowIndex,
        const F24Dot8 x, const F24Dot8 y0, const F24Dot8 y1);


    /**
     * ⬊
     */
    void LineDownR(ThreadMemory &memory, const TileIndex rowIndex0,
        const TileIndex rowIndex1, const F24Dot8 dx, const F24Dot8 dy,
        const F24Dot8Point p0, const F24Dot8Point p1);


    /**
     * ⬈
     */
    void LineUpR(ThreadMemory &memory, const TileIndex rowIndex0,
        const TileIndex rowIndex1, const F24Dot8 dx, const F24Dot8 dy,
        const F24Dot8Point p0, const F24Dot8Point p1);


    /**
     * ⬋
     */
    void LineDownL(ThreadMemory &memory, const TileIndex rowIndex0,
        const TileIndex rowIndex1, const F24Dot8 dx, const F24Dot8 dy,
        const F24Dot8Point p0, const F24Dot8Point p1);


    /**
     * ⬉
     */
    void LineUpL(ThreadMemory &memory, const TileIndex rowIndex0,
        const TileIndex rowIndex1, const F24Dot8 dx, const F24Dot8 dy,
        const F24Dot8Point p0, const F24Dot8Point p1);


    void Vertical_Down(ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1,
        const F24Dot8 x);


    void Vertical_Up(ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1,
        const F24Dot8 x);


    int32_t *GetStartCoversForRowAtIndex(ThreadMemory &memory, const int index);

    void UpdateStartCovers(
        ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1);
    void UpdateStartCoversFull_Down(ThreadMemory &memory, const int index);
    void UpdateStartCoversFull_Up(ThreadMemory &memory, const int index);


    /**
     * Value indicating the maximum cover value for a single pixel. Since
     * rasterizer operates with 24.8 fixed point numbers, this means 256 × 256
     * subpixel grid.
     *
     * Positive value is for lines that go up.
     */
    static constexpr int32_t FullPixelCoverPositive = 256;


    /**
     * Minimum cover value.
     *
     * Negative value is for lines that go down.
     */
    static constexpr int32_t FullPixelCoverNegative = -256;


    L *LA(const int verticalIndex);

private:
    // Initialized at the beginning, does not change later.
    const TileBounds mBounds;

    // Keeps pointers to start cover arrays for each row of tiles. Allocated
    // in task memory and zero-filled when the first start cover array is
    // requested. Each entry is then allocated on demand in frame memory.
    int32_t **mStartCoverTable = nullptr;

    // Zero length trailing array.
    alignas(64) L mLA[0];
};


template <typename T, typename L>
inline Linearizer<T, L> *Linearizer<T, L>::Create(ThreadMemory &memory,
    const TileBounds &bounds, const bool contains, const Geometry *geometry) {
    Linearizer *linearizer = static_cast<Linearizer *>(
        memory.TaskMalloc(sizeof(Linearizer) + (sizeof(L) * bounds.RowCount)));

    new (linearizer) Linearizer(bounds);

    L::Construct(linearizer->mLA, bounds.RowCount, bounds.ColumnCount, memory);

    if (contains) {
        linearizer->ProcessContained(geometry, memory);
    } else {
        const int tx = T::TileColumnIndexToPoints(bounds.X);
        const int ty = T::TileRowIndexToPoints(bounds.Y);
        const int ch = T::TileColumnIndexToPoints(bounds.ColumnCount);
        const int cv = T::TileRowIndexToPoints(bounds.RowCount);

        const ClipBounds clip(ch, cv);

        Matrix matrix(geometry->TM);
        matrix.PreTranslate(-tx, -ty);

        linearizer->ProcessUncontained(geometry, memory, clip, matrix);
    }

    return linearizer;
}


template <typename T, typename L>
inline TileBounds Linearizer<T, L>::GetTileBounds() const {
    return mBounds;
}


template <typename T, typename L>
inline int32_t **Linearizer<T, L>::GetStartCoverTable() const {
    return mStartCoverTable;
}


template <typename T, typename L>
inline const L *Linearizer<T, L>::GetLineArrayAtIndex(
    const TileIndex index) const {
    BLAZE_ASSERT(index >= 0);
    BLAZE_ASSERT(index < mBounds.RowCount);

    return mLA + index;
}


template <typename T, typename L>
inline Linearizer<T, L>::Linearizer(const TileBounds &bounds)
    : mBounds(bounds) {}


template <typename T, typename L>
inline void Linearizer<T, L>::ProcessContained(
    const Geometry *geometry, ThreadMemory &memory) {
    // In this case path is known to be completely within destination image.
    // Some checks can be skipped.

    const int tagCount = geometry->TagCount;
    const int pointCount = geometry->PointCount;
    const PathTag *tags = geometry->Tags;

    F24Dot8Point *pp = reinterpret_cast<F24Dot8Point *>(
        memory.TaskMalloc(sizeof(F24Dot8Point) * pointCount));

    F24Dot8Point origin;

    origin.X = T::TileColumnIndexToF24Dot8(mBounds.X);
    origin.Y = T::TileRowIndexToF24Dot8(mBounds.Y);

    F24Dot8Point size;

    size.X = T::TileColumnIndexToF24Dot8(mBounds.ColumnCount);
    size.Y = T::TileRowIndexToF24Dot8(mBounds.RowCount);

    FloatPointsToF24Dot8Points(
        geometry->TM, pp, geometry->Points, pointCount, origin, size);

    F24Dot8Point moveTo = *pp++;

    for (int i = 1; i < tagCount; i++) {
        switch (tags[i]) {
        case PathTag::Move: {
            // Complete previous path.
            AddContainedLineF24Dot8(memory, pp[-1], moveTo);

            moveTo = pp[0];

            pp++;

            break;
        }

        case PathTag::Line: {
            AddContainedLineF24Dot8(memory, pp[-1], pp[0]);

            pp++;

            break;
        }

        case PathTag::Quadratic: {
            AddContainedQuadraticF24Dot8(memory, pp - 1);

            pp += 2;

            break;
        }

        case PathTag::Cubic: {
            AddContainedCubicF24Dot8(memory, pp - 1);

            pp += 3;

            break;
        }

        case PathTag::Close: {
            break;
        }
        }
    }

    // Complete final path.
    AddContainedLineF24Dot8(memory, pp[-1], moveTo);
}


template <typename T, typename L>
inline void Linearizer<T, L>::ProcessUncontained(const Geometry *geometry,
    ThreadMemory &memory, const ClipBounds &clip, const Matrix &matrix) {
    const int tagCount = geometry->TagCount;
    const PathTag *tags = geometry->Tags;
    const FloatPoint *points = geometry->Points;

    FloatPoint segment[4];

    FloatPoint moveTo = matrix.Map(*points++);

    segment[0] = moveTo;

    for (int i = 1; i < tagCount; i++) {
        switch (tags[i]) {
        case PathTag::Move: {
            // Complete previous path.
            AddUncontainedLine(memory, clip, segment[0], moveTo);

            moveTo = matrix.Map(points[0]);

            points++;

            segment[0] = moveTo;

            break;
        }

        case PathTag::Line: {
            const FloatPoint p = matrix.Map(points[0]);

            points++;

            AddUncontainedLine(memory, clip, segment[0], p);

            segment[0] = p;

            break;
        }

        case PathTag::Quadratic: {
            segment[1] = matrix.Map(points[0]);
            segment[2] = matrix.Map(points[1]);

            points += 2;

            AddUncontainedQuadratic(memory, clip, segment);

            segment[0] = segment[2];

            break;
        }

        case PathTag::Cubic: {
            segment[1] = matrix.Map(points[0]);
            segment[2] = matrix.Map(points[1]);
            segment[3] = matrix.Map(points[2]);

            points += 3;

            AddUncontainedCubic(memory, clip, segment);

            segment[0] = segment[3];

            break;
        }

        case PathTag::Close: {
            break;
        }
        }
    }

    // Complete final path.
    AddUncontainedLine(memory, clip, segment[0], moveTo);
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddUncontainedLine(ThreadMemory &memory,
    const ClipBounds &clip, const FloatPoint p0, const FloatPoint p1) {
    BLAZE_ASSERT(IsFinite(p0.X));
    BLAZE_ASSERT(IsFinite(p0.Y));
    BLAZE_ASSERT(IsFinite(p1.X));
    BLAZE_ASSERT(IsFinite(p1.Y));

    const Float y0 = p0.Y;
    const Float y1 = p1.Y;

    if (y0 == y1) {
        // Horizontal line, completely discarded.
        return;
    }

    if (y0 <= 0 && y1 <= 0) {
        // Line is on top, completely discarded.
        return;
    }

    if (y0 >= clip.MaxY && y1 >= clip.MaxY) {
        // Line is on bottom, completely discarded.
        return;
    }

    const Float x0 = p0.X;
    const Float x1 = p1.X;

    if (x0 >= clip.MaxX && x1 >= clip.MaxX) {
        // Line is on the right, completely discarded.
        return;
    }

    if (x0 == x1) {
        // Vertical line.
        const F24Dot8 x0c = Clamp(ToF24Dot8(x0), 0, clip.FMax.X);
        const F24Dot8 p0y = Clamp(ToF24Dot8(y0), 0, clip.FMax.Y);
        const F24Dot8 p1y = Clamp(ToF24Dot8(y1), 0, clip.FMax.Y);

        if (x0c == 0) {
            UpdateStartCovers(memory, p0y, p1y);
        } else {
            F24Dot8Point a;
            F24Dot8Point b;

            a.X = x0c;
            a.Y = p0y;

            b.X = x0c;
            b.Y = p1y;

            AddContainedLineF24Dot8(memory, a, b);
        }

        return;
    }

    // Vertical clipping.
    //
    // Use absolute delta-y, but not delta-x. Absolute delta-y is needed for
    // calculating vertical t value at min-y and max-y. Meanwhile delta-x
    // needs to be exact since it is multiplied by t and it can go left or
    // right.
    const Float deltay_v = Abs(y1 - y0);
    const Float deltax_v = x1 - x0;

    // These will point to line start/end after vertical clipping.
    Float rx0 = x0;
    Float ry0 = y0;
    Float rx1 = x1;
    Float ry1 = y1;

    if (y1 > y0) {
        // Line is going ↓.
        if (y0 < 0) {
            // Cut at min-y.
            const Float t = -y0 / deltay_v;

            rx0 = x0 + (deltax_v * t);
            ry0 = 0;
        }

        if (y1 > clip.MaxY) {
            // Cut again at max-y.
            const Float t = (clip.MaxY - y0) / deltay_v;

            rx1 = x0 + (deltax_v * t);
            ry1 = clip.MaxY;
        }
    } else {
        // Line is going ↑.
        if (y0 > clip.MaxY) {
            // Cut at max-y.
            const Float t = (y0 - clip.MaxY) / deltay_v;

            rx0 = x0 + (deltax_v * t);
            ry0 = clip.MaxY;
        }

        if (y1 < 0) {
            // Cut again at min-y.
            const Float t = y0 / deltay_v;

            rx1 = x0 + (deltax_v * t);
            ry1 = 0;
        }
    }

    // Find out if remaining line is on the right.
    if (rx0 >= clip.MaxX && rx1 >= clip.MaxX) {
        // Line is on the right, completely discarded.
        return;
    }

    if (rx0 > 0 && rx1 > 0 && rx0 < clip.MaxX && rx1 < clip.MaxX) {
        // Inside.
        F24Dot8Point a;
        F24Dot8Point b;

        a.X = Clamp(ToF24Dot8(rx0), 0, clip.FMax.X);
        a.Y = Clamp(ToF24Dot8(ry0), 0, clip.FMax.Y);

        b.X = Clamp(ToF24Dot8(rx1), 0, clip.FMax.X);
        b.Y = Clamp(ToF24Dot8(ry1), 0, clip.FMax.Y);

        AddContainedLineF24Dot8(memory, a, b);

        return;
    }

    if (rx0 <= 0 && rx1 <= 0) {
        // Left.
        const F24Dot8 a = Clamp(ToF24Dot8(ry0), 0, clip.FMax.Y);
        const F24Dot8 b = Clamp(ToF24Dot8(ry1), 0, clip.FMax.Y);

        UpdateStartCovers(memory, a, b);

        return;
    }

    // Horizontal clipping.
    const Float deltay_h = ry1 - ry0;
    const Float deltax_h = Abs(rx1 - rx0);

    if (rx1 > rx0) {
        // Line is going →.
        Float bx1 = rx1;
        Float by1 = ry1;

        if (rx1 > clip.MaxX) {
            // Cut off at max-x.
            const Float t = (clip.MaxX - rx0) / deltax_h;

            by1 = ry0 + (deltay_h * t);
            bx1 = clip.MaxX;
        }

        if (rx0 < 0) {
            // Split at min-x.
            const Float t = -rx0 / deltax_h;

            const F24Dot8 a = Clamp(ToF24Dot8(ry0), 0, clip.FMax.Y);

            F24Dot8Point b;
            F24Dot8Point c;

            b.X = 0;
            b.Y = Clamp(ToF24Dot8(ry0 + (deltay_h * t)), 0, clip.FMax.Y);

            c.X = Clamp(ToF24Dot8(bx1), 0, clip.FMax.X);
            c.Y = Clamp(ToF24Dot8(by1), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b.Y);

            AddContainedLineF24Dot8(memory, b, c);
        } else {
            F24Dot8Point a;
            F24Dot8Point b;

            a.X = Clamp(ToF24Dot8(rx0), 0, clip.FMax.X);
            a.Y = Clamp(ToF24Dot8(ry0), 0, clip.FMax.Y);

            b.X = Clamp(ToF24Dot8(bx1), 0, clip.FMax.X);
            b.Y = Clamp(ToF24Dot8(by1), 0, clip.FMax.Y);

            AddContainedLineF24Dot8(memory, a, b);
        }
    } else {
        // Line is going ←.
        Float bx0 = rx0;
        Float by0 = ry0;

        if (rx0 > clip.MaxX) {
            // Cut off at max-x.
            const Float t = (rx0 - clip.MaxX) / deltax_h;

            by0 = ry0 + (deltay_h * t);
            bx0 = clip.MaxX;
        }

        if (rx1 < 0) {
            // Split at min-x.
            const Float t = rx0 / deltax_h;

            F24Dot8Point a;
            F24Dot8Point b;

            a.X = Clamp(ToF24Dot8(bx0), 0, clip.FMax.X);
            a.Y = Clamp(ToF24Dot8(by0), 0, clip.FMax.Y);

            b.X = 0;
            b.Y = Clamp(ToF24Dot8(ry0 + (deltay_h * t)), 0, clip.FMax.Y);

            const F24Dot8 c = Clamp(ToF24Dot8(ry1), 0, clip.FMax.Y);

            AddContainedLineF24Dot8(memory, a, b);

            UpdateStartCovers(memory, b.Y, c);
        } else {
            F24Dot8Point a;
            F24Dot8Point b;

            a.X = Clamp(ToF24Dot8(bx0), 0, clip.FMax.X);
            a.Y = Clamp(ToF24Dot8(by0), 0, clip.FMax.Y);

            b.X = Clamp(ToF24Dot8(rx1), 0, clip.FMax.X);
            b.Y = Clamp(ToF24Dot8(ry1), 0, clip.FMax.Y);

            AddContainedLineF24Dot8(memory, a, b);
        }
    }
}


static constexpr F24Dot8 MaximumDelta = 2048 << 8;


template <typename T, typename L>
inline void Linearizer<T, L>::AddContainedLineF24Dot8(
    ThreadMemory &memory, const F24Dot8Point p0, const F24Dot8Point p1) {
    BLAZE_ASSERT(p0.X >= 0);
    BLAZE_ASSERT(p0.X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(p0.Y >= 0);
    BLAZE_ASSERT(p0.Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(p1.X >= 0);
    BLAZE_ASSERT(p1.X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(p1.Y >= 0);
    BLAZE_ASSERT(p1.Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));

    if (p0.Y == p1.Y) {
        // Ignore horizontal lines.
        return;
    }

    if (p0.X == p1.X) {
        // Special case, vertical line, simplifies this thing a lot.
        if (p0.Y < p1.Y) {
            // Line is going down ↓
            Vertical_Down(memory, p0.Y, p1.Y, p0.X);
        } else {
            // Line is going up ↑

            // Y values are not equal, as this case is checked already.

            BLAZE_ASSERT(p0.Y != p1.Y);

            Vertical_Up(memory, p0.Y, p1.Y, p0.X);
        }

        return;
    }

    // First thing is to limit line size.
    const F24Dot8 dx = F24Dot8Abs(p1.X - p0.X);
    const F24Dot8 dy = F24Dot8Abs(p1.Y - p0.Y);

    if (dx > MaximumDelta || dy > MaximumDelta) {
        const F24Dot8Point m{ (p0.X + p1.X) >> 1, (p0.Y + p1.Y) >> 1 };

        AddContainedLineF24Dot8(memory, p0, m);
        AddContainedLineF24Dot8(memory, m, p1);

        return;
    }

    // Line is short enough to be handled using 32 bit fixed point arithmetic.
    if (p0.Y < p1.Y) {
        // Line is going down ↓
        const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(p0.Y);
        const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(p1.Y - 1);

        BLAZE_ASSERT(rowIndex0 <= rowIndex1);

        if (rowIndex0 == rowIndex1) {
            // Entire line is completely within horizontal band. For curves
            // this is common case.
            const F24Dot8 ty = T::TileRowIndexToF24Dot8(rowIndex0);
            const F24Dot8 y0 = p0.Y - ty;
            const F24Dot8 y1 = p1.Y - ty;

            LA(rowIndex0)->AppendLineDownRL(memory, p0.X, y0, p1.X, y1);
        } else if (p0.X < p1.X) {
            // Line is going from left to right →
            LineDownR(memory, rowIndex0, rowIndex1, dx, dy, p0, p1);
        } else {
            // Line is going right to left ←
            LineDownL(memory, rowIndex0, rowIndex1, dx, dy, p0, p1);
        }
    } else {
        // Line is going up ↑

        // Y values are not equal, as this case is checked already.

        BLAZE_ASSERT(p0.Y > p1.Y);

        const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(p0.Y - 1);
        const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(p1.Y);

        BLAZE_ASSERT(rowIndex1 <= rowIndex0);

        if (rowIndex0 == rowIndex1) {
            // Entire line is completely within horizontal band. For curves
            // this is common case.
            const F24Dot8 ty = T::TileRowIndexToF24Dot8(rowIndex0);
            const F24Dot8 y0 = p0.Y - ty;
            const F24Dot8 y1 = p1.Y - ty;

            LA(rowIndex0)->AppendLineUpRL(memory, p0.X, y0, p1.X, y1);
        } else if (p0.X < p1.X) {
            // Line is going from left to right →
            LineUpR(memory, rowIndex0, rowIndex1, dx, dy, p0, p1);
        } else {
            // Line is going right to left ←
            LineUpL(memory, rowIndex0, rowIndex1, dx, dy, p0, p1);
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddUncontainedQuadratic(
    ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[3]) {
    BLAZE_ASSERT(p != nullptr);

    const Float minx = Min3(p[0].X, p[1].X, p[2].X);

    if (minx >= clip.MaxX) {
        // Curve is to the right of clipping bounds.
        return;
    }

    const Float miny = Min3(p[0].Y, p[1].Y, p[2].Y);

    if (miny >= clip.MaxY) {
        // Curve is below clipping bounds.
        return;
    }

    const Float maxy = Max3(p[0].Y, p[1].Y, p[2].Y);

    if (maxy <= 0) {
        // Curve is above clipping bounds.
        return;
    }

    // First test if primitive intersects with any of horizontal axes of
    // clipping bounds.
    if (miny >= 0 && maxy <= clip.MaxY) {
        // Primitive is within clipping bounds vertically.
        const Float maxx = Max3(p[0].X, p[1].X, p[2].X);

        if (maxx <= 0) {
            // And it is completely to the left of clipping bounds without
            // intersecting anything.
            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (maxx <= clip.MaxX && minx >= 0) {
            // Curve is completely inside.
            F24Dot8Point q[3];

            q[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
            q[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

            q[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
            q[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

            q[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
            q[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

            AddContainedQuadraticF24Dot8(memory, q);

            return;
        }
    }

    // Remaining option is that primitive potentially intersects clipping
    // bounds.
    // First is to monotonize curve and attempt to clip it.

    const bool monoInX = QuadraticControlPointBetweenEndPointsX(p);
    const bool monoInY = QuadraticControlPointBetweenEndPointsY(p);

    if (monoInX && monoInY) {
        // Already monotonic in both directions. Quite common case, especially
        // with quadratics, return early.
        AddUncontainedMonotonicQuadratic(memory, clip, p);
    } else {
        FloatPoint monoY[5];
        FloatPoint monoX[5];

        if (monoInY) {
            // Here we know it has control points outside of end point range
            // in X direction.
            const int nX = CutQuadraticAtXExtrema(p, monoX);

            for (int j = 0; j < nX; j++) {
                AddUncontainedMonotonicQuadratic(memory, clip, monoX + (j * 2));
            }
        } else {
            const int nY = CutQuadraticAtYExtrema(p, monoY);

            for (int i = 0; i < nY; i++) {
                const FloatPoint *my = monoY + (i * 2);

                if (QuadraticControlPointBetweenEndPointsX(my)) {
                    AddUncontainedMonotonicQuadratic(memory, clip, my);
                } else {
                    const int nX = CutQuadraticAtXExtrema(my, monoX);

                    for (int j = 0; j < nX; j++) {
                        AddUncontainedMonotonicQuadratic(
                            memory, clip, monoX + (j * 2));
                    }
                }
            }
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddUncontainedMonotonicQuadratic(
    ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[3]) {
    BLAZE_ASSERT(p != nullptr);
    BLAZE_ASSERT(IsFinite(p[0].X));
    BLAZE_ASSERT(IsFinite(p[0].Y));
    BLAZE_ASSERT(IsFinite(p[1].X));
    BLAZE_ASSERT(IsFinite(p[1].Y));
    BLAZE_ASSERT(IsFinite(p[2].X));
    BLAZE_ASSERT(IsFinite(p[2].Y));

    // Assuming curve is monotonic.
    BLAZE_ASSERT(p[1].X <= Max(p[0].X, p[2].X));
    BLAZE_ASSERT(p[1].X >= Min(p[0].X, p[2].X));
    BLAZE_ASSERT(p[1].Y <= Max(p[0].Y, p[2].Y));
    BLAZE_ASSERT(p[1].Y >= Min(p[0].Y, p[2].Y));

    const Float sx = p[0].X;
    const Float px = p[2].X;

    if (sx >= clip.MaxX && px >= clip.MaxX) {
        // Completely on the right.
        return;
    }

    const Float sy = p[0].Y;
    const Float py = p[2].Y;

    if (sy <= 0 && py <= 0) {
        // Completely on top.
        return;
    }

    if (sy >= clip.MaxY && py >= clip.MaxY) {
        // Completely on bottom.
        return;
    }

    FloatPoint pts[3] = { p[0], p[1], p[2] };

    FloatPoint tmp[5];

    Float t = 0;

    if (sy > py) {
        // Curve is going ↑.
        if (sy > clip.MaxY) {
            // Cut-off at bottom.
            if (CutMonotonicQuadraticAtY(pts, clip.MaxY, t)) {
                // Cut quadratic at t and keep upper part of curve (since we
                // are handling ascending curve and cutting at off bottom).
                CutQuadraticAt(pts, tmp, t);

                pts[0] = tmp[2];
                pts[1] = tmp[3];

                // pts[2] already contains tmp[4].
            }
        }

        if (py < 0) {
            // Cut-off at top.
            if (CutMonotonicQuadraticAtY(pts, 0, t)) {
                // Cut quadratic at t and keep bottom part of curve (since we
                // are handling ascending curve and cutting off at top).
                CutQuadraticAt(pts, tmp, t);

                // pts[0] already contains tmp[0].

                pts[1] = tmp[1];
                pts[2] = tmp[2];
            }
        }

        AddVerticallyContainedMonotonicQuadratic(memory, clip, pts);
    } else if (sy < py) {
        // Curve is going ↓.
        if (py > clip.MaxY) {
            // Cut-off at bottom.
            if (CutMonotonicQuadraticAtY(pts, clip.MaxY, t)) {
                // Cut quadratic at t and keep upper part of curve (since we are
                // handling descending curve and cutting at off bottom).
                CutQuadraticAt(pts, tmp, t);

                // pts[0] already contains tmp[0].

                pts[1] = tmp[1];
                pts[2] = tmp[2];
            }
        }

        if (sy < 0) {
            // Cut-off at top.
            if (CutMonotonicQuadraticAtY(pts, 0, t)) {
                // Cut quadratic at t and keep bottom part of curve (since we
                // are handling descending curve and cutting off at top).
                CutQuadraticAt(pts, tmp, t);

                pts[0] = tmp[2];
                pts[1] = tmp[3];

                // pts[2] already contains tmp[4].
            }
        }

        AddVerticallyContainedMonotonicQuadratic(memory, clip, pts);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddVerticallyContainedMonotonicQuadratic(
    ThreadMemory &memory, const ClipBounds &clip, FloatPoint p[3]) {
    BLAZE_ASSERT(p != nullptr);
    BLAZE_ASSERT(IsFinite(p[0].X));
    BLAZE_ASSERT(IsFinite(p[0].Y));
    BLAZE_ASSERT(IsFinite(p[1].X));
    BLAZE_ASSERT(IsFinite(p[1].Y));
    BLAZE_ASSERT(IsFinite(p[2].X));
    BLAZE_ASSERT(IsFinite(p[2].Y));

    // Assuming curve is monotonic.
    BLAZE_ASSERT(p[1].X <= Max(p[0].X, p[2].X));
    BLAZE_ASSERT(p[1].X >= Min(p[0].X, p[2].X));
    BLAZE_ASSERT(p[1].Y <= Max(p[0].Y, p[2].Y));
    BLAZE_ASSERT(p[1].Y >= Min(p[0].Y, p[2].Y));

    const Float sx = p[0].X;
    const Float px = p[2].X;

    Float t = 0;

    FloatPoint tmp[5];

    if (sx > px) {
        // Curve is going ←.
        if (px >= clip.MaxX) {
            // Completely on right.
            return;
        }

        if (sx <= 0) {
            // Completely on left.

            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (sx > clip.MaxX) {
            // Cut-off at right.
            if (CutMonotonicQuadraticAtX(p, clip.MaxX, t)) {
                // Cut quadratic at t and keep left part of curve (since we are
                // handling right-to-left curve and cutting at off right part).
                CutQuadraticAt(p, tmp, t);

                p[0] = tmp[2];
                p[1] = tmp[3];

                // p[2] already contains tmp[4].
            }
        }

        if (px < 0) {
            // Split at min-x.
            if (CutMonotonicQuadraticAtX(p, 0, t)) {
                // Cut quadratic in two parts and keep both since we also need
                // the part on the left side of bounding box.
                CutQuadraticAt(p, tmp, t);

                F24Dot8Point q[3];

                q[0].X = Clamp(ToF24Dot8(tmp[0].X), 0, clip.FMax.X);
                q[0].Y = Clamp(ToF24Dot8(tmp[0].Y), 0, clip.FMax.Y);

                q[1].X = Clamp(ToF24Dot8(tmp[1].X), 0, clip.FMax.X);
                q[1].Y = Clamp(ToF24Dot8(tmp[1].Y), 0, clip.FMax.Y);

                q[2].X = Clamp(ToF24Dot8(tmp[2].X), 0, clip.FMax.X);
                q[2].Y = Clamp(ToF24Dot8(tmp[2].Y), 0, clip.FMax.Y);

                const F24Dot8 c =
                    Clamp(ToF24Dot8(tmp[4].Y), 0, clip.FMax.Y);

                AddContainedQuadraticF24Dot8(memory, q);

                UpdateStartCovers(memory, q[2].Y, c);

                return;
            }
        }

        // At this point we have entire curve inside bounding box.

        F24Dot8Point q[3];

        q[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
        q[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

        q[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
        q[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

        q[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
        q[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

        AddContainedQuadraticF24Dot8(memory, q);
    } else if (sx < px) {
        // Curve is going →.
        if (sx >= clip.MaxX) {
            // Completely on right.
            return;
        }

        if (px <= 0) {
            // Completely on left.

            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (px > clip.MaxX) {
            // Cut-off at right.
            if (CutMonotonicQuadraticAtX(p, clip.MaxX, t)) {
                // Cut quadratic at t and keep left part of curve (since we are
                // handling left-to-right curve and cutting at off right part).
                CutQuadraticAt(p, tmp, t);

                // p[0] already contains tmp[0].

                p[1] = tmp[1];
                p[2] = tmp[2];
            }
        }

        if (sx < 0) {
            // Split at min-x.
            if (CutMonotonicQuadraticAtX(p, 0, t)) {
                // Chop quadratic in two equal parts and keep both since we also
                // need the part on the left side of bounding box.
                CutQuadraticAt(p, tmp, t);

                const F24Dot8 a =
                    Clamp(ToF24Dot8(tmp[0].Y), 0, clip.FMax.Y);

                F24Dot8Point q[3];

                q[0].X = Clamp(ToF24Dot8(tmp[2].X), 0, clip.FMax.X);
                q[0].Y = Clamp(ToF24Dot8(tmp[2].Y), 0, clip.FMax.Y);

                q[1].X = Clamp(ToF24Dot8(tmp[3].X), 0, clip.FMax.X);
                q[1].Y = Clamp(ToF24Dot8(tmp[3].Y), 0, clip.FMax.Y);

                q[2].X = Clamp(ToF24Dot8(tmp[4].X), 0, clip.FMax.X);
                q[2].Y = Clamp(ToF24Dot8(tmp[4].Y), 0, clip.FMax.Y);

                UpdateStartCovers(memory, a, q[0].Y);

                AddContainedQuadraticF24Dot8(memory, q);

                return;
            }
        }

        // At this point we have entire curve inside bounding box.

        F24Dot8Point q[3];

        q[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
        q[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

        q[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
        q[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

        q[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
        q[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

        AddContainedQuadraticF24Dot8(memory, q);
    } else {
        // Vertical line.
        if (px < clip.MaxX) {
            if (px <= 0) {
                // Vertical line on the left.
                const F24Dot8 a =
                    Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
                const F24Dot8 b =
                    Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

                UpdateStartCovers(memory, a, b);
            } else {
                // Vertical line inside clip rect.
                F24Dot8Point q[3];

                q[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
                q[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

                q[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
                q[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

                q[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
                q[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

                AddContainedQuadraticF24Dot8(memory, q);
            }
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddContainedQuadraticF24Dot8(
    ThreadMemory &memory, const F24Dot8Point q[3]) {
    BLAZE_ASSERT(q != nullptr);
    BLAZE_ASSERT(q[0].X >= 0);
    BLAZE_ASSERT(q[0].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(q[0].Y >= 0);
    BLAZE_ASSERT(q[0].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(q[1].X >= 0);
    BLAZE_ASSERT(q[1].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(q[1].Y >= 0);
    BLAZE_ASSERT(q[1].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(q[2].X >= 0);
    BLAZE_ASSERT(q[2].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(q[2].Y >= 0);
    BLAZE_ASSERT(q[2].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));

    if (IsQuadraticFlatEnough(q)) {
        AddContainedLineF24Dot8(memory, q[0], q[2]);
    } else {
        F24Dot8Point split[5];

        SplitQuadratic(split, q);

        AddContainedQuadraticF24Dot8(memory, split);

        AddContainedQuadraticF24Dot8(memory, split + 2);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddUncontainedCubic(
    ThreadMemory &memory, const ClipBounds &clip, const FloatPoint p[4]) {
    BLAZE_ASSERT(p != nullptr);

    const Float minx = Min4(p[0].X, p[1].X, p[2].X, p[3].X);

    if (minx >= clip.MaxX) {
        // Curve is to the right of clipping bounds.
        return;
    }

    const Float miny = Min4(p[0].Y, p[1].Y, p[2].Y, p[3].Y);

    if (miny >= clip.MaxY) {
        // Curve is below clipping bounds.
        return;
    }

    const Float maxy = Max4(p[0].Y, p[1].Y, p[2].Y, p[3].Y);

    if (maxy <= 0) {
        // Curve is above clipping bounds.
        return;
    }

    // First test if primitive intersects with any of horizontal axes of
    // clipping bounds.
    if (miny >= 0 && maxy <= clip.MaxY) {
        // Primitive is within clipping bounds vertically.
        const Float maxx = Max4(p[0].X, p[1].X, p[2].X, p[3].X);

        if (maxx <= 0) {
            // And it is completely to the left of clipping bounds without
            // intersecting anything.

            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (maxx <= clip.MaxX && minx >= 0) {
            F24Dot8Point c[4];

            c[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
            c[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

            c[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
            c[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

            c[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
            c[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

            c[3].X = Clamp(ToF24Dot8(p[3].X), 0, clip.FMax.X);
            c[3].Y = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

            AddContainedCubicF24Dot8(memory, c);

            return;
        }
    }

    // Remaining option is that primitive potentially intersects clipping
    // bounds.
    //
    // Actual clipper expects monotonic cubics, so monotonize input.

    const bool monoInX = CubicControlPointsBetweenEndPointsX(p);
    const bool monoInY = CubicControlPointsBetweenEndPointsY(p);

    if (monoInX && monoInY) {
        // Already monotonic in both directions. Quite common case, return
        // early.
        AddUncontainedMonotonicCubic(memory, clip, p);
    } else {
        FloatPoint monoY[10];
        FloatPoint monoX[10];

        if (monoInY) {
            // Here we know it has control points outside of end point range
            // in X direction.
            const int nX = CutCubicAtXExtrema(p, monoX);

            for (int j = 0; j < nX; j++) {
                AddUncontainedMonotonicCubic(memory, clip, monoX + (j * 3));
            }
        } else {
            const int nY = CutCubicAtYExtrema(p, monoY);

            for (int i = 0; i < nY; i++) {
                const FloatPoint *my = monoY + (i * 3);

                if (CubicControlPointsBetweenEndPointsX(my)) {
                    AddUncontainedMonotonicCubic(memory, clip, my);
                } else {
                    const int nX = CutCubicAtXExtrema(my, monoX);

                    for (int j = 0; j < nX; j++) {
                        AddUncontainedMonotonicCubic(
                            memory, clip, monoX + (j * 3));
                    }
                }
            }
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddUncontainedMonotonicCubic(
    ThreadMemory &memory, const ClipBounds &clip, const FloatPoint *p) {
    BLAZE_ASSERT(p != nullptr);
    BLAZE_ASSERT(IsFinite(p[0].X));
    BLAZE_ASSERT(IsFinite(p[0].Y));
    BLAZE_ASSERT(IsFinite(p[1].X));
    BLAZE_ASSERT(IsFinite(p[1].Y));
    BLAZE_ASSERT(IsFinite(p[2].X));
    BLAZE_ASSERT(IsFinite(p[2].Y));
    BLAZE_ASSERT(IsFinite(p[3].X));
    BLAZE_ASSERT(IsFinite(p[3].Y));

    // Assuming curve is monotonic.

    const Float sx = p[0].X;
    const Float px = p[3].X;

    if (sx >= clip.MaxX && px >= clip.MaxX) {
        // Completely on the right.
        return;
    }

    const Float sy = p[0].Y;
    const Float py = p[3].Y;

    if (sy <= 0 && py <= 0) {
        // Completely on top.
        return;
    }

    if (sy >= clip.MaxY && py >= clip.MaxY) {
        // Completely on bottom.
        return;
    }

    FloatPoint pts[4] = { p[0], p[1], p[2], p[3] };

    FloatPoint tmp[7];

    Float t = 0;

    if (sy > py) {
        // Curve is ascending.
        if (sy > clip.MaxY) {
            // Cut-off at bottom.
            if (CutMonotonicCubicAtY(p, clip.MaxY, t)) {
                // Cut cubic at t and keep upper part of curve (since we are
                // handling ascending curve and cutting at off bottom).
                CutCubicAt(p, tmp, t);

                pts[0] = tmp[3];
                pts[1] = tmp[4];
                pts[2] = tmp[5];

                // pts[3] already contains tmp[6].
            }
        }

        if (py < 0) {
            // Cut-off at top.
            if (CutMonotonicCubicAtY(pts, 0, t)) {
                // Cut cubic at t and keep bottom part of curve (since we are
                // handling ascending curve and cutting off at top).
                CutCubicAt(pts, tmp, t);

                // pts[0] already contains tmp[0].

                pts[1] = tmp[1];
                pts[2] = tmp[2];
                pts[3] = tmp[3];
            }
        }

        AddVerticallyContainedMonotonicCubic(memory, clip, pts);
    } else if (sy < py) {
        // Curve is descending.
        if (py > clip.MaxY) {
            // Cut-off at bottom.
            if (CutMonotonicCubicAtY(pts, clip.MaxY, t)) {
                // Cut cubic at t and keep upper part of curve (since we are
                // handling descending curve and cutting at off bottom).
                CutCubicAt(pts, tmp, t);

                // pts[0] already contains tmp[0].

                pts[1] = tmp[1];
                pts[2] = tmp[2];
                pts[3] = tmp[3];
            }
        }

        if (sy < 0) {
            // Cut-off at top.
            if (CutMonotonicCubicAtY(pts, 0, t)) {
                // Cut cubic at t and keep bottom part of curve (since we are
                // handling descending curve and cutting off at top).
                CutCubicAt(pts, tmp, t);

                pts[0] = tmp[3];
                pts[1] = tmp[4];
                pts[2] = tmp[5];

                // pts[3] already contains tmp[6].
            }
        }

        AddVerticallyContainedMonotonicCubic(memory, clip, pts);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddVerticallyContainedMonotonicCubic(
    ThreadMemory &memory, const ClipBounds &clip, FloatPoint p[4]) {
    BLAZE_ASSERT(p != nullptr);
    BLAZE_ASSERT(IsFinite(p[0].X));
    BLAZE_ASSERT(IsFinite(p[0].Y));
    BLAZE_ASSERT(IsFinite(p[1].X));
    BLAZE_ASSERT(IsFinite(p[1].Y));
    BLAZE_ASSERT(IsFinite(p[2].X));
    BLAZE_ASSERT(IsFinite(p[2].Y));
    BLAZE_ASSERT(IsFinite(p[3].X));
    BLAZE_ASSERT(IsFinite(p[3].Y));

    const Float sx = p[0].X;
    const Float px = p[3].X;

    Float t = 0;

    FloatPoint tmp[7];

    if (sx > px) {
        // Curve is going from right to left.
        if (px >= clip.MaxX) {
            // Completely on right.
            return;
        }

        if (sx <= 0) {
            // Completely on left.

            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (sx > clip.MaxX) {
            // Cut-off at right.
            if (CutMonotonicCubicAtX(p, clip.MaxX, t)) {
                // Cut cubic at t and keep left part of curve (since we are
                // handling right-to-left curve and cutting at off right part).
                CutCubicAt(p, tmp, t);

                p[0] = tmp[3];
                p[1] = tmp[4];
                p[2] = tmp[5];

                // p[3] already contains tmp[6].
            }
        }

        if (px < 0) {
            // Cut-off at left.
            if (CutMonotonicCubicAtX(p, 0, t)) {
                // Cut cubic in two equal parts and keep both since we also
                // need the part on the left side of bounding box.
                CutCubicAt(p, tmp, t);

                // Since curve is going from right to left, the first one will
                // be inside and the second one will be on the left.
                F24Dot8Point c[4];

                c[0].X = ToF24Dot8(tmp[0].X);
                c[0].Y = ToF24Dot8(tmp[0].Y);

                c[1].X = ToF24Dot8(tmp[1].X);
                c[1].Y = ToF24Dot8(tmp[1].Y);

                c[2].X = ToF24Dot8(tmp[2].X);
                c[2].Y = ToF24Dot8(tmp[2].Y);

                c[3].X = ToF24Dot8(tmp[3].X);
                c[3].Y = ToF24Dot8(tmp[3].Y);

                AddPotentiallyUncontainedCubicF24Dot8(memory, clip.FMax, c);

                const F24Dot8 b =
                    Clamp(ToF24Dot8(tmp[6].Y), 0, clip.FMax.Y);

                UpdateStartCovers(memory, Clamp(c[3].Y, 0, clip.FMax.Y), b);

                return;
            }
        }

        // At this point we have entire curve inside bounding box.

        F24Dot8Point c[4];

        c[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
        c[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

        c[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
        c[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

        c[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
        c[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

        c[3].X = Clamp(ToF24Dot8(p[3].X), 0, clip.FMax.X);
        c[3].Y = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

        AddContainedCubicF24Dot8(memory, c);
    } else if (sx < px) {
        // Curve is going from left to right.
        if (sx >= clip.MaxX) {
            // Completely on right.
            return;
        }

        if (px <= 0) {
            // Completely on left.

            const F24Dot8 a = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
            const F24Dot8 b = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

            UpdateStartCovers(memory, a, b);

            return;
        }

        if (px > clip.MaxX) {
            // Cut-off at right.
            if (CutMonotonicCubicAtX(p, clip.MaxX, t)) {
                // Cut cubic at t and keep left part of curve (since we are
                // handling left-to-right curve and cutting at off right part).
                CutCubicAt(p, tmp, t);

                // p[0] already contains tmp[0].

                p[1] = tmp[1];
                p[2] = tmp[2];
                p[3] = tmp[3];
            }
        }

        if (sx < 0) {
            // Cut-off at left.
            if (CutMonotonicCubicAtX(p, 0, t)) {
                // Cut cubic in two equal parts and keep both since we also
                // need the part on the left side of bounding box.
                CutCubicAt(p, tmp, t);

                // Since curve is going from left to right, the first one will
                // be on the left and the second one will be inside.
                F24Dot8Point c[4];

                c[0].X = ToF24Dot8(tmp[3].X);
                c[0].Y = ToF24Dot8(tmp[3].Y);

                c[1].X = ToF24Dot8(tmp[4].X);
                c[1].Y = ToF24Dot8(tmp[4].Y);

                c[2].X = ToF24Dot8(tmp[5].X);
                c[2].Y = ToF24Dot8(tmp[5].Y);

                c[3].X = ToF24Dot8(tmp[6].X);
                c[3].Y = ToF24Dot8(tmp[6].Y);

                const F24Dot8 a =
                    Clamp(ToF24Dot8(tmp[0].Y), 0, clip.FMax.Y);

                UpdateStartCovers(memory, a, Clamp(c[0].Y, 0, clip.FMax.Y));

                AddPotentiallyUncontainedCubicF24Dot8(memory, clip.FMax, c);

                return;
            }
        }

        // At this point we have entire curve inside bounding box.

        F24Dot8Point c[4];

        c[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
        c[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

        c[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
        c[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

        c[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
        c[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

        c[3].X = Clamp(ToF24Dot8(p[3].X), 0, clip.FMax.X);
        c[3].Y = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

        AddContainedCubicF24Dot8(memory, c);
    } else {
        // Vertical line.
        if (px < clip.MaxX) {
            if (px <= 0) {
                // Vertical line on the left.
                const F24Dot8 a =
                    Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);
                const F24Dot8 b =
                    Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

                UpdateStartCovers(memory, a, b);
            } else {
                // Vertical line inside clip rect.
                F24Dot8Point c[4];

                c[0].X = Clamp(ToF24Dot8(p[0].X), 0, clip.FMax.X);
                c[0].Y = Clamp(ToF24Dot8(p[0].Y), 0, clip.FMax.Y);

                c[1].X = Clamp(ToF24Dot8(p[1].X), 0, clip.FMax.X);
                c[1].Y = Clamp(ToF24Dot8(p[1].Y), 0, clip.FMax.Y);

                c[2].X = Clamp(ToF24Dot8(p[2].X), 0, clip.FMax.X);
                c[2].Y = Clamp(ToF24Dot8(p[2].Y), 0, clip.FMax.Y);

                c[3].X = Clamp(ToF24Dot8(p[3].X), 0, clip.FMax.X);
                c[3].Y = Clamp(ToF24Dot8(p[3].Y), 0, clip.FMax.Y);

                AddContainedCubicF24Dot8(memory, c);
            }
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddPotentiallyUncontainedCubicF24Dot8(
    ThreadMemory &memory, const F24Dot8Point max, const F24Dot8Point c[4]) {
    // When cubic curve is not completely within destination image (determined
    // by testing if control points are inside of it), curve is clipped and
    // only parts within destination image need to be added to the output. To
    // make curve cutting simpler, before cutting, cubic curve is monotonized.
    // This allows to only look for one root when intersecting it with
    // vertical or horizontal lines (bounding box edges). However, there is
    // one small issue with cubic curves - there are situations where cubic
    // curve is monotonic despite the fact that it has control points outside
    // of bounding box defined by the first and the last points of that curve.
    // This leads to a problem when converting curve to 24.8 format. When it
    // is done points are clamped to destination image bounds so that when
    // later inserting generated segments to segment lists, bounds check can
    // be skipped. It is quicker to make sure curve fits into destination
    // image and then just assume that any segments that come out of it will
    // automatically fit into destination image. But if curve has control
    // points out of bounds even after monotonization, clamping control points
    // to image bounds will simply result in wrong output.
    //
    // This method tries to implement the solution. It checks if there are
    // points of a given curve outside of image bounds and if there are, it
    // splits curve in half and repeats recursively until curve gets too small
    // to subdivide. This solves the problem because after each subdivision,
    // curve control point bounding box gets tighter.

    BLAZE_ASSERT(c != nullptr);

    const F24Dot8 maxx = max.X;
    const F24Dot8 maxy = max.Y;

    if (c[0].X < 0 || c[0].X > maxx || c[0].Y < 0 || c[0].Y > maxy ||
        c[1].X < 0 || c[1].X > maxx || c[1].Y < 0 || c[1].Y > maxy ||
        c[2].X < 0 || c[2].X > maxx || c[2].Y < 0 || c[2].Y > maxy ||
        c[3].X < 0 || c[3].X > maxx || c[3].Y < 0 || c[3].Y > maxy) {
        // Potentially needs splitting unless it is already too small.
        const F24Dot8 dx = F24Dot8Abs(c[0].X - c[1].X) +
                           F24Dot8Abs(c[1].X - c[2].X) +
                           F24Dot8Abs(c[2].X - c[3].X);

        const F24Dot8 dy = F24Dot8Abs(c[0].Y - c[1].Y) +
                           F24Dot8Abs(c[1].Y - c[2].Y) +
                           F24Dot8Abs(c[2].Y - c[3].Y);

        if ((dx + dy) < F24Dot8_1) {
            F24Dot8Point pc[4];

            pc[0].X = Clamp(c[0].X, 0, maxx);
            pc[0].Y = Clamp(c[0].Y, 0, maxy);
            pc[1].X = Clamp(c[1].X, 0, maxx);
            pc[1].Y = Clamp(c[1].Y, 0, maxy);
            pc[2].X = Clamp(c[2].X, 0, maxx);
            pc[2].Y = Clamp(c[2].Y, 0, maxy);
            pc[3].X = Clamp(c[3].X, 0, maxx);
            pc[3].Y = Clamp(c[3].Y, 0, maxy);

            AddContainedCubicF24Dot8(memory, pc);
        } else {
            F24Dot8Point pc[7];

            SplitCubic(pc, c);

            AddPotentiallyUncontainedCubicF24Dot8(memory, max, pc);

            AddPotentiallyUncontainedCubicF24Dot8(memory, max, pc + 3);
        }
    } else {
        AddContainedCubicF24Dot8(memory, c);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AddContainedCubicF24Dot8(
    ThreadMemory &memory, const F24Dot8Point c[4]) {
    BLAZE_ASSERT(c != nullptr);
    BLAZE_ASSERT(c[0].X >= 0);
    BLAZE_ASSERT(c[0].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(c[0].Y >= 0);
    BLAZE_ASSERT(c[0].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(c[1].X >= 0);
    BLAZE_ASSERT(c[1].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(c[1].Y >= 0);
    BLAZE_ASSERT(c[1].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(c[2].X >= 0);
    BLAZE_ASSERT(c[2].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(c[2].Y >= 0);
    BLAZE_ASSERT(c[2].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(c[3].X >= 0);
    BLAZE_ASSERT(c[3].X <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(c[3].Y >= 0);
    BLAZE_ASSERT(c[3].Y <= T::TileRowIndexToF24Dot8(mBounds.RowCount));

    if (IsCubicFlatEnough(c)) {
        AddContainedLineF24Dot8(memory, c[0], c[3]);
    } else {
        F24Dot8Point split[7];

        SplitCubic(split, c);

        AddContainedCubicF24Dot8(memory, split);

        AddContainedCubicF24Dot8(memory, split + 3);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::AppendVerticalLine(ThreadMemory &memory,
    const TileIndex rowIndex, const F24Dot8 x, const F24Dot8 y0,
    const F24Dot8 y1) {
    BLAZE_ASSERT(rowIndex >= 0);
    BLAZE_ASSERT(rowIndex < mBounds.RowCount);
    BLAZE_ASSERT(x >= 0);
    BLAZE_ASSERT(x <= T::TileColumnIndexToF24Dot8(mBounds.ColumnCount));
    BLAZE_ASSERT(y0 >= 0);
    BLAZE_ASSERT(y0 <= T::TileHF24Dot8);
    BLAZE_ASSERT(y1 >= 0);
    BLAZE_ASSERT(y1 <= T::TileHF24Dot8);

    LA(rowIndex)->AppendVerticalLine(memory, x, y0, y1);
}


template <typename T, typename L>
inline void Linearizer<T, L>::LineDownR(ThreadMemory &memory,
    const TileIndex rowIndex0, const TileIndex rowIndex1, const F24Dot8 dx,
    const F24Dot8 dy, const F24Dot8Point p0, const F24Dot8Point p1) {
    BLAZE_ASSERT(rowIndex0 < rowIndex1);
    BLAZE_ASSERT((p1.Y - p0.Y) == dy);
    BLAZE_ASSERT((p1.X - p0.X) == dx);

    const F24Dot8 fy0 = p0.Y - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = p1.Y - T::TileRowIndexToF24Dot8(rowIndex1);

    F24Dot8 p = (T::TileHF24Dot8 - fy0) * dx;
    F24Dot8 delta = p / dy;

    F24Dot8 cx = p0.X + delta;

    LA(rowIndex0)->AppendLineDownR_V(memory, p0.X, fy0, cx, T::TileHF24Dot8);

    TileIndex idy = rowIndex0 + 1;

    if (idy != rowIndex1) {
        F24Dot8 mod = (p % dy) - dy;

        p = T::TileHF24Dot8 * dx;

        F24Dot8 lift = p / dy;
        F24Dot8 rem = p % dy;

        for (; idy != rowIndex1; idy++) {
            delta = lift;
            mod += rem;

            if (mod >= 0) {
                mod -= dy;
                delta++;
            }

            const F24Dot8 nx = cx + delta;

            LA(idy)->AppendLineDownR_V(memory, cx, 0, nx, T::TileHF24Dot8);

            cx = nx;
        }
    }

    LA(rowIndex1)->AppendLineDownR_V(memory, cx, 0, p1.X, fy1);
}


template <typename T, typename L>
inline void Linearizer<T, L>::LineUpR(ThreadMemory &memory,
    const TileIndex rowIndex0, const TileIndex rowIndex1, const F24Dot8 dx,
    const F24Dot8 dy, const F24Dot8Point p0, const F24Dot8Point p1) {
    BLAZE_ASSERT(rowIndex0 > rowIndex1);
    BLAZE_ASSERT((p0.Y - p1.Y) == dy);
    BLAZE_ASSERT((p1.X - p0.X) == dx);

    const F24Dot8 fy0 = p0.Y - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = p1.Y - T::TileRowIndexToF24Dot8(rowIndex1);

    F24Dot8 p = fy0 * dx;
    F24Dot8 delta = p / dy;

    F24Dot8 cx = p0.X + delta;

    LA(rowIndex0)->AppendLineUpR_V(memory, p0.X, fy0, cx, 0);

    TileIndex idy = rowIndex0 - 1;

    if (idy != rowIndex1) {
        F24Dot8 mod = (p % dy) - dy;

        p = T::TileHF24Dot8 * dx;

        F24Dot8 lift = p / dy;
        F24Dot8 rem = p % dy;

        for (; idy != rowIndex1; idy--) {
            delta = lift;
            mod += rem;

            if (mod >= 0) {
                mod -= dy;
                delta++;
            }

            const F24Dot8 nx = cx + delta;

            LA(idy)->AppendLineUpR_V(memory, cx, T::TileHF24Dot8, nx, 0);

            cx = nx;
        }
    }

    LA(rowIndex1)->AppendLineUpR_V(memory, cx, T::TileHF24Dot8, p1.X, fy1);
}


template <typename T, typename L>
inline void Linearizer<T, L>::LineDownL(ThreadMemory &memory,
    const TileIndex rowIndex0, const TileIndex rowIndex1, const F24Dot8 dx,
    const F24Dot8 dy, const F24Dot8Point p0, const F24Dot8Point p1) {
    BLAZE_ASSERT(rowIndex0 < rowIndex1);
    BLAZE_ASSERT((p1.Y - p0.Y) == dy);
    BLAZE_ASSERT((p0.X - p1.X) == dx);

    const F24Dot8 fy0 = p0.Y - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = p1.Y - T::TileRowIndexToF24Dot8(rowIndex1);

    F24Dot8 p = (T::TileHF24Dot8 - fy0) * dx;
    F24Dot8 delta = p / dy;

    F24Dot8 cx = p0.X - delta;

    LA(rowIndex0)->AppendLineDownL_V(memory, p0.X, fy0, cx, T::TileHF24Dot8);

    TileIndex idy = rowIndex0 + 1;

    if (idy != rowIndex1) {
        F24Dot8 mod = (p % dy) - dy;

        p = T::TileHF24Dot8 * dx;

        F24Dot8 lift = p / dy;
        F24Dot8 rem = p % dy;

        for (; idy != rowIndex1; idy++) {
            delta = lift;
            mod += rem;

            if (mod >= 0) {
                mod -= dy;
                delta++;
            }

            const F24Dot8 nx = cx - delta;

            LA(idy)->AppendLineDownL_V(memory, cx, 0, nx, T::TileHF24Dot8);

            cx = nx;
        }
    }

    LA(rowIndex1)->AppendLineDownL_V(memory, cx, 0, p1.X, fy1);
}


template <typename T, typename L>
inline void Linearizer<T, L>::LineUpL(ThreadMemory &memory,
    const TileIndex rowIndex0, const TileIndex rowIndex1, const F24Dot8 dx,
    const F24Dot8 dy, const F24Dot8Point p0, const F24Dot8Point p1) {
    BLAZE_ASSERT(rowIndex0 > rowIndex1);
    BLAZE_ASSERT((p0.Y - p1.Y) == dy);
    BLAZE_ASSERT((p0.X - p1.X) == dx);

    const F24Dot8 fy0 = p0.Y - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = p1.Y - T::TileRowIndexToF24Dot8(rowIndex1);

    F24Dot8 p = fy0 * dx;
    F24Dot8 delta = p / dy;

    F24Dot8 cx = p0.X - delta;

    LA(rowIndex0)->AppendLineUpL_V(memory, p0.X, fy0, cx, 0);

    TileIndex idy = rowIndex0 - 1;

    if (idy != rowIndex1) {
        F24Dot8 mod = (p % dy) - dy;

        p = T::TileHF24Dot8 * dx;

        F24Dot8 lift = p / dy;
        F24Dot8 rem = p % dy;

        for (; idy != rowIndex1; idy--) {
            delta = lift;
            mod += rem;

            if (mod >= 0) {
                mod -= dy;
                delta++;
            }

            const F24Dot8 nx = cx - delta;

            LA(idy)->AppendLineUpL_V(memory, cx, T::TileHF24Dot8, nx, 0);

            cx = nx;
        }
    }

    LA(rowIndex1)->AppendLineUpL_V(memory, cx, T::TileHF24Dot8, p1.X, fy1);
}


template <typename T, typename L>
inline void Linearizer<T, L>::Vertical_Down(
    ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1, const F24Dot8 x) {
    BLAZE_ASSERT(y0 < y1);

    const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(y0);
    const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(y1 - 1);
    const F24Dot8 fy0 = y0 - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = y1 - T::TileRowIndexToF24Dot8(rowIndex1);

    if (rowIndex0 == rowIndex1) {
        AppendVerticalLine(memory, rowIndex0, x, fy0, fy1);
    } else {
        AppendVerticalLine(memory, rowIndex0, x, fy0, T::TileHF24Dot8);

        for (TileIndex i = rowIndex0 + 1; i < rowIndex1; i++) {
            AppendVerticalLine(memory, i, x, 0, T::TileHF24Dot8);
        }

        AppendVerticalLine(memory, rowIndex1, x, 0, fy1);
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::Vertical_Up(
    ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1, const F24Dot8 x) {
    BLAZE_ASSERT(y0 > y1);

    const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(y0 - 1);
    const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(y1);
    const F24Dot8 fy0 = y0 - T::TileRowIndexToF24Dot8(rowIndex0);
    const F24Dot8 fy1 = y1 - T::TileRowIndexToF24Dot8(rowIndex1);

    if (rowIndex0 == rowIndex1) {
        AppendVerticalLine(memory, rowIndex0, x, fy0, fy1);
    } else {
        AppendVerticalLine(memory, rowIndex0, x, fy0, 0);

        for (TileIndex i = rowIndex0 - 1; i > rowIndex1; i--) {
            AppendVerticalLine(memory, i, x, T::TileHF24Dot8, 0);
        }

        AppendVerticalLine(memory, rowIndex1, x, T::TileHF24Dot8, fy1);
    }
}


template <typename T, typename L>
inline int32_t *Linearizer<T, L>::GetStartCoversForRowAtIndex(
    ThreadMemory &memory, const int index) {
    BLAZE_ASSERT(mStartCoverTable != nullptr);
    BLAZE_ASSERT(index >= 0);
    BLAZE_ASSERT(index < mBounds.RowCount);

    int32_t *p = mStartCoverTable[index];

    if (p != nullptr) {
        return p;
    }

    p = memory.FrameMallocArrayZeroFill<int32_t>(T::TileH);

    mStartCoverTable[index] = p;

    return p;
}


template <typename T, typename L>
inline void Linearizer<T, L>::UpdateStartCovers(
    ThreadMemory &memory, const F24Dot8 y0, const F24Dot8 y1) {
    BLAZE_ASSERT(y0 >= 0);
    BLAZE_ASSERT(y0 <= T::TileRowIndexToF24Dot8(mBounds.RowCount));
    BLAZE_ASSERT(y1 >= 0);
    BLAZE_ASSERT(y1 <= T::TileRowIndexToF24Dot8(mBounds.RowCount));

    if (y0 == y1) {
        // Not contributing to mask.
        return;
    }

    if (mStartCoverTable == nullptr) {
        // Allocate pointers to row masks.
        mStartCoverTable =
            memory.FrameMallocPointersZeroFill<int32_t>(mBounds.RowCount);
    }

    if (y0 < y1) {
        // Line is going down.
        const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(y0);
        const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(y1 - 1);
        const F24Dot8 fy0 = y0 - T::TileRowIndexToF24Dot8(rowIndex0);
        const F24Dot8 fy1 = y1 - T::TileRowIndexToF24Dot8(rowIndex1);

        int32_t *cmFirst = GetStartCoversForRowAtIndex(memory, rowIndex0);

        if (rowIndex0 == rowIndex1) {
            UpdateCoverTable_Down(cmFirst, fy0, fy1);
        } else {
            UpdateCoverTable_Down(cmFirst, fy0, T::TileHF24Dot8);

            for (TileIndex i = rowIndex0 + 1; i < rowIndex1; i++) {
                UpdateStartCoversFull_Down(memory, i);
            }

            int32_t *cmLast = GetStartCoversForRowAtIndex(memory, rowIndex1);

            UpdateCoverTable_Down(cmLast, 0, fy1);
        }
    } else {
        // Line is going up.
        const TileIndex rowIndex0 = T::F24Dot8ToTileRowIndex(y0 - 1);
        const TileIndex rowIndex1 = T::F24Dot8ToTileRowIndex(y1);
        const F24Dot8 fy0 = y0 - T::TileRowIndexToF24Dot8(rowIndex0);
        const F24Dot8 fy1 = y1 - T::TileRowIndexToF24Dot8(rowIndex1);

        int32_t *cmFirst = GetStartCoversForRowAtIndex(memory, rowIndex0);

        if (rowIndex0 == rowIndex1) {
            UpdateCoverTable_Up(cmFirst, fy0, fy1);
        } else {
            UpdateCoverTable_Up(cmFirst, fy0, 0);

            for (TileIndex i = rowIndex0 - 1; i > rowIndex1; i--) {
                UpdateStartCoversFull_Up(memory, i);
            }

            int32_t *cmLast = GetStartCoversForRowAtIndex(memory, rowIndex1);

            UpdateCoverTable_Up(cmLast, T::TileHF24Dot8, fy1);
        }
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::UpdateStartCoversFull_Down(
    ThreadMemory &memory, const int index) {
    BLAZE_ASSERT(mStartCoverTable != nullptr);
    BLAZE_ASSERT(index >= 0);
    BLAZE_ASSERT(index < mBounds.RowCount);

    int32_t *p = mStartCoverTable[index];

    if (p != nullptr) {
        // Accumulate.
        T::AccumulateStartCovers(p, FullPixelCoverNegative);
    } else {
        // Store first.
        p = memory.FrameMallocArray<int32_t>(T::TileH);

        T::FillStartCovers(p, FullPixelCoverNegative);

        mStartCoverTable[index] = p;
    }
}


template <typename T, typename L>
inline void Linearizer<T, L>::UpdateStartCoversFull_Up(
    ThreadMemory &memory, const int index) {
    BLAZE_ASSERT(mStartCoverTable != nullptr);
    BLAZE_ASSERT(index >= 0);
    BLAZE_ASSERT(index < mBounds.RowCount);

    int32_t *p = mStartCoverTable[index];

    if (p != nullptr) {
        // Accumulate.
        T::AccumulateStartCovers(p, FullPixelCoverPositive);
    } else {
        // Store first.
        p = memory.FrameMallocArray<int32_t>(T::TileH);

        T::FillStartCovers(p, FullPixelCoverPositive);

        mStartCoverTable[index] = p;
    }
}


template <typename T, typename L>
inline L *Linearizer<T, L>::LA(const int verticalIndex) {
    BLAZE_ASSERT(verticalIndex >= 0);
    BLAZE_ASSERT(verticalIndex < mBounds.RowCount);

    return mLA + verticalIndex;
}

} // namespace Blaze