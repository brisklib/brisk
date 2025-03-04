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

class RenderEncoderD3D11 final : public RenderEncoder,
                                 public std::enable_shared_from_this<RenderEncoderD3D11> {
public:
    VisualSettings visualSettings() const final;
    void setVisualSettings(const VisualSettings& visualSettings) final;

    void begin(RC<RenderTarget> target, std::optional<ColorF> clear = Palette::transparent) final;
    void batch(std::span<const RenderState> commands, std::span<const float> data) final;
    void end() final;
    void wait() final;

    RenderDevice* device() const final;

    RC<RenderTarget> currentTarget() {
        return m_currentTarget;
    }

    explicit RenderEncoderD3D11(RC<RenderDeviceD3D11> device);
    ~RenderEncoderD3D11();

private:
    RC<RenderDeviceD3D11> m_device;
    RC<RenderTarget> m_currentTarget;
    VisualSettings m_visualSettings;
    ComPtr<ID3D11Query> m_query;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    size_t m_constantBufferSize = 0;
    ComPtr<ID3D11Buffer> m_dataBuffer;
    size_t m_dataBufferSize = 0;
    ComPtr<ID3D11ShaderResourceView> m_dataSRV;
    ComPtr<ID3D11Texture2D> m_atlasTexture;
    ComPtr<ID3D11ShaderResourceView> m_atlasSRV;
    ComPtr<ID3D11ShaderResourceView> m_gradientSRV;
    ComPtr<ID3D11Texture2D> m_gradientTexture;
    GenerationStored m_atlas_generation;
    GenerationStored m_gradient_generation;
    Size m_frameSize;

    void updatePerFrameConstantBuffer(const ConstantPerFrame& constants);
    void updateDataBuffer(std::span<const float> data);
    void updateConstantBuffer(std::span<const RenderState> data);
    void updateAtlasTexture();
    void updateGradientTexture();
};

} // namespace Brisk
