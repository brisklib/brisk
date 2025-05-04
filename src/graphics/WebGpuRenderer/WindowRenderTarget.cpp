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

    createSurface(window);

    Size framebufferSize = window->framebufferSize();
    resizeBackbuffer(framebufferSize);
}

void WindowRenderTargetWebGpu::setVSyncInterval(int interval) {
    if (interval != m_vsyncInterval) {
        m_vsyncInterval = interval;
        recreateSwapChain();
    }
}

void WindowRenderTargetWebGpu::present() {
    m_surface.Present();
    m_device->m_instance.ProcessEvents();
}

void WindowRenderTargetWebGpu::recreateSwapChain() {
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
}

void WindowRenderTargetWebGpu::resizeBackbuffer(Size size) {
    if (size != m_size) {
        m_size = size;
        recreateSwapChain();
    }
}

Size WindowRenderTargetWebGpu::size() const {
    return m_window->framebufferSize();
}

int WindowRenderTargetWebGpu::vsyncInterval() const {
    return m_vsyncInterval;
}

const BackBufferWebGpu& WindowRenderTargetWebGpu::getBackBuffer() const {
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);
    m_backBuffer.color = surfaceTexture.texture;
    m_device->updateBackBuffer(m_backBuffer, m_type, m_depthStencilFmt, m_samples);
    return m_backBuffer;
}
} // namespace Brisk
