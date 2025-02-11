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
#include <brisk/core/Utilities.hpp>

#include "turbojpeg.h"

namespace Brisk {

static int toJPGSS(ColorSubsampling ss) {
    return staticMap(ss, ColorSubsampling::S444, TJSAMP_444, ColorSubsampling::S420, TJSAMP_420,
                     ColorSubsampling::S422, TJSAMP_422, TJSAMP_420);
}

static int toJPGFormat(PixelFormat fmt) {
    return staticMap(fmt,                          //
                     PixelFormat::RGB, TJPF_RGB,   //
                     PixelFormat::RGBA, TJPF_RGBA, //
                     PixelFormat::ARGB, TJPF_ARGB, //
                     PixelFormat::BGR, TJPF_BGR,   //
                     PixelFormat::BGRA, TJPF_BGRA, //
                     PixelFormat::ABGR, TJPF_ABGR, //
                     TJPF_GRAY);
}

Bytes jpegEncode(RC<Image> image, optional<int> quality, optional<ColorSubsampling> ss) {
    if (image->pixelType() != PixelType::U8Gamma) {
        throwException(EImageError("JPEG codec doesn't support encoding {} format", image->format()));
    }
    // All jpeg pixel formats are supported
    tjhandle jpeg = tjInitCompress();
    SCOPE_EXIT {
        tjDestroy(jpeg);
    };

    auto r = image->mapRead<ImageFormat::Unknown_U8Gamma>();

    Bytes result(tjBufSize(r.width(), r.height(), toJPGSS(ss.value_or(defaultColorSubsampling))));
    uint8_t* resultData      = (uint8_t*)result.data();
    unsigned long resultSize = result.size();

    if (tjCompress2(jpeg, r.data(), r.width(), r.byteStride(), r.height(), toJPGFormat(image->pixelFormat()),
                    &resultData, &resultSize,
                    image->pixelFormat() == PixelFormat::Greyscale
                        ? TJSAMP_GRAY
                        : toJPGSS(ss.value_or(defaultColorSubsampling)),
                    quality.value_or(defaultImageQuality),
                    TJFLAG_FASTDCT | TJFLAG_NOREALLOC | TJFLAG_PROGRESSIVE) != 0) {
        return {};
    }
    result.resize(resultSize);
    return result;
}

expected<RC<Image>, ImageIOError> jpegDecode(BytesView bytes, ImageFormat format) {
    if (toPixelType(format) != PixelType::U8Gamma && toPixelType(format) != PixelType::Unknown) {
        throwException(EImageError("JPEG codec doesn't support decoding to {} format", format));
    }
    tjhandle jpeg = tjInitDecompress();
    SCOPE_EXIT {
        tjDestroy(jpeg);
    };
    PixelFormat pixelFormat = toPixelFormat(format);

    Size size;
    int jpegSS;

    if (tjDecompressHeader2(jpeg, reinterpret_cast<uint8_t*>(const_cast<std::byte*>(bytes.data())),
                            bytes.size(), &size.width, &size.height, &jpegSS) != 0) {
        return unexpected(ImageIOError::CodecError);
    }
    if (pixelFormat == PixelFormat::Unknown) {
        pixelFormat = jpegSS == TJSAMP_GRAY ? PixelFormat::Greyscale : PixelFormat::RGB;
    }

    RC<Image> image = rcnew Image(size, imageFormat(PixelType::U8Gamma, pixelFormat));

    auto w          = image->mapWrite<ImageFormat::Unknown_U8Gamma>();

    if (tjDecompress2(jpeg, reinterpret_cast<uint8_t*>(const_cast<std::byte*>(bytes.data())), bytes.size(),
                      w.data(), w.width(), w.byteStride(), w.height(), toJPGFormat(pixelFormat),
                      TJFLAG_ACCURATEDCT) != 0) {
        return unexpected(ImageIOError::CodecError);
    }

    return image;
}

} // namespace Brisk
