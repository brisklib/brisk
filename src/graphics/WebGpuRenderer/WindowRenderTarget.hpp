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
#pragma once

#include "RenderDevice.hpp"

namespace Brisk {

class WindowRenderTargetWebGpu final : public WindowRenderTarget {
public:
    void resizeBackbuffer(Size size) final;
    Size size() const final;
    void present() final;
    int vsyncInterval() const final;
    void setVSyncInterval(int interval) final;

    WindowRenderTargetWebGpu(Rc<RenderDeviceWebGpu> device, const OsWindow* window,
                             PixelType type                = PixelType::U8Gamma,
                             DepthStencilType depthStencil = DepthStencilType::None, int samples = 1);

    const BackBufferWebGpu& getBackBuffer() const;

private:
    Rc<RenderDeviceWebGpu> m_device;
    const OsWindow* m_window;
    PixelType m_type;
    DepthStencilType m_depthStencilFmt;
    int m_samples;

    wgpu::Surface m_surface;

    mutable BackBufferWebGpu m_backBuffer;
    int m_vsyncInterval = 1;
    Size m_size{ -1, -1 };
    void recreateSwapChain();
    void createSurface(const OsWindow* window);
};
} // namespace Brisk
