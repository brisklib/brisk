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

#include "RenderDevice.hpp"

namespace Brisk {

class WindowRenderTargetWebGpu;
class ImageRenderTargetWebGpu;

const BackBufferWebGpu& getBackBuffer(RenderTarget* target);

class RenderEncoderWebGpu final : public RenderEncoder {
public:
    RenderDevice* device() const final {
        return m_device.get();
    }

    VisualSettings visualSettings() const final;
    void setVisualSettings(const VisualSettings& visualSettings) final;

    void begin(Rc<RenderTarget> target, std::optional<ColorF> clear = Palette::transparent) final;
    void batch(std::span<const RenderState> commands, std::span<const uint32_t> data) final;
    void end() final;
    void wait() final;
    Rc<RenderTarget> currentTarget() const;
    void beginFrame(uint64_t frameId);
    void endFrame(DurationCallback callback);

    explicit RenderEncoderWebGpu(Rc<RenderDeviceWebGpu> device);
    ~RenderEncoderWebGpu();

    constexpr static size_t maxTimestamps = maxDurations * 2;

private:
    friend bool webgpuFromContext(RenderContext&, wgpu::Device&, wgpu::TextureView&);

    struct FrameTiming {
        wgpu::QuerySet querySet;    // Timestamp queries for this frame
        wgpu::Buffer resolveBuffer; // Resolved timestamps (GPU-only)
        wgpu::Buffer resultBuffer;  // Mappable buffer for CPU access
        bool pending  = false;      // Whether results are still pending

        FrameTiming() = default;

        explicit FrameTiming(wgpu::Device& device);
    };

    Rc<RenderDeviceWebGpu> m_device;
    Rc<RenderTarget> m_currentTarget;
    VisualSettings m_visualSettings;
    wgpu::Buffer m_constantBuffer;
    wgpu::Buffer m_perFrameConstantBuffer;
    size_t m_constantBufferSize = 0;
    wgpu::Buffer m_dataBuffer;
    size_t m_dataBufferSize = 0;
    wgpu::Texture m_atlasTexture;
    wgpu::Texture m_gradientTexture;
    wgpu::TextureView m_gradientTextureView;
    wgpu::TextureView m_atlasTextureView;
    GenerationStored m_atlas_generation;
    GenerationStored m_gradient_generation;
    wgpu::CommandEncoder m_encoder;
    wgpu::RenderPassEncoder m_pass;
    wgpu::Queue m_queue;
    wgpu::TextureFormat m_renderFormat;
    wgpu::RenderPassColorAttachment m_colorAttachment;
    Size m_frameSize;
    uint64_t m_frameId;
    constexpr static size_t maxFrameTimings = 16;
    using FrameTimingList                   = SmallVector<FrameTiming, maxFrameTimings>;
    FrameTimingList m_frameTiming;
    size_t m_frameTimingIndex = static_cast<size_t>(-1);
    uint32_t m_timestampIndex = 0;
    std::shared_ptr<void> m_flag{ new std::byte{ 0 } };

    wgpu::BindGroup createBindGroup(ImageBackendWebGpu* sourceImage, ImageBackendWebGpu* backImage = nullptr);
    void updatePerFrameConstantBuffer(const ConstantPerFrame& constants);
    void updateDataBuffer(std::span<const uint32_t> data);
    void updateConstantBuffer(std::span<const RenderState> data);
    void updateAtlasTexture();
    void updateGradientTexture();
    size_t findFrameTimingSlot();
};

} // namespace Brisk
