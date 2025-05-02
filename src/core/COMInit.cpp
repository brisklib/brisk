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
#include <brisk/core/internal/COMInit.hpp>
#include <shobjidl.h>

namespace Brisk {

COMInitializer::COMInitializer() {
    HRESULT result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | ::COINIT_DISABLE_OLE1DDE);
    static_assert(sizeof(HRESULT) == sizeof(uint32_t));
    this->result = static_cast<uint32_t>(result);
}

COMInitializer::operator bool() const noexcept {
    return SUCCEEDED(result) || result == RPC_E_CHANGED_MODE;
}

COMInitializer::~COMInitializer() {
    if (SUCCEEDED(result)) {
        CoUninitialize();
    }
}
} // namespace Brisk
