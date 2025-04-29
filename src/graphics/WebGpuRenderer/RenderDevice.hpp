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

#include <brisk/graphics/WebGpu.hpp>

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

class WindowRenderTargetWebGpu;
class ImageRenderTargetWebGpu;
class RenderEncoderWebGpu;
class ImageBackendWebGpu;

namespace Internal {

template <typename T>
struct aligned_bytes {
    alignas(T) std::byte data[sizeof(T)];

    T* get() {
        return std::launder(reinterpret_cast<T*>(data));
    }
};
} // namespace Internal

class RenderDeviceWebGpu final : public RenderDevice,
                                 public std::enable_shared_from_this<RenderDeviceWebGpu> {
public:
    status<RenderDeviceError> init();

    RendererBackend backend() const noexcept {
        return RendererBackend::WebGpu;
    }

    RenderDeviceInfo info() const final;

    Rc<WindowRenderTarget> createWindowTarget(const OsWindow* window, PixelType type = PixelType::U8Gamma,
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

    RenderDeviceWebGpu(RendererDeviceSelection deviceSelection, OsDisplayHandle display);
    ~RenderDeviceWebGpu();

private:
    friend class WindowRenderTargetWebGpu;
    friend class ImageRenderTargetWebGpu;
    friend class RenderEncoderWebGpu;
    friend class ImageBackendWebGpu;
    friend bool webgpuFromContext(RenderContext&, wgpu::Device&, wgpu::TextureView&);

    RendererDeviceSelection m_deviceSelection;
    OsDisplayHandle m_display;
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
    bool m_timestampQuerySupported = false;

    bool createDevice();
    void createSamplers();
    void wait();
    wgpu::RenderPipeline createPipeline(wgpu::TextureFormat renderFormat, bool dualSourceBlending);
    bool updateBackBuffer(BackBufferWebGpu& buffer, PixelType type, DepthStencilType depthType, int samples);
};

} // namespace Brisk
