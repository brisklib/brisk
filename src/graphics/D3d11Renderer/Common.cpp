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
#include "Common.hpp"

namespace Brisk {

std::string hrDescription(HRESULT hr) {
    std::string result;
    result.resize(1000);

    result.resize(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, HRESULT_CODE(hr),
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                 result.data(), result.size(), NULL));
    while (!result.empty() && uint8_t(result.back()) <= ' ') {
        result.resize(result.size() - 1);
    }
    return result;
}

DXGI_FORMAT dxFormat(PixelType type, PixelFormat format) {
    constexpr DXGI_FORMAT _ = DXGI_FORMAT_UNKNOWN;
    if (+type >= enumSize<PixelType>)
        return _;
    if (+format >= enumSize<PixelFormat>)
        return _;
    constexpr static DXGI_FORMAT formats[enumSize<PixelFormat>][enumSize<PixelType>]{
        // clang-format off
/*                     U8                          U8Gamma                          U16                             F32                  */
/* RGB,            */ {_,                          _,                               _,                              DXGI_FORMAT_R32G32B32_FLOAT   },
/* RGBA,           */ {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R16G16B16A16_UNORM, DXGI_FORMAT_R32G32B32A32_FLOAT},
/* ARGB,           */ {_,                          _,                               _,                              _                             },
/* BGR,            */ {_,                          _,                               _,                              _                             },
/* BGRA,           */ {DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, _,                              _                             },
/* ABGR,           */ {_,                          _,                               _,                              _                             },
/* GreyscaleAlpha, */ {DXGI_FORMAT_R8G8_UNORM,     _,                               DXGI_FORMAT_R16G16_UNORM,       DXGI_FORMAT_R32G32_FLOAT      },
/* Greyscale,      */ {DXGI_FORMAT_R8_UNORM,       _,                               DXGI_FORMAT_R16_UNORM,          DXGI_FORMAT_R32_FLOAT         },
/* Alpha,          */ {DXGI_FORMAT_A8_UNORM,       _,                               _,                              _                             },
        // clang-format on
    };
    return formats[+format][+type];
}

DXGI_FORMAT dxFormatTypeless(PixelType type, PixelFormat format) {
    constexpr DXGI_FORMAT _ = DXGI_FORMAT_UNKNOWN;
    if (+type >= enumSize<PixelType>)
        return _;
    if (+format >= enumSize<PixelFormat>)
        return _;
    constexpr static DXGI_FORMAT formats[enumSize<PixelFormat>][enumSize<PixelType>]{
        // clang-format off
/*                     U8                             U8Gamma                        U16                               F32                  */
/* RGB,            */ {_,                             _,                             _,                                 DXGI_FORMAT_R32G32B32_TYPELESS   },
/* RGBA,           */ {DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R32G32B32A32_TYPELESS},
/* ARGB,           */ {_,                             _,                             _,                                 _                                },
/* BGR,            */ {_,                             _,                             _,                                 _                                },
/* BGRA,           */ {DXGI_FORMAT_B8G8R8A8_TYPELESS, DXGI_FORMAT_B8G8R8A8_TYPELESS, _,                                 _                                },
/* ABGR,           */ {_,                             _,                             _,                                 _                                },
/* GreyscaleAlpha, */ {DXGI_FORMAT_R8G8_TYPELESS,     _,                             DXGI_FORMAT_R16G16_TYPELESS,       DXGI_FORMAT_R32G32_TYPELESS      },
/* Greyscale,      */ {DXGI_FORMAT_R8_TYPELESS,       _,                             DXGI_FORMAT_R16_TYPELESS,          DXGI_FORMAT_R32_TYPELESS         },
/* Alpha,          */ {DXGI_FORMAT_R8_TYPELESS,       _,                             _,                                 _                                },
        // clang-format on
    };
    return formats[+format][+type];
}

D3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT fmt, Size size, int samples, D3D11_USAGE usage, UINT bind,
                             UINT cpuAccess) {
    D3D11_TEXTURE2D_DESC result{}; // zero-initialize
    result.Width          = size.width;
    result.Height         = size.height;
    result.MipLevels      = 1;
    result.ArraySize      = 1;
    result.Format         = fmt;
    result.SampleDesc     = { 1u, 0u };
    result.Usage          = usage;
    result.BindFlags      = bind;
    result.CPUAccessFlags = cpuAccess;
    result.MiscFlags      = 0;
    return result;
}
} // namespace Brisk
