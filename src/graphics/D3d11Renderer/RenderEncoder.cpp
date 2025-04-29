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
#include "RenderEncoder.hpp"
#include "ImageBackend.hpp"
#include <brisk/core/Utilities.hpp>
#include "../Atlas.hpp"
#include <brisk/core/Log.hpp>
#include <brisk/core/Time.hpp>
#include "ImageRenderTarget.hpp"
#include "WindowRenderTarget.hpp"

namespace Brisk {

VisualSettings RenderEncoderD3d11::visualSettings() const {
    return m_visualSettings;
}

void RenderEncoderD3d11::setVisualSettings(const VisualSettings& visualSettings) {
    m_visualSettings = visualSettings;
}

void RenderEncoderD3d11::begin(Rc<RenderTarget> target, std::optional<ColorF> clear) {
    m_currentTarget                     = std::move(target);
    ComPtr<ID3D11DeviceContext> context = m_device->m_context;
    m_frameSize                         = m_currentTarget->size();
    if (m_currentTarget->type() == RenderTargetType::Window) {
        static_cast<WindowRenderTarget*>(m_currentTarget.get())->resizeBackbuffer(m_frameSize);
    }
    D3D11_VIEWPORT viewport{}; // zero-initialize
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = m_frameSize.x;
    viewport.Height   = m_frameSize.y;
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;
    context->RSSetViewports(1, &viewport);

    [[maybe_unused]] ConstantPerFrame constantPerFrame{
        Simd<float, 4>(m_frameSize.width, m_frameSize.height, 1.f / m_frameSize.width,
                       1.f / m_frameSize.height),
        m_visualSettings.blueLightFilter,
        m_visualSettings.gamma,
        Internal::textRectPadding,
        Internal::textRectOffset,
        Internal::max2DTextureSize,
    };

    updatePerFrameConstantBuffer(constantPerFrame);

    ID3D11ShaderResourceView* dataSRV[1] = { nullptr };
    context->VSSetShaderResources(10, 1, dataSRV);
    context->PSSetShaderResources(10, 1, dataSRV);

    const BackBufferD3d11& backBuf     = getBackBuffer(m_currentTarget.get());
    ID3D11RenderTargetView* rtvList[1] = { backBuf.rtv.Get() };
    context->OMSetRenderTargets(1, rtvList, nullptr);

    if (clear)
        context->ClearRenderTargetView(backBuf.rtv.Get(), clear->array);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    static FLOAT blendFactor[4]{ 0.f, 0.f, 0.f, 0.f };
    context->OMSetBlendState(m_device->m_blendState.Get(), blendFactor, ~0);
    context->RSSetState(m_device->m_rasterizerState.Get());

    context->PSSetShader(m_device->m_pixelShader.Get(), nullptr, 0);
    context->VSSetShader(m_device->m_vertexShader.Get(), nullptr, 0);

    ID3D11Buffer* const cbuffers[1] = { m_device->m_perFrameConstantBuffer.Get() };
    context->VSSetConstantBuffers(2, 1, cbuffers);
    context->PSSetConstantBuffers(2, 1, cbuffers);

    ID3D11SamplerState* const samplers[2] = { m_device->m_boundSampler.Get(),
                                              m_device->m_gradientSampler.Get() };
    context->VSSetSamplers(6, 1, samplers);
    context->PSSetSamplers(6, 2, samplers);
}

void RenderEncoderD3d11::end() {
    ComPtr<ID3D11DeviceContext> context = m_device->m_context;
    D3D11_QUERY_DESC queryDesc{ D3D11_QUERY_EVENT, 0 };
    HRESULT hr = m_device->m_device->CreateQuery(&queryDesc, m_query.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
    context->End(m_query.Get());
    m_currentTarget = nullptr;
}

void RenderEncoderD3d11::batch(std::span<const RenderState> commands, std::span<const float> data) {
    ComPtr<ID3D11DeviceContext> context = m_device->m_context;

#if 1
    if (commands.size() == 1 && commands.front().shader == ShaderType::Blit) {

        if (m_frameTimingIndex < m_frameTiming.size() && m_batchIndex < maxDurations) {
            m_frameTiming[m_frameTimingIndex].begin(m_device->m_device.Get(), m_device->m_context.Get());
        }
        const BackBufferD3d11& backBuf = getBackBuffer(m_currentTarget.get());
        Size size = static_cast<ImageBackendD3d11*>(commands.front().imageBackend)->m_image->size();
        context->OMSetRenderTargets(0, nullptr, nullptr);
        context->CopyResource(
            backBuf.colorBuffer.Get(),
            static_cast<ImageBackendD3d11*>(commands.front().imageBackend)->m_texture.Get());
        ID3D11RenderTargetView* rtvList[1] = { backBuf.rtv.Get() };
        context->OMSetRenderTargets(1, rtvList, nullptr);
        if (m_frameTimingIndex < m_frameTiming.size() && m_batchIndex < maxDurations) {
            m_frameTiming[m_frameTimingIndex].end(m_device->m_context.Get());
        }
        context->Flush();

        ++m_batchIndex;
        return;
    }
#endif

    bool uploadResources = requiresAtlasOrGradient(commands);

    if (uploadResources || !m_atlasTexture || !m_gradientTexture) {
        std::lock_guard lk(m_device->m_resources.mutex);
        updateAtlasTexture();
        updateGradientTexture();
    }

    ID3D11ShaderResourceView* const resourceViews[2] = { m_gradientSRV.Get(), m_atlasSRV.Get() };
    context->PSSetShaderResources(8, 2, resourceViews);

    updateDataBuffer(data);

    if (m_frameTimingIndex < m_frameTiming.size() && m_batchIndex < maxDurations) {
        m_frameTiming[m_frameTimingIndex].begin(m_device->m_device.Get(), m_device->m_context.Get());
    }

    ID3D11ShaderResourceView* dataSRV[1] = { m_dataSRV.Get() };
    context->VSSetShaderResources(3, 1, dataSRV);
    context->PSSetShaderResources(3, 1, dataSRV);

    bool uniformOffsetSupported = m_device->m_context1; // if 11.1

    const size_t maxCommandsInBatch =
        uniformOffsetSupported ? maxD3d11ResourceBytes / sizeof(RenderState) : 1;

    Internal::ImageBackend* savedTexture = nullptr;

    constexpr size_t constantsPerCommand = sizeof(RenderState) / 16;

    Rectangle frameRect                  = Rectangle({}, m_frameSize);
    Rectangle currentClipRect            = noClipRect;

    for (size_t i = 0; i < commands.size(); ++i) {
        auto& cmd                             = commands[i];

        [[maybe_unused]] size_t offsetInBatch = i % maxCommandsInBatch;
        if (i % maxCommandsInBatch == 0) {
            updateConstantBuffer(
                std::span{ commands.data() + i, std::min(maxCommandsInBatch, commands.size() - i) });
        }

        if (cmd.imageBackend != savedTexture) {
            savedTexture                               = cmd.imageBackend;
            ID3D11ShaderResourceView* resourceViews[1] = { nullptr };
            if (cmd.imageBackend) {
                resourceViews[0] = static_cast<ImageBackendD3d11*>(cmd.imageBackend)->m_srv.Get();
            }
            context->VSSetShaderResources(10, 1, resourceViews);
            context->PSSetShaderResources(10, 1, resourceViews);
        }

        Rectangle clampedRect = cmd.shaderClip.intersection(frameRect);
        if (i == 0 || clampedRect != currentClipRect) {
            CD3D11_RECT d3d11Rect(clampedRect.x1, clampedRect.y1, clampedRect.x2, clampedRect.y2);
            context->RSSetScissorRects(1, &d3d11Rect);
            currentClipRect = clampedRect;
        }

        ID3D11Buffer* const cbuffers[1] = { m_constantBuffer.Get() };
        if (uniformOffsetSupported) {
            const UINT firstConstant[1] = { UINT(offsetInBatch * constantsPerCommand) };
            const UINT numConstants[1]  = { constantsPerCommand };
            m_device->m_context1->VSSetConstantBuffers1(1, 1, cbuffers, firstConstant, numConstants);
            m_device->m_context1->PSSetConstantBuffers1(1, 1, cbuffers, firstConstant, numConstants);
        } else {
            context->VSSetConstantBuffers(1, 1, cbuffers);
            context->PSSetConstantBuffers(1, 1, cbuffers);
        }

        context->DrawInstanced(4, cmd.instances, 0, 0);
    }
    if (m_frameTimingIndex < m_frameTiming.size() && m_batchIndex < maxDurations) {
        m_frameTiming[m_frameTimingIndex].end(m_device->m_context.Get());
    }
    context->Flush();

    ++m_batchIndex;
}

void RenderEncoderD3d11::wait() {
    processQueries();
    if (!m_query.Get())
        return;
    while (m_device->m_context->GetData(m_query.Get(), nullptr, 0, 0) == S_FALSE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    processQueries();
}

void RenderEncoderD3d11::updatePerFrameConstantBuffer(const ConstantPerFrame& constants) {
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_device->m_context->Map(m_device->m_perFrameConstantBuffer.Get(), 0,
                                          D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    CHECK_HRESULT(hr, return);
    SCOPE_EXIT {
        m_device->m_context->Unmap(m_device->m_perFrameConstantBuffer.Get(), 0);
    };
    memcpy(mapped.pData, &constants, sizeof(ConstantPerFrame));
}

void RenderEncoderD3d11::updateConstantBuffer(std::span<const RenderState> data) {
    if (!m_constantBuffer || data.size_bytes() != m_constantBufferSize) {
        D3D11_BUFFER_DESC bufDesc{}; // zero-initialize
        bufDesc.ByteWidth      = data.size_bytes();
        bufDesc.Usage          = D3D11_USAGE_DYNAMIC;
        bufDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA subData{}; // zero-initialize
        subData.pSysMem = data.data();

        HRESULT hr =
            m_device->m_device->CreateBuffer(&bufDesc, &subData, m_constantBuffer.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);
        m_constantBufferSize = data.size_bytes();
    } else {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_device->m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        CHECK_HRESULT(hr, return);
        SCOPE_EXIT {
            m_device->m_context->Unmap(m_constantBuffer.Get(), 0);
        };
        memcpy(mapped.pData, data.data(), data.size_bytes());
    }
}

// Update the data buffer and possibly recreate it.
void RenderEncoderD3d11::updateDataBuffer(std::span<const float> data) {
    static const float dummy[4] = { 0.f };
    if (data.empty()) {
        // Ensure that buffer is not empty
        data = dummy;
    }

    if (!m_dataBuffer || data.size_bytes() != m_dataBufferSize) {
        m_dataSRV.Reset();
        D3D11_BUFFER_DESC bufDesc{}; // zero-initialize
        bufDesc.ByteWidth      = data.size_bytes();
        bufDesc.Usage          = D3D11_USAGE_DYNAMIC;
        bufDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE;
        bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufDesc.MiscFlags      = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;

        D3D11_SUBRESOURCE_DATA subData{}; // zero-initialize
        subData.pSysMem = data.data();

        HRESULT hr =
            m_device->m_device->CreateBuffer(&bufDesc, &subData, m_dataBuffer.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // zero-initialize
        srvDesc.Format               = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFEREX;
        srvDesc.BufferEx.NumElements = data.size();
        srvDesc.BufferEx.Flags       = D3D11_BUFFEREX_SRV_FLAG_RAW;

        hr = m_device->m_device->CreateShaderResourceView(m_dataBuffer.Get(), &srvDesc,
                                                          m_dataSRV.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);
        m_dataBufferSize = data.size_bytes();
    } else if (data.size_bytes() > 0) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_device->m_context->Map(m_dataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        CHECK_HRESULT(hr, return);
        SCOPE_EXIT {
            m_device->m_context->Unmap(m_dataBuffer.Get(), 0);
        };
        memcpy(mapped.pData, data.data(), data.size_bytes());
    }
}

void RenderEncoderD3d11::updateAtlasTexture() {
    SpriteAtlas* atlas = m_device->m_resources.spriteAtlas.get();

    if (!m_atlasTexture || (m_atlas_generation <<= atlas->changed)) {
        Size newSize(Internal::max2DTextureSize, atlas->data().size() / Internal::max2DTextureSize);
        m_atlasTexture.Reset();
        D3D11_TEXTURE2D_DESC tex = texDesc(dxFormat(PixelType::U8, PixelFormat::Greyscale), newSize, 1);
        D3D11_SUBRESOURCE_DATA subData{}; // zero-initialize
        subData.pSysMem     = atlas->data().data();
        subData.SysMemPitch = newSize.width; // in bytes
        HRESULT hr =
            m_device->m_device->CreateTexture2D(&tex, &subData, m_atlasTexture.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // zero-initialize
        srvDesc.Format              = DXGI_FORMAT_R8_UNORM;
        srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = m_device->m_device->CreateShaderResourceView(m_atlasTexture.Get(), &srvDesc,
                                                          m_atlasSRV.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);
    }
}

void RenderEncoderD3d11::updateGradientTexture() {
    GradientAtlas* atlas = m_device->m_resources.gradientAtlas.get();

    Size newSize(gradientResolution, atlas->size());
    if (!m_gradientTexture || (m_gradient_generation <<= atlas->changed)) {
        m_gradientTexture.Reset();
        D3D11_TEXTURE2D_DESC tex = texDesc(dxFormat(PixelType::F32, PixelFormat::RGBA), newSize, 1);
        D3D11_SUBRESOURCE_DATA subData{}; // zero-initialize
        subData.pSysMem     = atlas->data().data();
        subData.SysMemPitch = newSize.width * sizeof(ColorF); // in bytes
        HRESULT hr =
            m_device->m_device->CreateTexture2D(&tex, &subData, m_gradientTexture.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // zero-initialize
        srvDesc.Format              = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        hr = m_device->m_device->CreateShaderResourceView(m_gradientTexture.Get(), &srvDesc,
                                                          m_gradientSRV.ReleaseAndGetAddressOf());
        CHECK_HRESULT(hr, return);
    }
}

RenderEncoderD3d11::RenderEncoderD3d11(Rc<RenderDeviceD3d11> device) : m_device(std::move(device)) {}

RenderEncoderD3d11::~RenderEncoderD3d11() = default;

RenderDevice* RenderEncoderD3d11::device() const {
    return m_device.get();
}

Rc<RenderTarget> RenderEncoderD3d11::currentTarget() const {
    return m_currentTarget;
}

size_t RenderEncoderD3d11::findFrameTimingSlot() {
    for (size_t i = 0; i < m_frameTiming.size(); ++i) {
        if (!m_frameTiming[i].pending) {
            m_frameTiming[i].pending = true;
            m_frameTiming[i].frameId = m_frameId;
            return i;
        }
    }
    BRISK_ASSERT_MSG("All frame timing slots are busy", m_frameTiming.size() < maxFrameTimings);

    m_frameTiming.emplace_back(m_frameId, m_device->m_device.Get());
    return m_frameTiming.size() - 1;
}

void RenderEncoderD3d11::beginFrame(uint64_t frameId) {
    m_frameId          = frameId;
    m_batchIndex       = 0;
    m_frameTimingIndex = findFrameTimingSlot();
}

void RenderEncoderD3d11::processQueries() {
    auto ctx = m_device->m_context;
    for (size_t i = 0; i < m_frameTiming.size(); ++i) {
        FrameTiming& timing = m_frameTiming[i];
        if (!timing.pending)
            continue;
        if (auto durations = timing.time(m_device->m_context.Get())) {
            if (m_durationCallback)
                m_durationCallback(timing.frameId, *durations);
        }
    }
}

void RenderEncoderD3d11::endFrame(DurationCallback callback) {
    m_durationCallback = std::move(callback);
    processQueries();
}

RenderEncoderD3d11::BatchTiming::BatchTiming(ID3D11Device* device) {
    D3D11_QUERY_DESC queryDesc{};
    queryDesc.Query = D3D11_QUERY_TIMESTAMP;
    device->CreateQuery(&queryDesc, startQuery.ReleaseAndGetAddressOf());
    device->CreateQuery(&queryDesc, endQuery.ReleaseAndGetAddressOf());
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    device->CreateQuery(&queryDesc, disjointQuery.ReleaseAndGetAddressOf());
}

std::optional<std::chrono::nanoseconds> RenderEncoderD3d11::BatchTiming::time(ID3D11DeviceContext* ctx) {
    uint64_t startTime = 0, endTime = 0;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData{};
    if (ctx->GetData(endQuery.Get(), &endTime, sizeof(endTime), 0) == S_OK &&
        ctx->GetData(startQuery.Get(), &startTime, sizeof(startTime), 0) == S_OK &&
        ctx->GetData(disjointQuery.Get(), &disjointData, sizeof(disjointData), 0) == S_OK) {
        double ns = 1'000'000'000.0 * static_cast<double>(endTime - startTime) / disjointData.Frequency;
        return std::chrono::nanoseconds(static_cast<int64_t>(ns));
    }
    return std::nullopt;
}

void RenderEncoderD3d11::FrameTiming::begin(ID3D11Device* device, ID3D11DeviceContext* ctx) {
    BRISK_ASSERT(pending);
    batches.emplace_back(device);
    ctx->Begin(batches.back().disjointQuery.Get());
    ctx->End(batches.back().startQuery.Get());
}

void RenderEncoderD3d11::FrameTiming::end(ID3D11DeviceContext* ctx) {
    BRISK_ASSERT(pending);
    ctx->End(batches.back().endQuery.Get());
    ctx->End(batches.back().disjointQuery.Get());
}

RenderEncoderD3d11::FrameTiming::FrameTiming(uint64_t frameId, ID3D11Device* device) {
    this->frameId = frameId;
    pending       = true;
}

std::optional<std::vector<std::chrono::nanoseconds>> RenderEncoderD3d11::FrameTiming::time(
    ID3D11DeviceContext* ctx) {
    BRISK_ASSERT(pending);
    if (batches.empty())
        return std::nullopt;
    auto t = batches.front().time(ctx);
    if (!t) // Return early if not ready
        return std::nullopt;
    std::vector<std::chrono::nanoseconds> result(batches.size());
    result.front() = *t;
    for (size_t i = 1; i < result.size(); ++i) {
        if (auto t = batches[i].time(ctx)) {
            result[i] = *t;
        } else {
            return std::nullopt;
        }
    }
    pending = false;
    batches.clear();
    return result;
}

const BackBufferD3d11& getBackBuffer(RenderTarget* target) {
    switch (target->type()) {
    case RenderTargetType::Window:
        return static_cast<WindowRenderTargetD3d11*>(target)->getBackBuffer();
    case RenderTargetType::Image:
        return static_cast<ImageRenderTargetD3d11*>(target)->getBackBuffer();
    default:
        BRISK_UNREACHABLE();
    }
}

} // namespace Brisk
