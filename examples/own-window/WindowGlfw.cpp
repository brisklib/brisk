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
