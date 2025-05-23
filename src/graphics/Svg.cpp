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
#include <brisk/graphics/Svg.hpp>
#include <lunasvg.h>

namespace Brisk {

constexpr static ImageFormat lunaFormat = ImageFormat::BGRA_U8Gamma;

namespace Internal {
struct SvgImpl : public lunasvg::Document {
public:
    using lunasvg::Document::Document;
};
} // namespace Internal

using Internal::SvgImpl;

SvgImage::SvgImage(std::string_view svg) {
    m_impl.reset(
        reinterpret_cast<SvgImpl*>(lunasvg::Document::loadFromData(svg.data(), svg.size()).release()));
}

SvgImage::SvgImage(BytesView svg) : SvgImage(toStringView(svg)) {}

SvgImage::~SvgImage() = default;

ImageFormat SvgImage::nativeFormat() {
    return lunaFormat;
}

void SvgImage::renderTo(const Rc<Image>& destination) const {
    if (destination->format() != lunaFormat) {
        throwException(EImageError("Image format must match SvgImage::nativeFormat()"));
    }
    auto wr = destination->mapWrite<lunaFormat>();
    lunasvg::Bitmap bmp((std::uint8_t*)wr.data(), wr.width(), wr.height(), wr.byteStride());
    m_impl->render(bmp);
}

Rc<Image> SvgImage::render(Size size, Color background, ImageFormat format) const {
    if (toPixelType(format) != PixelType::U8Gamma) {
        throwException(EImageError("Image format must be 8-bit gamma-corrected"));
    }
    lunasvg::Bitmap bmp =
        m_impl->renderToBitmap(size.width, size.height, std::bit_cast<uint32_t>(background));
    Rc<Image> image = rcnew Image(size, format);
    convertPixels(image->pixelFormat(), image->data().to<uint8_t>(), toPixelFormat(lunaFormat),
                  StridedData<const uint8_t>{
                      reinterpret_cast<const uint8_t*>(bmp.data()),
                      int32_t(bmp.stride()),
                  },
                  size);
    return image;
}

} // namespace Brisk
