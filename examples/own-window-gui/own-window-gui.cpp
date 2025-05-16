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
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/gui/Gui.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/graphics/Palette.hpp>

#include "../own-window/WindowGlfw.hpp"

namespace Example {

using namespace Brisk;

// Error callback function for GLFW
static void errorfun(int error_code, const char* description) {
    BRISK_ASSERT_MSG(description, false);
}

constexpr int numWindows = 2; // Number of windows to create

struct OneWindow {
    GLFWwindow* win;               // Pointer to the GLFW window
    NativeWindowGLFW osWin;        // Window adapter for platform abstraction
    Rc<WindowRenderTarget> target; // Render target associated with the window
    Rc<RenderEncoder> encoder;     // Render encoder for drawing
    Size windowSize;               // Window size in pixels
    Size framebufferSize;          // Framebuffer size in pixels
    float pixelRatio;              // Ratio between framebuffer pixels and GUI pixels
    InputQueue input;              // Input event queue
    WidgetTree tree{ &input };     // Widget tree for GUI management
};

int main() {
    std::array<OneWindow, numWindows> windows; // Array of window instances

    // Register fonts bundled with the application for GUI use
    registerBuiltinFonts();

    // Retrieve the default render device (e.g., GPU)
    // It is possible to get a render device associated with a specific display (not implemented in this
    // example)
    expected<Rc<RenderDevice>, RenderDeviceError> device = getRenderDevice();
    BRISK_ASSERT(device.has_value());

    // Set GLFW error callback
    glfwSetErrorCallback(&errorfun);

    // Initialize GLFW library
    glfwInit();

    // Disable OpenGL context creation for GLFW windows
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create and configure windows
    for (int i = 0; i < numWindows; ++i) {
        // Create a GLFW window (500x500 pixels)
        windows[i].win = glfwCreateWindow(500, 500, "test", nullptr, nullptr);
        BRISK_ASSERT(windows[i].win != nullptr);
        glfwSetWindowUserPointer(windows[i].win, &windows[i]);

        // Retrieve window content scale for high-DPI displays
        // A real application should also respond to scale changes
        SizeF scale;
        glfwGetWindowContentScale(windows[i].win, &scale.x, &scale.y);
        windows[i].pixelRatio = scale.x;

        // Set callback for mouse movement
        glfwSetCursorPosCallback(windows[i].win, [](GLFWwindow* gw, double xpos, double ypos) {
            OneWindow* win = reinterpret_cast<OneWindow*>(glfwGetWindowUserPointer(gw));
            EventMouseMoved event{};
            // Convert mouse coordinates to framebuffer pixels
            event.point = PointF(xpos, ypos) * SizeF(win->framebufferSize) / SizeF(win->windowSize);
            win->input.addEvent(std::move(event));
        });

        // Set callback for mouse button presses and releases
        glfwSetMouseButtonCallback(windows[i].win, [](GLFWwindow* gw, int button, int action, int mods) {
            OneWindow* win = reinterpret_cast<OneWindow*>(glfwGetWindowUserPointer(gw));
            PointOf<double> cur;
            glfwGetCursorPos(gw, &cur.x, &cur.y);
            EventMouseButton event{};
            // Convert mouse coordinates to framebuffer pixels
            event.point = PointF(cur) * SizeF(win->framebufferSize) / SizeF(win->windowSize);
            switch (button) {
            case GLFW_MOUSE_BUTTON_1:
                event.button = MouseButton::Btn1;
                break;
            case GLFW_MOUSE_BUTTON_2:
                event.button = MouseButton::Btn2;
                break;
            case GLFW_MOUSE_BUTTON_3:
                event.button = MouseButton::Btn3;
                break;
            default:
                return;
            }
            event.downPoint = std::nullopt;       // Not implemented in this example
            event.mods      = KeyModifiers::None; // Not implemented in this example
            if (action == GLFW_PRESS) {
                win->input.addEvent(EventMouseButtonPressed{ event });
            } else {
                win->input.addEvent(EventMouseButtonReleased{ event });
            }
        });

        windows[i].osWin  = NativeWindowGLFW(windows[i].win);
        // Create render target for the window
        windows[i].target = (*device)->createWindowTarget(&windows[i].osWin, PixelType::U8);
        windows[i].target->setVSyncInterval(1); // Enable vertical sync
        // Create a render encoder for drawing
        windows[i].encoder = (*device)->createEncoder();
    }

    // Ensure GLFW resources are cleaned up on exit
    SCOPE_EXIT {
        for (int i = 0; i < numWindows; ++i)
            glfwDestroyWindow(windows[i].win);
        glfwTerminate();
    };

    // Initialize GUI for each window
    for (int i = 0; i < numWindows; ++i) {
        // Set pixel ratio for GUI rendering (ratio between framebuffer pixels and GUI pixels)
        pixelRatio() = windows[i].pixelRatio;

        // Create widget tree with a button to close the application
        windows[i].tree.setRoot(rcnew Widget{
            backgroundColor = Palette::Standard::index(i * 7 + 3),
            stylesheet      = Graphene::stylesheet(),
            Graphene::darkColors(),
            alignItems     = Align::Center,
            justifyContent = Justify::Center,
            rcnew Button{
                rcnew Text{ "Close app" },
                onClick = staticLifetime |
                          [&, i]() {
                              glfwSetWindowShouldClose(windows[i].win, GLFW_TRUE);
                          },
            },
        });
    }


    bool exit                 = false;

    // Main application loop
    while (!exit) {
        // Process system events
        glfwPollEvents();

        // Process Brisk events
        mainScheduler->process();
        uiScheduler->process();

        for (int i = 0; i < numWindows; ++i) {
            // Update pixel ratio for rendering (ratio between framebuffer pixels and GUI pixels)
            pixelRatio() = windows[i].pixelRatio;
            // Update window and framebuffer sizes
            glfwGetWindowSize(windows[i].win, &windows[i].windowSize.width, &windows[i].windowSize.height);
            glfwGetFramebufferSize(windows[i].win, &windows[i].framebufferSize.width,
                                   &windows[i].framebufferSize.height);
            // Set GUI viewport size
            windows[i].tree.setViewportRectangle(Rectangle{ Point{ 0, 0 }, windows[i].framebufferSize });

            // Process input events and update widget layout
            windows[i].tree.update();

            // Render the widget tree
            {
                RenderPipeline pipeline(windows[i].encoder, windows[i].target);
                Canvas canvas(pipeline);
                windows[i].tree.paint(canvas, Palette::transparent, true);
            }

            // Present the rendered frame
            windows[i].target->present();

            // Check if the window should close
            if (glfwWindowShouldClose(windows[i].win)) {
                exit = true;
            }
        }
    }
    return 0;
}

} // namespace Example

#ifdef BRISK_WINDOWS
#define NOMINMAX 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

// Windows-specific entry point
int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    // Initialize Brisk with Windows-specific parameters (HINSTANCE and LPWSTR)
    Brisk::startup(hInstance, lpCmdLine);
    int exitCode = 1;
#ifdef BRISK_EXCEPTIONS
    try {
#endif
        exitCode = Example::main();
#ifdef BRISK_EXCEPTIONS
    } catch (std::exception& exc) {
        LOG_DEBUG(application, "Exception occurred: {}", exc.what());
    } catch (...) {
        LOG_DEBUG(application, "Unknown exception occurred");
    }
#endif
    // Clean up Brisk resources
    Brisk::shutdown();
    return exitCode;
}

#else

// Linux/macOS entry point
int main(int argc, char** argv) {
    // Initialize Brisk with command-line arguments (int and char**)
    Brisk::startup(argc, argv);
    int exitCode = 1;
#ifdef BRISK_EXCEPTIONS
    try {
#endif
        exitCode = Example::main();
#ifdef BRISK_EXCEPTIONS
    } catch (std::exception& exc) {
        LOG_DEBUG(application, "Exception occurred: {}", exc.what());
    } catch (...) {
        LOG_DEBUG(application, "Unknown exception occurred");
    }
#endif
    // Clean up Brisk resources
    Brisk::shutdown();
    return exitCode;
}

#endif
