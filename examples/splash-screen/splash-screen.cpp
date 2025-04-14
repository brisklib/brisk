#include <brisk/core/internal/Initialization.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/gui/GUIApplication.hpp>
#include <brisk/gui/GUIWindow.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/widgets/ImageView.hpp>
#include <brisk/widgets/Progress.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

// Defines a splash screen component that displays an image and a progress bar while the app loads.
class SplashScreen final : public Component {
public:
    using Component::Component;

    // Constructs the splash screen UI with a full-size image and a progress bar.
    RC<Widget> build() final {
        return rcnew Widget{
            layout     = Layout::Vertical, // Arranges widgets in a vertical stack.
            alignItems = Align::Stretch,   // Stretches child widgets to fill the width.
            rcnew ImageView{ Resources::loadCached("image.webp"),
                             flexGrow = 1 }, // Displays an image that expands to fill available space.
            rcnew Progress{
                backgroundColor = 0x000070_rgb,         // Sets a dark blue background for the progress bar.
                minimum         = 0,                    // Sets the minimum progress value.
                maximum         = 100,                  // Sets the maximum progress value.
                value           = Value{ &m_progress }, // Binds the progress bar to the m_progress variable.
                height          = 4_apx,                // Sets the progress bar height to 4 pixels.
                rcnew ProgressBar{ backgroundColor =
                                       0xFFC030_rgb }, // Adds a yellow-orange progress indicator.
            },
        };
    }

    // Configures the splash screen window to be undecorated, topmost, and centered on the screen.
    void configureWindow(RC<GUIWindow> window) final {
        window->setTitle(""); // Sets an empty title for a minimal appearance.
        window->setStyle(WindowStyle::Undecorated | WindowStyle::TopMost | WindowStyle::ExactSize);
        // Undecorated removes borders, TopMost keeps it above other windows, ExactSize prevents DPI scaling.
        Rectangle desktopRect = Display::primary()->workarea(); // Gets the primary display's usable area.
        Rectangle windowRect =
            desktopRect.alignedRect({ 768, 512 }, { 0.5f, 0.5f }); // Centers a 768x512 window.
        window->setRectangle(windowRect);                          // Applies the centered position and size.
    }

    std::atomic_int m_progress{ 0 }; // Tracks loading progress (0-100), starting at 0.
};

// Defines the main application component with a simple UI.
class AppComponent final : public Component {
public:
    explicit AppComponent() {}

    // Builds the main UI with a centered layout, text, and a quit button.
    RC<Widget> build() final {
        return rcnew Widget{
            stylesheet = Graphene::stylesheet(), // Applies the Graphene stylesheet for styling.
            Graphene::darkColors(),              // Sets a dark color theme for the UI.

            layout         = Layout::Vertical, // Arranges child widgets vertically.
            alignItems     = Align::Center,    // Centers child widgets horizontally.
            justifyContent = Justify::Center,  // Centers child widgets vertically.
            gapRow         = 8_px,             // Adds an 8-pixel gap between rows.
            rcnew Text{
                "abc",
                wordWrap = true, // Enables word wrapping for the instructional text.
            },
            rcnew Button{
                rcnew Text{ "Quit" }, // Creates a button labeled "Quit".
                onClick = staticLifetime |
                          []() {
                              guiApplication->quit(); // Exits the application when clicked.
                          },
            },
        };
    }

    // Configures the main application window with a title and standard style.
    void configureWindow(RC<GUIWindow> window) final {
        window->setTitle("Splash Screen Demo"_tr); // Sets a translatable window title.
        window->setSize({ 768, 512 });             // Sets the initial window size to 768x512 pixels.
        window->windowFit =
            WindowFit::MinimumSize; // Ensures the window is at least as large as its content requires.
        window->setStyle(WindowStyle::Normal); // Uses a standard window with borders and controls.
    }
};

} // namespace Brisk

using namespace Brisk;

// Main entry point for the Brisk application.
int briskMain() {
    GUIApplication application; // Initializes the GUI application instance.

    // Creates the splash screen component with an image and progress bar.
    auto splash = createComponent<SplashScreen>();

    // Adds the splash screen window to the application.
    application.addWindow(splash);

    // Starts the application, displaying the splash screen.
    application.start();

    // Simulates a 2-second app loading process, updating the progress bar dynamically.
    auto splashTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);
    while (std::chrono::steady_clock::now() < splashTime) {
        application.cycle(false); // Processes events without blocking during the loading simulation.
        // Updates the progress bar based on remaining time, increasing from 0 to 100.
        splash->m_progress = 100 - std::chrono::duration_cast<std::chrono::milliseconds>(
                                       splashTime - std::chrono::steady_clock::now())
                                           .count() *
                                       100 / 2000;
        bindings->notify(&splash->m_progress); // Notifies the UI to reflect the updated progress value.
    }

    // Adds the main application window after the loading simulation completes.
    application.addWindow(createComponent<AppComponent>());

    // Closes the splash screen window once the main window is ready.
    splash->closeWindow();

    // Enters the main application loop, handling events until the app is closed.
    return application.run();
}
