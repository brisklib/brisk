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
#include <brisk/core/System.hpp>
#include <fmt/format.h>
#include <brisk/core/internal/Debug.hpp>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include "winnt.h"

namespace Brisk {

HINSTANCE winInstance = {};

typedef LONG(WINAPI* PFN_RtlGetVersionInfo)(OSVERSIONINFOEXW*);

static OSVERSIONINFOEXW retrieveWinVersion() {
    // RtlGetVersion is used to get the real version of Windows because
    // plain GetVersion lies about version if Windows 10 manifest is not present.
    PFN_RtlGetVersionInfo RtlGetVersion =
        (PFN_RtlGetVersionInfo)GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlGetVersion");
    BRISK_ASSERT(RtlGetVersion != nullptr);
    OSVERSIONINFOEXW osVer{ .dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW) };
    LONG res = RtlGetVersion(&osVer);
    BRISK_ASSERT(res == 0);
    return osVer;
}

static OSVERSIONINFOEXW getWinVersion() {
    static OSVERSIONINFOEXW ver = retrieveWinVersion();
    return ver;
}

OSVersion osVersion() {
    auto ver = getWinVersion();
    return { uint16_t(ver.dwMajorVersion), uint16_t(ver.dwMinorVersion), uint32_t(ver.dwBuildNumber) };
}

std::string osName() {
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    char buf[256];
    DWORD size;
    std::string CurrentBuildNumber;
    std::string ProductName;
    size = sizeof(dwMajor);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                     "CurrentMajorVersionNumber", RRF_RT_REG_DWORD, nullptr, &dwMajor,
                     &size) == ERROR_SUCCESS) {}
    size = sizeof(dwMinor);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                     "CurrentMinorVersionNumber", RRF_RT_REG_DWORD, nullptr, &dwMinor,
                     &size) == ERROR_SUCCESS) {}
    size = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                     "CurrentBuildNumber", RRF_RT_REG_SZ, nullptr, &buf, &size) == ERROR_SUCCESS) {
        CurrentBuildNumber = buf;
    }
    size = sizeof(buf);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName",
                     RRF_RT_REG_SZ, nullptr, &buf, &size) == ERROR_SUCCESS) {
        ProductName = buf;
    } else {
        ProductName = "Windows";
    }
    return fmt::format("{} {}.{}.{}", ProductName, dwMajor, dwMinor, CurrentBuildNumber);
}

} // namespace Brisk
