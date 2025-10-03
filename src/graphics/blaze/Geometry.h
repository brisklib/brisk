
#pragma once


#include "Utils.h"
#include "Matrix.h"


namespace Blaze {

/**
 * One renderable item.
 */
struct Geometry final {

    /**
     * Constructs geometry.
     *
     * @param pathBounds Bounding box of a path transformed by transformation
     * matrix. This rectangle potentially can exceed bounds of destination
     * image.
     *
     * @param tags Pointer to tags. Must not be nullptr.
     *
     * @param points Pointer to points. Must not be nullptr.
     *
     * @param tm Transformation matrix.
     *
     * @param tagCount A number of tags. Must be greater than 0.
     *
     * @param color RGBA color, 8 bits per channel, color components
     * premultiplied by alpha.
     *
     * @param rule Fill rule to use.
     */
    Geometry(const IntRect &pathBounds, const PathTag *tags,
        const FloatPoint *points, const Matrix &tm, const int tagCount,
        const int pointCount, const uint32_t color, const FillRule rule)
        : PathBounds(pathBounds), Tags(tags), Points(points), TM(tm),
          TagCount(tagCount), PointCount(pointCount), Color(color), Rule(rule) {
        BLAZE_ASSERT(tags != nullptr);
        BLAZE_ASSERT(points != nullptr);
        BLAZE_ASSERT(tagCount > 0);
        BLAZE_ASSERT(pointCount > 0);
    }


    const IntRect PathBounds;
    const PathTag *Tags = nullptr;
    const FloatPoint *Points = nullptr;
    const Matrix TM;
    const int TagCount = 0;
    const int PointCount = 0;
    const uint32_t Color = 0;
    const FillRule Rule = FillRule::NonZero;
};

} // namespace Blaze