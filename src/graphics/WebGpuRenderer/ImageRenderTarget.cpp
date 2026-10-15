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
#include "ImageRenderTarget.hpp"

#include "ImageBackend.hpp"

namespace Brisk {

constexpr static PixelFormat format = PixelFormat::BGRA;

ImageRenderTargetWebGpu::ImageRenderTargetWebGpu(Rc<RenderDeviceWebGpu> device, Size frameSize,
                                                 PixelType type, DepthStencilType depthStencil, int samples)
    : m_device(std::move(device)), m_frameSize(frameSize), m_type(type), m_depthStencilType(depthStencil),
      m_samples(samples) {
    ensureOnRenderThread();

    std::ignore = updateImage();
}

ImageRenderTargetWebGpu::~ImageRenderTargetWebGpu() = default;

expected<void, GpuError> ImageRenderTargetWebGpu::updateImage() {
    ensureOnRenderThread();
    m_image            = rcnew Image(m_frameSize, imageFormat(m_type, format));
    auto backendResult = getOrCreateBackend(m_device, m_image, false, true);
    if (!backendResult)
        return unexpected(backendResult.error());
    ImageBackendWebGpu* backend = *backendResult;
    m_backBuffer.color          = backend->m_texture;
    return m_device->updateBackBuffer(m_backBuffer, m_type, m_depthStencilType, m_samples);
}

Size ImageRenderTargetWebGpu::size() const {
    ensureOnRenderThread();
    return m_frameSize;
}

status<GpuError> ImageRenderTargetWebGpu::setSize(Size newSize) {
    ensureOnRenderThread();
    if (newSize.width <= 0 || newSize.height <= 0 || newSize.longestSide() >= 16384)
        return unexpected(GpuError::SizeExceedsLimit);
    m_frameSize = newSize;
    return updateImage();
}

Rc<Image> ImageRenderTargetWebGpu::image(bool reset) const {
    ensureOnRenderThread();
    Rc<Image> image = m_image;
    if (reset) {
        std::ignore = const_cast<ImageRenderTargetWebGpu*>(this)->updateImage();
    }
    return image;
}
} // namespace Brisk
