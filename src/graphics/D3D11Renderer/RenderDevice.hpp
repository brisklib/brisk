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

#include "Common.hpp"

#include <brisk/graphics/Renderer.hpp>
#include "../Atlas.hpp"

namespace Brisk {

class WindowRenderTargetD3D11;
class ImageRenderTargetD3D11;
class RenderEncoderD3D11;
class ImageBackendD3D11;

class RenderDeviceD3D11 final : public RenderDevice, public std::enable_shared_from_this<RenderDeviceD3D11> {
public:
    status<RenderDeviceError> init();

    RendererBackend backend() const noexcept {
        return RendererBackend::D3D11;
    }

    RenderDeviceInfo info() const final;

    RC<WindowRenderTarget> createWindowTarget(const OSWindow* window, PixelType type = PixelType::U8Gamma,
                                              DepthStencilType depthStencil = DepthStencilType::None,
                                              int samples                   = 1) final;

    RC<ImageRenderTarget> createImageTarget(Size frameSize, PixelType type = PixelType::U8Gamma,
                                            DepthStencilType depthStencil = DepthStencilType::None,
                                            int samples                   = 1) final;

    RC<RenderEncoder> createEncoder() final;

    RenderResources& resources() final {
        return m_resources;
    }

    RenderLimits limits() const final;

    void createImageBackend(RC<Image> image) final;

    RenderDeviceD3D11(RendererDeviceSelection deviceSelection);
    ~RenderDeviceD3D11();

private:
    friend class WindowRenderTargetD3D11;
    friend class ImageRenderTargetD3D11;
    friend class RenderEncoderD3D11;
    friend class ImageBackendD3D11;

    RendererDeviceSelection m_deviceSelection;
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
    ComPtr<ID3D11SamplerState> m_boundSampler;
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

    bool updateBackBuffer(BackBufferD3D11& buffer, PixelType type, DepthStencilType depthType, int samples);
};

} // namespace Brisk
