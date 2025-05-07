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
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Stream.hpp>

namespace Brisk {

std::optional<ImageCodec> guessImageCodec(BytesView bytes) {
    if (bytes.size() <= 4)
        return std::nullopt;
    FourCC cc = readFromBytes<FourCC>(bytes);
    if (cc.matches("\x42\x4D??")) {
        return ImageCodec::BMP;
    }
    if (cc.matches("\xFF\xD8??") || cc.matches("\xFF\xD9??")) {
        return ImageCodec::JPEG;
    }
    if (cc.matches("\x89\x50\x4E\x47")) {
        return ImageCodec::PNG;
    }
    if (cc.matches("RIFF")) {
        return ImageCodec::WEBP;
    }
    return std::nullopt;
}

Bytes imageEncode(ImageCodec codec, Rc<Image> image, std::optional<int> quality,
                  std::optional<ColorSubsampling> ss) {
    switch (codec) {
    case ImageCodec::BMP:
        return bmpEncode(std::move(image));
    case ImageCodec::PNG:
        return pngEncode(std::move(image));
    case ImageCodec::JPEG:
        return jpegEncode(std::move(image), quality, ss);
    case ImageCodec::WEBP:
        return webpEncode(std::move(image), static_cast<std::optional<float>>(quality));
    default:
        return {};
    }
}

expected<Rc<Image>, ImageIoError> imageDecode(ImageCodec codec, BytesView bytes, ImageFormat format,
                                              bool premultiplyAlpha) {
    switch (codec) {
    case ImageCodec::BMP:
        return bmpDecode(bytes, format, premultiplyAlpha);
    case ImageCodec::PNG:
        return pngDecode(bytes, format, premultiplyAlpha);
    case ImageCodec::JPEG:
        return jpegDecode(bytes, format);
    case ImageCodec::WEBP:
        return webpDecode(bytes, format, premultiplyAlpha);
    default:
        return {};
    }
}

expected<Rc<Image>, ImageIoError> imageDecode(BytesView bytes, ImageFormat format, bool premultiplyAlpha) {
    auto codec = guessImageCodec(bytes);
    if (!codec)
        return unexpected(ImageIoError::CodecError);
    return imageDecode(*codec, bytes, format, premultiplyAlpha);
}

} // namespace Brisk
