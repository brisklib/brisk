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
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/core/Utilities.hpp>
#define STBI_WRITE_NO_STDIO 1
#define STB_IMAGE_WRITE_STATIC 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1

#define STBI_NO_STDIO 1
#define STB_IMAGE_STATIC 1
#define STB_IMAGE_IMPLEMENTATION 1
#define STBI_NO_HDR 1

BRISK_GNU_ATTR_PRAGMA(GCC diagnostic push)
BRISK_GNU_ATTR_PRAGMA(GCC diagnostic ignored "-Wunused-function")
#include <stb_image.h>
#include <stb_image_write.h>
BRISK_GNU_ATTR_PRAGMA(GCC diagnostic pop)

#include <brisk/core/Stream.hpp>

namespace Brisk {

static void stbi_write(void* context, void* data, int size) {
    MemoryStream& result = *reinterpret_cast<MemoryStream*>(context);
    std::ignore          = result.write((const std::byte*)data, size);
}

Bytes bmpEncode(RC<Image> image) {
    if (image->pixelType() != PixelType::U8Gamma) {
        throwException(EImageError("BMP codec doesn't support encoding {} format", image->format()));
    }
    // All pixel formats are supported
    auto r   = image->mapRead();
    int comp = pixelComponents(image->pixelFormat());
    MemoryStream strm;
    if (r.byteStride() == r.width() * comp) {
        stbi_write_bmp_to_func(&stbi_write, &strm, image->width(), image->height(), comp, r.data());
    } else {
        Bytes tmp(r.memorySize());
        r.writeTo(tmp);
        stbi_write_bmp_to_func(&stbi_write, &strm, image->width(), image->height(), comp, tmp.data());
    }
    return std::move(strm.data());
}

struct stbi_delete {
    void operator()(stbi_uc* ptr) {
        stbi_image_free(ptr);
    }
};

static expected<RC<Image>, ImageIOError> stbiDecode(BytesView bytes, ImageFormat format,
                                                    bool premultiplyAlpha) {
    if (toPixelType(format) != PixelType::U8Gamma && toPixelType(format) != PixelType::Unknown) {
        throwException(EImageError("BMP codec doesn't support decoding to {} format", format));
    }
    PixelFormat pixelFormat = toPixelFormat(format);
    int width, height, comp;
    std::unique_ptr<stbi_uc, stbi_delete> mem(
        stbi_load_from_memory((const uint8_t*)bytes.data(), bytes.size(), &width, &height, &comp,
                              pixelFormat == PixelFormat::Unknown ? 0 : pixelComponents(pixelFormat)));
    if (!mem)
        return unexpected(ImageIOError::CodecError);

    // comp contains the original pixel format
    if (pixelFormat != PixelFormat::Unknown)
        comp = pixelComponents(pixelFormat);
    PixelFormat fmt = componentsToFormat(comp);
    if (fmt == PixelFormat::Unknown)
        return unexpected(ImageIOError::InvalidFormat);
    RC<Image> image = rcnew Image(Size{ width, height }, imageFormat(PixelType::U8Gamma, fmt));
    auto w          = image->mapWrite();
    w.readFrom(BytesView{ (const std::byte*)mem.get(), size_t(width * height * comp) });
    if (premultiplyAlpha)
        w.premultiplyAlpha();
    return image;
}

expected<RC<Image>, ImageIOError> bmpDecode(BytesView bytes, ImageFormat format, bool premultiplyAlpha) {
    return stbiDecode(bytes, format, premultiplyAlpha);
}

} // namespace Brisk
