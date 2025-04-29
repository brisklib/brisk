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

class ImageRenderTargetD3d11;
class RenderEncoderD3d11;

class ImageBackendD3d11 final : public Internal::ImageBackend {
public:
    explicit ImageBackendD3d11(Rc<RenderDeviceD3d11> device, Image* image, bool uploadImage,
                               bool renderTarget);
    ~ImageBackendD3d11() final = default;

    Rc<RenderDevice> device() const noexcept;

    void begin(AccessMode mode, Rectangle rect) final;
    void end(AccessMode mode, Rectangle rect) final;

    void readFromGpu(const ImageData<UntypedPixel>& data, Point origin);
    void writeToGpu(const ImageData<UntypedPixel>& data, Point origin);

    void invalidate();

private:
    friend class ImageRenderTargetD3d11;
    friend class RenderEncoderD3d11;
    Rc<RenderDeviceD3d11> m_device;
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11ShaderResourceView> m_srv;

    Image* m_image;
    bool m_invalidated = false;
    DXGI_FORMAT m_dxformat;
};

ImageBackendD3d11* getOrCreateBackend(Rc<RenderDeviceD3d11> device, Rc<Image> image, bool uploadImage,
                                      bool renderTarget);
} // namespace Brisk
