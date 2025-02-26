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
#include <dawn/native/DawnNative.h>
#include <sstream>
#include "../Atlas.hpp"

#include <dawn/webgpu_cpp_print.h>

namespace Brisk {

template <typename T>
inline std::string str(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
};

class WindowRenderTargetWebGPU;
class ImageRenderTargetWebGPU;
class RenderEncoderWebGPU;
class ImageBackendWebGPU;

namespace Internal {

template <typename T>
struct aligned_bytes {
    alignas(T) std::byte data[sizeof(T)];

    T* get() {
        return std::launder(reinterpret_cast<T*>(data));
    }
};
} // namespace Internal

class RenderDeviceWebGPU final : public RenderDevice,
                                 public std::enable_shared_from_this<RenderDeviceWebGPU> {
public:
    status<RenderDeviceError> init();

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

    RenderDeviceWebGPU(RendererDeviceSelection deviceSelection);
    ~RenderDeviceWebGPU();

private:
    friend class WindowRenderTargetWebGPU;
    friend class ImageRenderTargetWebGPU;
    friend class RenderEncoderWebGPU;
    friend class ImageBackendWebGPU;

    RendererDeviceSelection m_deviceSelection;
    std::unique_ptr<dawn::native::Instance> m_nativeInstance;
    dawn::native::Adapter m_nativeAdapter;
    wgpu::Instance m_instance;
    wgpu::Adapter m_adapter;
    wgpu::Device m_device;
    wgpu::ShaderModule m_shader;
    wgpu::PipelineLayoutDescriptor m_pipelineLayout;

    wgpu::Sampler m_atlasSampler;
    wgpu::Sampler m_gradientSampler;
    wgpu::Sampler m_boundSampler;
    wgpu::Buffer m_perFrameConstantBuffer;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::Texture m_dummyTexture;
    wgpu::TextureView m_dummyTextureView;
    using PipelineCacheKey = std::tuple<wgpu::TextureFormat, bool>;
    std::map<PipelineCacheKey, wgpu::RenderPipeline> m_pipelineCache;
    RenderResources m_resources;
    RenderLimits m_limits;

    bool createDevice();
    void createSamplers();
    void wait();
    wgpu::RenderPipeline createPipeline(wgpu::TextureFormat renderFormat, bool dualSourceBlending);
    bool updateBackBuffer(BackBufferWebGPU& buffer, PixelType type, DepthStencilType depthType, int samples);
};

class BackBufferProviderWebGPU {
public:
    virtual const BackBufferWebGPU& getBackBuffer() const = 0;
};

} // namespace Brisk
