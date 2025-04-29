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
 * Converts pixel type and format to a WebGpu texture format.
 * @param type The pixel type.
 * @param format The pixel format, defaults to RGBA.
 * @return The corresponding WebGpu texture format.
 */
wgpu::TextureFormat wgFormat(PixelType type, PixelFormat format = PixelFormat::RGBA);

/**
 * Structure representing a WebGpu back buffer with color texture and view.
 */
struct BackBufferWebGpu {
    wgpu::Texture color;         /** The color texture of the back buffer. */
    wgpu::TextureView colorView; /** The texture view for the color texture. */
};

/**
 * @brief Retrieves WebGpu device and backbuffer texture view from a RenderContext.
 *
 * This function extracts the WebGpu device and the backbuffer texture view from the provided
 * RenderContext, populating the output parameters if successful.
 *
 * @param[in,out] context The RenderContext containing WebGpu-related data.
 * @param[out] wgDevice Reference to a wgpu::Device object to be filled with the WebGpu device.
 * @param[out] backBuffer Reference to a wgpu::TextureView object to be filled with the backbuffer texture
 * view.
 * @return True if the device and backbuffer were successfully retrieved, false otherwise.
 * @note This function flushes the current batch in the RenderContext before returning to ensure correct
 * interleaving of rendering commands.
 */
bool webgpuFromContext(RenderContext& context, wgpu::Device& wgDevice, wgpu::TextureView& backBuffer);

} // namespace Brisk
