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
#include <brisk/window/Window.hpp>

#include <atomic>

#include <chrono>
#include <future>
#include <memory>
#include <spdlog/fmt/fmt.h>

#include <brisk/core/Encoding.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/RawCanvas.hpp>
#include <brisk/graphics/SVG.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/core/Compression.hpp>
#include <brisk/core/internal/AutoreleasePool.hpp>

#include "FrameTimePredictor.hpp"
#include "PlatformWindow.hpp"

namespace Brisk {

double frameStartTime = 0.0;
static BindingRegistration frameStartTime_reg{ &frameStartTime, nullptr };

namespace Internal {

std::atomic_bool debugShowRenderTimeline{ false };
BRISK_UI_THREAD Window* currentWindow = nullptr;

RC<Window> currentWindowPtr() {
    return currentWindow ? currentWindow->shared_from_this() : nullptr;
}
} // namespace Internal

void Window::iconify() {
    mustBeUIThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->iconify();
    });
}

void Window::maximize() {
    mustBeUIThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->maximize();
    });
}

void Window::restore() {
    mustBeUIThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->restore();
    });
}

void Window::focus() {
    mustBeUIThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->focus();
    });
}

void Window::mustBeUIThread() const {
    if (m_attached && windowApplication)
        windowApplication->mustBeUIThread();
}

void Window::setVisible(bool newVisible) {
    mustBeUIThread();
    // Do not compare with current value of m_visible to allow setting the same value
    m_visible = newVisible;
    if (!m_platformWindow) {
        return;
    }
    mainScheduler->dispatch([this]() {
        m_platformWindow->updateVisibility();
    });
}

bool Window::isFocused() const {
    mustBeUIThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([this] {
        return m_platformWindow->isFocused();
    });
}

bool Window::isIconified() const {
    mustBeUIThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([this] {
        return m_platformWindow->isIconified();
    });
}

bool Window::isMaximized() const {
    mustBeUIThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([=, this] {
        return m_platformWindow->isMaximized();
    });
}

bool Window::isVisible() const {
    mustBeUIThread();
    return m_visible;
}

Size Window::getSize() const {
    mustBeUIThread();
    return m_windowSize;
}

Size Window::getFramebufferSize() const {
    mustBeUIThread();
    if (!m_platformWindow)
        return {};
    return m_framebufferSize;
}

Size Window::framebufferSize() const {
    // OSWindow interface (Renderer)
    return m_framebufferSize;
}

std::string Window::getTitle() const {
    mustBeUIThread();
    return m_title;
}

void Window::setTitle(std::string title) {
    mustBeUIThread();
    if (title != m_title) {
        m_title = std::move(title);
        mainScheduler->dispatch([=, this] {
            if (m_platformWindow)
                m_platformWindow->setTitle(m_title);
        });
    }
}

void Window::setRectangle(Rectangle rect) {
    mustBeUIThread();
    // Do not compare with current values of m_desired* to allow setting the same value
    m_position   = rect.p1;
    m_windowSize = rect.size();
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow) {
            m_platformWindow->setPosition(m_position);
            m_platformWindow->setSize(m_windowSize);
        }
    });
}

void Window::setPosition(Point pos) {
    mustBeUIThread();
    // Do not compare with current values of m_desired* to allow setting the same value
    m_position = pos;
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow)
            m_platformWindow->setPosition(m_position);
    });
}

void Window::setMinimumSize(Size size) {
    mustBeUIThread();
    // Do not compare with current values of m_minimumSize to allow setting the same value
    m_minimumSize = size;
    m_maximumSize = Size{ PlatformWindow::dontCare, PlatformWindow::dontCare };
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow)
            m_platformWindow->setSizeLimits(m_minimumSize, m_maximumSize);
    });
}

void Window::setFixedSize(Size size) {
    mustBeUIThread();
    // Do not compare with current values of m_fixedSize to allow setting the same value
    m_minimumSize = size;
    m_maximumSize = size;
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow)
            m_platformWindow->setSizeLimits(m_minimumSize, m_maximumSize);
    });
}

void Window::setSize(Size size) {
    mustBeUIThread();
    if (size != m_windowSize) {
        m_windowSize = size;
        mainScheduler->dispatch([this, size = m_windowSize] {
            if (m_platformWindow)
                m_platformWindow->setSize(size);
        });
    }
}

void Window::setStyle(WindowStyle style) {
    mustBeUIThread();
    if (style != m_style) {
        m_style = style;
        mainScheduler->dispatch([style = m_style, this] {
            if (m_platformWindow)
                m_platformWindow->setStyle(style);
        });
    }
}

void Window::determineWindowDPI() {
    const float spr = (m_useMonitorScale.load() ? m_windowPixelRatio.load() : 1.f) * m_pixelRatioScale;
    if (m_canvasPixelRatio != spr) {
        m_canvasPixelRatio = spr;
        LOG_INFO(window, "Pixel Ratio for window = {} scaled: {}", m_windowPixelRatio.load(), spr);
        uiScheduler->dispatch([this] {
            Brisk::pixelRatio() = m_canvasPixelRatio;
            dpiChanged();
        });
    }
}

void Window::windowPixelRatioChanged() {
    //
}

void Window::visibilityChanged(bool newIsVisible) {
    onVisibilityChanged(newIsVisible);
}

void Window::attachedToApplication() {
    m_attached = true;
    bindings->connect(Value{ &m_pixelRatioScale, this, &Window::determineWindowDPI },
                      Value{ &windowApplication->uiScale });
    bindings->connect(Value{ &m_useMonitorScale, this, &Window::determineWindowDPI },
                      Value{ &windowApplication->useMonitorScale });
    bindings->connect(Value{ &m_syncInterval,
                             [this]() {
                                 if (m_target)
                                     m_target->setVSyncInterval(m_syncInterval);
                             } },
                      Value{ &windowApplication->syncInterval });
}

Window::Window() {
    m_frameTimePredictor.reset(new Internal::FrameTimePredictor{});

    LOG_INFO(window, "Done creating Window");
}

using high_res_clock = std::chrono::steady_clock;

void Window::paintDebug(RenderContext& context) {
    RawCanvas canvas(context);

    if (Internal::debugShowRenderTimeline) {
        Size framebufferSize = m_framebufferSize;
        const int lanes      = 6;
        canvas.drawRectangle(Rectangle{ 0, framebufferSize.height - lanes * idp(laneHeight),
                                        framebufferSize.width, framebufferSize.height },
                             0.f, 0.f, strokeWidth = 0.f, fillColor = 0x000000'D0_rgba);
        renderDebugTimeline("updateAndPaint ", canvas, m_drawingPerformance, 0, 0, 50.f * 60.f);
        renderDebugTimeline("render         ", canvas, m_renderPerformance, 1, 2, 50.f * 60.f);
        renderDebugTimeline("swap           ", canvas, m_swapPerformance, 2, 4, 50.f * 60.f);
        renderDebugTimeline("blit           ", canvas, m_blitPerformance, 3, 6, 50.f * 60.f);
        renderDebugTimeline("vblank         ", canvas, m_vblankPerformance, 4, 8, 50.f * 60.f);
        renderDebugInfo(canvas, 5);
    }
}

void Window::doPaint() {
    PerformanceDuration start_time = perfNow();
    ObjCPool pool;
    high_res_clock::time_point renderStart;

    renderStart                                 = high_res_clock::now();

    pixelRatio()                                = m_canvasPixelRatio;

    [[maybe_unused]] const Size framebufferSize = m_framebufferSize;

    currentFramePresentationTime                = m_nextFrameTime.value_or(now());

    constexpr size_t reserveCommands            = 100;
    constexpr size_t reserveData                = 65536;

    RC<RenderTarget> target                     = m_target;
    if (!m_captureCallback.empty()) {
        auto device                       = getRenderDevice();
        RC<ImageRenderTarget> imageTarget = (*device)->createImageTarget(m_target->size());
        m_capturedFrame                   = imageTarget->image();
        target                            = imageTarget;
    }
    m_encoder->setVisualSettings(m_renderSettings);

    beforeFrame();

    PerformanceDuration gpuDuration = PerformanceDuration(0);
    {
        RenderPipeline pipeline(m_encoder, target);
        paint(pipeline);
        paintDebug(pipeline);
    }

    if (m_capturedFrame) {
        m_encoder->setVisualSettings(VisualSettings{});
        RenderPipeline pipeline2(m_encoder, m_target);
        RawCanvas canvas(pipeline2);
        canvas.drawTexture(RectangleF(PointF(0, 0), m_target->size()), m_capturedFrame, Matrix{});
    }

    // region Print Performance Counters
    if (m_statTimer.elapsed(1.0)) {
        m_drawingPerformance.report();
        m_renderPerformance.report();
        m_swapPerformance.report();
        m_gpuPerformance.report();
        m_vblankPerformance.report();
        m_uiThreadPerformance.report();

        m_drawingPerformance.reset();
        m_renderPerformance.reset();
        m_swapPerformance.reset();
        m_gpuPerformance.reset();
        m_vblankPerformance.reset();
        m_uiThreadPerformance.reset();
    }

    m_lastFrameRenderTime =
        std::chrono::duration_cast<std::chrono::microseconds>(high_res_clock::now() - renderStart);

    auto pNow = perfNow();
    m_uiThreadPerformance.addMeasurement(start_time, pNow - gpuDuration);
    m_swapPerformance.addMeasurement(pNow - gpuDuration, pNow);

    {
        Stopwatch w(m_swapPerformance);
        m_target->present();
    }
    if (m_capturedFrame) {
        m_encoder->wait();
        auto captureCallback = std::move(m_captureCallback);
        m_captureCallback    = nullptr;
        auto capturedFrame   = std::move(m_capturedFrame);
        m_capturedFrame      = nullptr;

        std::move(captureCallback)(std::move(capturedFrame));
    }
    m_frameTimePredictor->markFrameTime();
    m_nextFrameTime = m_frameTimePredictor->predictNextFrameTime();

    ++m_frameNumber;
}

void Window::beforeDestroying() {}

Window::~Window() {
    mainScheduler->dispatchAndWait([this]() {
        closeWindow();
    });
}

WindowStyle Window::getStyle() const {
    mustBeUIThread();
    return m_style;
}

PointOf<float> Window::getMousePosition() const {
    mustBeUIThread();
    return m_mousePoint;
}

PointOf<int> Window::getPosition() const {
    mustBeUIThread();
    return m_position;
}

void Window::setCursor(Cursor cursor) {
    mustBeUIThread();
    if (cursor != m_cursor) {
        m_cursor = cursor;
        mainScheduler->dispatch([=, this] {
            if (m_platformWindow)
                m_platformWindow->setCursor(m_cursor);
        });
    }
}

float Window::canvasPixelRatio() const {
    return m_canvasPixelRatio;
}

float Window::windowPixelRatio() const {
    return m_windowPixelRatio;
}

Rectangle Window::getBounds() const {
    mustBeUIThread();
    return Rectangle{ Point(0, 0), m_windowSize };
}

Rectangle Window::getRectangle() const {
    mustBeUIThread();
    return Rectangle{ m_position, m_windowSize };
}

Rectangle Window::getFramebufferBounds() const {
    mustBeUIThread();
    return Rectangle{ Point(0, 0), m_framebufferSize };
}

void Window::show() {
    setVisible(true);
}

void Window::hide() {
    setVisible(false);
}

void Window::close() {
    hide();
    m_closing = true; // forces application to remove this window from the windows list
    mainScheduler->dispatch([this]() {
        closeWindow(); // destroys m_platformWindow
    });
}

void Window::getHandle(OSWindowHandle& handle) const {
    if (!m_platformWindow)
        return;
    mainScheduler->dispatchAndWait([&] {
        m_platformWindow->getHandle(handle);
    });
}

void Window::initializeRenderer() {
    if (m_target)
        return;
    auto device = getRenderDevice();
    m_encoder   = (*device)->createEncoder();
    m_target    = (*device)->createWindowTarget(this);
    m_target->setVSyncInterval(m_syncInterval);
}

void Window::finalizeRenderer() {
    if (!m_target)
        return;
    m_target.reset();
    m_encoder.reset();
}

Bytes Window::windowPlacement() const {
    mustBeUIThread();
    if (!m_platformWindow) {
        return {};
    }
    return uiScheduler->dispatchAndWait([this]() -> Bytes {
        return m_platformWindow->placement();
    });
}

void Window::setWindowPlacement(BytesView data) {
    mustBeUIThread();
    if (!m_platformWindow) {
        return;
    }
    uiScheduler->dispatchAndWait([this, data]() { // must wait, otherwise dangling reference
        m_platformWindow->setPlacement(data);
    });
}

void Window::renderDebugInfo(RawCanvas& canvas, int lane) {
    const int laneY    = getFramebufferSize().height - (lane + 1) * idp(laneHeight);
    auto info          = (*getRenderDevice())->info();
    std::string status = fmt::format("{}x{} pixel={} device=[{}] {} commands={} distinct={} data={}kb",
                                     getFramebufferSize().width, getFramebufferSize().height,
                                     m_canvasPixelRatio.load(), info.api, info.device, 0, 0, 0);

    canvas.drawText(RectangleF(0, laneY, getFramebufferSize().width, laneY + idp(laneHeight) - 1), 0.f, 0.5f,
                    status, Font{ Font::Default, dp(12) }, Palette::white);
}

void Window::renderDebugTimeline(const std::string& title, RawCanvas& canvas,
                                 const PerformanceStatistics& stat, int lane, int color,
                                 double pixelsPerSecond) {
    const int laneY = getFramebufferSize().height - (lane + 1) * idp(laneHeight);
    int index       = stat.slicesPos() - 1;
    int counter     = 0;
    double now      = toSeconds(currentFramePresentationTime - appStartTime);
    double maxTime  = getFramebufferSize().width / dp(pixelsPerSecond);
    while (index >= 0 && counter <= stat.slices().size()) {
        PerformanceStatistics::TimeSlice slice = stat.slices()[index % stat.slices().size()];
        if (now - toSeconds(slice.start) > maxTime * 0.9)
            break;
        float sliceOffset = std::fmod(toSeconds(slice.start), maxTime) * dp(pixelsPerSecond);
        float sliceEnd = sliceOffset + (toSeconds(slice.stop) - toSeconds(slice.start)) * dp(pixelsPerSecond);
        RectangleF r(sliceOffset, laneY, sliceEnd, laneY + idp(laneHeight) - 1);
        r.x2 = std::max(r.x1 + 1.0_dp, r.x2);
        canvas.drawRectangle(r, 0.f, 0.f,
                             fillColor   = ColorF(Palette::Standard::index(color)).multiplyAlpha(0.75f),
                             strokeWidth = 0.f);
        --index;
        ++counter;
    }
    canvas.drawText(RectangleF(0, laneY, getFramebufferSize().width, laneY + idp(laneHeight) - 1), 0.f, 0.5f,
                    title + ": " + stat.lastReport(), Font{ Font::Default, dp(12) }, Palette::white);
}

void Window::disableKeyHandling() {
    m_keyHandling = false;
}

void Window::captureFrame(function<void(RC<Image>)> callback) {
    m_captureCallback = std::move(callback);
}

CloseAction Window::shouldClose() {
    return CloseAction::Close;
}

void Window::beforeOpeningWindow() {
    //
}

void Window::openWindow() {
    mustBeMainThread();
    if (m_platformWindow)
        return;
    m_platformWindow.reset(new PlatformWindow(this, m_windowSize, m_position, m_style));
    determineWindowDPI();
    initializeRenderer();
    m_rendering = true;
    beforeOpeningWindow();
    if (auto owner = m_owner.lock())
        m_platformWindow->setOwner(std::move(owner));
    m_platformWindow->updateVisibility();
}

void Window::closeWindow() {
    mustBeMainThread();
    if (!m_platformWindow)
        return;
    windowApplication->updateAndWait();
    m_rendering = false;
    finalizeRenderer();
    m_platformWindow.reset();
}

void Window::keyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    if (!m_keyHandling)
        return;
    m_mods = mods;
    onKeyEvent(key, scancode, action, mods);
}

void Window::charEvent(char32_t character) {
    if (!m_keyHandling)
        return;
    onCharEvent(character);
}

void Window::mouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point) {
    m_mods        = mods;
    m_mousePoint  = point;

    bool dblClick = false;
    bool triClick = false;
    if (button == MouseButton::Left && action == MouseAction::Press) {
        m_downPoint = m_mousePoint;
        if (currentTime() - m_firstClickTime <= windowApplication->doubleClickTime() &&
            m_downPoint->distance(m_firstClickPos) <= windowApplication->doubleClickDistance()) {
            dblClick = true;
        }
        m_firstClickPos  = *m_downPoint;
        m_firstClickTime = currentTime();
        triClick         = dblClick && m_doubleClicked;
        m_doubleClicked  = dblClick;
    }
    onMouseEvent(button, action, mods, point, triClick ? 3 : dblClick ? 2 : 1);
    if (button == MouseButton::Left && action == MouseAction::Release) {
        m_downPoint = std::nullopt;
    }
}

void Window::mouseMove(PointF point) {
    m_mousePoint = point;
    onMouseMove(point);
}

void Window::wheelEvent(float x, float y) {
    onWheelEvent(x, y);
}

void Window::mouseEnter() {
    onMouseEnter();
}

void Window::mouseLeave() {
    onMouseLeave();
}

void Window::filesDropped(std::vector<std::string> files) {
    onFilesDropped(files);
}

void Window::focusChange(bool newIsFocused) {
    onFocusChange(newIsFocused);
}

void Window::dpiChanged() {}

void Window::onKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {}

void Window::onCharEvent(char32_t character) {}

void Window::onMouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point,
                          int conseqClicks) {}

void Window::onMouseMove(PointF point) {}

void Window::onWheelEvent(float x, float y) {}

void Window::onMouseEnter() {}

void Window::onMouseLeave() {}

void Window::onFilesDropped(std::vector<std::string> files) {}

void Window::onFocusChange(bool gained) {}

void Window::onVisibilityChanged(bool newVisible) {}

void Window::onWindowResized(Size windowSize, Size framebufferSize) {}

void Window::onWindowMoved(Point position) {}

void Window::paint(RenderContext& context) {}

void Window::beforeFrame() {}

void Window::closeAttempt() {
    switch (shouldClose()) {
    case CloseAction::Close:
        return close();
    case CloseAction::Hide:
        return hide();
    default:
        break;
    }
}

void Window::windowResized(Size windowSize, Size framebufferSize) {
    if (windowSize != m_windowSize || framebufferSize != m_framebufferSize) {
        m_windowSize      = windowSize;
        m_framebufferSize = framebufferSize;
        onWindowResized(m_windowSize, m_framebufferSize);
    }
}

void Window::windowMoved(Point position) {
    if (position != m_position) {
        m_position = position;
        onWindowMoved(m_position);
    }
}

void Window::setOwner(RC<Window> window) {
    m_owner = window;
    if (!m_platformWindow)
        return;
    mainScheduler->dispatchAndWait([this, window]() {
        m_platformWindow->setOwner(window);
    });
}

void Window::enterModal() {
    setStyle(getStyle() | WindowStyle::Disabled);
}

void Window::exitModal() {
    setStyle(getStyle() & ~WindowStyle::Disabled);
    focus();
}

ModalMode::~ModalMode() {
    if (owner)
        owner->exitModal();
}

ModalMode::ModalMode()
    : owner(Internal::currentWindow ? Internal::currentWindow->shared_from_this() : nullptr) {
    if (owner)
        owner->enterModal();
}

PlatformWindow* Window::platformWindow() {
    return m_platformWindow.get();
}

void Window::windowStateChanged(bool isIconified, bool isMaximized) {
    onWindowStateChanged(isIconified, isMaximized);
}

void Window::onWindowStateChanged(bool isIconified, bool isMaximized) {}

RC<WindowRenderTarget> Window::target() const {
    return m_target;
}
} // namespace Brisk
