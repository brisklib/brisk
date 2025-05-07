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

#ifndef BRISK_D3D11
#error "Brisk was not compiled with D3D11 support"
#endif

#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/Image.hpp>

#include <d3d11.h>
#include <wrl/client.h>

namespace Brisk {

/**
 * @brief Converts a pixel type and format to a DXGI format.
 *
 * This function maps the specified pixel type and format to the corresponding DXGI format for use in DirectX
 * rendering.
 *
 * @param[in] type The pixel type to convert.
 * @param[in] format The pixel format to convert.
 * @return The corresponding DXGI_FORMAT value.
 */
DXGI_FORMAT dxFormat(PixelType type, PixelFormat format);

/**
 * @brief Converts a pixel type and format to a typeless DXGI format.
 *
 * This function maps the specified pixel type and format to a typeless DXGI format, which can be used for
 * flexible resource creation in DirectX rendering.
 *
 * @param[in] type The pixel type to convert.
 * @param[in] format The pixel format to convert.
 * @return The corresponding typeless DXGI_FORMAT value.
 */
DXGI_FORMAT dxFormatTypeless(PixelType type, PixelFormat format);

/**
 * @brief Converts a pixel type and format to a DXGI format without sRGB.
 *
 * This function maps the specified pixel type (with gamma removed) and format to a DXGI format, ensuring
 * no sRGB gamma correction is applied.
 *
 * @param[in] type The pixel type to convert.
 * @param[in] format The pixel format to convert.
 * @return The corresponding DXGI_FORMAT value without sRGB.
 * @note This function uses noGamma to remove gamma from the pixel type before calling dxFormat.
 */
inline DXGI_FORMAT dxFormatNoSrgb(PixelType type, PixelFormat format) {
    return dxFormat(noGamma(type), format);
}

/**
 * @brief Retrieves the underlying D3D11 texture of an Image.
 *
 * This function returns the D3D11 texture associated with the provided Image resource, or nullptr if no
 * texture has been created for the Image.
 *
 * @param[in] image The Image resource to query.
 * @return A ID3D11Texture2D object representing the Image's texture, or nullptr if no texture exists.
 * @note To ensure a GPU texture is created for the Image, call RenderDevice::createImageBackend before
 * invoking this function.
 */
Microsoft::WRL::ComPtr<ID3D11Texture2D> textureFromImage(Rc<Image> image);

} // namespace Brisk
