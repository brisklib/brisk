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
#pragma once

#include "RenderDevice.hpp"

namespace Brisk {

class WindowRenderTargetWebGpu final : public WindowRenderTarget {
public:
    status<GpuError> resizeBackbuffer(Size size) final;
    Size size() const final;
    expected<PresentStatus, GpuError> present() final;
    int vsyncInterval() const final;
    void setVSyncInterval(int interval) final;

    bool valid() const noexcept final {
        return m_device && m_device->valid() && static_cast<bool>(m_surface);
    }

    WindowRenderTargetWebGpu(Rc<RenderDeviceWebGpu> device, const NativeWindow* window,
                             PixelType type                = PixelType::U8Gamma,
                             DepthStencilType depthStencil = DepthStencilType::None, int samples = 1);

    const BackBufferWebGpu& getBackBuffer() const;

private:
    Rc<RenderDeviceWebGpu> m_device;
    const NativeWindow* m_window;
    PixelType m_type;
    DepthStencilType m_depthStencilFmt;
    int m_samples;

    wgpu::Surface m_surface;

    mutable BackBufferWebGpu m_backBuffer;
    int m_vsyncInterval = 1;
    Size m_size{ -1, -1 };
    status<GpuError> recreateSwapChain();
    status<GpuError> createSurface(const NativeWindow* window);
};
} // namespace Brisk
