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
#include "RenderEncoder.hpp"
#include "ImageBackend.hpp"
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Log.hpp>
#include "../Atlas.hpp"
#include <brisk/core/Threading.hpp>
#include "ImageRenderTarget.hpp"
#include "WindowRenderTarget.hpp"

namespace Brisk {

VisualSettings RenderEncoderWebGpu::visualSettings() const {
    return m_visualSettings;
}

void RenderEncoderWebGpu::setVisualSettings(const VisualSettings& visualSettings) {
    m_visualSettings = visualSettings;
}

void RenderEncoderWebGpu::begin(Rc<RenderTarget> target, std::optional<ColorF> clear) {
    BRISK_ASSERT(static_cast<bool>(m_currentTarget == nullptr));
    BRISK_ASSERT(static_cast<bool>(!m_queue.Get()));
    m_currentTarget = std::move(target);
    m_queue         = m_device->m_device.GetQueue();
    BRISK_ASSERT(static_cast<bool>(m_queue.Get()));
    m_frameSize = m_currentTarget->size();
    if (m_currentTarget->type() == RenderTargetType::Window) {
        static_cast<WindowRenderTarget*>(m_currentTarget.get())->resizeBackbuffer(m_frameSize);
    }

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

    const BackBufferWebGpu& backBuf = getBackBuffer(m_currentTarget.get());

    wgpu::RenderPassColorAttachment renderPassColorAttachment{
        .view    = backBuf.colorView,
        .loadOp  = clear ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load,
        .storeOp = wgpu::StoreOp::Store,
    };
    clear                                = clear.value_or(ColorF{});
    renderPassColorAttachment.clearValue = wgpu::Color{ clear->r, clear->g, clear->b, clear->a },

    m_colorAttachment                    = renderPassColorAttachment;
    m_renderFormat                       = backBuf.color.GetFormat();
}

void RenderEncoderWebGpu::end() {
    BRISK_ASSERT(m_currentTarget);
    BRISK_ASSERT(m_queue.Get());
    m_queue         = nullptr;
    m_currentTarget = nullptr;
}

void RenderEncoderWebGpu::batch(std::span<const RenderState> commands, std::span<const uint32_t> data) {
    BRISK_ASSERT(m_currentTarget);
    BRISK_ASSERT(m_queue.Get());

    bool uploadResources = requiresAtlasOrGradient(commands);

    // Preparing things
    if (uploadResources || !m_atlasTexture || !m_gradientTexture) {
        std::lock_guard lk(m_device->m_resources.mutex);
        updateAtlasTexture();
        updateGradientTexture();
    }
    updateConstantBuffer(commands);
    updateDataBuffer(data);

    // Starting render pass
    wgpu::RenderPassDescriptor renderpass{
        .colorAttachmentCount = 1,
        .colorAttachments     = &m_colorAttachment,
    };
    m_encoder = m_device->m_device.CreateCommandEncoder();

    if (m_device->m_timestampQuerySupported && m_frameTimingIndex < m_frameTiming.size() &&
        m_timestampIndex < maxTimestamps)
        m_encoder.WriteTimestamp(m_frameTiming[m_frameTimingIndex].querySet, m_timestampIndex++);

    m_pass                        = m_encoder.BeginRenderPass(&renderpass);
    wgpu::RenderPipeline pipeline = m_device->createPipeline(m_renderFormat, true);
    m_pass.SetPipeline(pipeline);

    // Actual rendering
    Internal::ImageBackend* savedTexture = nullptr;
    // OPTIMIZATION: separate groups for constants and texture
    wgpu::BindGroup bindGroup;

    Rectangle frameRect       = Rectangle({}, m_frameSize);
    Rectangle currentClipRect = noClipRect;

    for (size_t i = 0; i < commands.size(); ++i) {
        const RenderState& cmd = commands[i];
        Rectangle clampedRect  = cmd.scissor.intersection(frameRect);
        if (clampedRect.empty())
            continue;
        const uint32_t offs[] = {
            uint32_t(i * sizeof(RenderState)),
        };

        if (!bindGroup || cmd.imageBackend != savedTexture) {
            savedTexture = cmd.imageBackend;
            bindGroup    = createBindGroup(static_cast<ImageBackendWebGpu*>(cmd.imageBackend));
        }

        if (clampedRect != currentClipRect) {
            m_pass.SetScissorRect(clampedRect.x1, clampedRect.y1, clampedRect.width(), clampedRect.height());
            currentClipRect = clampedRect;
        }

        m_pass.SetBindGroup(0, bindGroup, 1, offs);
        m_pass.Draw(4, cmd.instances);
    }

    // Finishing things
    m_pass.End();
    m_pass = nullptr;
    if (m_device->m_timestampQuerySupported && m_frameTimingIndex < m_frameTiming.size() &&
        m_timestampIndex < maxTimestamps)
        m_encoder.WriteTimestamp(m_frameTiming[m_frameTimingIndex].querySet, m_timestampIndex++);

    wgpu::CommandBuffer commandBuffer = m_encoder.Finish();
    m_queue.Submit(1, &commandBuffer);
    m_encoder                = nullptr;

    m_colorAttachment.loadOp = wgpu::LoadOp::Load;
}

wgpu::BindGroup RenderEncoderWebGpu::createBindGroup(ImageBackendWebGpu* imageBackend) {

    std::array<wgpu::BindGroupEntry, 8> entries = {
        wgpu::BindGroupEntry{
            .binding = 1,
            .buffer  = m_constantBuffer,
            .size    = sizeof(RenderState),
        },
        wgpu::BindGroupEntry{
            .binding = 2,
            .buffer  = m_perFrameConstantBuffer,
        },
        wgpu::BindGroupEntry{
            .binding = 3,
            .buffer  = m_dataBuffer,
        },
        wgpu::BindGroupEntry{
            .binding     = 9,
            .textureView = m_atlasTextureView,
        },
        wgpu::BindGroupEntry{
            .binding = 7,
            .sampler = m_device->m_gradientSampler,
        },
        wgpu::BindGroupEntry{
            .binding     = 8,
            .textureView = m_gradientTextureView,
        },
        wgpu::BindGroupEntry{
            .binding = 6,
            .sampler = m_device->m_boundSampler,
        },
        wgpu::BindGroupEntry{
            .binding     = 10,
            .textureView = imageBackend ? imageBackend->m_textureView : m_device->m_dummyTextureView,
        },
    };
    wgpu::BindGroupDescriptor bingGroupDesc{
        .layout     = m_device->m_bindGroupLayout,
        .entryCount = entries.size(),
        .entries    = entries.data(),
    };
    return m_device->m_device.CreateBindGroup(&bingGroupDesc);
}

void RenderEncoderWebGpu::wait() {
    m_device->wait();
}

void RenderEncoderWebGpu::updatePerFrameConstantBuffer(const ConstantPerFrame& constants) {
    if (!m_perFrameConstantBuffer) {
        wgpu::BufferDescriptor desc{
            .label = "PerFrameConstantBuffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size  = sizeof(constants),
        };
        m_perFrameConstantBuffer = m_device->m_device.CreateBuffer(&desc);
    }
    m_queue.WriteBuffer(m_perFrameConstantBuffer, 0,
                        reinterpret_cast<const uint8_t*>(std::addressof(constants)), sizeof(constants));
}

void RenderEncoderWebGpu::updateConstantBuffer(std::span<const RenderState> data) {
    if (!m_constantBuffer || m_constantBuffer.GetSize() != data.size_bytes()) {
        wgpu::BufferDescriptor desc{
            .label = "ConstantBuffer",
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size  = data.size_bytes(),
        };
        m_constantBuffer = m_device->m_device.CreateBuffer(&desc);
    }
    m_queue.WriteBuffer(m_constantBuffer, 0, reinterpret_cast<const uint8_t*>(data.data()),
                        data.size_bytes());
}

// Update the data buffer and possibly recreate it.
void RenderEncoderWebGpu::updateDataBuffer(std::span<const uint32_t> data) {
    size_t alignedDataSize = std::max(data.size_bytes(), size_t(16));
    if (!m_dataBuffer || alignedDataSize > m_dataBuffer.GetSize()) {
        wgpu::BufferDescriptor desc{
            .label = "DataBuffer",
            .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
            .size  = alignedDataSize,
        };
        m_dataBuffer = m_device->m_device.CreateBuffer(&desc);
    }
    if (!data.empty())
        m_queue.WriteBuffer(m_dataBuffer, 0, reinterpret_cast<const uint8_t*>(data.data()),
                            data.size_bytes());
}

void RenderEncoderWebGpu::updateAtlasTexture() {
    SpriteAtlas* atlas = m_device->m_resources.spriteAtlas.get();
    Size newSize(Internal::max2DTextureSize, atlas->data().size() / Internal::max2DTextureSize);

    if (!m_atlasTexture || (m_atlas_generation <<= atlas->changed)) {
        if (!m_atlasTexture || newSize != Size(m_atlasTexture.GetWidth(), m_atlasTexture.GetHeight())) {
            wgpu::TextureFormat fmt = wgFormat(PixelType::U8, PixelFormat::Greyscale);
            wgpu::TextureDescriptor desc{
                .label  = "AtlasTexture",
                .usage  = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
                .size   = wgpu::Extent3D{ uint32_t(newSize.width), uint32_t(newSize.height) },
                .format = fmt,
            };
            m_atlasTexture = m_device->m_device.CreateTexture(&desc);
            wgpu::TextureViewDescriptor viewDesc{};
            viewDesc.dimension = wgpu::TextureViewDimension::e2D;
            viewDesc.format    = fmt;
            m_atlasTextureView = m_atlasTexture.CreateView(&viewDesc);
        }

        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = m_atlasTexture;
        wgpu::TexelCopyBufferLayout source{};
        source.bytesPerRow = Internal::max2DTextureSize;
        wgpu::Extent3D texSize{ uint32_t(newSize.width), uint32_t(newSize.height), 1u };
        m_queue.WriteTexture(&destination, atlas->data().data(), atlas->data().size(), &source, &texSize);
    }
}

void RenderEncoderWebGpu::updateGradientTexture() {
    GradientAtlas* atlas = m_device->m_resources.gradientAtlas.get();
    Size newSize(gradientResolution, atlas->data().size());
    if (!m_gradientTexture || (m_gradient_generation <<= atlas->changed)) {
        if (!m_gradientTexture ||
            newSize != Size(m_gradientTexture.GetWidth(), m_gradientTexture.GetHeight())) {
            wgpu::TextureFormat fmt = wgFormat(PixelType::F32, PixelFormat::RGBA);
            wgpu::TextureDescriptor desc{
                .label  = "GradientTexture",
                .usage  = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
                .size   = wgpu::Extent3D{ uint32_t(newSize.width), uint32_t(newSize.height) },
                .format = fmt,
            };
            m_gradientTexture = m_device->m_device.CreateTexture(&desc);
            wgpu::TextureViewDescriptor viewDesc{};
            viewDesc.dimension    = wgpu::TextureViewDimension::e2D;
            viewDesc.format       = fmt;
            m_gradientTextureView = m_gradientTexture.CreateView(&viewDesc);
        }

        wgpu::TexelCopyTextureInfo destination{};
        destination.texture = m_gradientTexture;
        wgpu::TexelCopyBufferLayout source{};
        source.bytesPerRow = sizeof(GradientData);
        wgpu::Extent3D texSize{ uint32_t(newSize.width), uint32_t(newSize.height), 1u };
        m_queue.WriteTexture(&destination, atlas->data().data(), atlas->data().size() * sizeof(GradientData),
                             &source, &texSize);
    }
}

RenderEncoderWebGpu::RenderEncoderWebGpu(Rc<RenderDeviceWebGpu> device) : m_device(std::move(device)) {}

RenderEncoderWebGpu::~RenderEncoderWebGpu() {
    m_device->m_instance.ProcessEvents();
}

size_t RenderEncoderWebGpu::findFrameTimingSlot() {
    for (size_t i = 0; i < m_frameTiming.size(); ++i) {
        if (!m_frameTiming[i].pending) {
            m_frameTiming[i].pending = true;
            return i;
        }
    }
    BRISK_ASSERT_MSG("All frame timing slots are busy", m_frameTiming.size() < maxFrameTimings);

    m_frameTiming.emplace_back(m_device->m_device);
    return m_frameTiming.size() - 1;
}

void RenderEncoderWebGpu::beginFrame(uint64_t frameId) {
    if (m_device->m_timestampQuerySupported) {
        m_frameId          = frameId;
        m_timestampIndex   = 0;
        m_frameTimingIndex = findFrameTimingSlot();
    }
}

void RenderEncoderWebGpu::endFrame(DurationCallback callback) {
    if (m_device->m_timestampQuerySupported && m_timestampIndex > 0) {
        BRISK_ASSERT(m_frameTimingIndex < m_frameTiming.size());
        FrameTiming& timing = m_frameTiming[m_frameTimingIndex];

        m_encoder           = m_device->m_device.CreateCommandEncoder();
        m_encoder.ResolveQuerySet(timing.querySet, 0, maxTimestamps, timing.resolveBuffer, 0);
        m_encoder.CopyBufferToBuffer(timing.resolveBuffer, 0, timing.resultBuffer, 0,
                                     maxTimestamps * sizeof(uint64_t));
        wgpu::CommandBuffer commands = m_encoder.Finish();
        m_device->m_device.GetQueue().Submit(1, &commands);

        struct CallbackData {
            std::weak_ptr<void> flag;
            DurationCallback callback;
            FrameTiming* frameTiming;
            uint64_t frameId;
            uint32_t numTimestamps;
        };

        CallbackData* callbackData =
            new CallbackData{ m_flag, std::move(callback), &timing, m_frameId, m_timestampIndex };

        timing.resultBuffer.MapAsync(
            wgpu::MapMode::Read, 0, maxTimestamps * sizeof(uint64_t), wgpu::CallbackMode::AllowProcessEvents,
            [](wgpu::MapAsyncStatus status, wgpu::StringView, CallbackData* callbackData) {
                if (auto lk = callbackData->flag.lock(); !lk) {
                    return;
                }
                if (status == wgpu::MapAsyncStatus::Success) {
                    std::span<const std::chrono::nanoseconds> timestamps{
                        static_cast<const std::chrono::nanoseconds*>(
                            callbackData->frameTiming->resultBuffer.GetConstMappedRange()),
                        callbackData->numTimestamps,
                    };
                    std::array<std::chrono::nanoseconds, maxDurations> durations;
                    for (size_t i = 0; i < callbackData->numTimestamps / 2; ++i) {
                        durations[i] = timestamps[i * 2 + 1] - timestamps[i * 2];
                    }
                    Internal::suppressExceptions(
                        callbackData->callback, callbackData->frameId,
                        std::span{ durations }.subspan(0, callbackData->numTimestamps / 2));

                    callbackData->frameTiming->resultBuffer.Unmap();
                } else {
                    BRISK_LOG_WARN("Frame {} failed to map: {}", callbackData->frameId, (uint32_t)status);
                }
                callbackData->frameTiming->pending = false; // Mark as complete
                delete callbackData;
            },
            callbackData);
    }
}

Rc<RenderTarget> RenderEncoderWebGpu::currentTarget() const {
    return m_currentTarget;
}

RenderEncoderWebGpu::FrameTiming::FrameTiming(wgpu::Device& device) {
    wgpu::QuerySetDescriptor querySetDesc{};
    querySetDesc.type  = wgpu::QueryType::Timestamp;
    querySetDesc.count = RenderEncoderWebGpu::maxTimestamps;
    querySet           = device.CreateQuerySet(&querySetDesc);

    // Resolve buffer
    wgpu::BufferDescriptor resolveDesc{};
    resolveDesc.size  = RenderEncoderWebGpu::maxTimestamps * sizeof(uint64_t);
    resolveDesc.usage = wgpu::BufferUsage::QueryResolve | wgpu::BufferUsage::CopySrc;
    resolveBuffer     = device.CreateBuffer(&resolveDesc);

    // Result buffer
    wgpu::BufferDescriptor resultDesc{};
    resultDesc.size  = RenderEncoderWebGpu::maxTimestamps * sizeof(uint64_t);
    resultDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    resultBuffer     = device.CreateBuffer(&resultDesc);

    pending          = true;
}

const BackBufferWebGpu& getBackBuffer(RenderTarget* target) {
    switch (target->type()) {
    case RenderTargetType::Window:
        return static_cast<WindowRenderTargetWebGpu*>(target)->getBackBuffer();
    case RenderTargetType::Image:
        return static_cast<ImageRenderTargetWebGpu*>(target)->getBackBuffer();
    default:
        BRISK_UNREACHABLE();
    }
}
} // namespace Brisk
