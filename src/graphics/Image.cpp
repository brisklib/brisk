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
#include <brisk/graphics/Image.hpp>

namespace Brisk {

namespace {

template <typename T>
using cvtPixelsFn = void (*)(T* dst, const T* src, uint32_t size);

template <PixelFormat fmt>
constexpr std::nullptr_t pixFmtOrder4{};
template <PixelFormat fmt>
constexpr std::nullptr_t pixFmtOrder3{};

template <typename T, size_t N, size_t... X>
BRISK_INLINE Simd<T, N> permute(const Simd<T, N>& value, size_constants<X...> indices)
    requires(N % sizeof...(X) == 0 && std::has_single_bit(N / sizeof...(X)))
{
    if constexpr (sizeof...(X) < N) {
        return permute(value, size_constants<X..., (sizeof...(X) + X)...>{});
    } else if constexpr (sizeof...(X) == N) {
        return value.shuffle(indices);
    }
}

template <>
constexpr size_constants<0, 1, 2, 3> pixFmtOrder4<PixelFormat::RGBA>{};
template <>
constexpr size_constants<1, 2, 3, 0> pixFmtOrder4<PixelFormat::ARGB>{};
template <>
constexpr size_constants<2, 1, 0, 3> pixFmtOrder4<PixelFormat::BGRA>{};
template <>
constexpr size_constants<3, 2, 1, 0> pixFmtOrder4<PixelFormat::ABGR>{};

template <>
constexpr size_constants<0, 1, 2> pixFmtOrder3<PixelFormat::RGB>{};
template <>
constexpr size_constants<2, 1, 0> pixFmtOrder3<PixelFormat::BGR>{};

template <PixelType typ, PixelFormat dstFmt, PixelFormat srcFmt, typename T = PixelTypeOf<typ>>
void cvtPixels(T* dst, const T* src, uint32_t size) {
    if constexpr (srcFmt == dstFmt) {
        memcpy(dst, src, size * pixelComponents(dstFmt));
    } else if constexpr (pixelComponents(dstFmt) == 4 && pixelComponents(srcFmt) == 4) {
        // reorder

        constexpr size_t N = 4;

        int32_t x          = 0;
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x + N - 1 < size; x += N) {
            Simd<T, 4 * N> t = Simd<T, 4 * N>::read(src);
            t                = permute(permute(t, pixFmtOrder4<srcFmt>), pixFmtOrder4<dstFmt>);
            t.write(dst);
            src += 4 * N;
            dst += 4 * N;
        }
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x < size; ++x) {
            Simd<T, 4> t = Simd<T, 4>::read(src);
            t            = permute(permute(t, pixFmtOrder4<srcFmt>), pixFmtOrder4<dstFmt>);
            t.write(dst);
            src += 4;
            dst += 4;
        }

    } else if constexpr (pixelComponents(dstFmt) == 3 && pixelComponents(srcFmt) == 3) {

        constexpr size_t N = 4;

        int32_t x          = 0;
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x + N - 1 < size; x += N) {
            Simd<T, 3 * N> t = Simd<T, 3 * N>::read(src);
            t                = permute(permute(t, pixFmtOrder3<srcFmt>), pixFmtOrder3<dstFmt>);
            t.write(dst);
            src += 3 * N;
            dst += 3 * N;
        }
        BRISK_CLANG_PRAGMA(clang loop unroll(disable))
        for (; x < size; ++x) {
            Simd<T, 3> t = Simd<T, 3>::read(src);
            t            = permute(permute(t, pixFmtOrder3<srcFmt>), pixFmtOrder3<dstFmt>);
            t.write(dst);
            src += 3;
            dst += 3;
        }
    } else {
        Pixel<typ, dstFmt>* dest         = reinterpret_cast<Pixel<typ, dstFmt>*>(dst);
        const Pixel<typ, srcFmt>* source = reinterpret_cast<const Pixel<typ, srcFmt>*>(src);
        for (int32_t x = 0; x < size; ++x) {
            dest[x] = cvtPixel<dstFmt>(source[x]);
        }
    }
}

constexpr int numPixelFormats = std::size(pixelFormatDesc);

template <PixelType typ> //                  dst              src
const cvtPixelsFn<PixelTypeOf<typ>> cvtTable[numPixelFormats][numPixelFormats]{
    /* RGB  */ {
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::RGB>, // reorder
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::RGBA>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::ARGB>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::BGR>, // reorder
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::BGRA>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::ABGR>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::RGB, PixelFormat::Alpha>,
    },
    /* RGBA */
    {
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::RGBA>, // reorder
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::ARGB>, // reorder
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::BGRA>, // reorder
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::ABGR>, // reorder
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::RGBA, PixelFormat::Alpha>,
    },
    /* ARGB */
    {
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::RGBA>, // reorder
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::ARGB>, // reorder
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::BGRA>, // reorder
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::ABGR>, // reorder
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::ARGB, PixelFormat::Alpha>,
    },
    /* BGR  */
    {
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::RGB>, // reorder
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::RGBA>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::ARGB>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::BGR>, // reorder
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::BGRA>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::ABGR>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::BGR, PixelFormat::Alpha>,
    },
    /* BGRA */
    {
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::RGBA>, // reorder
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::ARGB>, // reorder
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::BGRA>, // reorder
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::ABGR>, // reorder
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::BGRA, PixelFormat::Alpha>,
    },
    /* ABGR */
    {
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::RGBA>, // reorder
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::ARGB>, // reorder
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::BGRA>, // reorder
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::ABGR>, // reorder
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::ABGR, PixelFormat::Alpha>,
    },
    /* GreyscaleAlpha */
    {
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::RGBA>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::ARGB>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::BGRA>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::ABGR>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::GreyscaleAlpha, PixelFormat::Alpha>,
    },
    /* Greyscale */
    {
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::RGBA>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::ARGB>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::BGRA>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::ABGR>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::Greyscale, PixelFormat::Alpha>,
    },
    /* A   */
    {
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::RGB>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::RGBA>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::ARGB>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::BGR>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::BGRA>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::ABGR>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::GreyscaleAlpha>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::Greyscale>,
        &cvtPixels<typ, PixelFormat::Alpha, PixelFormat::Alpha>,
    },
};

} // namespace

void convertPixels(PixelFormat dstFmt, StridedData<uint8_t> dst, PixelFormat srcFmt,
                   StridedData<const uint8_t> src, Size size) {
    cvtPixelsFn<uint8_t> fn = cvtTable<PixelType::U8Gamma>[+dstFmt][+srcFmt];
    for (int32_t y = 0; y < size.height; ++y) {
        fn(dst.line(y), src.line(y), size.width);
    }
}
} // namespace Brisk
