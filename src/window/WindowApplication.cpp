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
#include <brisk/window/WindowApplication.hpp>
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/Settings.hpp>
#include <brisk/core/Utilities.hpp>

#include <brisk/window/Window.hpp>
#include <brisk/core/Log.hpp>

#include "PlatformWindow.hpp"
#include <brisk/graphics/Fonts.hpp>

namespace Brisk {

bool isStandaloneApp = false;

Nullable<WindowApplication> windowApplication;

void WindowApplication::quit(int exitCode) {
    mustBeMainThread();
    m_exitCode = exitCode;
}

const std::vector<Rc<Window>>& WindowApplication::windows() const {
    mustBeMainThread();
    return m_windows;
}

void WindowApplication::serialize(const Serialization& serialization) {
    serialization(Value{ &discreteGpu }, "discreteGpu");
    serialization(Value{ &syncInterval }, "syncInterval");
    serialization(Value{ &uiScale }, "uiScale");
    serialization(Value{ &blueLightFilter }, "blueLightFilter");
    serialization(Value{ &globalGamma }, "globalGamma");
    serialization(Value{ &subPixelText }, "subPixelText");
}

WindowApplication::WindowApplication() {
    BRISK_ASSERT(windowApplication.get() == nullptr);
    windowApplication = this;
    mustBeMainThread();

    BRISK_ASSERT(mainScheduler);
    Internal::wakeUpMainThread = []() {
        PlatformWindow::postEmptyEvent();
    };

    auto dblClickParams   = PlatformWindow::dblClickParams();
    m_doubleClickTime     = dblClickParams.time;
    m_doubleClickDistance = dblClickParams.distance;

    BRISK_LOG_INFO("Double click time={}s distance={}px", m_doubleClickTime, m_doubleClickDistance);

    PlatformWindow::initialize();

    afterRenderQueue = rcnew TaskQueue();

    if (settings) {
        Json data = settings->data("/display");
        deserializeFrom(data);
    }
}

WindowApplication::~WindowApplication() {
    BRISK_ASSERT(windowApplication.get() == this);
    mustBeMainThread();

    if (settings) {
        Json data;
        serializeTo(data);
        settings->setData("/display", data);
    }

    m_windows.clear();

    PlatformWindow::finalize();

    onApplicationClose->process();
    windowApplication = nullptr;
}

void WindowApplication::processEvents(bool wait) {
    mustBeMainThread();

    if (wait)
        PlatformWindow::waitEvents();
    else
        PlatformWindow::pollEvents();
}

constexpr static int maximumFPS = 120;

void WindowApplication::renderWindows() {
    mustBeMainThread();
    using std::chrono::steady_clock;
    steady_clock::time_point stopTime = steady_clock::now() + std::chrono::milliseconds(1000 / maximumFPS);
    std::vector<Rc<Window>> windows   = this->windows();
    for (Rc<Window> w : windows) {
        if (w->m_rendering) {
            Window* curWindow = w.get();
            std::swap(Internal::currentWindow, curWindow);
            Internal::currentWindow = w.get();
            SCOPE_EXIT {
                std::swap(Internal::currentWindow, curWindow);
            };
            w->doPaint();
        }
    }
    afterRenderQueue->process();
    std::this_thread::sleep_until(stopTime);
}

void WindowApplication::start() {
    mustBeMainThread();
    if (m_active) {
        BRISK_LOG_WARN("WindowApplication::start called twice");
        return;
    }
    m_active = true;
    openWindows();
}

void WindowApplication::updateAndWait() {
    mustBeMainThread();
    removeClosed();
    if (!afterRenderQueue->isOnThread())
        afterRenderQueue->waitForCompletion();
}

void WindowApplication::removeClosed() {
    mustBeMainThread();
    bool changed = false;
    for (int i = m_windows.size() - 1; i >= 0; --i) {
        if (m_windows[i]->m_closing) {
            m_windows.erase(m_windows.begin() + i);
            changed = true;
            if (i == 0 && m_quitCondition == QuitCondition::FirstWindowClosed) {
                quit();
            }
        }
    }
    if (m_windows.empty() && (m_quitCondition == QuitCondition::AllWindowsClosed
#if !defined BRISK_MACOS
                              || m_quitCondition == QuitCondition::PlatformDependant
#endif
                              )) {
        quit();
    }
}

void WindowApplication::cycle(bool wait) {
    mustBeMainThread();
    removeClosed();
    processEvents(wait);
    {
        mainScheduler->process();
        processTimers();
        renderWindows();
    }
}

void WindowApplication::stop() {
    mustBeMainThread();
    if (!m_active) {
        BRISK_LOG_WARN("WindowApplication::stop called twice");
        return;
    }
    closeWindows();
    m_active = false;
}

int WindowApplication::run() {
    mustBeMainThread();

    start();

    while (!hasQuit()) {
        cycle(true);
    }

    stop();

    return m_exitCode.value_or(0);
}

int WindowApplication::run(Rc<Window> mainWindow) {
    addWindow(std::move(mainWindow));
    return run();
}

bool WindowApplication::hasWindow(const Rc<Window>& window) {
    return std::find(m_windows.begin(), m_windows.end(), window) != m_windows.end();
}

void WindowApplication::systemModal(function<void(NativeWindow*)> body) {
    ModalMode modal;
    body(modal.owner.get());
}

void WindowApplication::modalRun(Rc<Window> modalWindow) {
    ModalMode modal;
    if (modal.owner) {
        modalWindow->setOwner(modal.owner);
        modalWindow->m_modal = true;
    }

    BRISK_ASSERT(m_active);

    modalWindow->openWindow();
    while (!hasQuit() && hasWindow(modalWindow)) {
        cycle(true);
    }
}

void WindowApplication::addWindow(Rc<Window> window, bool makeVisible) {
    m_windows.push_back(window);
    window->attachedToApplication();

    if (makeVisible && m_active) {
        window->openWindow();
    }
}

bool WindowApplication::hasQuit() const {
    return m_exitCode.has_value();
}

double WindowApplication::doubleClickDistance() const {
    return m_doubleClickDistance;
}

double WindowApplication::doubleClickTime() const {
    return m_doubleClickTime;
}

void WindowApplication::openWindows() {
    mustBeMainThread();
    for (Rc<Window> w : m_windows) {
        w->openWindow();
    }
}

void WindowApplication::closeWindows() {
    mustBeMainThread();
    for (Rc<Window> w : m_windows) {
        w->closeWindow();
    }
}

bool WindowApplication::isActive() const {
    return m_active;
}

void WindowApplication::setQuitCondition(QuitCondition value) {
    m_quitCondition = value;
}

QuitCondition WindowApplication::quitCondition() const noexcept {
    return m_quitCondition;
}
} // namespace Brisk
