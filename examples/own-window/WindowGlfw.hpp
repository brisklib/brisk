#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <brisk/graphics/Renderer.hpp>

namespace Brisk {

class OsWindowGLFW final : public OsWindow {
public:
    Size framebufferSize() const final {
        Size size;
        glfwGetFramebufferSize(win, &size.x, &size.y);
        return size;
    }

    OsWindowHandle getHandle() const final;

    OsWindowGLFW() = default;

    explicit OsWindowGLFW(GLFWwindow* win) : win(win) {}

private:
    GLFWwindow* win = nullptr;
};

} // namespace Brisk
