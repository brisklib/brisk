#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <brisk/graphics/Renderer.hpp>

namespace Brisk {

class NativeWindowGLFW final : public NativeWindow {
public:
    Size framebufferSize() const final {
        Size size;
        glfwGetFramebufferSize(win, &size.x, &size.y);
        return size;
    }

    NativeWindowHandle getHandle() const final;

    NativeWindowGLFW() = default;

    explicit NativeWindowGLFW(GLFWwindow* win) : win(win) {}

private:
    GLFWwindow* win = nullptr;
};

} // namespace Brisk
