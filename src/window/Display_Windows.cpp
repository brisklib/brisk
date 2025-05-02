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
#define BRISK_ALLOW_OS_HEADERS 1
#include "brisk/graphics/OsWindowHandle.hpp"
#include <brisk/window/Display.hpp>
#include <brisk/core/Reflection.hpp>
#include <brisk/core/Encoding.hpp>
#include <shared_mutex>

#include <dwmapi.h>
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <ShellScalingApi.h>

namespace Brisk {

static BOOL CALLBACK monitorCallback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data);

class DisplayMSWin final : public Display {
public:
    Point position() const {
        std::shared_lock lk(m_mutex);
        return m_rect.p1;
    }

    Rectangle workarea() const {
        std::shared_lock lk(m_mutex);
        return m_workarea;
    }

    Size resolution() const {
        return nativeResolution();
    }

    Size nativeResolution() const {
        std::shared_lock lk(m_mutex);
        return Size(m_mode.dmPelsWidth, m_mode.dmPelsHeight);
    }

    Size size() const {
        return nativeResolution();
    }

    SizeF physicalSize() const {
        // No lock needed
        return m_physSize;
    }

    int dpi() const {
        std::shared_lock lk(m_mutex);
        return m_dpi;
    }

    const std::string& name() const {
        // No lock needed
        return m_name;
    }

    const std::string& id() const {
        // No lock needed
        return m_id;
    }

    const std::string& adapterName() const {
        // No lock needed
        return m_adapterName;
    }

    const std::string& adapterId() const {
        // No lock needed
        return m_adapterId;
    }

    float contentScale() const {
        std::shared_lock lk(m_mutex);
        return m_dpi / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
    }

    Point desktopToMonitor(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt - m_rect.p1;
    }

    Point monitorToDesktop(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt + m_rect.p1;
    }

    bool containsWindow(OsWindowHandle handle) const {
        std::shared_lock lk(m_mutex);
        return MonitorFromWindow(handle.hWnd(), MONITOR_DEFAULTTONULL) == m_handle;
    }

    OsDisplayHandle getHandle() const {
        return OsDisplayHandle(m_handle);
    }

    DisplayFlags flags() const {
        std::shared_lock lk(m_mutex);
        return m_flags;
    }

    double refreshRate() const {
        std::shared_lock lk(m_mutex);
        return 1.0 / static_cast<double>(m_frameDuration);
    }

    int backingScaleFactor() const {
        return 1;
    }

    DisplayMSWin(std::string adapterId, std::string displayId, const DISPLAY_DEVICEW& adapter,
                 const DISPLAY_DEVICEW& display);

private:
    mutable std::shared_mutex m_mutex;
    std::string m_adapterName;
    std::string m_adapterId;
    std::string m_name;
    std::string m_id;
    DISPLAY_DEVICEW m_adapter;
    DISPLAY_DEVICEW m_display;
    DEVMODEW m_mode{ .dmSize = sizeof(DEVMODEW) };
    Size m_physSize{};
    int m_dpi;
    HMONITOR m_handle{};
    Rectangle m_rect{};
    Rectangle m_workarea{};
    DisplayFlags m_flags{ DisplayFlags::None };
    Fraction<uint32_t> m_frameDuration{ 1, 60 };
    uint32_t m_counter = 0;

    friend BOOL CALLBACK monitorCallback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data);
    friend void Internal::updateDisplays();
    void update();
};

namespace {

std::shared_mutex displayMutex;

struct Monitor {
    Rc<DisplayMSWin> display;
};

struct Adapter {
    std::map<std::string, Monitor> monitors;
};

std::map<std::string, Adapter> adapters;
Rc<DisplayMSWin> primaryDisplay;
uint32_t counter = 0;

std::map<std::string, std::string> friendlyNames;

} // namespace

static void retrieveFriendlyNames() {
    std::vector<DISPLAYCONFIG_PATH_INFO> paths;
    std::vector<DISPLAYCONFIG_MODE_INFO> modes;
    LONG result;
    do {
        UINT32 pathCount, modeCount;
        result = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE, &pathCount,
                                             &modeCount);
        if (result != ERROR_SUCCESS) {
            return;
        }
        paths.resize(pathCount);
        modes.resize(modeCount);
        result = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE, &pathCount, paths.data(),
                                    &modeCount, modes.data(), nullptr);
        paths.resize(pathCount);
        modes.resize(modeCount);
    } while (result == ERROR_INSUFFICIENT_BUFFER);
    if (result != ERROR_SUCCESS) {
        return;
    }

    for (auto& path : paths) {
        // Find the target (monitor) friendly name
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
        targetName.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size      = sizeof(targetName);
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id        = path.targetInfo.id;
        result                      = DisplayConfigGetDeviceInfo(&targetName.header);
        if (result != ERROR_SUCCESS) {
            return;
        }
        friendlyNames[wcsToUtf8(targetName.monitorDevicePath)] =
            wcsToUtf8(targetName.monitorFriendlyDeviceName);

        // Find the adapter device name
        DISPLAYCONFIG_ADAPTER_NAME adapterName{};
        adapterName.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapterName.header.size      = sizeof(adapterName);
        adapterName.header.adapterId = path.targetInfo.adapterId;
        result                       = DisplayConfigGetDeviceInfo(&adapterName.header);
        if (result != ERROR_SUCCESS) {
            return;
        }
    }
}

constexpr DWORD activeAndAttached = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

void Internal::updateDisplays() {
    mustBeMainThread();
    std::unique_lock lk(displayMutex);
    ++counter;
    retrieveFriendlyNames();
    for (int adapter = 0;; adapter++) {
        DISPLAY_DEVICEW adapterDevice{ .cb = sizeof(DISPLAY_DEVICEW) };
        if (!EnumDisplayDevicesW(nullptr, adapter, &adapterDevice, 0))
            break;
        if ((adapterDevice.StateFlags & activeAndAttached) != activeAndAttached)
            continue;

        std::string adapterId = wcsToUtf8(adapterDevice.DeviceID);

        int display           = 0;
        for (;; display++) {
            DISPLAY_DEVICEW displayDevice{ .cb = sizeof(DISPLAY_DEVICEW) };
            if (!EnumDisplayDevicesW(adapterDevice.DeviceName, display, &displayDevice,
                                     EDD_GET_DEVICE_INTERFACE_NAME)) {
                if (display == 0) {
                    displayDevice = adapterDevice;
                } else {
                    break;
                }
            }
            if ((displayDevice.StateFlags & activeAndAttached) != activeAndAttached)
                continue;
            std::string displayId = wcsToUtf8(displayDevice.DeviceID);

            Monitor& mon          = adapters[adapterId].monitors[displayId];
            if (!mon.display) {
                mon.display = rcnew DisplayMSWin(adapterId, displayId, adapterDevice, displayDevice);
            }
            std::unique_lock lk(mon.display->m_mutex);
            if (display == 0 && (adapterDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)) {
                mon.display->m_flags |= DisplayFlags::Primary;
                primaryDisplay = mon.display;
            }
            mon.display->update();
            mon.display->m_counter = counter;
        }
    }
    for (auto&& a : adapters) {
        for (auto it = a.second.monitors.begin(); it != a.second.monitors.end();) {
            if (it->second.display->m_counter != counter) {
                it = a.second.monitors.erase(it);
            } else {
                ++it;
            }
        }
    }
}

static BOOL CALLBACK monitorCallback(HMONITOR handle, HDC dc, LPRECT rect, LPARAM data) {
    DisplayMSWin& display = *reinterpret_cast<DisplayMSWin*>(data);
    MONITORINFOEXW info{ { .cbSize = sizeof(MONITORINFOEXW) } };
    if (GetMonitorInfoW(handle, (MONITORINFO*)&info)) {
        if (wcscmp(info.szDevice, display.m_adapter.DeviceName) == 0) {
            display.m_handle = handle;
            display.m_rect   = Rectangle(info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right,
                                         info.rcMonitor.bottom);
            display.m_workarea =
                Rectangle(info.rcWork.left, info.rcWork.top, info.rcWork.right, info.rcWork.bottom);
        }
    }
    return TRUE;
}

void DisplayMSWin::update() {
    EnumDisplaySettingsW(m_adapter.DeviceName, ENUM_CURRENT_SETTINGS, &m_mode);

    DWM_TIMING_INFO ti{ sizeof(DWM_TIMING_INFO) };
    HRESULT res = DwmGetCompositionTimingInfo(NULL, &ti);
    if (res == S_OK && ti.rateRefresh.uiNumerator > 0 && ti.rateRefresh.uiDenominator > 0) {
        m_frameDuration = { ti.rateRefresh.uiDenominator, ti.rateRefresh.uiNumerator };
    } else {
        m_frameDuration = { 1, m_mode.dmDisplayFrequency };
    }

    EnumDisplayMonitors(nullptr, nullptr, &monitorCallback, (LPARAM)this);

    SizeOf<UINT> dpi;
    GetDpiForMonitor(m_handle, MDT_EFFECTIVE_DPI, &dpi.x, &dpi.y);
    m_dpi = dpi.longestSide();
}

DisplayMSWin::DisplayMSWin(std::string adapterId, std::string displayId, const DISPLAY_DEVICEW& adapter,
                           const DISPLAY_DEVICEW& display)
    : m_adapterId(std::move(adapterId)), m_id(std::move(displayId)), m_adapter(adapter), m_display(display) {
    m_name        = wcsToUtf8(m_display.DeviceString);
    m_adapterName = wcsToUtf8(m_adapter.DeviceString);
    if (auto it = friendlyNames.find(m_adapterId); it != friendlyNames.end())
        m_name = it->second;
    if (auto it = friendlyNames.find(m_id); it != friendlyNames.end())
        m_name = it->second;
    HDC dc            = CreateDCW(L"DISPLAY", m_adapter.DeviceName, nullptr, nullptr);
    m_physSize.width  = GetDeviceCaps(dc, HORZSIZE);
    m_physSize.height = GetDeviceCaps(dc, VERTSIZE);
    DeleteDC(dc);
}

/*static*/ std::vector<Rc<Display>> Display::all() {
    std::shared_lock lk(displayMutex);
    std::vector<Rc<Display>> result;
    for (auto&& a : adapters) {
        for (auto&& m : a.second.monitors) {
            result.push_back(m.second.display);
        }
    }

    return result;
}

/*static*/ Rc<Display> Display::primary() {
    std::shared_lock lk(displayMutex);
    return primaryDisplay;
}

} // namespace Brisk
