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

void rasterize(StridedData mask, const IntRect& pathBounds, const PathTag* tags, const FloatPoint* points,
               const int tagCount, const int pointCount, const FillRule fillRule) {

    Matrix translate = Matrix::CreateTranslation(-float(pathBounds.MinX), -float(pathBounds.MinY));
    Geometry g(IntRect{ 0, 0, pathBounds.MaxX - pathBounds.MinX, pathBounds.MaxY - pathBounds.MinY }, tags,
               points, translate, tagCount, pointCount, 0xffffffffu, fillRule);

    if (!threads) {
        threads.emplace();
    }

    Rasterizer<TileDescriptor_8x8>::Rasterize(
        g, IntSize{ pathBounds.MaxX - pathBounds.MinX, pathBounds.MaxY - pathBounds.MinY }, *threads,
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
    Rectangle bounds = path.boundingBoxApprox().roundOutward();
    if (bounds.empty())
        return DenseMask{};
    DenseMask result(bounds);

    Blaze::rasterize(Blaze::StridedData{ result.line(0), uint32_t(result.stride) },
                     Blaze::IntRect{ bounds.x1, bounds.y1, bounds.x2, bounds.y2 },
                     reinterpret_cast<const Blaze::PathTag*>(path.elements().data()),
                     reinterpret_cast<const Blaze::FloatPoint*>(path.points().data()),
                     int(path.elements().size()), int(path.points().size()),
                     fillRule == FillRule::Winding ? Blaze::FillRule::NonZero : Blaze::FillRule::EvenOdd);

    return result;
}

} // namespace Brisk::Internal
