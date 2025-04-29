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

#include <webp/encode.h>
#include <webp/decode.h>

namespace Brisk {

namespace {
struct webp_deleter {
    void operator()(uint8_t* ptr) {
        WebPFree(ptr);
    }
};
} // namespace

[[nodiscard]] Bytes webpEncode(Rc<Image> image, std::optional<float> quality, bool lossless) {
    if (image->pixelType() != PixelType::U8Gamma) {
        throwException(EImageError("Webp codec doesn't support encoding {} format", image->format()));
    }

    auto rd         = image->mapRead<ImageFormat::Unknown_U8Gamma>();

    uint8_t* output = nullptr;
    Bytes result;
    size_t sz;
    if (lossless) {
        switch (image->pixelFormat()) {
        case PixelFormat::RGBA:
            sz = WebPEncodeLosslessRGBA(rd.data(), rd.width(), rd.height(), rd.byteStride(), &output);
            break;
        case PixelFormat::RGB:
            sz = WebPEncodeLosslessRGB(rd.data(), rd.width(), rd.height(), rd.byteStride(), &output);
            break;
        case PixelFormat::BGRA:
            sz = WebPEncodeLosslessBGRA(rd.data(), rd.width(), rd.height(), rd.byteStride(), &output);
            break;
        case PixelFormat::BGR:
            sz = WebPEncodeLosslessBGR(rd.data(), rd.width(), rd.height(), rd.byteStride(), &output);
            break;
        default:
            return result;
        }
    } else {
        switch (image->pixelFormat()) {
        case PixelFormat::RGBA:
            sz = WebPEncodeRGBA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                quality.value_or(defaultImageQuality), &output);
            break;
        case PixelFormat::RGB:
            sz = WebPEncodeRGB(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                               quality.value_or(defaultImageQuality), &output);
            break;
        case PixelFormat::BGRA:
            sz = WebPEncodeBGRA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                quality.value_or(defaultImageQuality), &output);
            break;
        case PixelFormat::BGR:
            sz = WebPEncodeBGR(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                               quality.value_or(defaultImageQuality), &output);
            break;
        default:
            return result;
        }
    }
    SCOPE_EXIT {
        WebPFree(output);
    };

    if (!sz)
        return result;
    result.resize(sz);
    std::memcpy(result.data(), output, sz);
    WebPFree(output);
    return result;
}

[[nodiscard]] expected<Rc<Image>, ImageIoError> webpDecode(BytesView bytes, ImageFormat format,
                                                           bool premultiplyAlpha) {
    if (toPixelType(format) != PixelType::U8Gamma && toPixelType(format) != PixelType::Unknown) {
        throwException(EImageError("Webp codec doesn't support decoding to {} format", format));
    }

    int width = 0, height = 0;
    std::unique_ptr<uint8_t[], webp_deleter> pixels;
    switch (toPixelFormat(format)) {
    case PixelFormat::RGBA:
        pixels.reset(WebPDecodeRGBA((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::RGB:
        pixels.reset(WebPDecodeRGB((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::BGRA:
        pixels.reset(WebPDecodeBGRA((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::BGR:
        pixels.reset(WebPDecodeBGR((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    default:
        return unexpected(ImageIoError::InvalidFormat);
    }
    if (!pixels)
        return unexpected(ImageIoError::InvalidFormat);
    Rc<Image> img = rcnew Image(Size{ width, height }, format);
    auto wr       = img->mapWrite();
    wr.readFrom({ (const std::byte*)pixels.get(), size_t(width * height * 4) });
    if (premultiplyAlpha)
        wr.premultiplyAlpha();
    return img;
}

} // namespace Brisk
