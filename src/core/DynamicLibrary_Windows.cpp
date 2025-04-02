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
#include <brisk/core/Encoding.hpp>
#include <brisk/core/DynamicLibrary.hpp>

#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

namespace Brisk {

DynamicLibrary::~DynamicLibrary() {
    FreeLibrary((HMODULE)m_handle);
}

RC<DynamicLibrary> DynamicLibrary::load(const std::string& name) noexcept {
    void* handle = LoadLibraryW(utf8ToWcs(name).c_str());
    if (!handle)
        return nullptr;
    return rcnew DynamicLibrary(handle);
}

DynamicLibrary::FuncPtr DynamicLibrary::getFunc(const std::string& name) const noexcept {
    return reinterpret_cast<FuncPtr>(GetProcAddress((HMODULE)m_handle, name.c_str()));
}

DynamicLibrary::DynamicLibrary(void* handle) noexcept : m_handle(handle) {}
} // namespace Brisk
