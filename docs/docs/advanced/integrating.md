# Integrating Brisk into an Existing Application

Brisk is a flexible C++ GUI framework that can seamlessly integrate into your existing application without taking control of the event loop, window creation, or application lifetime. By passing Brisk a native window handle, you can render its GUI and manage input events like mouse and keyboard interactions, minimizing disruption to your existing codebase.

!!! note "Example"
    The `own-window-gui.cpp` example in the Brisk repository provides a complete demonstration of integrating Brisk GUI into an existing window. Refer to it for practical guidance.

## Initializing Brisk

To use Brisk, your application must initialize and shut down the framework properly. Call the appropriate `startup` function in your `main` function, depending on your platform, and ensure `shutdown` is called when the application exits.

```c++
#include <brisk/core/Initialization.hpp>

int main(int argc, char* argv[]) {
    // Initialize Brisk
    Brisk::startup(argc, argv);

    // Your application code here

    // Shut down Brisk
    Brisk::shutdown();
    return 0;
}

// or

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
    // Initialize Brisk with Windows-specific parameters (HINSTANCE and LPWSTR)
    Brisk::startup(hInstance, lpCmdLine);

    // Your application code here

    Brisk::shutdown();
}
```

## Global Setup

Before creating windows, set up a render device to handle Brisk's rendering. Use the `getRenderDevice` function and check for errors.

```c++
#include <brisk/graphics/RenderDevice.hpp>

expected<Rc<RenderDevice>, RenderDeviceError> deviceResult = getRenderDevice();
if (!deviceResult) {
    // Handle error (e.g., log error or exit)
    std::cerr << "Failed to create render device: " << deviceResult.error() << std::endl;
    return -1;
}
Rc<RenderDevice> renderDevice = *deviceResult;
```

## Per-Window Setup

Each GUI window in Brisk requires specific components to manage rendering and input. These include a native window implementation, a render target, a render encoder, an input queue, and a widget tree.

```c++
Rc<NativeWindow> nativeWindow; // Custom implementation of NativeWindow interface
Rc<WindowRenderTarget> target; // Render target for the window
Rc<RenderEncoder> encoder;     // Encoder for rendering commands
InputQueue input;             // Queue for input events (mouse, keyboard, etc.)
WidgetTree tree{ &input };    // Widget tree for GUI management
```

### Implementing the NativeWindow Interface

You must create a class that implements the `NativeWindow` interface, providing the framebuffer size and native window handle.

```c++
/// @brief Interface for a native window in Brisk.
class NativeWindow {
public:
    /// @brief Returns the size of the framebuffer.
    /// @return The framebuffer size as a Size object.
    virtual Size framebufferSize() const = 0;

    /// @brief Gets the native OS window handle.
    /// @return The platform-specific window handle.
    virtual NativeWindowHandle getHandle() const = 0;

    virtual ~NativeWindow() = default;
};
```

For example, if you're using GLFW, you might implement it as follows:

```c++
class NativeWindowGLFW : public NativeWindow {
public:
    explicit NativeWindowGLFW(GLFWwindow* win) : win(win) {}

    Size framebufferSize() const override {
        Size size;
        glfwGetFramebufferSize(win, &size.x, &size.y);
        return size;
    }

    NativeWindowHandle getHandle() const override {
#ifdef BRISK_WINDOWS
        return NativeWindowHandle(glfwGetWin32Window(win));
#elif defined(BRISK_MACOS)
        return NativeWindowHandle((NSWindow*)glfwGetCocoaWindow(win));
#elif defined(BRISK_LINUX)
        return NativeWindowHandle(win);
#endif
    }

private:
    GLFWwindow* win;
};
```

### Creating Render Target and Encoder

Create a render target and encoder for each window to enable rendering. Optionally, you can share a single encoder across multiple windows to optimize resource usage.

```c++
nativeWindow = rcnew NativeWindowGLFW(yourGLFWwindow);
target = renderDevice->createWindowTarget(nativeWindow, PixelType::U8);
encoder = renderDevice->createEncoder();
```

## Event Loop Integration

Incorporate Brisk's scheduling into your application's event loop to process GUI updates and input events. Call the `process` method for both the main and UI schedulers.

```c++
#include <brisk/core/Threading.hpp>

void eventLoop() {
    while (/* your loop condition */) {
        mainScheduler->process(); // Process main application tasks
        uiScheduler->process();   // Process UI-related tasks

        // Your existing event loop code
    }
}
```

### Rendering GUI Windows

Add the following code to your rendering loop for each GUI window to update the widget tree, render the GUI, and present the frame.

```c++
// Set the viewport to match the framebuffer size
tree.setViewportRectangle(Rectangle{ Point{ 0, 0 }, nativeWindow->framebufferSize() });

// Process input events and update widget layout
tree.update();

// Render the GUI
{
    RenderPipeline pipeline(encoder, target);
    Canvas canvas(pipeline);
    tree.paint(canvas, Palette::transparent, true);
}

// Present the rendered frame to the display
target->present();
```

## Handling Input Events

To enable user interaction with your Brisk GUI, you must capture input events from your windowing system and forward them to Brisk's input queue. Below is an example of handling mouse movement and button events using GLFW, which you can adapt to your windowing system.

### Mouse Movement

Set up a callback to handle mouse cursor movement and convert the coordinates to framebuffer pixels before adding the event to the input queue.

```c++
/// @brief Sets the callback for mouse movement events.
/// @param gw The GLFW window handle.
/// @param xpos The x-coordinate of the cursor in window space.
/// @param ypos The y-coordinate of the cursor in window space.
glfwSetCursorPosCallback(win, [](GLFWwindow* gw, double xpos, double ypos) {
    OneWindow* win = reinterpret_cast<OneWindow*>(glfwGetWindowUserPointer(gw));
    EventMouseMoved event{};
    // Convert window coordinates to framebuffer pixels
    event.point = PointF(xpos, ypos) * SizeF(win->framebufferSize) / SizeF(win->windowSize);
    win->input.addEvent(std::move(event));
});
```

### Mouse Button Presses and Releases

Handle mouse button events by mapping GLFW button codes to Brisk's `MouseButton` enum and distinguishing between press and release actions.

```c++
/// @brief Sets the callback for mouse button press and release events.
/// @param gw The GLFW window handle.
/// @param button The mouse button identifier.
/// @param action The action (GLFW_PRESS or GLFW_RELEASE).
/// @param mods Modifier keys (not used in this example).
glfwSetMouseButtonCallback(win, [](GLFWwindow* gw, int button, int action, int mods) {
    OneWindow* win = reinterpret_cast<OneWindow*>(glfwGetWindowUserPointer(gw));
    PointOf<double> cur;
    glfwGetCursorPos(gw, &cur.x, &cur.y);
    EventMouseButton event{};
    // Convert window coordinates to framebuffer pixels
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
        return; // Ignore unsupported buttons
    }
    event.downPoint = std::nullopt;       // Not implemented in this example
    event.mods      = KeyModifiers::None; // Not implemented in this example
    if (action == GLFW_PRESS) {
        win->input.addEvent(EventMouseButtonPressed{ event });
    } else {
        win->input.addEvent(EventMouseButtonReleased{ event });
    }
});
```

### Notes on Event Handling

- **Coordinate Conversion**: Mouse coordinates from GLFW are in window space, so you must scale them to framebuffer pixels to match Brisk's coordinate system. This ensures accurate interaction with GUI elements.
- **Window Association**: The `OneWindow` struct (or equivalent) should store the window's framebuffer size, window size, and input queue, accessible via the GLFW window's user pointer.
- **Extensibility**: You can extend this approach to handle other input events, such as keyboard input or mouse wheel scrolling, by setting up additional GLFW callbacks and mapping them to Brisk's event types.

## Creating and Managing Widgets

Define your GUI by setting a root widget for the widget tree. Widgets can be styled and configured with properties like alignment, colors, and event handlers. Below is an example of creating a centered button that closes the application when clicked.

```c++
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Text.hpp>

tree.setRoot(rcnew Widget{
    stylesheet      = Graphene::stylesheet(),
    Graphene::darkColors(),
    alignItems      = Align::Center,
    justifyContent  = Justify::Center,
    rcnew Button{
        rcnew Text{ "Close App" },
        onClick = staticLifetime | []() {
            // Trigger application exit (e.g., set a flag or call exit)
        },
    },
});
```
