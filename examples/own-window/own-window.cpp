#define BRISK_ALLOW_OS_HEADERS 1
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/gui/GUIApplication.hpp>
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/graphics/Palette.hpp>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
extern "C" id objc_retain(id value);
#endif
#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#undef None

namespace Example {

using namespace Brisk;
using Brisk::Font;
using Brisk::Rectangle;

class OSWindowGLFW final : public OSWindow {
public:
    Size framebufferSize() const final {
        Size size;
        glfwGetFramebufferSize(win, &size.x, &size.y);
        return size;
    }

    OSWindowHandle getHandle() const final {
#ifdef BRISK_WINDOWS
        return OSWindowHandle(glfwGetWin32Window(win));
#endif
#ifdef BRISK_MACOS
        return OSWindowHandle((NSWindow*)glfwGetCocoaWindow(win));
#endif
#ifdef BRISK_LINUX
        return OSWindowHandle(win);
#endif
    }

    OSWindowGLFW() = default;

    explicit OSWindowGLFW(GLFWwindow* win) : win(win) {}

private:
    GLFWwindow* win = nullptr;
};

static void errorfun(int error_code, const char* description) {
    BRISK_ASSERT_MSG(description, false);
}

constexpr int numWindows = 2;

struct OneWindow {
    GLFWwindow* win;
    OSWindowGLFW osWin;
    RC<WindowRenderTarget> target;
    RC<RenderEncoder> encoder;
    double previousFrameTime = -1;
    double waitTime;
    double frameInterval;
};

int main() {

    std::array<OneWindow, numWindows> windows;

    registerBuiltinFonts();

    expected<RC<RenderDevice>, RenderDeviceError> device = getRenderDevice();
    BRISK_ASSERT(device.has_value());

    glfwSetErrorCallback(&errorfun);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    for (int i = 0; i < numWindows; ++i) {
        windows[i].win = glfwCreateWindow(500, 500, "test", nullptr, nullptr);
        BRISK_ASSERT(windows[i].win != nullptr);
        windows[i].osWin = OSWindowGLFW(windows[i].win);
    }
    SCOPE_EXIT {
        for (int i = 0; i < numWindows; ++i)
            glfwDestroyWindow(windows[i].win);
        glfwTerminate();
    };
    for (int i = 0; i < numWindows; ++i) {
        windows[i].target = (*device)->createWindowTarget(&windows[i].osWin, PixelType::U8);
        windows[i].target->setVSyncInterval(1);
    }

    RC<RenderEncoder> encoder = (*device)->createEncoder();
    double sumWaitTimeR       = -1;

    bool exit                 = false;

    while (!exit) {
        glfwPollEvents();
        double sumWaitTime = 0;
        for (int i = 0; i < numWindows; ++i) {
            sumWaitTime += windows[i].frameInterval;
        }
        sumWaitTime /= numWindows;
        if (sumWaitTimeR >= 0)
            sumWaitTime = mix(0.9, sumWaitTime, sumWaitTimeR);
        sumWaitTimeR = sumWaitTime;
        for (int i = 0; i < numWindows; ++i) {
            Size winSize;
            glfwGetFramebufferSize(windows[i].win, &winSize.width, &winSize.height);
            Rectangle bounds = Rectangle{ Point{ 0, 0 }, winSize };
            Rectangle inner  = bounds.withPadding(40);
            {
                RenderPipeline pipeline(encoder, windows[i].target, 0x222426_rgb);
                static int frame = 0;
                ++frame;
                Canvas canvas(pipeline);
                canvas.setStrokeColor(Palette::black);
                canvas.setStrokeWidth(16.f);
                canvas.setFillPaint(LinearGradient(inner.at(0.f, 0.f), inner.at(1.f, 1.f),
                                                   Palette::Standard::green, Palette::Standard::red));
                canvas.drawRect(inner, inner.shortestSide() * 0.5f);
                canvas.setFillColor(Palette::white);
                canvas.setFont(Font{ Font::Default, 40.f });
                canvas.fillText(fmt::format(R"({}x{}
    wait = {:.1f}ms
    total = {:.1f}ms 
    rate = {:.1f}fps)",
                                            winSize.x, winSize.y, 1000 * windows[i].waitTime,
                                            1000 * sumWaitTime, 1.0 / sumWaitTime),
                                inner, PointF(0.5f, 0.5f));
                canvas.setFillColor(Palette::black);
                canvas.fillRect(Rectangle{ Point{ frame % winSize.x, 0 }, Size{ 5, winSize.y } });
            }
        }
        for (int i = 0; i < numWindows; ++i) {
            double beforeFrameTime = currentTime();
            windows[i].target->present();
            double frameTime             = currentTime();
            double previousFrameTime     = windows[i].previousFrameTime;
            windows[i].previousFrameTime = frameTime;
            windows[i].frameInterval     = frameTime - previousFrameTime;
            windows[i].waitTime          = frameTime - beforeFrameTime;

            if (glfwWindowShouldClose(windows[i].win)) {
                exit = true;
            }
        }
    }
    return 0;
}

} // namespace Example

int briskMain() {
    return Example::main();
}
