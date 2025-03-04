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

namespace Brisk {

VisualSettings RenderEncoderWebGPU::visualSettings() const {
    return m_visualSettings;
}

void RenderEncoderWebGPU::setVisualSettings(const VisualSettings& visualSettings) {
    m_visualSettings = visualSettings;
}

void RenderEncoderWebGPU::begin(RC<RenderTarget> target, std::optional<ColorF> clear) {
    m_currentTarget = std::move(target);
    m_queue         = m_device->m_device.GetQueue();
    m_frameSize     = m_currentTarget->size();
    if (auto win = std::dynamic_pointer_cast<WindowRenderTarget>(m_currentTarget)) {
        win->resizeBackbuffer(m_frameSize);
    }
    {
        std::lock_guard lk(m_device->m_resources.mutex);
        updateAtlasTexture();
        updateGradientTexture();
    }

    [[maybe_unused]] ConstantPerFrame constantPerFrame{
        SIMD<float, 4>(m_frameSize.width, m_frameSize.height, 1.f / m_frameSize.width,
                       1.f / m_frameSize.height),
        m_visualSettings.blueLightFilter,
        m_visualSettings.gamma,
        Internal::textRectPadding,
        Internal::textRectOffset,
        Internal::max2DTextureSize,
    };

    updatePerFrameConstantBuffer(constantPerFrame);

    const BackBufferWebGPU& backBuf =
        dynamic_cast<BackBufferProviderWebGPU*>(m_currentTarget.get())->getBackBuffer();

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

void RenderEncoderWebGPU::end() {
    m_queue         = nullptr;
    m_currentTarget = nullptr;
}

void RenderEncoderWebGPU::batch(std::span<const RenderState> commands, std::span<const float> data) {
    // Preparing things
    m_encoder = m_device->m_device.CreateCommandEncoder();
    {
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
        const uint32_t offs[]  = {
            uint32_t(i * sizeof(RenderState)),
        };

        if (!bindGroup || cmd.imageBackend != savedTexture) {
            savedTexture = cmd.imageBackend;
            bindGroup    = createBindGroup(static_cast<ImageBackendWebGPU*>(cmd.imageBackend));
        }

        Rectangle clampedRect = cmd.shaderClip.intersection(frameRect);
        if (clampedRect != currentClipRect) {
            m_pass.SetScissorRect(clampedRect.x1, clampedRect.y1, clampedRect.width(), clampedRect.height());
            currentClipRect = clampedRect;
        }

        m_pass.SetBindGroup(0, bindGroup, 1, offs);
        m_pass.Draw(4, cmd.instances);
    }

    // Finishing things
    m_pass.End();
    m_pass                            = nullptr;
    wgpu::CommandBuffer commandBuffer = m_encoder.Finish();
    m_queue.Submit(1, &commandBuffer);
    m_encoder                = nullptr;

    m_colorAttachment.loadOp = wgpu::LoadOp::Load;
}

wgpu::BindGroup RenderEncoderWebGPU::createBindGroup(ImageBackendWebGPU* imageBackend) {

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

void RenderEncoderWebGPU::wait() {
    m_device->wait();
}

void RenderEncoderWebGPU::updatePerFrameConstantBuffer(const ConstantPerFrame& constants) {
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

void RenderEncoderWebGPU::updateConstantBuffer(std::span<const RenderState> data) {
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
void RenderEncoderWebGPU::updateDataBuffer(std::span<const float> data) {
    if (!m_dataBuffer || m_dataBuffer.GetSize() != data.size_bytes()) {
        wgpu::BufferDescriptor desc{
            .label = "DataBuffer",
            .usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst,
            .size  = data.size_bytes(),
        };
        m_dataBuffer = m_device->m_device.CreateBuffer(&desc);
    }
    m_queue.WriteBuffer(m_dataBuffer, 0, reinterpret_cast<const uint8_t*>(data.data()), data.size_bytes());
}

void RenderEncoderWebGPU::updateAtlasTexture() {
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

        wgpu::ImageCopyTexture destination{};
        destination.texture = m_atlasTexture;
        wgpu::TextureDataLayout source{};
        source.bytesPerRow = Internal::max2DTextureSize;
        wgpu::Extent3D texSize{ uint32_t(newSize.width), uint32_t(newSize.height), 1u };
        m_queue.WriteTexture(&destination, atlas->data().data(), atlas->data().size(), &source, &texSize);
    }
}

void RenderEncoderWebGPU::updateGradientTexture() {
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

        wgpu::ImageCopyTexture destination{};
        destination.texture = m_gradientTexture;
        wgpu::TextureDataLayout source{};
        source.bytesPerRow = sizeof(GradientData);
        wgpu::Extent3D texSize{ uint32_t(newSize.width), uint32_t(newSize.height), 1u };
        m_queue.WriteTexture(&destination, atlas->data().data(), atlas->data().size() * sizeof(GradientData),
                             &source, &texSize);
    }
}

RenderEncoderWebGPU::RenderEncoderWebGPU(RC<RenderDeviceWebGPU> device) : m_device(std::move(device)) {}

RenderEncoderWebGPU::~RenderEncoderWebGPU() = default;

} // namespace Brisk
