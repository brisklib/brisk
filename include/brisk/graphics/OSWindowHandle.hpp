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
#include <wayland-client.h>
#undef None
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
