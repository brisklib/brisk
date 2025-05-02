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
#define BRISK_ALLOW_OS_HEADERS
#include "RenderDevice.hpp"
#include "WindowRenderTarget.hpp"
#include "ImageRenderTarget.hpp"
#include "RenderEncoder.hpp"
#include "ImageBackend.hpp"
#include <brisk/core/Cryptography.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Log.hpp>

#ifdef BRISK_WINDOWS
#include "../AdapterForMonitor.hpp"
#include <dawn/native/D3DBackend.h>
#endif

namespace Brisk {

static fs::path gpuCacheFolder() {
    fs::path folder = defaultFolder(DefaultFolder::AppUserData) / "gpu_cache";
    std::error_code ec;
    fs::create_directories(folder, ec);
    return folder;
}

static size_t loadCached(const void* key, size_t keySize, void* value, size_t valueSize, void* userdata) {
    BytesView keyBytes(reinterpret_cast<const std::byte*>(key), keySize);
    auto hash       = sha256(keyBytes);
    auto valueBytes = readBytes(gpuCacheFolder() / toHex(hash));
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
    BytesView keyBytes(reinterpret_cast<const std::byte*>(key), keySize);
    BytesView valueBytes(reinterpret_cast<const std::byte*>(value), valueSize);
    auto hash   = sha256(keyBytes);
    std::ignore = writeBytes(gpuCacheFolder() / toHex(hash), valueBytes);
}

bool RenderDeviceWebGpu::createDevice() {

    const char* instanceToggles[] = {
        "allow_unsafe_apis",
    };
    wgpu::DawnTogglesDescriptor instanceToggleDesc;
    instanceToggleDesc.enabledToggles     = instanceToggles;
    instanceToggleDesc.enabledToggleCount = std::size(instanceToggles);
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.capabilities.timedWaitAnyEnable   = true;
    instanceDesc.capabilities.timedWaitAnyMaxCount = 1;
    instanceDesc.nextInChain                       = &instanceToggleDesc;

    m_nativeInstance =
        std::make_unique<dawn::native::Instance>(reinterpret_cast<WGPUInstanceDescriptor*>(&instanceDesc));

    wgpu::RequestAdapterOptions opt;

#ifdef BRISK_WINDOWS
    opt.backendType = wgpu::BackendType::D3D12;

    dawn::native::d3d::RequestAdapterOptionsLUID optLUID;
    if (m_display) {
        ComPtr<IDXGIFactory> factory;
        HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
        if (SUCCEEDED(hr)) {
            ComPtr<IDXGIAdapter> adapter = adapterForMonitor(m_display.hMonitor(), factory);

            if (adapter) {
                DXGI_ADAPTER_DESC adapterDesc;
                adapter->GetDesc(&adapterDesc);
                optLUID.adapterLUID = adapterDesc.AdapterLuid;
                opt.nextInChain     = &optLUID;
            }
        }
    }

#elif defined BRISK_APPLE
    opt.backendType = wgpu::BackendType::Metal;
#else
    opt.backendType = wgpu::BackendType::Vulkan;
#endif
    opt.powerPreference = staticMap(m_deviceSelection, RendererDeviceSelection::HighPerformance,
                                    wgpu::PowerPreference::HighPerformance, RendererDeviceSelection::LowPower,
                                    wgpu::PowerPreference::LowPower, wgpu::PowerPreference::Undefined);

    std::vector<dawn::native::Adapter> adapters = m_nativeInstance->EnumerateAdapters(&opt);
    if (adapters.empty() &&
        (opt.powerPreference != wgpu::PowerPreference::Undefined || opt.nextInChain != nullptr)) {
        opt.powerPreference = wgpu::PowerPreference::Undefined;
        opt.nextInChain     = nullptr;
        adapters            = m_nativeInstance->EnumerateAdapters(&opt);
    }

#ifdef BRISK_DEBUG_GPU
    for (size_t i = 0; i < adapters.size(); ++i) {
        wgpu::AdapterInfo info;
        wgpu::Adapter(adapters[i].Get()).GetInfo(&info);
        LOG_INFO(wgpu, "GPU adapter [{}] {} {} {} {:08X}:{:08X}", i, std::string_view(info.vendor),
                 std::string_view(info.device), std::string_view(info.description), info.vendorID,
                 info.deviceID);
    }
    if (adapters.empty()) {
        LOG_WARN(wgpu, "No GPU adapters found");
    }
#endif

    if (adapters.empty())
        return false;
    m_nativeAdapter = std::move(adapters.front());

    adapters.clear();
    m_adapter = wgpu::Adapter::Acquire(m_nativeAdapter.Get());

    wgpu::DeviceDescriptor deviceDesc{};
    static const wgpu::FeatureName feat[] = {
        wgpu::FeatureName::DualSourceBlending, wgpu::FeatureName::DawnNative,
        wgpu::FeatureName::Float32Filterable,  wgpu::FeatureName::Float32Blendable,
        wgpu::FeatureName::TimestampQuery, // < Must be last (optional)
    };
    deviceDesc.requiredFeatureCount = std::size(feat);
    deviceDesc.requiredFeatures     = feat;
    m_timestampQuerySupported       = m_adapter.HasFeature(wgpu::FeatureName::TimestampQuery);
    if (!m_timestampQuerySupported) {
        --deviceDesc.requiredFeatureCount;
    }

    wgpu::DawnCacheDeviceDescriptor deviceCache{};
    deviceCache.loadDataFunction  = &loadCached;
    deviceCache.storeDataFunction = &storeCached;
    deviceCache.functionUserdata  = nullptr;
    deviceDesc.nextInChain        = &deviceCache;

    wgpu::DawnTogglesDescriptor deviceToggleDesc;
#ifdef BRISK_DEBUG_GPU
    const char* deviceTogglesEnable[] = {
        "dump_shaders",
        "disable_symbol_renaming",
    };
    deviceToggleDesc.enabledToggles     = deviceTogglesEnable;
    deviceToggleDesc.enabledToggleCount = std::size(deviceTogglesEnable);
#endif
    const char* deviceTogglesDisable[]   = { "timestamp_quantization" };
    deviceToggleDesc.disabledToggles     = deviceTogglesDisable;
    deviceToggleDesc.disabledToggleCount = std::size(deviceTogglesDisable);

    deviceCache.nextInChain              = &deviceToggleDesc;
    deviceDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            LOG_ERROR(wgpu, "WGPU Error: {} {}", str(type), std::string_view(message));
            BRISK_ASSERT(false);
        });
    m_device = m_adapter.CreateDevice(&deviceDesc);
    if (!m_device)
        return false;

    BRISK_ASSERT(m_device.HasFeature(wgpu::FeatureName::DawnNative));
    m_device.SetLoggingCallback([](wgpu::LoggingType type, wgpu::StringView message) {
        LOG_INFO(wgpu, "WGPU Info: {} {}", str(type), std::string_view(message));
    });

    m_instance = wgpu::Instance::Acquire(m_nativeInstance->Get());
    m_instance.ProcessEvents();

#if 0
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = m_device.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    m_device.EnumerateFeatures(features.data());
    for (wgpu::FeatureName f : features) {
        LOG_INFO(wgpu, "feature {}", str(f));
    }
#endif

    return true;
}

RenderDeviceWebGpu::RenderDeviceWebGpu(RendererDeviceSelection deviceSelection, OsDisplayHandle display)
    : m_deviceSelection(deviceSelection), m_display(display) {}

status<RenderDeviceError> RenderDeviceWebGpu::init() {
    if (!createDevice()) {
        return unexpected(RenderDeviceError::Unsupported);
    }

    auto wgslShader = Resources::loadText("webgpu/webgpu.wgsl");

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
                    .minBindingSize = sizeof(Simd<float, 4>),
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

    wgpu::Limits limits;
    if (!m_device.GetLimits(&limits)) {
        return unexpected(RenderDeviceError::Unsupported);
    }

    m_limits.maxGradients = 1024;
    m_limits.maxAtlasSize =
        std::min(limits.maxTextureDimension2D * limits.maxTextureDimension2D, 128u * 1048576u);
    m_limits.maxDataSize = limits.maxBufferSize / sizeof(float);

    m_resources.spriteAtlas.reset(
        new SpriteAtlas(256 * 1024, m_limits.maxAtlasSize, 256 * 1024, &m_resources.mutex));

    m_resources.gradientAtlas.reset(new GradientAtlas(m_limits.maxGradients, &m_resources.mutex));

    return {};
}

wgpu::RenderPipeline RenderDeviceWebGpu::createPipeline(wgpu::TextureFormat renderFormat,
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

void RenderDeviceWebGpu::createSamplers() {
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

RenderDeviceInfo RenderDeviceWebGpu::info() const {
    wgpu::AdapterInfo props;
    m_adapter.GetInfo(&props);
    RenderDeviceInfo info;
    info.api        = "WebGPU/" + std::string(wgpuBackends[uint32_t(props.backendType)]);
    info.apiVersion = 0;
    info.vendor     = props.vendor;
    info.device = fmt::format("{}/{}", std::string_view(props.device), std::string_view(props.description));
    return info;
}

Rc<WindowRenderTarget> RenderDeviceWebGpu::createWindowTarget(const OsWindow* window, PixelType type,
                                                              DepthStencilType depthStencil, int samples) {
    return rcnew WindowRenderTargetWebGpu(shared_from_this(), window, type, depthStencil, samples);
}

Rc<ImageRenderTarget> RenderDeviceWebGpu::createImageTarget(Size frameSize, PixelType type,
                                                            DepthStencilType depthStencil, int samples) {
    return rcnew ImageRenderTargetWebGpu(shared_from_this(), frameSize, type, depthStencil, samples);
}

Rc<RenderEncoder> RenderDeviceWebGpu::createEncoder() {
    return rcnew RenderEncoderWebGpu(shared_from_this());
}

RenderDeviceWebGpu::~RenderDeviceWebGpu() {
    m_resources.reset();
    m_pipelineCache          = {};
    m_device                 = nullptr;
    m_shader                 = nullptr;
    m_atlasSampler           = nullptr;
    m_gradientSampler        = nullptr;
    m_boundSampler           = nullptr;
    m_perFrameConstantBuffer = nullptr;
    m_bindGroupLayout        = nullptr;
    m_dummyTexture           = nullptr;
    m_dummyTextureView       = nullptr;

    m_instance.ProcessEvents();

    std::ignore     = m_adapter.MoveToCHandle(); // Avoid reference decrement
    m_nativeAdapter = nullptr;

    std::ignore     = m_instance.MoveToCHandle(); // Avoid reference decrement
    m_nativeInstance.reset();
}

bool RenderDeviceWebGpu::updateBackBuffer(BackBufferWebGpu& buffer, PixelType type,
                                          DepthStencilType depthType, int samples) {
    buffer.colorView = buffer.color.CreateView();
    return true;
}

void RenderDeviceWebGpu::wait() {
    wgpu::FutureWaitInfo future;
    future.future = m_device.GetQueue().OnSubmittedWorkDone(wgpu::CallbackMode::AllowProcessEvents,
                                                            [](wgpu::QueueWorkDoneStatus status) {});
    m_instance.ProcessEvents();
    m_instance.WaitAny(1, &future, 1'000'000'000); // 1 second
    m_instance.ProcessEvents();
}

void RenderDeviceWebGpu::createImageBackend(Rc<Image> image) {
    BRISK_ASSERT(image);
    if (wgFormat(image->pixelType(), image->pixelFormat()) == wgpu::TextureFormat::Undefined) {
        throwException(EImageError("WebGPU backend does not support the image type or format: {}, {}. "
                                   "Consider converting the image before sending it to the GPU.",
                                   image->pixelType(), image->pixelFormat()));
    }
    std::ignore = getOrCreateBackend(shared_from_this(), std::move(image), true, false);
}

RenderLimits RenderDeviceWebGpu::limits() const {
    return m_limits;
}
} // namespace Brisk
