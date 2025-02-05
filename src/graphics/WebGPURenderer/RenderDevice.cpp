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
#include "RenderDevice.hpp"
#include "WindowRenderTarget.hpp"
#include "ImageRenderTarget.hpp"
#include "RenderEncoder.hpp"
#include "ImageBackend.hpp"
#include <brisk/core/Cryptography.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Log.hpp>

namespace Brisk {

static fs::path cacheFolder() {
    fs::path folder =
        defaultFolder(DefaultFolder::UserData) / appMetadata.vendor / appMetadata.name / "gpu_cache";
    std::error_code ec;
    fs::create_directories(folder, ec);
    return folder;
}

static size_t loadCached(const void* key, size_t keySize, void* value, size_t valueSize, void* userdata) {
    bytes_view keyBytes(reinterpret_cast<const uint8_t*>(key), keySize);
    auto hash       = sha256(keyBytes);
    auto valueBytes = readBytes(cacheFolder() / toHex(hash));
    if (!valueBytes) {
        return 0;
    }
    if (valueBytes->size() <= valueSize && value != nullptr) {
        memcpy(value, valueBytes->data(), valueBytes->size());
    }
    return valueBytes->size();
}

static void storeCached(const void* key, size_t keySize, const void* value, size_t valueSize,
                        void* userdata) {
    bytes_view keyBytes(reinterpret_cast<const uint8_t*>(key), keySize);
    bytes_view valueBytes(reinterpret_cast<const uint8_t*>(value), valueSize);
    auto hash   = sha256(keyBytes);
    std::ignore = writeBytes(cacheFolder() / toHex(hash), valueBytes);
}

bool RenderDeviceWebGPU::createDevice() {

    const char* instanceToggles[] = {
        "allow_unsafe_apis",
    };
    wgpu::DawnTogglesDescriptor instanceToggleDesc;
    instanceToggleDesc.enabledToggles     = instanceToggles;
    instanceToggleDesc.enabledToggleCount = std::size(instanceToggles);
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.features.timedWaitAnyEnable   = true;
    instanceDesc.features.timedWaitAnyMaxCount = 1;
    instanceDesc.nextInChain                   = &instanceToggleDesc;

    m_nativeInstance =
        std::make_unique<dawn::native::Instance>(reinterpret_cast<WGPUInstanceDescriptor*>(&instanceDesc));
    wgpuInstanceReference(m_nativeInstance->Get());
    m_instance = wgpu::Instance::Acquire(m_nativeInstance->Get());
    wgpu::RequestAdapterOptions opt;
#ifdef BRISK_WINDOWS
    opt.backendType = wgpu::BackendType::D3D12;
#elif defined BRISK_APPLE
    opt.backendType = wgpu::BackendType::Metal;
#else
    opt.backendType = wgpu::BackendType::Vulkan;
#endif
    opt.powerPreference = staticMap(m_deviceSelection, RendererDeviceSelection::HighPerformance,
                                    wgpu::PowerPreference::HighPerformance, RendererDeviceSelection::LowPower,
                                    wgpu::PowerPreference::LowPower, wgpu::PowerPreference::Undefined);

    std::vector<dawn::native::Adapter> adapters = m_nativeInstance->EnumerateAdapters(&opt);
    if (adapters.empty() && opt.powerPreference != wgpu::PowerPreference::Undefined) {
        opt.powerPreference = wgpu::PowerPreference::Undefined;
        adapters            = m_nativeInstance->EnumerateAdapters(&opt);
    }
    if (adapters.empty())
        return false;
    dawn::native::Adapter adapter = adapters[0];
    wgpuAdapterReference(adapter.Get());
    m_adapter = wgpu::Adapter::Acquire(adapter.Get());

    wgpu::DeviceDescriptor deviceDesc{};
    static const wgpu::FeatureName feat[] = {
        wgpu::FeatureName::DualSourceBlending,
        wgpu::FeatureName::DawnNative,
        wgpu::FeatureName::Float32Filterable,
    };
    deviceDesc.requiredFeatureCount = std::size(feat);
    deviceDesc.requiredFeatures     = feat;

    wgpu::DawnCacheDeviceDescriptor deviceCache{};
    deviceCache.loadDataFunction  = &loadCached;
    deviceCache.storeDataFunction = &storeCached;
    deviceCache.functionUserdata  = nullptr;
    deviceDesc.nextInChain        = &deviceCache;

#if BRISK_DEBUG_GPU
    const char* deviceToggles[] = {
        "dump_shaders",
        "disable_symbol_renaming",
    };
    wgpu::DawnTogglesDescriptor deviceToggleDesc;
    deviceToggleDesc.enabledToggles     = deviceToggles;
    deviceToggleDesc.enabledToggleCount = std::size(deviceToggles);
    deviceCache.nextInChain             = &deviceToggleDesc;
#endif
    WGPUDevice device = adapter.CreateDevice(&deviceDesc);
    if (!device)
        return false;

    wgpuDeviceReference(device);
    m_device = wgpu::Device::Acquire(device);

    BRISK_ASSERT(m_device.HasFeature(wgpu::FeatureName::DawnNative));
    m_device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, const char* message, void* userdata) {
            LOG_ERROR(wgpu, "WGPU Error: {} {}", str(wgpu::ErrorType(type)), message);
            BRISK_ASSERT(false);
        },
        nullptr);
    m_device.SetLoggingCallback(
        [](WGPULoggingType type, char const* message, void* userdata) {
            LOG_INFO(wgpu, "WGPU Info: {} {}", str(wgpu::LoggingType(type)), message);
        },
        nullptr);

    std::vector<wgpu::FeatureName> features;
    size_t featureCount = m_device.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    m_device.EnumerateFeatures(features.data());
    for (wgpu::FeatureName f : features) {
        LOG_INFO(wgpu, "feature {}", str(f));
    }

    return true;
}

RenderDeviceWebGPU::RenderDeviceWebGPU(RendererDeviceSelection deviceSelection)
    : m_deviceSelection(deviceSelection) {}

status<RenderDeviceError> RenderDeviceWebGPU::init() {
    if (!createDevice()) {
        return unexpected(RenderDeviceError::Unsupported);
    }

    auto wgslShader = loadResourceText("webgpu/webgpu.wgsl");

    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = wgslShader.c_str();

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{ .nextInChain = &wgslDesc };
    m_shader                                          = m_device.CreateShaderModule(&shaderModuleDescriptor);

    std::array<wgpu::BindGroupLayoutEntry, 8> entries = {
        wgpu::BindGroupLayoutEntry{
            // constant
            .binding    = 1,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer =
                wgpu::BufferBindingLayout{
                    .type             = wgpu::BufferBindingType::Uniform,
                    .hasDynamicOffset = true,
                    .minBindingSize   = sizeof(RenderState),
                },
        },
        wgpu::BindGroupLayoutEntry{
            // constantPerFrame
            .binding    = 2,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer =
                wgpu::BufferBindingLayout{
                    .type           = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = sizeof(ConstantPerFrame),
                },
        },
        wgpu::BindGroupLayoutEntry{
            // data
            .binding    = 3,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer =
                wgpu::BufferBindingLayout{
                    .type           = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = sizeof(SIMD<float, 4>),
                },
        },
        wgpu::BindGroupLayoutEntry{
            // fontTex_t
            .binding    = 9,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture =
                wgpu::TextureBindingLayout{
                    .sampleType = wgpu::TextureSampleType::Float,
                },
        },
        wgpu::BindGroupLayoutEntry{
            // grad_s
            .binding    = 7,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler =
                wgpu::SamplerBindingLayout{
                    .type = wgpu::SamplerBindingType::Filtering,
                },
        },
        wgpu::BindGroupLayoutEntry{
            // grad_t
            .binding    = 8,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture =
                wgpu::TextureBindingLayout{
                    .sampleType = wgpu::TextureSampleType::Float,
                },
        },
        wgpu::BindGroupLayoutEntry{
            // boundTexture_s
            .binding    = 6,
            .visibility = wgpu::ShaderStage::Fragment,
            .sampler =
                wgpu::SamplerBindingLayout{
                    .type = wgpu::SamplerBindingType::Filtering,
                },
        },
        wgpu::BindGroupLayoutEntry{
            // boundTexture_t
            .binding    = 10,
            .visibility = wgpu::ShaderStage::Fragment,
            .texture =
                wgpu::TextureBindingLayout{
                    .sampleType = wgpu::TextureSampleType::Float,
                },
        },
    };

    // Create a bind group layout
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
    bindGroupLayoutDesc.entryCount        = entries.size();
    bindGroupLayoutDesc.entries           = entries.data();
    m_bindGroupLayout                     = m_device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    m_pipelineLayout.bindGroupLayoutCount = 1;
    m_pipelineLayout.bindGroupLayouts     = &m_bindGroupLayout;

    createSamplers();

    wgpu::SupportedLimits limits;
    if (!m_device.GetLimits(&limits)) {
        return unexpected(RenderDeviceError::Unsupported);
    }

    m_limits.maxGradients = 1024;
    m_limits.maxAtlasSize =
        std::min(limits.limits.maxTextureDimension2D * limits.limits.maxTextureDimension2D, 128u * 1048576u);
    m_limits.maxDataSize = limits.limits.maxBufferSize / sizeof(float);

    m_resources.spriteAtlas.reset(
        new SpriteAtlas(4 * 1048576, m_limits.maxAtlasSize, 4 * 1048576, &m_resources.mutex));

    m_resources.gradientAtlas.reset(new GradientAtlas(m_limits.maxGradients, &m_resources.mutex));

    return {};
}

wgpu::RenderPipeline RenderDeviceWebGPU::createPipeline(wgpu::TextureFormat renderFormat,
                                                        bool dualSourceBlending) {
    if (auto it = m_pipelineCache.find(std::make_tuple(renderFormat, dualSourceBlending));
        it != m_pipelineCache.end()) {
        return it->second;
    }
    wgpu::BlendState blendState{};
    if (dualSourceBlending) {
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrc1;
        blendState.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    } else {
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.alpha.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    }

    wgpu::ColorTargetState colorTargetState{
        .format = renderFormat,
        .blend  = &blendState,
    };

    wgpu::FragmentState fragmentState{
        .module      = m_shader,
        .targetCount = 1,
        .targets     = &colorTargetState,
    };

    wgpu::RenderPipelineDescriptor descriptor{};
    descriptor.layout             = m_device.CreatePipelineLayout(&m_pipelineLayout);
    descriptor.vertex.module      = m_shader;
    descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
    descriptor.fragment           = &fragmentState;
    wgpu::RenderPipeline pipeline = m_device.CreateRenderPipeline(&descriptor);
    m_pipelineCache.insert_or_assign(std::make_tuple(renderFormat, dualSourceBlending), pipeline);
    return pipeline;
}

void RenderDeviceWebGPU::createSamplers() {
    {
        wgpu::TextureDescriptor desc{
            .label  = "DummyTexture",
            .usage  = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
            .size   = wgpu::Extent3D{ 1, 1 },
            .format = wgpu::TextureFormat::RGBA8Unorm,
        };
        m_dummyTexture     = m_device.CreateTexture(&desc);
        m_dummyTextureView = m_dummyTexture.CreateView();
    }
    {
        wgpu::SamplerDescriptor samplerDesc{
            .label     = "GradientSampler",
            .magFilter = wgpu::FilterMode::Linear,
        };
        m_gradientSampler = m_device.CreateSampler(&samplerDesc);
    }
    {
        wgpu::SamplerDescriptor samplerDesc{
            .label        = "BoundTextureSampler",
            .addressModeU = wgpu::AddressMode::Repeat,
            .addressModeV = wgpu::AddressMode::Repeat,
            .addressModeW = wgpu::AddressMode::Repeat,
            .magFilter    = wgpu::FilterMode::Linear,
            .minFilter    = wgpu::FilterMode::Linear,
        };
        m_boundSampler = m_device.CreateSampler(&samplerDesc);
    }
}

static const std::string_view wgpuBackends[] = {
    "Undefined", "Null", "WebGPU", "D3D11", "D3D12", "Metal", "Vulkan", "OpenGL", "OpenGLES",
};

RenderDeviceInfo RenderDeviceWebGPU::info() const {
    wgpu::AdapterProperties props;
    m_adapter.GetProperties(&props);
    RenderDeviceInfo info;
    info.api        = "WebGPU/" + std::string(wgpuBackends[uint32_t(props.backendType)]);
    info.apiVersion = 0;
    info.vendor     = props.vendorName;
    info.device     = std::string(props.name) + "/" + props.driverDescription;
    return info;
}

RC<WindowRenderTarget> RenderDeviceWebGPU::createWindowTarget(const OSWindow* window, PixelType type,
                                                              DepthStencilType depthStencil, int samples) {
    return rcnew WindowRenderTargetWebGPU(shared_from_this(), window, type, depthStencil, samples);
}

RC<ImageRenderTarget> RenderDeviceWebGPU::createImageTarget(Size frameSize, PixelType type,
                                                            DepthStencilType depthStencil, int samples) {
    return rcnew ImageRenderTargetWebGPU(shared_from_this(), frameSize, type, depthStencil, samples);
}

RC<RenderEncoder> RenderDeviceWebGPU::createEncoder() {
    return rcnew RenderEncoderWebGPU(shared_from_this());
}

RenderDeviceWebGPU::~RenderDeviceWebGPU() {
    m_instance.ProcessEvents();
}

bool RenderDeviceWebGPU::updateBackBuffer(BackBufferWebGPU& buffer, PixelType type,
                                          DepthStencilType depthType, int samples) {
    buffer.colorView = buffer.color.CreateView();
    return true;
}

void RenderDeviceWebGPU::wait() {
    wgpu::FutureWaitInfo future;
    future.future = m_device.GetQueue().OnSubmittedWorkDone(wgpu::QueueWorkDoneCallbackInfo{
        .callback = [](WGPUQueueWorkDoneStatus status, void* userdata) {},
        .userdata = nullptr,
    });
    m_instance.WaitAny(1, &future, 1'000'000'000); // 1 second
}

void RenderDeviceWebGPU::createImageBackend(RC<Image> image) {
    BRISK_ASSERT(image);
    if (wgFormat(image->pixelType(), image->pixelFormat()) == wgpu::TextureFormat::Undefined) {
        throwException(EImageError("WebGPU backend does not support the image type or format: {}, {}. "
                                   "Consider converting the image before sending it to the GPU.",
                                   image->pixelType(), image->pixelFormat()));
    }
    std::ignore = getOrCreateBackend(shared_from_this(), std::move(image), true, false);
}

RenderLimits RenderDeviceWebGPU::limits() const {
    return m_limits;
}
} // namespace Brisk
