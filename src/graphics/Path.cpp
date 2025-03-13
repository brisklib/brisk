/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <vpath.h>
#include <vrle.h>
#include <vraster.h>

#include <brisk/graphics/Path.hpp>
#include "vdasher.h"
#include <brisk/graphics/Image.hpp>

namespace Brisk {
static_assert(sizeof(VPath) == sizeof(void*));
static_assert(sizeof(VPoint) == sizeof(Point));
static_assert(sizeof(VPointF) == sizeof(PointF));
static_assert(sizeof(VRect) == sizeof(Rectangle));
static_assert(sizeof(VRectF) == sizeof(RectangleF));

[[maybe_unused]] static VPath* v(Path* this_) {
    return reinterpret_cast<VPath*>(this_);
}

[[maybe_unused]] static const VPath* v(const Path* this_) {
    return reinterpret_cast<const VPath*>(this_);
}

[[maybe_unused]] static VPointF v(PointF pt) {
    return { pt.x, pt.y };
}

[[maybe_unused]] static VRectF v(RectangleF rc) {
    return { rc.x1, rc.y1, rc.width(), rc.height() };
}

[[maybe_unused]] static VRect v(Rectangle rc) {
    return { rc.x1, rc.y1, rc.width(), rc.height() };
}

[[maybe_unused]] static VPath::Direction v(Path::Direction d) {
    return static_cast<VPath::Direction>(static_cast<std::underlying_type_t<Path::Direction>>(d));
}

[[maybe_unused]] static VPath::Element v(Path::Element d) {
    return static_cast<VPath::Element>(static_cast<std::underlying_type_t<Path::Element>>(d));
}

[[maybe_unused]] static ::FillRule v(FillRule d) {
    return static_cast<::FillRule>(static_cast<std::underlying_type_t<FillRule>>(d));
}

[[maybe_unused]] static ::JoinStyle v(JoinStyle d) {
    return static_cast<::JoinStyle>(static_cast<std::underlying_type_t<JoinStyle>>(d));
}

[[maybe_unused]] static ::CapStyle v(CapStyle d) {
    return static_cast<::CapStyle>(static_cast<std::underlying_type_t<CapStyle>>(d));
}

[[maybe_unused]] static PointF w(VPointF pt) {
    return { pt.x(), pt.y() };
}

[[maybe_unused]] static Size w(VSize sz) {
    return { sz.width(), sz.height() };
}

Path::Path() {
    new (v(this)) VPath();
}

Path::~Path() {
    v(this)->~VPath();
}

Path::Path(Path&& other) {
    new (v(this)) VPath(static_cast<VPath&&>(*v(&other)));
}

Path::Path(const Path& other) {
    new (v(this)) VPath(static_cast<const VPath&>(*v(&other)));
}

Path& Path::operator=(Path&& other) {
    v(this)->operator=(static_cast<VPath&&>(*v(&other)));
    return *this;
}

Path& Path::operator=(const Path& other) {
    v(this)->operator=(static_cast<const VPath&>(*v(&other)));
    return *this;
}

bool Path::empty() const {
    return v(this)->empty();
}

void Path::moveTo(PointF p) {
    return v(this)->moveTo(v(p));
}

void Path::moveTo(float x, float y) {
    return v(this)->moveTo(x, y);
}

void Path::lineTo(PointF p) {
    return v(this)->lineTo(v(p));
}

void Path::lineTo(float x, float y) {
    return v(this)->lineTo(x, y);
}

void Path::cubicTo(PointF c1, PointF c2, PointF e) {
    return v(this)->cubicTo(v(c1), v(c2), v(e));
}

void Path::cubicTo(float c1x, float c1y, float c2x, float c2y, float ex, float ey) {
    return v(this)->cubicTo(c1x, c1y, c2x, c2y, ex, ey);
}

void Path::quadraticTo(PointF c, PointF e) {
    PointF s                  = v(this)->points().empty() ? PointF{ 0.f, 0.f } : w(v(this)->points().back());
    constexpr float twoThirds = 0.66667f;
    PointF c1                 = s + twoThirds * (c - s);
    PointF c2                 = s + twoThirds * (c - e);
    return v(this)->cubicTo(v(c1), v(c2), v(e));
}

void Path::quadraticTo(float c1x, float c1y, float ex, float ey) {
    return quadraticTo({ c1x, c1y }, { ex, ey });
}

void Path::arcTo(RectangleF rect, float startAngle, float sweepLength, bool forceMoveTo) {
    return v(this)->arcTo(v(rect), startAngle, sweepLength, forceMoveTo);
}

void Path::close() {
    v(this)->close();
}

bool Path::isClosed() const {
    int closeNum   = 0;
    const VPath* r = v(this);
    for (VPath::Element el : r->elements()) {
        closeNum += el == VPath::Element::Close;
    }
    return closeNum == r->segments();
}

void Path::reset() {
    v(this)->reset();
}

void Path::addCircle(float cx, float cy, float radius, Direction dir) {
    v(this)->addCircle(cx, cy, radius, v(dir));
}

void Path::addEllipse(RectangleF rect, Direction dir) {
    v(this)->addOval(v(rect), v(dir));
}

void Path::addRoundRect(RectangleF rect, float rx, float ry, bool squircle, Direction dir) {
    v(this)->addRoundRect(v(rect), rx, ry, squircle, v(dir));
}

void Path::addRoundRect(RectangleF rect, float roundness, bool squircle, Direction dir) {
    v(this)->addRoundRect(v(rect), roundness, squircle, v(dir));
}

void Path::addRoundRect(RectangleF rect, CornersF r, bool squircle, Direction dir) {
    v(this)->addRoundRect(v(rect), std::bit_cast<std::array<float, 4>>(r),
                          std::bit_cast<std::array<float, 4>>(r), squircle, v(dir));
}

void Path::addRect(RectangleF rect, Direction dir) {
    v(this)->addRect(v(rect), v(dir));
}

void Path::addPolystar(float points, float innerRadius, float outerRadius, float innerRoundness,
                       float outerRoundness, float startAngle, float cx, float cy, Direction dir) {
    v(this)->addPolystar(points, innerRadius, outerRadius, innerRoundness, outerRoundness, startAngle, cx, cy,
                         v(dir));
}

void Path::addPolygon(float points, float radius, float roundness, float startAngle, float cx, float cy,
                      Direction dir) {
    v(this)->addPolygon(points, radius, roundness, startAngle, cx, cy, v(dir));
}

void Path::addPath(const Path& path) {
    v(this)->addPath(*v(&path));
}

void Path::addPath(const Path& path, const Matrix& m) {
    size_t numPoints = v(this)->points().size();
    v(this)->addPath(*v(&path));
    auto& points = v(this)->writablePoints();
    if (points.size() > numPoints) {
        m.transform(std::span<PointF>{ reinterpret_cast<PointF*>(points.data()) + numPoints,
                                       points.size() - numPoints });
    }
}

void Path::transform(const Matrix& m) {
    if (m.isIdentity())
        return;
    auto& points = v(this)->writablePoints();
    m.transform(std::span<PointF>{ reinterpret_cast<PointF*>(points.data()), points.size() });
}

Path Path::clone() const {
    VPath c;
    c.clone(*v(this));
    return *reinterpret_cast<Path*>(&c);
}

Path Path::transformed(const Matrix& m) const& {
    Path copy = clone();
    copy.transform(m);
    return copy;
}

Path Path::transformed(const Matrix& m) && {
    transform(m);
    return std::move(*this);
}

Path Path::dashed(std::span<const float> pattern, float offset) const {
    VDasher dasher(pattern.data(), pattern.size());
    Path result;
    *v(&result) = dasher.dashed(*v(this));
    return result;
}

float Path::length() const {
    return v(this)->length();
}

RectangleF Path::boundingBoxApprox() const {
    RectangleF result{ HUGE_VALF, HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    for (auto p : v(this)->points()) {
        result.x1 = std::min(result.x1, p.x());
        result.y1 = std::min(result.y1, p.y());
        result.x2 = std::max(result.x2, p.x());
        result.y2 = std::max(result.y2, p.y());
    }
    return result;
}

std::optional<std::tuple<RectangleF, float>> Path::asRoundRectangle() const {
    if (auto rect = asRectangle()) {
        return std::make_tuple(*rect, 0.f);
    }
    auto r    = v(this);
    auto& pts = r->points();
    if (r->segments() != 1 || pts.size() != 17 || r->elements().size() != 10 ||
        !fuzzyCompare(pts[16], pts[0]))
        return std::nullopt;
    if (r->elements()[0] != VPath::Element::MoveTo     //
        || r->elements()[1] != VPath::Element::LineTo  //
        || r->elements()[2] != VPath::Element::CubicTo //
        || r->elements()[3] != VPath::Element::LineTo  //
        || r->elements()[4] != VPath::Element::CubicTo //
        || r->elements()[5] != VPath::Element::LineTo  //
        || r->elements()[6] != VPath::Element::CubicTo //
        || r->elements()[7] != VPath::Element::LineTo  //
        || r->elements()[8] != VPath::Element::CubicTo //
        || r->elements()[9] != VPath::Element::Close)
        return std::nullopt;
    RectangleF rect{
        PointF(pts[8].x(), pts[12].y()),
        PointF(pts[0].x(), pts[4].y()),
    };
    if (rect.width() < 0)
        return std::nullopt;
    float rx = pts[5].x() - pts[8].x();
    float ry = pts[4].y() - pts[1].y();
    if (rx != ry)
        return std::nullopt;
    VPath path;
    path.addRoundRect(v(rect), rx);
    for (size_t i = 0; i < 17; ++i) {
        if (!fuzzyCompare(path.points()[i], r->points()[i]))
            return std::nullopt;
    }
    return std::make_tuple(rect, rx);
}

std::optional<RectangleF> Path::asRectangle() const {
    auto r    = v(this);
    auto& pts = r->points();
    if (r->segments() != 1 || r->elements().size() != 6 || pts.size() != 5 || !fuzzyCompare(pts[4], pts[0]))
        return std::nullopt;
    if (r->elements()[0] != VPath::Element::MoveTo || r->elements()[1] != VPath::Element::LineTo ||
        r->elements()[2] != VPath::Element::LineTo || r->elements()[3] != VPath::Element::LineTo ||
        r->elements()[4] != VPath::Element::LineTo || r->elements()[5] != VPath::Element::Close)
        return std::nullopt;
    if (!(vCompare(pts[1].x(), pts[0].x()) || vCompare(pts[3].x(), pts[2].x()) ||
          vCompare(pts[3].y(), pts[0].y()) || vCompare(pts[2].y(), pts[1].y())))
        return std::nullopt;
    return RectangleF{
        std::min(pts[0].x(), pts[3].x()),
        std::min(pts[0].y(), pts[1].y()),
        std::max(pts[0].x(), pts[3].x()),
        std::max(pts[0].y(), pts[1].y()),
    };
}

std::optional<std::array<PointF, 2>> Path::asLine() const {
    auto r    = v(this);
    auto& pts = r->points();
    if (r->segments() != 1 || r->elements().size() != 2 || r->points().size() != 2)
        return std::nullopt;
    return std::array<PointF, 2>{
        w(r->points()[0]),
        w(r->points()[1]),
    };
}

BRISK_INLINE static void blendRow(PixelGreyscale8* dst, uint8_t src, uint32_t len) {
    for (uint32_t j = 0; j < len; ++j) {
        dst->grey = dst->grey + (src * (255 - dst->grey) >> 8);
        ++dst;
    }
}

static RC<Image> rleToMask(VRle rle, VRect bounds) {
    if (rle.empty())
        return nullptr;
    RC<Image> bitmap = rcnew Image(w(bounds.size()), ImageFormat::Greyscale_U8Gamma);
    auto w           = bitmap->mapWrite<ImageFormat::Greyscale_U8Gamma>();
    w.forPixels([](int32_t, int32_t, auto& pix) {
        pix = { 0 };
    });
    rle.intersect(
        VRect(0, 0, bounds.width(), bounds.height()),
        [](size_t count, const VRle::Span* spans, void* userData) {
            auto& w    = *static_cast<ImageAccess<ImageFormat::Greyscale_U8Gamma, AccessMode::W>*>(userData);
            int16_t yy = INT16_MIN;
            PixelGreyscale8* line = nullptr;
            for (size_t i = 0; i < count; ++i) {
                if (spans[i].y != yy) [[unlikely]] {
                    line = w.line(spans[i].y);
                }
                PixelGreyscale8* row = line + spans[i].x;
                if (row)
                    blendRow(row, spans[i].coverage, spans[i].len);
            }
        },
        &w);
    return bitmap;
}

static std::tuple<RC<Image>, Rectangle> rasterizeToImage(Path path, const FillOrStrokeParams& params,
                                                         Rectangle clip) {
    VRasterizer rasterizer;
    if (const FillParams* fill = get_if<FillParams>(&params)) {
        rasterizer.rasterize(*v(&path), v(fill->fillRule), clip == noClipRect ? VRect{} : v(clip));
    } else if (const StrokeParams* stroke = get_if<StrokeParams>(&params)) {
        rasterizer.rasterize(*v(&path), v(stroke->capStyle), v(stroke->joinStyle), stroke->strokeWidth,
                             stroke->miterLimit, clip == noClipRect ? VRect{} : v(clip));
    }
    VRle rle     = rasterizer.rle();
    VRect bounds = rle.boundingRect();
    rle.translate(VPoint{ -bounds.x(), -bounds.y() });
    return { rleToMask(rle, bounds), Rectangle{ bounds.x(), bounds.y(), bounds.right(), bounds.bottom() } };
}

RasterizedPath Internal::rasterizePath(Path path, const FillOrStrokeParams& params, Rectangle clipRect) {
    std::tuple<RC<Image>, Rectangle> imageAndRect = rasterizeToImage(path, params, clipRect);
    if (!std::get<0>(imageAndRect)) {
        return RasterizedPath{ nullptr, {} };
    }
    auto r = std::get<0>(imageAndRect)->mapRead();
    RasterizedPath result;
    result.sprite = makeSprite(r.size());
    r.writeTo(result.sprite->bytes());
    result.bounds = std::get<1>(imageAndRect);

    return result;
}

} // namespace Brisk
