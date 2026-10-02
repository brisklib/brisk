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
#include <webp/decode.h>
#include <webp/encode.h>
#include <webp/mux.h>

#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/ImageFormats.hpp>

namespace Brisk {

namespace {
struct webp_deleter {
    void operator()(uint8_t* ptr) {
        WebPFree(ptr);
    }
};

struct WebpData : WebPData {

    WebpData() {
        bytes = nullptr;
        size  = 0;
    }

    ~WebpData() {
        WebPFree((void*)bytes);
    }

    WebpData(const WebpData&)            = delete;
    WebpData& operator=(const WebpData&) = delete;

    WebpData(WebpData&& other) noexcept {
        swap(other);
    }

    WebpData& operator=(WebpData&& other) noexcept {
        swap(other);
        return *this;
    }

    void swap(WebpData& other) {
        std::swap(bytes, other.bytes);
        std::swap(size, other.size);
    }

    uint8_t** mutableBytes() {
        return const_cast<uint8_t**>(&bytes);
    }

    Bytes toBytes() const {
        return Bytes(reinterpret_cast<const std::byte*>(bytes),
                     reinterpret_cast<const std::byte*>(bytes) + size);
    }
};

} // namespace

[[nodiscard]] static WebpData encodeToWebpData(Rc<Image> image, std::optional<float> quality, bool lossless) {
    if (image->pixelType() != PixelType::U8Gamma) {
        throwException(EImageError("Webp codec doesn't support encoding {} format", image->format()));
    }

    auto rd = image->mapRead<ImageFormat::Unknown_U8Gamma>();

    WebpData result{};
    if (lossless) {
        switch (image->pixelFormat()) {
        case PixelFormat::RGBA:
            result.size = WebPEncodeLosslessRGBA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                                 result.mutableBytes());
            break;
        case PixelFormat::RGB:
            result.size = WebPEncodeLosslessRGB(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                                result.mutableBytes());
            break;
        case PixelFormat::BGRA:
            result.size = WebPEncodeLosslessBGRA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                                 result.mutableBytes());
            break;
        case PixelFormat::BGR:
            result.size = WebPEncodeLosslessBGR(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                                result.mutableBytes());
            break;
        default:
            throwException(EImageError("WebP codec doesn't support encoding {} format", image->format()));
        }
    } else {
        switch (image->pixelFormat()) {
        case PixelFormat::RGBA:
            result.size = WebPEncodeRGBA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                         quality.value_or(defaultImageQuality), result.mutableBytes());
            break;
        case PixelFormat::RGB:
            result.size = WebPEncodeRGB(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                        quality.value_or(defaultImageQuality), result.mutableBytes());
            break;
        case PixelFormat::BGRA:
            result.size = WebPEncodeBGRA(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                         quality.value_or(defaultImageQuality), result.mutableBytes());
            break;
        case PixelFormat::BGR:
            result.size = WebPEncodeBGR(rd.data(), rd.width(), rd.height(), rd.byteStride(),
                                        quality.value_or(defaultImageQuality), result.mutableBytes());
            break;
        default:
            throwException(EImageError("WebP codec doesn't support encoding {} format", image->format()));
        }
    }
    if (result.size == 0)
        throwException(EImageError("WebP encoding failed"));
    return result;
}

[[nodiscard]] Bytes webpEncode(Rc<Image> image, std::optional<float> quality, bool lossless) {
    WebpData result = encodeToWebpData(std::move(image), quality, lossless);
    return result.toBytes();
}

[[nodiscard]] expected<Rc<Image>, ImageIoError> webpDecode(BytesView bytes, ImageFormat format,
                                                           bool premultiplyAlpha) {
    if (toPixelType(format) != PixelType::U8Gamma && toPixelType(format) != PixelType::Unknown) {
        return unexpected(ImageIoError::InvalidDestFormat);
    }

    int width = 0, height = 0;
    std::unique_ptr<uint8_t[], webp_deleter> pixels;
    PixelFormat pixelFormat = toPixelFormat(format);
    bool extractAlpha       = pixelFormat == PixelFormat::Alpha;
    bool extractGrayAlpha   = pixelFormat == PixelFormat::GreyscaleAlpha;
    switch (pixelFormat) {
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
    case PixelFormat::Greyscale:
        pixels.reset(WebPDecodeRGBA((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::Alpha:
    case PixelFormat::GreyscaleAlpha:
        pixels.reset(WebPDecodeRGBA((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        break;
    case PixelFormat::Unknown:
        // Auto-detect: WebP always produces RGBA
        pixels.reset(WebPDecodeRGBA((const uint8_t*)bytes.data(), bytes.size(), &width, &height));
        pixelFormat = PixelFormat::RGBA;
        break;
    default:
        return unexpected(ImageIoError::InvalidFormat);
    }
    if (!pixels)
        return unexpected(ImageIoError::InvalidFormat);
    Rc<Image> img = rcnew Image(Size{ width, height }, imageFormat(PixelType::U8Gamma, pixelFormat));
    auto wr       = img->mapWrite();
    if (pixelFormat == PixelFormat::Greyscale) {
        for (int y = 0; y < height; ++y) {
            auto dst = wr.line(y);
            for (int x = 0; x < width; ++x) {
                const uint8_t* rgba = pixels.get() + 4 * (y * width + x);
                dst[x] = std::byte{ uint8_t((77 * rgba[0] + 150 * rgba[1] + 29 * rgba[2] + 128) >> 8) };
            }
        }
    } else if (!extractAlpha && !extractGrayAlpha) {
        wr.readFrom({ (const std::byte*)pixels.get(),
                      size_t(width * height * pixelComponents(pixelFormat)) });
    } else {
        auto src = pixels.get();
        for (int y = 0; y < height; ++y) {
            auto dst = wr.line(y);
            for (int x = 0; x < width; ++x) {
                const uint8_t* rgba = src + 4 * (y * width + x);
                if (extractAlpha) {
                    dst[x] = std::byte{ rgba[3] };
                } else {
                    dst[2 * x]     = std::byte{ uint8_t((77 * rgba[0] + 150 * rgba[1] + 29 * rgba[2] + 128) >> 8) };
                    dst[2 * x + 1] = std::byte{ rgba[3] };
                }
            }
        }
    }
    if (premultiplyAlpha)
        wr.premultiplyAlpha();
    return img;
}

struct WebpAnimationEncoder::Private {
    WebPMux* mux;
};

WebpAnimationEncoder::WebpAnimationEncoder(std::optional<float> quality, bool lossless)
    : m_quality(quality), m_lossless(lossless), m_priv(new Private{}) {
    m_priv->mux = WebPMuxNew();
    if (!m_priv->mux)
        throwException(EImageError("Failed to create WebP animation mux"));
}

WebpAnimationEncoder::~WebpAnimationEncoder() {
    WebPMuxDelete(m_priv->mux);
}

void WebpAnimationEncoder::addFrame(Rc<Image> image, std::chrono::milliseconds duration) {
    if (duration.count() < 0 || duration.count() > std::numeric_limits<int>::max())
        throwException(EArgument("WebP animation frame duration is out of range"));
    WebpData result = encodeToWebpData(std::move(image), m_quality, m_lossless);
    if (result.size == 0) {
        m_error = true;
        return;
    }

    WebPMuxFrameInfo frame{};
    frame.id        = WEBP_CHUNK_ANMF;
    frame.duration  = duration.count();
    frame.bitstream = result;

    if (WebPMuxError err = WebPMuxPushFrame(m_priv->mux, &frame, /* copy data */ 1); err != WEBP_MUX_OK)
        throwException(EImageError("Failed to add WebP animation frame: {}", int(err)));
}

Bytes WebpAnimationEncoder::encode(Color backgroundColor, int repeats) {
    if (m_error)
        return {};
    if (repeats < 0)
        throwException(EArgument("WebP animation repeat count cannot be negative"));
    // Set animation parameters
    WebPMuxAnimParams anim_params = {
        std::bit_cast<uint32_t>(backgroundColor.v.shuffle(/* ARGB*/ size_constants<3, 0, 1, 2>{})),
        repeats,
    };
    if (WebPMuxError err = WebPMuxSetAnimationParams(m_priv->mux, &anim_params); err != WEBP_MUX_OK)
        throwException(EImageError("Failed to set WebP animation parameters: {}", int(err)));
    WebpData output;
    if (WebPMuxError err = WebPMuxAssemble(m_priv->mux, &output); err != WEBP_MUX_OK)
        throwException(EImageError("Failed to assemble WebP animation: {}", int(err)));
    return output.toBytes();
}

} // namespace Brisk
