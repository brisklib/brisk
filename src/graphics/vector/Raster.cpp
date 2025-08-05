/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "Raster.hpp"

#include <climits>
#include <cstring>
#include <memory>
#include "v_ft_raster.h"
#include "v_ft_stroker.h"

namespace Brisk {

template <typename T>
class dyn_array {
public:
    explicit dyn_array(size_t size) : mCapacity(size), mData(std::make_unique<T[]>(mCapacity)) {}

    void reserve(size_t size) {
        if (mCapacity > size)
            return;
        mCapacity = size;
        mData     = std::make_unique<T[]>(mCapacity);
    }

    T* data() const {
        return mData.get();
    }

    dyn_array& operator=(dyn_array&&) noexcept = delete;

private:
    size_t mCapacity{ 0 };
    std::unique_ptr<T[]> mData{ nullptr };
};

struct FTOutline {
public:
    FTOutline() {
        reset();
    }

    void reset();
    void grow(size_t, size_t);
    void convert(const Path& path);
    void convert(CapStyle, JoinStyle, float, float);
    void moveTo(PointF pt);
    void lineTo(PointF pt);
    void cubicTo(PointF ctr1, PointF ctr2, const PointF end);
    void close();
    void end();

    SW_FT_Pos TO_FT_COORD(float x) {
        return SW_FT_Pos(x * 64);
    } // to freetype 26.6 coordinate.

    SW_FT_Outline ft;
    bool closed{ false };
    SW_FT_Stroker_LineCap ftCap;
    SW_FT_Stroker_LineJoin ftJoin;
    SW_FT_Fixed ftWidth;
    SW_FT_Fixed ftMiterLimit;
    dyn_array<SW_FT_Vector> mPointMemory{ 100 };
    dyn_array<char> mTagMemory{ 100 };
    dyn_array<short> mContourMemory{ 10 };
    dyn_array<char> mContourFlagMemory{ 10 };
};

void FTOutline::reset() {
    ft.n_points = ft.n_contours = 0;
    ft.flags                    = 0x0;
}

void FTOutline::grow(size_t points, size_t segments) {
    reset();
    mPointMemory.reserve(points + segments);
    mTagMemory.reserve(points + segments);
    mContourMemory.reserve(segments);
    mContourFlagMemory.reserve(segments);

    ft.points        = mPointMemory.data();
    ft.tags          = mTagMemory.data();
    ft.contours      = mContourMemory.data();
    ft.contours_flag = mContourFlagMemory.data();
}

void FTOutline::convert(const Path& path) {
    const std::vector<Path::Element>& elements = path.elements();
    const std::vector<PointF>& points          = path.points();

    grow(points.size(), path.segments());

    size_t index = 0;
    for (auto element : elements) {
        switch (element) {
        case Path::Element::MoveTo:
            moveTo(points[index]);
            index++;
            break;
        case Path::Element::LineTo:
            lineTo(points[index]);
            index++;
            break;
        case Path::Element::CubicTo:
            cubicTo(points[index], points[index + 1], points[index + 2]);
            index = index + 3;
            break;
        case Path::Element::Close:
            close();
            break;
        }
    }
    end();
}

void FTOutline::convert(CapStyle cap, JoinStyle join, float width, float miterLimit) {
    // map strokeWidth to freetype. It uses as the radius of the pen not the
    // diameter
    width        = width / 2.0f;
    // convert to freetype co-ordinate
    // IMP: stroker takes radius in 26.6 co-ordinate
    ftWidth      = SW_FT_Fixed(width * (1 << 6));
    // IMP: stroker takes meterlimit in 16.16 co-ordinate
    ftMiterLimit = SW_FT_Fixed(miterLimit * (1 << 16));

    // map to freetype capstyle
    switch (cap) {
    case CapStyle::Square:
        ftCap = SW_FT_STROKER_LINECAP_SQUARE;
        break;
    case CapStyle::Round:
        ftCap = SW_FT_STROKER_LINECAP_ROUND;
        break;
    default:
        ftCap = SW_FT_STROKER_LINECAP_BUTT;
        break;
    }
    switch (join) {
    case JoinStyle::Bevel:
        ftJoin = SW_FT_STROKER_LINEJOIN_BEVEL;
        break;
    case JoinStyle::Round:
        ftJoin = SW_FT_STROKER_LINEJOIN_ROUND;
        break;
    default:
        ftJoin = SW_FT_STROKER_LINEJOIN_MITER_FIXED;
        break;
    }
}

void FTOutline::moveTo(PointF pt) {
    assert(ft.n_points <= SHRT_MAX - 1);

    ft.points[ft.n_points].x = TO_FT_COORD(pt.x);
    ft.points[ft.n_points].y = TO_FT_COORD(pt.y);
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_ON;
    if (ft.n_points) {
        ft.contours[ft.n_contours] = ft.n_points - 1;
        ft.n_contours++;
    }
    // mark the current contour as open
    // will be updated if ther is a close tag at the end.
    ft.contours_flag[ft.n_contours] = 1;

    ft.n_points++;
}

void FTOutline::lineTo(PointF pt) {
    assert(ft.n_points <= SHRT_MAX - 1);

    ft.points[ft.n_points].x = TO_FT_COORD(pt.x);
    ft.points[ft.n_points].y = TO_FT_COORD(pt.y);
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_ON;
    ft.n_points++;
}

void FTOutline::cubicTo(PointF cp1, PointF cp2, const PointF ep) {
    assert(ft.n_points <= SHRT_MAX - 3);

    ft.points[ft.n_points].x = TO_FT_COORD(cp1.x);
    ft.points[ft.n_points].y = TO_FT_COORD(cp1.y);
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_CUBIC;
    ft.n_points++;

    ft.points[ft.n_points].x = TO_FT_COORD(cp2.x);
    ft.points[ft.n_points].y = TO_FT_COORD(cp2.y);
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_CUBIC;
    ft.n_points++;

    ft.points[ft.n_points].x = TO_FT_COORD(ep.x);
    ft.points[ft.n_points].y = TO_FT_COORD(ep.y);
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_ON;
    ft.n_points++;
}

void FTOutline::close() {
    assert(ft.n_points <= SHRT_MAX - 1);

    // mark the contour as a close path.
    ft.contours_flag[ft.n_contours] = 0;

    int index;
    if (ft.n_contours) {
        index = ft.contours[ft.n_contours - 1] + 1;
    } else {
        index = 0;
    }

    // make sure atleast 1 point exists in the segment.
    if (ft.n_points == index) {
        closed = false;
        return;
    }

    ft.points[ft.n_points].x = ft.points[index].x;
    ft.points[ft.n_points].y = ft.points[index].y;
    ft.tags[ft.n_points]     = SW_FT_CURVE_TAG_ON;
    ft.n_points++;
}

void FTOutline::end() {
    assert(ft.n_contours <= SHRT_MAX - 1);

    if (ft.n_points) {
        ft.contours[ft.n_contours] = ft.n_points - 1;
        ft.n_contours++;
    }
}

static void rleGenerationCb(int count, const SW_FT_Span* spans, void* user) {
    Rle* rle      = static_cast<Rle*>(user);
    auto* rleSpan = reinterpret_cast<const Rle::Span*>(spans);
    rle->addSpans(rleSpan, count);
}

static void bboxCb(int x, int y, int w, int h, void* user) {
    Rle* rle = static_cast<Rle*>(user);
    rle->setBoundingRect({ x, y, x + w, y + h });
}

struct Rasterizer {
    FTOutline outline;
    SW_FT_Stroker stroker;
    Rle mRle;
    float mStrokeWidth;
    float mMiterLimit;
    Rectangle mClip;
    FillRule mFillRule;
    CapStyle mCap;
    JoinStyle mJoin;
    bool mGenerateStroke;

    Rasterizer() {
        SW_FT_Stroker_New(&stroker);
    }

    ~Rasterizer() {
        SW_FT_Stroker_Done(stroker);
    }

    Rle& rle() {
        return mRle;
    }

    void update(FillRule fillRule, Rectangle clip) {
        mRle.reset();
        mFillRule       = fillRule;
        mClip           = clip;
        mGenerateStroke = false;
    }

    void update(CapStyle cap, JoinStyle join, float width, float miterLimit, Rectangle clip) {
        mRle.reset();
        mCap            = cap;
        mJoin           = join;
        mStrokeWidth    = width;
        mMiterLimit     = miterLimit;
        mClip           = clip;
        mGenerateStroke = true;
    }

    void render(const Path& path) {
        if (path.points().size() > SHRT_MAX || path.points().size() + path.segments() > SHRT_MAX) {
            return;
        }

        if (mGenerateStroke) { // Stroke Task
            outline.convert(path);
            outline.convert(mCap, mJoin, mStrokeWidth, mMiterLimit);

            uint32_t points, contors;

            SW_FT_Stroker_Set(stroker, outline.ftWidth, outline.ftCap, outline.ftJoin, outline.ftMiterLimit);
            SW_FT_Stroker_ParseOutline(stroker, &outline.ft);
            SW_FT_Stroker_GetCounts(stroker, &points, &contors);

            outline.grow(points, contors);

            SW_FT_Stroker_Export(stroker, &outline.ft);

        } else { // Fill Task
            outline.convert(path);
            int fillRuleFlag = SW_FT_OUTLINE_NONE;
            switch (mFillRule) {
            case FillRule::EvenOdd:
                fillRuleFlag = SW_FT_OUTLINE_EVEN_ODD_FILL;
                break;
            default:
                fillRuleFlag = SW_FT_OUTLINE_NONE;
                break;
            }
            outline.ft.flags = fillRuleFlag;
        }

        SW_FT_Raster_Params params;

        mRle.reset();

        params.flags      = SW_FT_RASTER_FLAG_DIRECT | SW_FT_RASTER_FLAG_AA;
        params.gray_spans = &rleGenerationCb;
        params.bbox_cb    = &bboxCb;
        params.user       = &mRle;
        params.source     = &outline.ft;

        if (!mClip.empty()) {
            params.flags |= SW_FT_RASTER_FLAG_CLIP;

            params.clip_box.xMin = mClip.x1;
            params.clip_box.yMin = mClip.y1;
            params.clip_box.xMax = mClip.x2;
            params.clip_box.yMax = mClip.y2;
        }
        // compute rle
        sw_ft_grays_raster.raster_render(nullptr, &params);
    }
};

Rle rasterize(const Path& path, FillRule fillRule, Rectangle clip) {
    if (path.empty()) {
        return {};
    }
    Rasterizer rasterizer;
    rasterizer.update(fillRule, clip);
    rasterizer.render(path);
    return std::move(rasterizer.rle());
}

Rle rasterize(const Path& path, CapStyle cap, JoinStyle join, float width, float miterLimit, Rectangle clip) {
    if (path.empty() || vIsZero(width)) {
        return {};
    }
    Rasterizer rasterizer;
    rasterizer.update(cap, join, width, miterLimit, clip);
    rasterizer.render(path);
    return std::move(rasterizer.rle());
}

} // namespace Brisk
