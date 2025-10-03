#include "Mask.h"
#include "Blaze.h"
#include <optional>

namespace Blaze {

namespace {

std::optional<Threads> threads;

struct StridedData {
    uint8_t* data;
    uint32_t stride;
};

void rasterize(StridedData mask, const IntRect& rasterBounds, const IntRect& pathBounds, const PathTag* tags,
               const FloatPoint* points, const int tagCount, const int pointCount, const FillRule fillRule) {

    Matrix translate = Matrix::CreateTranslation(-float(rasterBounds.MinX), -float(rasterBounds.MinY));
    IntRect translatedBounds = pathBounds;
    translatedBounds.MinX -= rasterBounds.MinX;
    translatedBounds.MinY -= rasterBounds.MinY;
    translatedBounds.MaxX -= rasterBounds.MinX;
    translatedBounds.MaxY -= rasterBounds.MinY;
    Geometry g(translatedBounds, tags, points, translate, tagCount, pointCount, 0xffffffffu, fillRule);

    if (!threads) {
        threads.emplace();
    }

    Rasterizer<TileDescriptor_8x8>::Rasterize(
        g, { rasterBounds.MaxX - rasterBounds.MinX, rasterBounds.MaxY - rasterBounds.MinY }, *threads,
        [](int xpos, int xend, int y, int32_t alpha, void* user, const Geometry* geometry) {
            const StridedData* stridedData = reinterpret_cast<const StridedData*>(user);
            uint8_t* row                   = stridedData->data + y * stridedData->stride + xpos;
            int len                        = xend - xpos;
            memset(row, alpha, len);
        },
        &mask);
    threads->ResetFrameMemory();
}
} // namespace
} // namespace Blaze

namespace Brisk::Internal {
DenseMask rasterizePath(const Path& path, FillRule fillRule, Rectangle clip) {
    Rectangle pathBounds   = path.boundingBoxApprox().roundOutward();
    Rectangle rasterBounds = pathBounds;
    if (clip != noClipRect) {
        rasterBounds = rasterBounds.intersection(clip);
    }
    rasterBounds = rasterBounds.intersection({ 0, 0, 16'384, 16'384 });
    if (rasterBounds.empty())
        return DenseMask{};
    DenseMask result(rasterBounds);

    Blaze::rasterize(
        Blaze::StridedData{ result.line(0), uint32_t(result.stride) },
        Blaze::IntRect(rasterBounds.x1, rasterBounds.y1, rasterBounds.width(), rasterBounds.height()),
        Blaze::IntRect(pathBounds.x1, pathBounds.y1, pathBounds.width(), pathBounds.height()),
        reinterpret_cast<const Blaze::PathTag*>(path.elements().data()),
        reinterpret_cast<const Blaze::FloatPoint*>(path.points().data()), int(path.elements().size()),
        int(path.points().size()),
        fillRule == FillRule::Winding ? Blaze::FillRule::NonZero : Blaze::FillRule::EvenOdd);

    return result;
}

} // namespace Brisk::Internal
