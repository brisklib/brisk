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

#include "Common.hpp"

#include <brisk/graphics/Renderer.hpp>
#include "../Atlas.hpp"

namespace Brisk {

class WindowRenderTargetD3d11;
class ImageRenderTargetD3d11;
class RenderEncoderD3d11;
class ImageBackendD3d11;

class RenderDeviceD3d11 final : public RenderDevice, public std::enable_shared_from_this<RenderDeviceD3d11> {
public:
    status<RenderDeviceError> init();

    RendererBackend backend() const noexcept {
        return RendererBackend::D3d11;
    }

    RenderDeviceInfo info() const final;

    Rc<WindowRenderTarget> createWindowTarget(const NativeWindow* window, PixelType type = PixelType::U8Gamma,
                                              DepthStencilType depthStencil = DepthStencilType::None,
                                              int samples                   = 1) final;

    Rc<ImageRenderTarget> createImageTarget(Size frameSize, PixelType type = PixelType::U8Gamma,
                                            DepthStencilType depthStencil = DepthStencilType::None,
                                            int samples                   = 1) final;

    Rc<RenderEncoder> createEncoder() final;

    RenderResources& resources() final {
        return m_resources;
    }

    RenderLimits limits() const final;

    void createImageBackend(Rc<Image> image) final;

    RenderDeviceD3d11(RendererDeviceSelection deviceSelection, NativeDisplayHandle display);
    ~RenderDeviceD3d11();

private:
    friend class WindowRenderTargetD3d11;
    friend class ImageRenderTargetD3d11;
    friend class RenderEncoderD3d11;
    friend class ImageBackendD3d11;

    RendererDeviceSelection m_deviceSelection;
    NativeDisplayHandle m_display;
    ComPtr<IDXGIFactory> m_factory;
    ComPtr<IDXGIFactory2> m_factory2;
    ComPtr<IDXGIDevice> m_dxgiDevice;
    ComPtr<IDXGIDevice1> m_dxgiDevice1;
    ComPtr<IDXGIAdapter> m_adapter;
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11Device1> m_device1;
    ComPtr<ID3D11Device2> m_device2;
    ComPtr<ID3D11Device3> m_device3;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<ID3D11DeviceContext1> m_context1;
    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    D3D_FEATURE_LEVEL m_featureLevel;
    ComPtr<ID3D11BlendState> m_blendState;
    ComPtr<ID3D11RasterizerState> m_rasterizerState;
    ComPtr<ID3D11SamplerState> m_atlasSampler;
    ComPtr<ID3D11SamplerState> m_gradientSampler;
    ComPtr<ID3D11SamplerState> m_sampler;
    ComPtr<ID3D11Buffer> m_perFrameConstantBuffer;
    int m_windowTargets = 0;
    RenderResources m_resources;
    void incrementWindowTargets();
    void decrementWindowTargets();
    bool createDevice(UINT flags);
    void createBlendState();
    void createRasterizerState();
    void createSamplers();
    void createPerFrameConstantBuffer();

    bool updateBackBuffer(BackBufferD3d11& buffer, PixelType type, DepthStencilType depthType, int samples);
};

} // namespace Brisk
