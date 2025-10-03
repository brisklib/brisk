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

class WindowRenderTargetD3d11;
class ImageRenderTargetD3d11;

const BackBufferD3d11& getBackBuffer(RenderTarget* target);

class RenderEncoderD3d11 final : public RenderEncoder,
                                 public std::enable_shared_from_this<RenderEncoderD3d11> {
public:
    VisualSettings visualSettings() const final;
    void setVisualSettings(const VisualSettings& visualSettings) final;

    void begin(Rc<RenderTarget> target, std::optional<ColorF> clear = Palette::transparent) final;
    void batch(std::span<const RenderState> commands, std::span<const uint32_t> data) final;
    void end() final;
    void wait() final;

    RenderDevice* device() const final;

    Rc<RenderTarget> currentTarget() const;

    void beginFrame(uint64_t frameId);
    void endFrame(DurationCallback callback);

    explicit RenderEncoderD3d11(Rc<RenderDeviceD3d11> device);
    ~RenderEncoderD3d11();

private:
    struct BatchTiming {
        ComPtr<ID3D11Query> startQuery;
        ComPtr<ID3D11Query> endQuery;
        ComPtr<ID3D11Query> disjointQuery;
        BatchTiming(ID3D11Device* device);

        std::optional<std::chrono::nanoseconds> time(ID3D11DeviceContext* ctx);
    };

    struct FrameTiming {
        uint64_t frameId;
        SmallVector<BatchTiming, 1> batches;
        bool pending = false;

        void begin(ID3D11Device* device, ID3D11DeviceContext* ctx);
        void end(ID3D11DeviceContext* ctx);

        std::optional<std::vector<std::chrono::nanoseconds>> time(ID3D11DeviceContext* ctx);

        FrameTiming(uint64_t frameId, ID3D11Device* device);
    };

    Rc<RenderDeviceD3d11> m_device;
    Rc<RenderTarget> m_currentTarget;
    VisualSettings m_visualSettings;
    ComPtr<ID3D11Query> m_query;
    ComPtr<ID3D11Buffer> m_constantBuffer;
    size_t m_constantBufferSize = 0;
    ComPtr<ID3D11Buffer> m_dataBuffer;
    ComPtr<ID3D11Buffer> m_dataBufferStaging;
    size_t m_dataBufferSize = 0;
    ComPtr<ID3D11ShaderResourceView> m_dataSRV;
    ComPtr<ID3D11Texture2D> m_atlasTexture;
    ComPtr<ID3D11ShaderResourceView> m_atlasSRV;
    ComPtr<ID3D11ShaderResourceView> m_gradientSRV;
    ComPtr<ID3D11Texture2D> m_gradientTexture;
    GenerationStored m_atlas_generation;
    GenerationStored m_gradient_generation;
    Size m_frameSize;
    uint64_t m_frameId;
    constexpr static size_t maxFrameTimings = 16;
    using FrameTimingList                   = SmallVector<FrameTiming, maxFrameTimings>;
    FrameTimingList m_frameTiming;
    size_t m_frameTimingIndex = static_cast<size_t>(-1);
    uint32_t m_batchIndex     = 0;
    std::shared_ptr<void> m_flag{ new std::byte{ 0 } };
    DurationCallback m_durationCallback;

    void updatePerFrameConstantBuffer(const ConstantPerFrame& constants);
    void updateDataBuffer(std::span<const uint32_t> data);
    void updateConstantBuffer(std::span<const RenderState> data);
    void updateAtlasTexture();
    void updateGradientTexture();
    size_t findFrameTimingSlot();
    void processQueries();
};

} // namespace Brisk
