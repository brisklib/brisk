/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
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
#include <brisk/graphics/ImageTransform.hpp>
#include <brisk/core/Utilities.hpp>

#define STB_IMAGE_RESIZE_STATIC 1
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1

BRISK_GNU_ATTR_PRAGMA(GCC diagnostic push)
BRISK_GNU_ATTR_PRAGMA(GCC diagnostic ignored "-Wunused-function")
#if __has_include(<stb_image_resize2.h>)
#include <stb_image_resize2.h>
#define STB_RESIZE_NEW_API 1
#else
#include <stb_image_resize.h>
#endif
BRISK_GNU_ATTR_PRAGMA(GCC diagnostic pop)

namespace Brisk {

#ifdef STB_RESIZE_NEW_API
static stbir_pixel_layout pixelLayout(PixelFormat fmt) {
    return staticMap(fmt, PixelFormat::Greyscale, STBIR_1CHANNEL, //
                     PixelFormat::Alpha, STBIR_1CHANNEL,          //
                     PixelFormat::GreyscaleAlpha, STBIR_RA,       //
                     PixelFormat::RGB, STBIR_RGB,                 //
                     PixelFormat::BGR, STBIR_BGR,                 //
                     PixelFormat::RGBA, STBIR_RGBA_PM,            //
                     PixelFormat::BGRA, STBIR_BGRA_PM,            //
                     PixelFormat::ARGB, STBIR_ARGB_PM,            //
                     PixelFormat::ABGR, STBIR_ABGR_PM,            //
                     STBIR_1CHANNEL);
}

void imageResizeTo(Rc<Image> destination, Rc<Image> source, ResizingFilter filter) {
    if (source->format() != destination->format())
        throwException(EArgument("imageResizeTo: incompatible type or format: destination={} source={}",
                                 destination->format(), source->format()));
    auto r = source->mapRead();
    auto w = destination->mapWrite();

    switch (source->pixelType()) {
    case PixelType::U8Gamma:
        // Ignore the return value because all preconditions have been checked
        std::ignore = stbir_resize(reinterpret_cast<const void*>(r.data()), r.width(), r.height(),
                                   r.byteStride(), reinterpret_cast<void*>(w.data()), w.width(), w.height(),
                                   w.byteStride(), pixelLayout(source->pixelFormat()), STBIR_TYPE_UINT8_SRGB,
                                   STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter));
        break;
    case PixelType::U8:
        // Ignore the return value because all preconditions have been checked
        std::ignore = stbir_resize(reinterpret_cast<const void*>(r.data()), r.width(), r.height(),
                                   r.byteStride(), reinterpret_cast<void*>(w.data()), w.width(), w.height(),
                                   w.byteStride(), pixelLayout(source->pixelFormat()), STBIR_TYPE_UINT8,
                                   STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter));
        break;
    case PixelType::U16:
        // Ignore the return value because all preconditions have been checked
        std::ignore = stbir_resize(reinterpret_cast<const void*>(r.data()), r.width(), r.height(),
                                   r.byteStride(), reinterpret_cast<void*>(w.data()), w.width(), w.height(),
                                   w.byteStride(), pixelLayout(source->pixelFormat()), STBIR_TYPE_UINT16,
                                   STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter));
        break;
    case PixelType::F32:
        // Ignore the return value because all preconditions have been checked
        std::ignore = stbir_resize(reinterpret_cast<const void*>(r.data()), r.width(), r.height(),
                                   r.byteStride(), reinterpret_cast<void*>(w.data()), w.width(), w.height(),
                                   w.byteStride(), pixelLayout(source->pixelFormat()), STBIR_TYPE_FLOAT,
                                   STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter));
        break;
    default:
        BRISK_UNREACHABLE();
    }
}
#else
static int alphaChannelIndex(PixelFormat fmt) {
    return staticMap(fmt, PixelFormat::RGBA, 3, PixelFormat::BGRA, 3, PixelFormat::ARGB, 0, PixelFormat::ABGR,
                     0, STBIR_ALPHA_CHANNEL_NONE);
}

void imageResizeTo(Rc<Image> destination, Rc<Image> source, ResizingFilter filter) {
    if (source->format() != destination->format())
        throwException(EArgument("imageResizeTo: incompatible type or format: destination={} source={}",
                                 destination->format(), source->format()));
    auto r = source->mapRead();
    auto w = destination->mapWrite();

    switch (source->pixelType()) {
    case PixelType::U8Gamma:
        // Ignore the return value because all preconditions have been checked
        std::ignore = stbir_resize_uint8_generic(
            reinterpret_cast<const uint8_t*>(r.data()), r.width(), r.height(), r.byteStride(),
            reinterpret_cast<uint8_t*>(w.data()), w.width(), w.height(), w.byteStride(), r.components(),
            alphaChannelIndex(source->pixelFormat()), 0, STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter),
            STBIR_COLORSPACE_SRGB, nullptr);
        break;
    case PixelType::U8:
        std::ignore = stbir_resize_uint8_generic(
            reinterpret_cast<const uint8_t*>(r.data()), r.width(), r.height(), r.byteStride(),
            reinterpret_cast<uint8_t*>(w.data()), w.width(), w.height(), w.byteStride(), r.components(),
            alphaChannelIndex(source->pixelFormat()), 0, STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter),
            STBIR_COLORSPACE_LINEAR, nullptr);
        break;
    case PixelType::U16:
        std::ignore = stbir_resize_uint16_generic(
            reinterpret_cast<const uint16_t*>(r.data()), r.width(), r.height(), r.byteStride(),
            reinterpret_cast<uint16_t*>(w.data()), w.width(), w.height(), w.byteStride(), r.components(),
            alphaChannelIndex(source->pixelFormat()), 0, STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter),
            STBIR_COLORSPACE_LINEAR, nullptr);
        break;
    case PixelType::F32:
        std::ignore = stbir_resize_float_generic(
            reinterpret_cast<const float*>(r.data()), r.width(), r.height(), r.byteStride(),
            reinterpret_cast<float*>(w.data()), w.width(), w.height(), w.byteStride(), r.components(),
            alphaChannelIndex(source->pixelFormat()), 0, STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter),
            STBIR_COLORSPACE_LINEAR, nullptr);
        break;
    default:
        BRISK_UNREACHABLE();
    }
}
#endif

} // namespace Brisk
