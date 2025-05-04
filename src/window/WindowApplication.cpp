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

bool isStandaloneApp  = false;
bool separateUIThread = true;

Nullable<WindowApplication> windowApplication;

Rc<TaskQueue> uiScheduler;

void WindowApplication::quit(int exitCode) {
    m_exitCode = exitCode;
    if (Internal::wakeUpMainThread) {
        Internal::wakeUpMainThread();
    }
}

std::vector<Rc<Window>> WindowApplication::windows() const {
    if (isMainThread())
        return m_mainData.m_windows;
    else
        return m_uiData.m_windows;
}

void WindowApplication::serialize(const Serialization& serialization) {
    serialization(Value{ &discreteGpu }, "discreteGpu");
    serialization(Value{ &syncInterval }, "syncInterval");
    serialization(Value{ &uiScale }, "uiScale");
    serialization(Value{ &blueLightFilter }, "blueLightFilter");
    serialization(Value{ &globalGamma }, "globalGamma");
    serialization(Value{ &subPixelText }, "subPixelText");
}

WindowApplication::WindowApplication() : m_separateUIThread(separateUIThread) {
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

    LOG_INFO(window, "Double click time={}s distance={}px", m_doubleClickTime, m_doubleClickDistance);

    PlatformWindow::initialize();

    if (m_separateUIThread) {
        m_uiThread = std::thread(&WindowApplication::uiThreadBody, this);
        m_uiThreadStarted.acquire();
    } else {
        uiScheduler      = rcnew TaskQueue();
        afterRenderQueue = rcnew TaskQueue();
    }

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

    if (m_separateUIThread) {
        m_uiThreadTerminate = true;
        while (!m_uiThreadTerminated) {
            mainScheduler->process();
            std::this_thread::yield();
        }
        m_uiThread.join();
        m_uiThread = {};
    }

    m_mainData.m_windows.clear();

    PlatformWindow::finalize();

    onApplicationClose->process();
    m_mainData        = {};
    m_uiData          = {};
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
    uiScheduler->process();
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
    fonts->garbageCollectCache();
    std::this_thread::sleep_until(stopTime);
}

void WindowApplication::uiThreadBody() {
    uiScheduler      = rcnew TaskQueue();
    afterRenderQueue = rcnew TaskQueue();
    setThreadName("UIThread");
    m_uiThreadStarted.release();
    while (!m_uiThreadTerminate) {
        renderWindows();
    }
    m_uiThreadTerminated = true;
    uiScheduler          = nullptr;
    afterRenderQueue     = nullptr;
}

void WindowApplication::start() {
    mustBeMainThread();
    if (m_active) {
        LOG_WARN(application, "WindowApplication::start called twice");
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
    bool changed       = false;
    QuitCondition cond = m_quitCondition.load(std::memory_order::relaxed);
    for (int i = m_mainData.m_windows.size() - 1; i >= 0; --i) {
        if (m_mainData.m_windows[i]->m_closing) {
            m_mainData.m_windows.erase(m_mainData.m_windows.begin() + i);
            changed = true;
            if (i == 0 && cond == QuitCondition::FirstWindowClosed) {
                quit();
            }
        }
    }
    if (m_mainData.m_windows.empty() && (cond == QuitCondition::AllWindowsClosed
#if !defined BRISK_MACOS
                                         || cond == QuitCondition::PlatformDependant
#endif
                                         )) {
        quit();
    }
    if (changed)
        windowsChanged();
}

void WindowApplication::cycle(bool wait) {
    mustBeMainThread();
    removeClosed();
    processEvents(wait);
    {
        mainScheduler->process();
        processTimers();
        if (!m_separateUIThread)
            renderWindows();
    }
}

void WindowApplication::stop() {
    mustBeMainThread();
    if (!m_active) {
        LOG_WARN(application, "WindowApplication::stop called twice");
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

    return m_exitCode;
}

int WindowApplication::run(Rc<Window> mainWindow) {
    addWindow(std::move(mainWindow));
    return run();
}

bool WindowApplication::hasWindow(const Rc<Window>& window) {
    return mainScheduler->dispatchAndWait([&]() {
        return std::find(m_mainData.m_windows.begin(), m_mainData.m_windows.end(), window) !=
               m_mainData.m_windows.end();
    });
}

VoidFunc WindowApplication::idleFunc() {
    VoidFunc func;
    if (m_separateUIThread && uiScheduler->isOnThread())
        func = [this]() {
            renderWindows();
        };
    return func;
}

void WindowApplication::systemModal(function<void(NativeWindow*)> body) {
    ModalMode modal;

    waitFuture(idleFunc(), mainScheduler->dispatch([&]() {
        body(modal.owner.get());
    }));
}

void WindowApplication::modalRun(Rc<Window> modalWindow) {
    ModalMode modal;
    if (modal.owner) {
        modalWindow->setOwner(modal.owner);
        modalWindow->m_modal = true;
    }

    BRISK_ASSERT(m_active);

    waitFuture(idleFunc(), mainScheduler->dispatch([this, modalWindow]() {
        modalWindow->openWindow();
        while (!hasQuit() && hasWindow(modalWindow)) {
            cycle(true);
        }
    }));
}

void WindowApplication::addWindow(Rc<Window> window, bool makeVisible) {
    waitFuture(idleFunc(), mainScheduler->dispatch([this, window = std::move(window), makeVisible]() {
        m_mainData.m_windows.push_back(window);
        window->attachedToApplication();
        windowsChanged();

        if (makeVisible && m_active) {
            window->openWindow();
        }
    }));
}

void WindowApplication::mustBeUIThread() {
    if (m_uiThread.get_id() != std::thread::id{}) {
        BRISK_ASSERT(std::this_thread::get_id() == m_uiThread.get_id());
    }
}

bool WindowApplication::hasQuit() const {
    return m_exitCode.load() != noExitCode;
}

double WindowApplication::doubleClickDistance() const {
    return m_doubleClickDistance;
}

double WindowApplication::doubleClickTime() const {
    return m_doubleClickTime;
}

void WindowApplication::openWindows() {
    mustBeMainThread();
    for (Rc<Window> w : m_mainData.m_windows) {
        w->openWindow();
    }
}

void WindowApplication::closeWindows() {
    mustBeMainThread();
    for (Rc<Window> w : m_mainData.m_windows) {
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

void WindowApplication::windowsChanged() {
    auto windows = m_mainData.m_windows;
    uiScheduler->dispatch([this, windows = std::move(windows)]() {
        m_uiData.m_windows = std::move(windows);
    });
}
} // namespace Brisk
