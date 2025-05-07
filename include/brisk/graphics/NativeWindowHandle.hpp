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
 */                                                                                                          \
#pragma once

#include <brisk/core/Brisk.h>

#ifdef BRISK_ALLOW_OS_HEADERS
#ifdef BRISK_WINDOWS
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif
#ifdef BRISK_APPLE
#if defined(__OBJC__)
#import "Cocoa/Cocoa.h"
#endif
#endif
#ifdef BRISK_LINUX
#include <GLFW/glfw3.h>
#endif
#endif

namespace Brisk {

struct NativeWindowHandle {
    void* ptr                     = nullptr;

    NativeWindowHandle() noexcept = default;

    explicit operator bool() const noexcept {
        return static_cast<bool>(ptr);
    }

#ifdef BRISK_ALLOW_OS_HEADERS
#ifdef BRISK_WINDOWS
    HWND hWnd() const noexcept {
        return static_cast<HWND>(ptr);
    }

    explicit NativeWindowHandle(HWND hWnd) noexcept : ptr(hWnd) {}
#endif

#ifdef BRISK_APPLE
#if defined(__OBJC__)
    NSWindow* nsWindow() const noexcept {
        return (__bridge NSWindow*)ptr;
    }

    explicit NativeWindowHandle(NSWindow* nsWindow) noexcept : ptr((__bridge void*)nsWindow) {}
#endif
#endif

#ifdef BRISK_LINUX
    GLFWwindow* glfwWindow() const noexcept {
        return static_cast<GLFWwindow*>(ptr);
    }

    explicit NativeWindowHandle(GLFWwindow* glfwWindow) noexcept : ptr(glfwWindow) {}
#endif
#endif
};

} // namespace Brisk
