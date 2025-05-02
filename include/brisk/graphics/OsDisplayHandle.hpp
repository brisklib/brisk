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

#include <brisk/core/Brisk.h>
#include <cstdint>

#ifdef BRISK_ALLOW_OS_HEADERS
#ifdef BRISK_WINDOWS
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif
#ifdef BRISK_APPLE
#include <CoreGraphics/CGDirectDisplay.h>
#endif
#ifdef BRISK_LINUX
#include <GLFW/glfw3.h>
#endif
#endif

namespace Brisk {

struct OsDisplayHandle {
    void* ptr                  = nullptr;

    OsDisplayHandle() noexcept = default;

    explicit operator bool() const noexcept {
        return static_cast<bool>(ptr);
    }

#ifdef BRISK_ALLOW_OS_HEADERS
#ifdef BRISK_WINDOWS
    HMONITOR hMonitor() const noexcept {
        return static_cast<HMONITOR>(ptr);
    }

    explicit OsDisplayHandle(HMONITOR hMonitor) noexcept : ptr(hMonitor) {}
#endif

#ifdef BRISK_APPLE
    CGDirectDisplayID displayId() const noexcept {
        return (CGDirectDisplayID)(uintptr_t)ptr;
    }

    explicit OsDisplayHandle(CGDirectDisplayID displayId) noexcept : ptr((void*)(uintptr_t)displayId) {}
#endif

#ifdef BRISK_LINUX
    GLFWmonitor* glfwMonitor() const noexcept {
        return static_cast<GLFWmonitor*>(ptr);
    }

    explicit OsDisplayHandle(GLFWmonitor* glfwMonitor) noexcept : ptr(glfwMonitor) {}
#endif
#endif
};
} // namespace Brisk
