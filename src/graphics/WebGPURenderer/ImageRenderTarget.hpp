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

class ImageRenderTargetWebGPU final : public ImageRenderTarget {
public:
    Size size() const final;
    void setSize(Size newSize) final;

    RC<Image> image() const final;

    ImageRenderTargetWebGPU(RC<RenderDeviceWebGPU> device, Size frameSize, PixelType type,
                            DepthStencilType depthStencil, int samples);
    ~ImageRenderTargetWebGPU();

    const BackBufferWebGPU& getBackBuffer() const {
        return m_backBuffer;
    }

private:
    RC<RenderDeviceWebGPU> m_device;
    Size m_frameSize;
    PixelType m_type;
    DepthStencilType m_depthStencilType;
    int m_samples;
    RC<Image> m_image;
    BackBufferWebGPU m_backBuffer;
    bool updateImage();
};
} // namespace Brisk
