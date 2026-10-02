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
#pragma once

#include <mutex>
#include <semaphore>
#include <set>

#include <brisk/core/Binding.hpp>
#include <brisk/core/Serialization.hpp>
#include <brisk/graphics/Renderer.hpp>

#include "Types.hpp"

namespace Brisk {

extern bool isStandaloneApp;

class Window;
class WindowApplication;

using WindowWeakPtr = std::weak_ptr<Window>;

namespace Internal {
extern Window* currentWindow;
} // namespace Internal

enum class QuitCondition {
    FirstWindowClosed,
    AllWindowsClosed,
    Never,
    PlatformDependant, // Never on macOS, AllWindowsClosed on others
};

extern Nullable<WindowApplication> windowApplication;

class WindowApplication : public SerializableInterface {
public:
    /**
     * @brief Runs the application event loop.
     *
     * Creates PlatformWindow for each added Window instance and makes it visible if not hidden
     *
     * @param idle Function to call on every cycle
     * @return int Exit code if @c quit called, otherwise 0
     */
    [[nodiscard]] int run();

    /**
     * @brief Sets main window and runs the application event loop
     *
     * @see @ref WindowApplication::run()
     */
    [[nodiscard]] int run(Rc<Window> mainWindow);

    /**
     * @brief Quits the application and returns from run
     *
     * @param exitCode Exit code to return from run
     */
    void quit(int exitCode = 0);

    /**
     * @brief Adds window to the window application
     *
     * @param window window to add
     */
    void addWindow(Rc<Window> window, bool makeVisible = true);

    /**
     * @brief Adds window to the window application and open it as a modal window
     *
     * @param window window to be shown
     */
    template <std::derived_from<Window> TWindow>
    Rc<TWindow> showModalWindow(Rc<TWindow> window) {
        addWindow(window, false);
        modalRun(window);
        return window;
    }

    template <std::derived_from<Window> TWindow, typename... Args>
    Rc<TWindow> showModalWindow(Args&&... args) {
        return showModalWindow(std::make_shared<TWindow>(std::forward<Args>(args)...));
    }

    /**
     * @brief Checks if the specific window is registered to the WindowApplication
     * @param window
     */
    bool hasWindow(const Rc<Window>& window);

    void modalRun(Rc<Window> modalWindow);

    /**
     * @brief Returns true if the main loop is active
     */
    bool isActive() const;

    /**
     * @brief Returns the windows list.
     */
    const std::vector<Rc<Window>>& windows() const;

    /**
     * @brief Returns true if @c quit has called
     */
    bool hasQuit() const;

    // Internal methods
    WindowApplication();
    ~WindowApplication();
    double doubleClickTime() const;
    double doubleClickDistance() const;
    Callbacks<> onApplicationClose;
    void systemModal(function<void(NativeWindow*)> body);

    /**
     * @brief Start the main loop
     * @remark This function is internal. Use only if you know what you do
     */
    void start();

    /**
     * @brief Stops the main loop
     * @remark This function is internal. Use only if you know what you do
     */
    void stop();

    // Enum with three options: check messages, check messages and wait for them, don't check messages
    // Rephrase to get correct wording
    enum class ProcessEventsMode {
        CheckAndWait,
        CheckOnly,
        DontCheck,
    };

    /**
     * @brief Run one cycle of the main loop
     * @param mode Mode of processing events
     * @remark This function is internal. Use only if you know what you do
     */
    void cycle(ProcessEventsMode mode);

    QuitCondition quitCondition() const noexcept;
    void setQuitCondition(QuitCondition value);

protected:
    void serialize(const Serialization& serialization) override;

private:
    void processEvents(bool wait);

    void openWindows();
    void closeWindows();
    void removeClosed();

    std::vector<Rc<Window>> m_windows;

    bool m_active                = false;
    double m_doubleClickTime     = 0.5;
    double m_doubleClickDistance = 3.0;
    std::optional<int32_t> m_exitCode;
    QuitCondition m_quitCondition = QuitCondition::AllWindowsClosed;
    void renderWindows();

private:
    bool m_discreteGpu      = false;
    int m_syncInterval      = 1;
    float m_uiScale         = 1;
    float m_blueLightFilter = 0;
    float m_globalGamma     = 1;
    bool m_subPixelText     = true;

    BindingRegistration m_registration{ this };

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &WindowApplication::m_discreteGpu, "discreteGpu" },
            /*1*/ Internal::PropField{ &WindowApplication::m_syncInterval, "syncInterval" },
            /*2*/ Internal::PropField{ &WindowApplication::m_uiScale, "uiScale" },
            /*3*/ Internal::PropField{ &WindowApplication::m_blueLightFilter, "blueLightFilter" },
            /*4*/ Internal::PropField{ &WindowApplication::m_globalGamma, "globalGamma" },
            /*5*/ Internal::PropField{ &WindowApplication::m_subPixelText, "subPixelText" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<WindowApplication, bool, 0> discreteGpu;
    Property<WindowApplication, int, 1> syncInterval;
    Property<WindowApplication, float, 2> uiScale;
    Property<WindowApplication, float, 3> blueLightFilter;
    Property<WindowApplication, float, 4> globalGamma;
    Property<WindowApplication, bool, 5> subPixelText;
    BRISK_PROPERTIES_END
};
} // namespace Brisk
