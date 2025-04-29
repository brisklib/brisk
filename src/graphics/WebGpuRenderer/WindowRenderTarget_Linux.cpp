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
#include "WindowRenderTarget.hpp"
#include <brisk/graphics/OsWindowHandle.hpp>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11 1
#define GLFW_EXPOSE_NATIVE_WAYLAND 1
#include <GLFW/glfw3native.h>
#undef None

namespace Brisk {

void WindowRenderTargetWebGpu::createSurface(const OsWindow* window) {
    OsWindowHandle handle = window->getHandle();

    wgpu::SurfaceDescriptor surfaceDesc;
    wgpu::SurfaceDescriptorFromXlibWindow surfaceDescX11{};
    wgpu::SurfaceDescriptorFromWaylandSurface surfaceDescWL{};

    if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
        surfaceDescWL.display   = glfwGetWaylandDisplay();
        surfaceDescWL.surface   = glfwGetWaylandWindow(handle.glfwWindow());
        surfaceDesc.nextInChain = &surfaceDescWL;
    } else {
        surfaceDescX11.display  = glfwGetX11Display();
        surfaceDescX11.window   = glfwGetX11Window(handle.glfwWindow());
        surfaceDesc.nextInChain = &surfaceDescX11;
    }
    m_surface = m_device->m_instance.CreateSurface(&surfaceDesc);
}

} // namespace Brisk
