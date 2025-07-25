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

#include <string>
#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/D3d11.hpp>

#include "../AdapterForMonitor.hpp"

namespace Brisk {

constexpr inline PixelFormat backBufferFormat = PixelFormat::BGRA;

constexpr inline size_t maxD3d11ResourceBytes = 128 * 1048576; // Guaranteed in D3D11.0

D3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT fmt, Size size, int samples, D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
                             UINT bind      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                             UINT cpuAccess = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);

std::string hrDescription(HRESULT hr);

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class EDirect3D : public ELogic {
public:
    using ELogic::ELogic;
};

inline void handleD3d11Err(HRESULT hr) {
    throwException(EDirect3D("Direct3D11 Error: {}", hrDescription(hr)));
}

#define CHECK_HRESULT(hr, fail)                                                                              \
    do {                                                                                                     \
        if (FAILED(hr)) {                                                                                    \
            handleD3d11Err(hr);                                                                              \
            fail;                                                                                            \
        }                                                                                                    \
    } while (0)

class RenderDeviceD3d11;

struct BackBufferD3d11 {
    ComPtr<ID3D11Texture2D> colorBuffer;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11Texture2D> depthStencil;
    ComPtr<ID3D11DepthStencilView> dsv;
};

} // namespace Brisk
