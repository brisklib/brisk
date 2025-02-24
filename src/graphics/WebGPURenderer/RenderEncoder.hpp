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

#include "RenderDevice.hpp"

namespace Brisk {

class RenderEncoderWebGPU final : public RenderEncoder {
public:
    RenderDevice* device() const final {
        return m_device.get();
    }

    VisualSettings visualSettings() const final;
    void setVisualSettings(const VisualSettings& visualSettings) final;

    void begin(RC<RenderTarget> target, std::optional<ColorF> clear = Palette::transparent) final;
    void batch(std::span<const RenderState> commands, std::span<const float> data) final;
    void end() final;
    void wait() final;

    explicit RenderEncoderWebGPU(RC<RenderDeviceWebGPU> device);
    ~RenderEncoderWebGPU();

private:
    RC<RenderDeviceWebGPU> m_device;
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
    wgpu::BindGroup createBindGroup(ImageBackendWebGPU* imageBackend);
    void updatePerFrameConstantBuffer(const ConstantPerFrame& constants);
    void updateDataBuffer(std::span<const float> data);
    void updateConstantBuffer(std::span<const RenderState> data);
    void updateAtlasTexture();
    void updateGradientTexture();
};

} // namespace Brisk
