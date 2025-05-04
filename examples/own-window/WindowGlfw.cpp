#define BRISK_ALLOW_OS_HEADERS 1

#include "WindowGlfw.hpp"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#undef None

namespace Brisk {

NativeWindowHandle NativeWindowGLFW::getHandle() const {
#ifdef BRISK_WINDOWS
    return NativeWindowHandle(glfwGetWin32Window(win));
#endif
#ifdef BRISK_MACOS
    return NativeWindowHandle((NSWindow*)glfwGetCocoaWindow(win));
#endif
#ifdef BRISK_LINUX
    return NativeWindowHandle(win);
#endif
}

} // namespace Brisk
