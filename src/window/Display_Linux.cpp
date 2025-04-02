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
#define BRISK_ALLOW_OS_HEADERS 1
#include "brisk/graphics/OSWindowHandle.hpp"
#include <brisk/window/Display.hpp>
#include <brisk/core/Utilities.hpp>

#include <shared_mutex>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11 1
#define GLFW_EXPOSE_NATIVE_WAYLAND 1
#include <GLFW/glfw3native.h>
#undef None

namespace Brisk {

void pollDisplays();

class DisplayLinux final : public Display {
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
        return m_resolution;
    }

    Size size() const {
        return nativeResolution();
    }

    SizeF physicalSize() const {
        // No lock needed
        return m_physSize;
    }

    int dpi() const {
        return std::round(contentScale() * 96);
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
        // no need to lock because contentScaleX/contentScaleY don't change
        return SizeF(m_contentScaleX, m_contentScaleY).longestSide();
    }

    Point desktopToMonitor(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt - m_rect.p1;
    }

    Point monitorToDesktop(Point pt) const {
        std::shared_lock lk(m_mutex);
        return pt + m_rect.p1;
    }

    bool containsWindow(OSWindowHandle handle) const {
        std::shared_lock lk(m_mutex);
        return glfwGetWindowMonitor(handle.glfwWindow()) == m_monitor;
    }

    OSDisplayHandle getHandle() const {
        return OSDisplayHandle(m_monitor);
    }

    DisplayFlags flags() const {
        std::shared_lock lk(m_mutex);
        return m_flags;
    }

    double refreshRate() const {
        std::shared_lock lk(m_mutex);
        return m_refreshRate;
    }

    int backingScaleFactor() const {
        return 1;
    }

    DisplayLinux(GLFWmonitor* monitor);

private:
    GLFWmonitor* m_monitor;
    mutable std::shared_mutex m_mutex;
    std::string m_adapterName;
    std::string m_adapterId;
    std::string m_name;
    std::string m_id;
    Rectangle m_workarea;
    Rectangle m_rect;
    double m_refreshRate;
    DisplayFlags m_flags;
    Size m_physSize;
    Size m_resolution;
    int m_counter = 0;
    static float m_contentScaleX;
    static float m_contentScaleY;
    friend void pollDisplays();
};

float DisplayLinux::m_contentScaleX = 1.f;
float DisplayLinux::m_contentScaleY = 1.f;

std::shared_mutex displayMutex;

static std::map<RROutput, RC<DisplayLinux>> displays;
RC<DisplayLinux> primaryDisplay;

void pollDisplays() {
    glfwInit();

    int mcount             = 0;

    GLFWmonitor** monitors = glfwGetMonitors(&mcount);
    for (int i = 0; i < mcount; ++i) {

        RROutput rrout            = glfwGetX11Monitor(monitors[i]);

        RC<DisplayLinux>& monitor = displays[rrout];

        if (!monitor)
            monitor = rcnew DisplayLinux(monitors[i]);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        monitor->m_resolution   = Size(mode->width, mode->height);
        Point pos;
        glfwGetMonitorPos(monitors[i], &pos.x, &pos.y);
        monitor->m_rect      = Rectangle(pos, monitor->m_resolution);
        monitor->m_id        = std::to_string(rrout);
        monitor->m_adapterId = monitor->m_id;
        monitor->m_name      = glfwGetMonitorName(monitors[i]);

        Rectangle area;
        glfwGetMonitorWorkarea(monitors[i], &area.x1, &area.y1, &area.x2, &area.y2);
        area.p2 += area.p1;

        monitor->m_refreshRate = mode->refreshRate;

        if (i == 0) {
            monitor->m_flags |= DisplayFlags::Primary;
            primaryDisplay = monitor;
        }

        monitor->m_workarea = area;
        ++monitor->m_counter;
    }
}

std::vector<RC<Display>> Display::all() {
    std::shared_lock lk(displayMutex);
    std::vector<RC<Display>> result;
    for (const auto& a : displays) {
        result.push_back(a.second);
    }
    return result;
}

/*static*/ RC<Display> Display::primary() {
    std::shared_lock lk(displayMutex);
    return primaryDisplay;
}

void Internal::updateDisplays() {
    std::unique_lock lk(displayMutex);
    pollDisplays();
}

DisplayLinux::DisplayLinux(GLFWmonitor* monitor) : m_monitor(monitor) {}

} // namespace Brisk
