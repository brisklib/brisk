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

#ifndef BRISK_WEBGPU
#error "Brisk was not compiled with WebGPU support"
#endif

#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <dawn/webgpu_cpp.h>

namespace Brisk {

/**
 * Converts pixel type and format to a WebGPU texture format.
 * @param type The pixel type.
 * @param format The pixel format, defaults to RGBA.
 * @return The corresponding WebGPU texture format.
 */
wgpu::TextureFormat wgFormat(PixelType type, PixelFormat format = PixelFormat::RGBA);

/**
 * Structure representing a WebGPU back buffer with color texture and view.
 */
struct BackBufferWebGPU {
    wgpu::Texture color;         /** The color texture of the back buffer. */
    wgpu::TextureView colorView; /** The texture view for the color texture. */
};

/**
 * Interface for providing access to a WebGPU back buffer.
 * Implemented by WebGPU RenderTargets (e.g., WindowRenderTarget, ImageRenderTarget).
 */
class BackBufferProviderWebGPU {
public:
    /**
     * Gets the back buffer.
     * @return Reference to the BackBufferWebGPU object.
     */
    virtual const BackBufferWebGPU& getBackBuffer() const = 0;
};

/**
 * Interface for providing a WebGPU devide. Implemented by RenderEncoder
 */
class DeviceProviderWebGPU {
public:
    /**
     * Gets the command encoder.
     * @return The WebGPU command encoder.
     */
    virtual wgpu::Device getDevice() const = 0;
};

bool webgpuFromContext(RenderContext& context, wgpu::Device& wgDevice, wgpu::TextureView& backBuffer);

} // namespace Brisk
