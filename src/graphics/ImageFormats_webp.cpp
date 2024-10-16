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

[[nodiscard]] bytes webpEncode(RC<Image> image, optional<float> quality, bool lossless) {
    auto rd         = image->mapRead();

    uint8_t* output = nullptr;
    bytes result;
    size_t sz;
    if (lossless) {
        switch (image->format()) {
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
        switch (image->format()) {
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

[[nodiscard]] expected<RC<Image>, ImageIOError> webpDecode(bytes_view bytes, PixelFormat format) {
    int width = 0, height = 0;
    std::unique_ptr<uint8_t[], webp_deleter> pixels;
    switch (format) {
    case PixelFormat::RGBA:
        pixels.reset(WebPDecodeRGBA(bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::RGB:
        pixels.reset(WebPDecodeRGB(bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::BGRA:
        pixels.reset(WebPDecodeBGRA(bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::BGR:
        pixels.reset(WebPDecodeBGR(bytes.data(), bytes.size(), &width, &height));
        break;
    default:
        return unexpected(ImageIOError::InvalidFormat);
    }
    if (!pixels)
        return unexpected(ImageIOError::InvalidFormat);
    RC<Image> img = rcnew Image(Size{ width, height }, format);
    auto wr       = img->mapWrite();
    wr.readFrom({ pixels.get(), size_t(width * height) });
    return img;
}

} // namespace Brisk
