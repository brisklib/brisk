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
#define BRISK_ALLOW_OS_HEADERS 1
#include "WindowRenderTarget.hpp"
#include <brisk/graphics/OSWindowHandle.hpp>

namespace Brisk {

WindowRenderTargetD3D11::WindowRenderTargetD3D11(RC<RenderDeviceD3D11> device, const OSWindow* window,
                                                 PixelType type, DepthStencilType depthStencil, int samples)
    : m_device(std::move(device)), m_window(window), m_type(type), m_depthStencilFmt(depthStencil),
      m_samples(samples) {

    m_device->incrementWindowTargets();

    OSWindowHandle handle   = window->getHandle();
    Size framebufferSize    = window->framebufferSize();

    // D3D11 doesn't use sRGB format for buffer itself, so we should specify sRGB for view
    DXGI_FORMAT colorFormat = dxFormatNoSrgb(m_type, PixelFormat::BGRA);

    HRESULT hr;
    if (m_device->m_factory2) {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc1{}; // zero-initialize
        swapChainDesc1.Width       = framebufferSize.width;
        swapChainDesc1.Height      = framebufferSize.height;
        swapChainDesc1.Format      = colorFormat;
        swapChainDesc1.SampleDesc  = { 1u, 0u };
        swapChainDesc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc1.BufferCount = 2;
        swapChainDesc1.Scaling     = DXGI_SCALING_NONE;
        swapChainDesc1.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        hr = m_device->m_factory2->CreateSwapChainForHwnd(m_device->m_device.Get(), handle.hWnd(),
                                                          &swapChainDesc1, nullptr, nullptr,
                                                          m_swapChain1.ReleaseAndGetAddressOf());
        if (FAILED(hr)) {
            // Try again with non-flip effect
            swapChainDesc1.Scaling    = DXGI_SCALING_STRETCH;
            swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            hr = m_device->m_factory2->CreateSwapChainForHwnd(m_device->m_device.Get(), handle.hWnd(),
                                                              &swapChainDesc1, nullptr, nullptr,
                                                              m_swapChain1.ReleaseAndGetAddressOf());
            CHECK_HRESULT(hr, return);
        } else {
            m_swapChain1->QueryInterface(m_swapChain.ReleaseAndGetAddressOf());
        }
    } else {
        DXGI_SWAP_CHAIN_DESC swapChainDesc{}; // zero-initialize
        swapChainDesc.BufferDesc.Width       = framebufferSize.width;
        swapChainDesc.BufferDesc.Height      = framebufferSize.height;
        swapChainDesc.BufferDesc.RefreshRate = { 1, 0 };
        swapChainDesc.BufferDesc.Format      = colorFormat;
        swapChainDesc.SampleDesc             = { 1u, 0u };
        swapChainDesc.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount            = 1;
        swapChainDesc.OutputWindow           = handle.hWnd();
        swapChainDesc.Windowed               = TRUE;
        swapChainDesc.SwapEffect             = DXGI_SWAP_EFFECT_DISCARD;
        hr = m_device->m_factory->CreateSwapChain(m_device->m_device.Get(), &swapChainDesc,
                                                  m_swapChain.ReleaseAndGetAddressOf());

        CHECK_HRESULT(hr, return);
    }

    createBackBuffer(framebufferSize);
}

WindowRenderTargetD3D11::~WindowRenderTargetD3D11() {
    m_device->decrementWindowTargets();
}

void WindowRenderTargetD3D11::setVSyncInterval(int interval) {
    if (interval != m_vsyncInterval) {
        m_vsyncInterval = interval;
    }
}

void WindowRenderTargetD3D11::present() {
    m_swapChain->Present(m_vsyncInterval, 0);
}

void WindowRenderTargetD3D11::createBackBuffer(Size size) {
    HRESULT hr;

    hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(m_backBuffer.colorBuffer.ReleaseAndGetAddressOf()));
    CHECK_HRESULT(hr, return);

    m_device->updateBackBuffer(m_backBuffer, m_type, m_depthStencilFmt, m_samples);
}

void WindowRenderTargetD3D11::resizeBackbuffer(Size size) {
    if (size != m_size) {
        m_device->m_context->OMSetRenderTargets(0, nullptr, nullptr);

        m_backBuffer = {};

        auto hr      = m_swapChain->ResizeBuffers(0, size.width, size.height, DXGI_FORMAT_UNKNOWN, 0);
        CHECK_HRESULT(hr, return);

        /* Recreate back buffer and reset default render target */
        createBackBuffer(size);
        m_size = size;
    }
}

Size WindowRenderTargetD3D11::size() const {
    return m_window->framebufferSize();
}

int WindowRenderTargetD3D11::vsyncInterval() const {
    return m_vsyncInterval;
}

} // namespace Brisk
