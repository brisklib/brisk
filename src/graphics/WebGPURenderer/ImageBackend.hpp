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

class ImageRenderTargetWebGPU;
class RenderEncoderWebGPU;

class ImageBackendWebGPU final : public Internal::ImageBackend {
public:
    explicit ImageBackendWebGPU(RC<RenderDeviceWebGPU> device, Image* image, bool uploadImage,
                                bool renderTarget);
    ~ImageBackendWebGPU() final = default;

    RC<RenderDevice> device() const noexcept;

    void begin(AccessMode mode, Rectangle rect) final;
    void end(AccessMode mode, Rectangle rect) final;

    void readFromGPU(const ImageData<UntypedPixel>& data, Point origin);
    void writeToGPU(const ImageData<UntypedPixel>& data, Point origin);

    void invalidate();

private:
    friend class ImageRenderTargetWebGPU;
    friend class RenderEncoderWebGPU;
    RC<RenderDeviceWebGPU> m_device;
    wgpu::Texture m_texture;
    wgpu::TextureView m_textureView;

    Image* m_image;
    bool m_invalidated = false;
    wgpu::TextureFormat m_wgformat;
};

ImageBackendWebGPU* getOrCreateBackend(RC<RenderDeviceWebGPU> device, RC<Image> image, bool uploadImage,
                                       bool renderTarget);
} // namespace Brisk
