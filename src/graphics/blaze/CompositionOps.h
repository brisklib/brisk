
#pragma once


#include "Geometry.h"
#include "ImageData.h"
#include "Utils.h"

namespace Blaze {

#ifdef BLAZE_MACHINE_64
static BLAZE_FORCE_INLINE uint32_t ApplyAlpha(
    const uint32_t x, const uint32_t a) {
    const uint64_t a0 =
        (((uint64_t(x)) | ((uint64_t(x)) << 24)) & 0x00ff00ff00ff00ff) * a;
    const uint64_t a1 =
        (a0 + ((a0 >> 8) & 0xff00ff00ff00ff) + 0x80008000800080) >> 8;
    const uint64_t a2 = a1 & 0x00ff00ff00ff00ff;

    return (uint32_t(a2)) | (uint32_t(a2 >> 24));
}

#elif defined BLAZE_MACHINE_32

static BLAZE_FORCE_INLINE uint32_t ApplyAlpha(
    const uint32_t x, const uint32_t a) {
    const uint32_t a0 = (x & 0x00ff00ff) * a;
    const uint32_t a1 = (a0 + ((a0 >> 8) & 0x00ff00ff) + 0x00800080) >> 8;
    const uint32_t a2 = a1 & 0x00ff00ff;

    const uint32_t b0 = ((x >> 8) & 0x00ff00ff) * a;
    const uint32_t b1 = (b0 + ((b0 >> 8) & 0x00ff00ff) + 0x00800080);
    const uint32_t b2 = b1 & 0xff00ff00;

    return a2 | b2;
}

#else
#error I don't know about register size.
#endif


static BLAZE_FORCE_INLINE uint32_t BlendSourceOver(
    const uint32_t d, const uint32_t s) {
    return s + ApplyAlpha(d, 255 - (s >> 24));
}


static BLAZE_FORCE_INLINE void CompositeSpanSourceOver(const int pos,
    const int end, uint32_t *d, const int32_t alpha, const uint32_t color) {
    BLAZE_ASSERT(pos >= 0);
    BLAZE_ASSERT(pos < end);
    BLAZE_ASSERT(d != nullptr);
    BLAZE_ASSERT(alpha <= 255);

    // For opaque colors, use opaque span composition version.
    BLAZE_ASSERT((color >> 24) < 255);

    const int e = end;
    const uint32_t cba = ApplyAlpha(color, alpha);

    for (int x = pos; x < e; x++) {
        const uint32_t dd = d[x];

        if (dd == 0) {
            d[x] = cba;
        } else {
            d[x] = BlendSourceOver(dd, cba);
        }
    }
}


static BLAZE_FORCE_INLINE void CompositeSpanSourceOverOpaque(const int pos,
    const int end, uint32_t *d, const int32_t alpha, const uint32_t color) {
    BLAZE_ASSERT(pos >= 0);
    BLAZE_ASSERT(pos < end);
    BLAZE_ASSERT(d != nullptr);
    BLAZE_ASSERT(alpha <= 255);
    BLAZE_ASSERT((color >> 24) == 255);

    const int e = end;

    if (alpha == 255) {
        // Solid span, write only.
        for (int x = pos; x < e; x++) {
            d[x] = color;
        }
    } else {
        // Transparent span.
        const uint32_t cba = ApplyAlpha(color, alpha);

        for (int x = pos; x < e; x++) {
            const uint32_t dd = d[x];

            if (dd == 0) {
                d[x] = cba;
            } else {
                d[x] = BlendSourceOver(dd, cba);
            }
        }
    }
}

using CompositeFunc = void (*)(
    int xpos, int xend, int y, int32_t alpha, void *user, const Geometry *);


struct SpanBlender {
    constexpr explicit SpanBlender(const ImageData &image)
        : Data(image.Data), ByteStride(image.BytesPerRow) {
        BLAZE_ASSERT(Data != nullptr);
        BLAZE_ASSERT(ByteStride > 0);
        BLAZE_ASSERT(image.Width > 0);
        BLAZE_ASSERT(image.Height > 0);
    }

    void CompositeSpan(int pos, int end, int y, const int32_t alpha,
        const Geometry *geometry) const {
        CompositeSpanSourceOver(pos, end,
            reinterpret_cast<uint32_t *>(Data + y * ByteStride), alpha,
            geometry->Color);
    }

    static void CompositeImpl(int xpos, int xend, int y, int32_t alpha,
        void *user, const Geometry *geometry) {
        BLAZE_ASSERT(user != nullptr);

        SpanBlender *blender = reinterpret_cast<SpanBlender *>(user);

        blender->CompositeSpan(xpos, xend, y, alpha, geometry);
    }
    uint8_t *const Data;
    const uint32_t ByteStride;

    operator CompositeFunc() const {
        return &CompositeImpl;
    }
};

} // namespace Blaze