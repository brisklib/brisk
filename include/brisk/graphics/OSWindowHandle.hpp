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
 */                                                                                                          \
#pragma once

#include <brisk/core/Brisk.h>
#include "Renderer.hpp"

#ifdef BRISK_WINDOWS
#include "windows.h"
#endif
#ifdef BRISK_APPLE
#if defined(__OBJC__)
#import "Cocoa/Cocoa.h"
#else
#include <objc/objc.h>
#endif
#endif
#ifdef BRISK_LINUX
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#undef None
#undef Status
#undef True
#undef False
#undef Bool
#undef RootWindow
#undef CurrentTime
#undef Success
#undef DestroyAll
#undef COUNT
#undef CREATE
#undef DeviceAdded
#undef DeviceRemoved
#undef Always

#include <wayland-client.h>

#endif

namespace Brisk {

struct OSWindowHandle;

#ifdef BRISK_WINDOWS
struct OSWindowHandle {
    HWND window;
};
#endif
#ifdef BRISK_APPLE
#if defined(__OBJC__)
struct OSWindowHandle {
    NSWindow* window;
};
#else
struct OSWindowHandle {
    id window;
};
#endif
#endif
#ifdef BRISK_LINUX
struct OSWindowHandle {
    bool wayland;
    ::Display* x11Display;
    ::Window x11Window;
    ::wl_display* wlDisplay;
    ::wl_surface* wlWindow;
};
#endif

#ifdef BRISK_WINDOWS
inline HWND handleFromWindow(OSWindow* window, HWND fallback = 0) {
    OSWindowHandle handle{ fallback };
    if (window)
        window->getHandle(handle);
    return handle.window;
}
#endif

} // namespace Brisk
