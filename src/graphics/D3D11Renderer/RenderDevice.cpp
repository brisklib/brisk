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
#include "RenderDevice.hpp"
#include "WindowRenderTarget.hpp"
#include "ImageRenderTarget.hpp"
#include "RenderEncoder.hpp"
#include "ImageBackend.hpp"
#include <brisk/core/Threading.hpp>
#include <brisk/core/Encoding.hpp>

#include <brisk/core/Resources.hpp>

namespace Brisk {

static D3D_DRIVER_TYPE driverTypes[] = {
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_WARP,
    D3D_DRIVER_TYPE_SOFTWARE,
};

static D3D_FEATURE_LEVEL featureLevels[] = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
};

bool RenderDeviceD3D11::createDevice(UINT flags) {

    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));
    if (!SUCCEEDED(hr))
        return false;

    if (m_display) {
        m_adapter = adapterForMonitor(m_display.hMonitor(), m_factory);
        if (m_adapter) {
            hr = D3D11CreateDevice(m_adapter.Get(),                             //
                                   D3D_DRIVER_TYPE_UNKNOWN,                     //
                                   0,                                           //
                                   flags,                                       //
                                   std::data(featureLevels),                    //
                                   static_cast<UINT>(std::size(featureLevels)), //
                                   D3D11_SDK_VERSION,                           //
                                   m_device.ReleaseAndGetAddressOf(),           //
                                   &m_featureLevel,                             //
                                   m_context.ReleaseAndGetAddressOf()           //
            );
            if (SUCCEEDED(hr))
                return true;
        }
    }

    if (m_deviceSelection != RendererDeviceSelection::Default) {
        ComPtr<IDXGIFactory6> factory6;

        hr = m_factory.As(&factory6);
        if (!SUCCEEDED(hr))
            return false;
        if (factory6) {
            DXGI_GPU_PREFERENCE pref = m_deviceSelection == RendererDeviceSelection::HighPerformance
                                           ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
                                           : DXGI_GPU_PREFERENCE_MINIMUM_POWER;

            for (UINT idx = 0; SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                     idx, pref, IID_PPV_ARGS(m_adapter.ReleaseAndGetAddressOf())));
                 idx++) {
                hr = D3D11CreateDevice(m_adapter.Get(),                             //
                                       D3D_DRIVER_TYPE_UNKNOWN,                     //
                                       0,                                           //
                                       flags,                                       //
                                       std::data(featureLevels),                    //
                                       static_cast<UINT>(std::size(featureLevels)), //
                                       D3D11_SDK_VERSION,                           //
                                       m_device.ReleaseAndGetAddressOf(),           //
                                       &m_featureLevel,                             //
                                       m_context.ReleaseAndGetAddressOf()           //
                );
                if (SUCCEEDED(hr))
                    return true;
            }
        }
        m_adapter = nullptr;
    }

    for (D3D_DRIVER_TYPE driverType : driverTypes) {
        hr = D3D11CreateDevice(m_adapter.Get(),                             //
                               driverType,                                  //
                               0,                                           //
                               flags,                                       //
                               std::data(featureLevels),                    //
                               static_cast<UINT>(std::size(featureLevels)), //
                               D3D11_SDK_VERSION,                           //
                               m_device.ReleaseAndGetAddressOf(),           //
                               &m_featureLevel,                             //
                               m_context.ReleaseAndGetAddressOf()           //
        );
        if (SUCCEEDED(hr))
            return true;
    }
    return false;
}

RenderDeviceD3D11::RenderDeviceD3D11(RendererDeviceSelection deviceSelection, OSDisplayHandle display)
    : m_deviceSelection(deviceSelection), m_display(display) {}

status<RenderDeviceError> RenderDeviceD3D11::init() {
#ifndef NDEBUG
    if (!createDevice(D3D11_CREATE_DEVICE_DEBUG)) {
        if (!createDevice(0)) {
            return unexpected(RenderDeviceError::Unsupported);
        }
    }
#else
    if (!createDevice(0)) {
        return unexpected(RenderDeviceError::Unsupported);
    }
#endif

    std::ignore = m_device.As(&m_device1);
    std::ignore = m_device.As(&m_device2);
    std::ignore = m_device.As(&m_device3);

    std::ignore = m_context.As(&m_context1);

    HRESULT hr  = m_device.As(&m_dxgiDevice);
    CHECK_HRESULT(hr, return unexpected(RenderDeviceError::InternalError));

    std::ignore = m_dxgiDevice.As(&m_dxgiDevice1);

    if (m_dxgiDevice1)
        m_dxgiDevice1->SetMaximumFrameLatency(1);

    hr = m_dxgiDevice->GetAdapter(m_adapter.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return unexpected(RenderDeviceError::InternalError));

    DXGI_ADAPTER_DESC desc;
    m_adapter->GetDesc(&desc);

#ifdef BRISK_DEBUG_GPU
    fmt::print("DXGI_ADAPTER_DESC\n");
    fmt::print("    Description           {}\n", wcsToUtf8(desc.Description));
    fmt::print("    VendorId              {:04X}\n", desc.VendorId);
    fmt::print("    DeviceId              {:04X}\n", desc.DeviceId);
    fmt::print("    SubSysId              {:04X}\n", desc.SubSysId);
    fmt::print("    Revision              {:04X}\n", desc.Revision);
    fmt::print("    DedicatedVideoMemory  {}MiB\n", desc.DedicatedVideoMemory / 1'048'576);
    fmt::print("    DedicatedSystemMemory {}MiB\n", desc.DedicatedSystemMemory / 1'048'576);
    fmt::print("    SharedSystemMemory    {}MiB\n", desc.SharedSystemMemory / 1'048'576);
#endif

    hr = m_adapter->GetParent(IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));
    CHECK_HRESULT(hr, return unexpected(RenderDeviceError::InternalError));

    std::ignore          = m_factory.As(&m_factory2);

    auto shader_vertex   = Resources::load("d3d11/vertex.fxc");
    auto shader_fragment = Resources::load("d3d11/fragment.fxc");

    hr                   = m_device->CreateVertexShader(shader_vertex.data(), shader_vertex.size(), nullptr,
                                                        m_vertexShader.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return unexpected(RenderDeviceError::ShaderError));
    hr = m_device->CreatePixelShader(shader_fragment.data(), shader_fragment.size(), nullptr,
                                     m_pixelShader.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return unexpected(RenderDeviceError::ShaderError));

    createBlendState();
    createRasterizerState();
    createSamplers();
    createPerFrameConstantBuffer();

    m_resources.spriteAtlas.reset(
        new SpriteAtlas(256 * 1024, maxD3D11ResourceBytes, 256 * 1024, &m_resources.mutex));

    m_resources.gradientAtlas.reset(new GradientAtlas(1024, &m_resources.mutex));

    return {};
}

void RenderDeviceD3D11::createSamplers() {
    D3D11_SAMPLER_DESC samplerDesc{}; // zero-initialize
    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MaxAnisotropy  = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    HRESULT hr = m_device->CreateSamplerState(&samplerDesc, m_atlasSampler.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    hr = m_device->CreateSamplerState(&samplerDesc, m_gradientSampler.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
    samplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    hr = m_device->CreateSamplerState(&samplerDesc, m_boundSampler.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
}

void RenderDeviceD3D11::createBlendState() {
    D3D11_BLEND_DESC blendDesc{}; // zero-initialize
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC1_COLOR;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
    HRESULT hr = m_device->CreateBlendState(&blendDesc, m_blendState.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
}

void RenderDeviceD3D11::createRasterizerState() {
    D3D11_RASTERIZER_DESC rasterDesc{}; // zero-initialize
    rasterDesc.FillMode        = D3D11_FILL_SOLID;
    rasterDesc.CullMode        = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.ScissorEnable   = TRUE;
    HRESULT hr = m_device->CreateRasterizerState(&rasterDesc, m_rasterizerState.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
}

void RenderDeviceD3D11::createPerFrameConstantBuffer() {
    D3D11_BUFFER_DESC bufDesc{}; // zero-initialize
    bufDesc.ByteWidth      = sizeof(ConstantPerFrame);
    bufDesc.Usage          = D3D11_USAGE_DYNAMIC;
    bufDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = m_device->CreateBuffer(&bufDesc, nullptr, m_perFrameConstantBuffer.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
}

static std::string_view findVendor(uint16_t vendor) {
    return staticMap<std::string_view>(vendor,                                 //
                                       0x05ac, "Apple Inc.",                   //
                                       0x1002, "Advanced Micro Devices, Inc.", //
                                       0x10de, "NVIDIA Corporation",           //
                                       0x1414, "Microsoft Corporation",        //
                                       0x15ad, "VMware Inc.",                  //
                                       0x8086, "Intel Corporation",            //
                                       0x80ee, "Oracle Corporation",           //
                                       "(Unknown)");
}

RenderDeviceInfo RenderDeviceD3D11::info() const {
    DXGI_ADAPTER_DESC desc;
    m_adapter->GetDesc(&desc);
    RenderDeviceInfo info;
    info.api        = "Direct3D11";
    info.apiVersion = staticMap(m_featureLevel,               //
                                D3D_FEATURE_LEVEL_11_1, 1101, //
                                D3D_FEATURE_LEVEL_11_0, 1100, //
                                D3D_FEATURE_LEVEL_10_1, 1001, //
                                D3D_FEATURE_LEVEL_10_0, 1000, //
                                0);
    info.vendor     = findVendor(desc.VendorId);
    info.device     = wcsToUtf8(WStringView{ desc.Description });
    return info;
}

RC<WindowRenderTarget> RenderDeviceD3D11::createWindowTarget(const OSWindow* window, PixelType type,
                                                             DepthStencilType depthStencil, int samples) {
    return rcnew WindowRenderTargetD3D11(shared_from_this(), window, type, depthStencil, samples);
}

RC<ImageRenderTarget> RenderDeviceD3D11::createImageTarget(Size frameSize, PixelType type,
                                                           DepthStencilType depthStencil, int samples) {
    return rcnew ImageRenderTargetD3D11(shared_from_this(), frameSize, type, depthStencil, samples);
}

RC<RenderEncoder> RenderDeviceD3D11::createEncoder() {
    return rcnew RenderEncoderD3D11(shared_from_this());
}

RenderDeviceD3D11::~RenderDeviceD3D11() = default;

bool RenderDeviceD3D11::updateBackBuffer(BackBufferD3D11& buffer, PixelType type, DepthStencilType depthType,
                                         int samples) {
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{}; // zero-initialize
    rtvDesc.ViewDimension = samples > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = linearColor ? dxFormat(type, backBufferFormat) : dxFormatNoSrgb(type, backBufferFormat);

    D3D11_TEXTURE2D_DESC colorDesc;
    buffer.colorBuffer->GetDesc(&colorDesc);
    HRESULT hr;

    hr = m_device->CreateRenderTargetView(buffer.colorBuffer.Get(), &rtvDesc,
                                          buffer.rtv.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return false);

    if (depthType != DepthStencilType::None) {
        DXGI_FORMAT depthFmt =
            depthType == DepthStencilType::D24S8 ? DXGI_FORMAT_D32_FLOAT_S8X24_UINT : DXGI_FORMAT_D32_FLOAT;
        D3D11_TEXTURE2D_DESC depthBufDesc =
            texDesc(depthFmt, Size(colorDesc.Width, colorDesc.Height), samples, D3D11_USAGE_DEFAULT,
                    D3D11_BIND_DEPTH_STENCIL, 0);
        hr = m_device->CreateTexture2D(&depthBufDesc, nullptr, buffer.depthStencil.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return false);
        hr = m_device->CreateDepthStencilView(buffer.depthStencil.Get(), nullptr,
                                              buffer.dsv.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return false);
    } else {
        buffer.depthStencil.Reset();
        buffer.dsv.Reset();
    }
    return true;
}

void RenderDeviceD3D11::incrementWindowTargets() {
    mustBeMainThread();
    ++m_windowTargets;
    if (m_dxgiDevice1)
        m_dxgiDevice1->SetMaximumFrameLatency(m_windowTargets);
}

void RenderDeviceD3D11::decrementWindowTargets() {
    mustBeMainThread();
    --m_windowTargets;
    if (m_dxgiDevice1)
        m_dxgiDevice1->SetMaximumFrameLatency(m_windowTargets);
}

void RenderDeviceD3D11::createImageBackend(RC<Image> image) {
    BRISK_ASSERT(image);
    if (dxFormat(image->pixelType(), image->pixelFormat()) == DXGI_FORMAT_UNKNOWN) {
        throwException(EImageError("Direct3D11 backend does not support the image type or format: {}, {}. "
                                   "Consider converting the image before sending it to the GPU.",
                                   image->pixelType(), image->pixelFormat()));
    }
    std::ignore = getOrCreateBackend(shared_from_this(), std::move(image), true, false);
}

RenderLimits RenderDeviceD3D11::limits() const {
    return RenderLimits{ .maxDataSize  = maxD3D11ResourceBytes / sizeof(float),
                         .maxAtlasSize = maxD3D11ResourceBytes,
                         .maxGradients = 1024 };
}
} // namespace Brisk
