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

#include <d3d11_3.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace Brisk {
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

inline ComPtr<IDXGIAdapter> adapterForMonitor(HMONITOR monitor, ComPtr<IDXGIFactory> dxgiFactory) {

    for (UINT adapterIndex = 0;; ++adapterIndex) {
        ComPtr<IDXGIAdapter> adapter;
        if (FAILED(dxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()))) {
            break; // No more adapters
        }

        for (UINT outputIndex = 0;; ++outputIndex) {
            ComPtr<IDXGIOutput> output;
            if (FAILED(adapter->EnumOutputs(outputIndex, output.ReleaseAndGetAddressOf()))) {
                break; // No more outputs
            }

            DXGI_OUTPUT_DESC outputDesc;
            output->GetDesc(&outputDesc);

            if (outputDesc.Monitor == monitor) {
                return adapter;
            }
        }
    }

    return nullptr;
}

} // namespace Brisk
