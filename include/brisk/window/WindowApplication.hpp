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
#pragma once

#include "Types.hpp"
#include <brisk/graphics/Renderer.hpp>
#include <brisk/core/Binding.hpp>
#include <brisk/core/Serialization.hpp>
#include <mutex>
#include <semaphore>
#include <set>

namespace Brisk {

extern bool isStandaloneApp;

class Window;
class WindowApplication;

using WindowWeakPtr = std::weak_ptr<Window>;

namespace Internal {
extern Window* currentWindow;
} // namespace Internal

/**
 * @brief Controls whether the application should render in a separate threads. Default true
 */
extern bool separateRenderThread;

enum class QuitCondition {
    FirstWindowClosed,
    AllWindowsClosed,
    PlatformDependant, // Never on macOS, AllWindowsClosed on others
};

extern WindowApplication* windowApplication;
extern RC<TaskQueue> uiThread;

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
    [[nodiscard]] int run(RC<Window> mainWindow);

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
    void addWindow(RC<Window> window, bool makeVisible = true);

    /**
     * @brief Adds window to the window application and open it as a modal window
     *
     * @param window window to be shown
     */
    template <std::derived_from<Window> TWindow>
    RC<TWindow> showModalWindow(RC<TWindow> window) {
        addWindow(window, false);
        modalRun(window);
        return window;
    }

    template <std::derived_from<Window> TWindow, typename... Args>
    RC<TWindow> showModalWindow(Args&&... args) {
        return showModalWindow(std::make_shared<TWindow>(std::forward<Args>(args)...));
    }

    /**
     * @brief Checks if the specific window is registered to the WindowApplication
     * @remark Safe to call from main or UI thread
     * @param window
     */
    bool hasWindow(const RC<Window>& window);

    void modalRun(RC<Window> modalWindow);

    /**
     * @brief Returns true if the main loop is active
     */
    bool isActive() const;

    /**
     * @brief Returns a copy of the windows list.
     * @remark Safe to call from main or UI thread
     */
    std::vector<RC<Window>> windows() const;

    /**
     * @brief Returns true if @c quit has called
     */
    bool hasQuit() const;

    // Internal methods
    WindowApplication();
    ~WindowApplication();
    void mustBeUIThread();
    double doubleClickTime() const;
    double doubleClickDistance() const;
    RC<TaskQueue> afterRenderQueue;
    RC<TaskQueue> onApplicationClose = rcnew TaskQueue();
    VoidFunc idleFunc();
    void systemModal(function<void(OSWindow*)> body);
    void updateAndWait();

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

    /**
     * @brief Run one cycle of the main loop
     * @param wait Wait for OS events
     * @remark This function is internal. Use only if you know what you do
     */
    void cycle(bool wait);

    QuitCondition quitCondition() const noexcept;
    void setQuitCondition(QuitCondition value);

protected:
    void serialize(const Serialization& serialization) override;

private:
    void processEvents(bool wait);

    void openWindows();
    void closeWindows();
    void removeClosed();

    struct {
        std::vector<RC<Window>> m_windows;
    } m_mainData;

    struct {
        std::vector<RC<Window>> m_windows;
    } m_uiData;

    std::atomic_bool m_active{ false };
    void windowsChanged();
    double m_doubleClickTime            = 0.5;
    double m_doubleClickDistance        = 3.0;
    constexpr static int32_t noExitCode = INT32_MIN;
    std::atomic_int32_t m_exitCode{ noExitCode };
    const bool m_separateRenderThread;
    std::thread m_uiThread;
    std::atomic_bool m_uiThreadTerminate{ false };
    std::atomic_bool m_uiThreadTerminated{ false };
    std::atomic<QuitCondition> m_quitCondition{ QuitCondition::AllWindowsClosed };
    std::binary_semaphore m_uiThreadStarted{ 0 };
    void renderWindows();
    void uiThreadBody();

private:
    std::atomic<bool> m_discreteGPU      = false;
    std::atomic<int> m_syncInterval      = 1;
    std::atomic<float> m_uiScale         = 1;
    std::atomic<bool> m_useMonitorScale  = true;
    std::atomic<float> m_blueLightFilter = 0;
    std::atomic<float> m_globalGamma     = 1;
    std::atomic<bool> m_subPixelText     = true;

    BindingRegistration m_registration{ this, nullptr };

public:
    using This = WindowApplication;

    BRISK_PROPERTIES_BEGIN
    Property<This, bool, &This::m_discreteGPU> discreteGPU;
    Property<This, int, &This::m_syncInterval> syncInterval;
    Property<This, float, &This::m_uiScale> uiScale;
    Property<This, bool, &This::m_useMonitorScale> useMonitorScale;
    Property<This, float, &This::m_blueLightFilter> blueLightFilter;
    Property<This, float, &This::m_globalGamma> globalGamma;
    Property<This, bool, &This::m_subPixelText> subPixelText;
    BRISK_PROPERTIES_END
};
} // namespace Brisk
