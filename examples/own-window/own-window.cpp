#include <brisk/core/internal/Initialization.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/graphics/Palette.hpp>
#include "WindowGlfw.hpp"

namespace Example {

using namespace Brisk;

static void errorfun(int error_code, const char* description) {
    BRISK_ASSERT_MSG(description, false);
}

constexpr int numWindows = 2;

struct OneWindow {
    GLFWwindow* win;
    OsWindowGLFW osWin;
    Rc<WindowRenderTarget> target;
    Rc<RenderEncoder> encoder;
    double previousFrameTime = -1;
    double waitTime;
    double frameInterval;
};

int main() {

    std::array<OneWindow, numWindows> windows;

    registerBuiltinFonts();

    expected<Rc<RenderDevice>, RenderDeviceError> device = getRenderDevice();
    BRISK_ASSERT(device.has_value());

    glfwSetErrorCallback(&errorfun);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    for (int i = 0; i < numWindows; ++i) {
        windows[i].win = glfwCreateWindow(500, 500, "test", nullptr, nullptr);
        BRISK_ASSERT(windows[i].win != nullptr);
        windows[i].osWin = OsWindowGLFW(windows[i].win);
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

    Rc<RenderEncoder> encoder = (*device)->createEncoder();
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
