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
#include "WindowRenderTarget.hpp"

namespace Brisk {

WindowRenderTargetWebGpu::WindowRenderTargetWebGpu(Rc<RenderDeviceWebGpu> device, const NativeWindow* window,
                                                   PixelType type, DepthStencilType depthStencil, int samples)
    : m_device(std::move(device)), m_window(window), m_type(type), m_depthStencilFmt(depthStencil),
      m_samples(samples) {
    ensureOnRenderThread();

    std::ignore = createSurface(window);

    Size framebufferSize = window->framebufferSize();
    std::ignore = resizeBackbuffer(framebufferSize);
}

void WindowRenderTargetWebGpu::setVSyncInterval(int interval) {
    ensureOnRenderThread();
    if (interval != m_vsyncInterval) {
        m_vsyncInterval = interval;
        std::ignore = recreateSwapChain();
    }
}

expected<PresentStatus, GpuError> WindowRenderTargetWebGpu::present() {
    ensureOnRenderThread();
    if (!valid())
        return unexpected(GpuError::DeviceLost);
    m_surface.Present();
    m_device->m_instance.ProcessEvents();
    if (!m_device->valid())
        return unexpected(GpuError::DeviceLost);
    return PresentStatus::Presented;
}

status<GpuError> WindowRenderTargetWebGpu::recreateSwapChain() {
    ensureOnRenderThread();
    if (!m_surface || !m_device->valid())
        return unexpected(GpuError::DeviceLost);
    m_backBuffer = {};
    wgpu::SurfaceConfiguration swapChainDesc{
        .device      = m_device->m_device,
        .format      = wgpu::TextureFormat::BGRA8Unorm,
        .usage       = wgpu::TextureUsage::RenderAttachment,
        // Android requires RGBA8UNorm
        .width       = std::max(uint32_t(m_size.width), 1u),
        .height      = std::max(uint32_t(m_size.height), 1u),
        .presentMode = m_vsyncInterval == 0 ? wgpu::PresentMode::Mailbox : wgpu::PresentMode::Fifo,
    };
    m_surface.Configure(&swapChainDesc);
    return {};
}

status<GpuError> WindowRenderTargetWebGpu::resizeBackbuffer(Size size) {
    ensureOnRenderThread();
    if (size.width <= 0 || size.height <= 0 || size.longestSide() >= 16384)
        return unexpected(GpuError::SizeExceedsLimit);
    if (size != m_size) {
        m_size = size;
        return recreateSwapChain();
    }
    return {};
}

Size WindowRenderTargetWebGpu::size() const {
    ensureOnRenderThread();
    return m_window->framebufferSize();
}

int WindowRenderTargetWebGpu::vsyncInterval() const {
    ensureOnRenderThread();
    return m_vsyncInterval;
}

const BackBufferWebGpu& WindowRenderTargetWebGpu::getBackBuffer() const {
    ensureOnRenderThread();
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);
    m_backBuffer.color = surfaceTexture.texture;
    std::ignore = m_device->updateBackBuffer(m_backBuffer, m_type, m_depthStencilFmt, m_samples);
    return m_backBuffer;
}
} // namespace Brisk
