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
#include "ImageRenderTarget.hpp"
#include "ImageBackend.hpp"

namespace Brisk {

constexpr static PixelFormat format = PixelFormat::RGBA;

ImageRenderTargetD3D11::ImageRenderTargetD3D11(RC<RenderDeviceD3D11> device, Size frameSize, PixelType type,
                                               DepthStencilType depthStencil, int samples)
    : m_device(std::move(device)), m_frameSize(frameSize), m_type(type), m_depthStencilType(depthStencil),
      m_samples(samples) {

    if (!updateImage()) {
        return;
    }
}

ImageRenderTargetD3D11::~ImageRenderTargetD3D11() = default;

bool ImageRenderTargetD3D11::updateImage() {
    m_image                    = rcnew Image(m_frameSize, imageFormat(m_type, format));
    ImageBackendD3D11* backend = getOrCreateBackend(m_device, m_image, false, true);
    m_backBuffer.colorBuffer   = backend->m_texture;
    if (!m_device->updateBackBuffer(m_backBuffer, m_type, m_depthStencilType, m_samples)) {
        return false;
    }
    return true;
}

Size ImageRenderTargetD3D11::size() const {
    return m_frameSize;
}

void ImageRenderTargetD3D11::setSize(Size newSize) {
    if (newSize != m_frameSize) {
        m_frameSize = newSize;
        if (!updateImage()) {
            return;
        }
    }
}

RC<Image> ImageRenderTargetD3D11::image() const {
    return m_image;
}
} // namespace Brisk
