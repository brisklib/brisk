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
#include <brisk/window/Window.hpp>

#include <atomic>

#include <chrono>
#include <future>
#include <memory>
#include <numeric>
#include <spdlog/fmt/fmt.h>

#include <brisk/core/Encoding.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/Svg.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/core/Compression.hpp>
#include <brisk/core/internal/AutoreleasePool.hpp>

#include "FrameTimePredictor.hpp"
#include "PlatformWindow.hpp"
#include <brisk/core/Version.hpp>

namespace Brisk {

double frameStartTime = 0.0;
static BindingRegistration frameStartTime_reg{ &frameStartTime, nullptr };

namespace Internal {

constinit bool bufferedRendering     = true;
constinit bool forceRenderEveryFrame = false;

std::atomic_bool debugShowRenderTimeline{ false };
Window* currentWindow = nullptr;

Rc<Window> currentWindowPtr() {
    return currentWindow ? currentWindow->shared_from_this() : nullptr;
}
} // namespace Internal

void Window::iconify() {
    mustBeUiThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->iconify();
    });
}

void Window::maximize() {
    mustBeUiThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->maximize();
    });
}

void Window::restore() {
    mustBeUiThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->restore();
    });
}

void Window::focus() {
    mustBeUiThread();
    if (!m_platformWindow)
        return;
    mainScheduler->dispatch([this] {
        m_platformWindow->focus();
    });
}

void Window::mustBeUiThread() const {
    if (m_attached && windowApplication)
        windowApplication->mustBeUiThread();
}

void Window::setVisible(bool newVisible) {
    mustBeUiThread();
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
    mustBeUiThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([this] {
        return m_platformWindow->isFocused();
    });
}

bool Window::isIconified() const {
    mustBeUiThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([this] {
        return m_platformWindow->isIconified();
    });
}

bool Window::isMaximized() const {
    mustBeUiThread();
    if (!m_platformWindow)
        return false;
    return mainScheduler->dispatchAndWait([=, this] {
        return m_platformWindow->isMaximized();
    });
}

bool Window::isVisible() const {
    mustBeUiThread();
    return m_visible;
}

Size Window::getSize() const {
    mustBeUiThread();
    return m_windowSize;
}

Size Window::getFramebufferSize() const {
    mustBeUiThread();
    if (!m_platformWindow)
        return {};
    return m_framebufferSize;
}

Size Window::framebufferSize() const {
    // NativeWindow interface (Renderer)
    return m_framebufferSize;
}

std::string Window::getTitle() const {
    mustBeUiThread();
    return m_title;
}

void Window::setTitle(std::string title) {
    mustBeUiThread();
    if (title != m_title) {
        m_title = std::move(title);
        mainScheduler->dispatch([=, this] {
            if (m_platformWindow)
                m_platformWindow->setTitle(m_title);
        });
    }
}

void Window::setRectangle(Rectangle rect) {
    mustBeUiThread();
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
    mustBeUiThread();
    // Do not compare with current values of m_desired* to allow setting the same value
    m_position = pos;
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow)
            m_platformWindow->setPosition(m_position);
    });
}

void Window::setMinimumSize(Size size) {
    setMinimumMaximumSizes(size, m_maximumSize);
}

void Window::setMaximumSize(Size size) {
    setMinimumMaximumSizes(m_minimumSize, size);
}

void Window::setMinimumMaximumSizes(Size minSize, Size maxSize) {
    mustBeUiThread();
    // Do not compare with current values of m_maximumSize to allow setting the same value
    m_minimumSize = minSize;
    m_maximumSize = maxSize;
    mainScheduler->dispatch([=, this] {
        if (m_platformWindow)
            m_platformWindow->setSizeLimits(m_minimumSize, m_maximumSize);
    });
}

void Window::setSize(Size size) {
    mustBeUiThread();
    if (size != m_windowSize) {
        m_windowSize = size;
        mainScheduler->dispatch([this, size = m_windowSize] {
            if (m_platformWindow)
                m_platformWindow->setSize(size);
        });
    }
}

void Window::setStyle(WindowStyle style) {
    mustBeUiThread();
    if (style != m_style) {
        m_style = style;
        mainScheduler->dispatch([style = m_style, this] {
            if (m_platformWindow)
                m_platformWindow->setStyle(style);
        });
    }
}

void Window::recomputeScales() {
    mustBeMainThread();
    const float pixelRatio =
        m_contentScale.load(std::memory_order_relaxed) * m_canvasScale.load(std::memory_order_relaxed);
    BRISK_LOG_INFO("Pixel Scales content ({}) * canvas ({}) = {}",
                   m_contentScale.load(std::memory_order_relaxed),
                   m_canvasScale.load(std::memory_order_relaxed), pixelRatio);
    uiScheduler->dispatch([this, pixelRatio] {
        if (pixelRatio != m_pixelRatio) {
            m_pixelRatio        = pixelRatio;
            Brisk::pixelRatio() = pixelRatio;
            pixelRatioChanged();
        }
    });
}

void Window::visibilityChanged(bool newIsVisible) {
    onVisibilityChanged(newIsVisible);
}

void Window::attachedToApplication() {
    m_attached = true;
    bindings->connect(Value{ &m_canvasScale, this, &Window::recomputeScales },
                      Value{ &windowApplication->uiScale });
    bindings->connect(Value{ &m_syncInterval,
                             [this]() {
                                 if (m_target)
                                     m_target->setVSyncInterval(m_syncInterval);
                             } },
                      Value{ &windowApplication->syncInterval });
}

Window::Window() {
    m_frameTimePredictor.reset(new Internal::FrameTimePredictor{});

    BRISK_LOG_INFO("Done creating Window");
}

Rc<RenderDevice> Window::renderDevice() {
    if (!m_renderDevice) {
        Rc<Display> display = this->display();
        auto result         = createRenderDevice(defaultBackend, deviceSelection,
                                         display ? display->getHandle() : NativeDisplayHandle{});
        BRISK_ASSERT(result.has_value());
        m_renderDevice = *result;
    }
    return m_renderDevice;
}

using high_res_clock = std::chrono::steady_clock;

void Window::paintImmediate(RenderContext& context) {}

constexpr static int debugPanelHeight = 100;

static double roundFPS(double fps) {
    if (fps < 15)
        return 15;
    return std::exp2(std::round(std::log2(fps / 60))) * 60;
}

void Window::paintStat(Canvas& canvas, Rectangle rect) {
    canvas.setFillColor(0x000000'80_rgba);
    canvas.fillRect(rect);
    Rectangle graphRect   = rect.withPadding(150_idp, 12_idp, 0, 0);
    RenderDeviceInfo info = renderDevice()->info();
    std::string status =
        fmt::format("Brisk {} Window {}x{} Pixel {:.2f} Renderer {} GPU {} Buffered: {} EveryFrame: {}",
                    BRISK_VERSION
#ifdef BRISK_DEBUG
                    " [Debug]"
#else
                    " [Optimized]"
#endif
                    ,
                    m_framebufferSize.width, m_framebufferSize.height, pixelRatio(), info.api, info.device,
                    m_bufferedRendering.load(std::memory_order_relaxed),
                    m_forceRenderEveryFrame.load(std::memory_order_relaxed));

    uint64_t frameNumber = m_frameNumber.load(std::memory_order_relaxed);

    using namespace std::chrono_literals;

    double frameDurationNS = 1'000'000'000.0 / roundFPS(m_frameTimePredictor->fps);

    FrameStat sum          = m_renderStat.sum();

    for (size_t i = 0; i < m_renderStat.capacity; ++i) {
        auto& stat = m_renderStat[frameNumber - m_renderStat.capacity + 1 + i];
        size_t j   = (i + frameNumber) % m_renderStat.capacity;
        Rectangle frameRect(
            graphRect.x1 + (graphRect.x2 - graphRect.x1) * (j + 0) / m_renderStat.capacity - 0, graphRect.y1,
            graphRect.x1 + (graphRect.x2 - graphRect.x1) * (j + 1) / m_renderStat.capacity - 1, graphRect.y2);
        auto bar = [&](double begin, double end) {
            return Rectangle{ frameRect.x1, mix(begin, frameRect.y2, frameRect.y1), frameRect.x2,
                              mix(end, frameRect.y2, frameRect.y1) };
        };

        double windowUpdate = stat.windowUpdate.count() / frameDurationNS * 0.5;
        double windowPaint  = stat.windowPaint.count() / frameDurationNS * 0.5;
        double gpuRender    = stat.gpuRender.count() / frameDurationNS * 0.5;

        double base         = 0;
        auto cumulativeBar  = [&](double value) {
            Rectangle result = bar(base, base + value);
            base += value;
            return result;
        };
        canvas.setFillColor(Palette::Standard::index(0).multiplyAlpha(0.75f));
        canvas.fillRect(cumulativeBar(windowUpdate));
        canvas.setFillColor(Palette::Standard::index(4).multiplyAlpha(0.75f));
        canvas.fillRect(cumulativeBar(windowPaint));

        canvas.setFillColor(Palette::white);
        canvas.fillRect(bar(0, gpuRender).withPadding(0, 0, frameRect.width() * 2 / 3, 0));
    }

    canvas.setFont({ Font::Monospace, dp(10) });
    canvas.setFillColor(Palette::white);

    double timeScale = 0.001 / RenderStat::capacity;
    canvas.fillText(fmt::format(
                        R"({}
FPS          : {:7.1f}fps
Frame        : {:7.1f}µs
Window update: {:7.1f}µs
Window paint : {:7.1f}µs
GPU render   : {:7.1f}µs)",
                        status, m_frameTimePredictor->fps, sum.fullFrame.count() * timeScale,
                        sum.windowUpdate.count() * timeScale, sum.windowPaint.count() * timeScale,
                        sum.gpuRender.count() * timeScale),
                    rect, PointF(0.f, 0.f));
}

void Window::paintDebug(RenderContext& context) {
    Canvas canvas(context);

    if (Internal::debugShowRenderTimeline) {
        Rectangle rect{ 0, m_framebufferSize.height - idp(debugPanelHeight), m_framebufferSize.width,
                        m_framebufferSize.height };
        context.setGlobalScissor(rect);
        paintStat(canvas, rect);
    }
}

void Window::doPaint() {
    mustBeUiThread();
    BRISK_ASSERT(m_encoder);
    PerformanceDuration start_time = perfNow();
    ObjCPool pool;
    high_res_clock::time_point renderStart;

    renderStart                  = high_res_clock::now();

    Brisk::pixelRatio()          = m_pixelRatio;

    currentFramePresentationTime = m_nextFrameTime.value_or(currentTime());

    Rc<RenderTarget> target      = m_target;
    bool bufferedRendering       = m_bufferedRendering || !m_captureCallback.empty();
    bool textureReset            = false;
    Size targetSize              = m_target->size();
    if (targetSize.area() <= 0) {
        return;
    }

    beforeFrame();

    uint64_t frameNumber = m_frameNumber.load(std::memory_order_relaxed);

    m_renderStat.beginFrame(frameNumber);

    bool paintFrame;
    {
        Stopwatch perfUpdate(m_renderStat.back().windowUpdate);
        paintFrame = update();
    }
    bool renderFrame = paintFrame || m_forceRenderEveryFrame;

    if (bufferedRendering) {
        if (!m_bufferedFrameTarget) {
            m_bufferedFrameTarget = renderDevice()->createImageTarget(targetSize);
            textureReset          = true;
        } else {
            if (targetSize != m_bufferedFrameTarget->size()) {
                m_bufferedFrameTarget->setSize(targetSize);
                textureReset = true;
            }
        }
        target = m_bufferedFrameTarget;
    }
    m_encoder->setVisualSettings(m_renderSettings);

    renderFrame = renderFrame && m_encoder && m_target;

    if (renderFrame) {
        m_encoder->beginFrame(frameNumber);
        if (paintFrame || !bufferedRendering || textureReset) {
            RenderPipeline pipeline(m_encoder, target,
                                    bufferedRendering ? std::nullopt : std::optional(Palette::transparent),
                                    noClipRect);

            Stopwatch perfPaint(m_renderStat.back().windowPaint);
            paint(pipeline, !bufferedRendering || textureReset);
            if (!bufferedRendering) {
                paintDebug(pipeline);
                paintImmediate(pipeline);
            }
        }

        if (bufferedRendering) {
            m_encoder->setVisualSettings(VisualSettings{});
            RenderPipeline pipeline2(m_encoder, m_target, Palette::transparent);
            Stopwatch perfPaint(m_renderStat.back().windowPaint);
            pipeline2.blit(m_bufferedFrameTarget->image());
            paintDebug(pipeline2);
            paintImmediate(pipeline2);
        }
        m_encoder->endFrame([this](uint64_t frameIndex, std::span<const std::chrono::nanoseconds> durations) {
            if (m_renderStat.hasFrame(frameIndex)) {
                m_renderStat[frameIndex].gpuRender =
                    std::accumulate(durations.begin(), durations.end(), std::chrono::nanoseconds{ 0 });
            }
        });
    }

    m_lastFrameRenderTime =
        std::chrono::duration_cast<std::chrono::microseconds>(high_res_clock::now() - renderStart);

    if (renderFrame) {
        m_target->present();
    }
    m_renderStat.back().fullFrame = std::chrono::duration_cast<std::chrono::nanoseconds>(
        FractionalSeconds{ m_frameTimePredictor->markFrameTime() });

    if (m_captureCallback && renderFrame) {
        BRISK_ASSERT(bufferedRendering);
        m_encoder->wait();
        auto captureCallback    = std::move(m_captureCallback);
        m_captureCallback       = nullptr;
        Rc<Image> capturedFrame = m_bufferedFrameTarget->image();

        std::move(captureCallback)(std::move(capturedFrame));
    }
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
    mustBeUiThread();
    return m_style;
}

PointOf<float> Window::getMousePosition() const {
    mustBeUiThread();
    return m_mousePoint;
}

PointOf<int> Window::getPosition() const {
    mustBeUiThread();
    return m_position;
}

void Window::setCursor(Cursor cursor) {
    mustBeUiThread();
    if (cursor != m_cursor) {
        m_cursor = cursor;
        mainScheduler->dispatch([=, this] {
            if (m_platformWindow)
                m_platformWindow->setCursor(m_cursor);
        });
    }
}

float Window::canvasScale() const noexcept {
    return m_canvasScale;
}

float Window::pixelRatio() const noexcept {
    return m_pixelRatio;
}

float Window::contentScale() const noexcept {
    return m_contentScale;
}

Rectangle Window::getBounds() const {
    mustBeUiThread();
    return Rectangle{ Point(0, 0), m_windowSize };
}

Rectangle Window::getRectangle() const {
    mustBeUiThread();
    return Rectangle{ m_position, m_windowSize };
}

Rectangle Window::getFramebufferBounds() const {
    mustBeUiThread();
    return Rectangle{ Point(0, 0), m_framebufferSize };
}

void Window::show() {
    setVisible(true);
}

void Window::hide() {
    setVisible(false);
}

void Window::close() {
    mustBeUiThread();
    hide();
    m_closing = true; // forces application to remove this window from the windows list
    auto wk   = weak_from_this();
    mainScheduler->dispatch([wk]() {
        if (auto lk = std::static_pointer_cast<Window>(wk.lock()))
            lk->closeWindow(); // destroys m_platformWindow
    });
}

NativeWindowHandle Window::getHandle() const {
    if (!m_platformWindow)
        return NativeWindowHandle{};
    return m_platformWindow->getHandle();
}

void Window::initializeRenderer() {
    if (m_target)
        return;
    m_encoder = renderDevice()->createEncoder();
    m_target  = renderDevice()->createWindowTarget(this);
    m_target->setVSyncInterval(m_syncInterval);
}

void Window::finalizeRenderer() {
    if (!m_target)
        return;
    m_target.reset();
    m_encoder.reset();
}

Bytes Window::windowPlacement() const {
    mustBeUiThread();
    if (!m_platformWindow) {
        return {};
    }
    return uiScheduler->dispatchAndWait([this]() -> Bytes {
        return m_platformWindow->placement();
    });
}

void Window::setWindowPlacement(BytesView data) {
    mustBeUiThread();
    if (!m_platformWindow) {
        return;
    }
    uiScheduler->dispatchAndWait([this, data]() { // must wait, otherwise dangling reference
        m_platformWindow->setPlacement(data);
    });
}

void Window::disableKeyHandling() {
    m_keyHandling = false;
}

void Window::captureFrame(function<void(Rc<Image>)> callback) {
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
    recomputeScales();
    initializeRenderer();
    m_rendering = true;
    beforeOpeningWindow();
    m_platformWindow->setSizeLimits(m_minimumSize, m_maximumSize);
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

void Window::pixelRatioChanged() {}

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

void Window::onNonClientClicked() {}

bool Window::update() {
    return false;
}

void Window::paint(RenderContext& context, bool fullRepaint) {}

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

void Window::windowNonClientClicked() {
    onNonClientClicked();
}

void Window::setOwner(Rc<Window> window) {
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
    mustBeUiThread();
    if (m_style && WindowStyle::Disabled) {
        m_style &= ~WindowStyle::Disabled;
        mainScheduler->dispatch([style = m_style, this] {
            if (m_platformWindow) {
                m_platformWindow->setStyle(style);
                m_platformWindow->focus();
            }
        });
    }
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

Rc<WindowRenderTarget> Window::target() const {
    return m_target;
}

void Window::setBufferedRendering(bool bufferedRendering) {
    m_bufferedRendering = bufferedRendering;
}

bool Window::bufferedRendering() const noexcept {
    return m_bufferedRendering;
}

void Window::setForceRenderEveryFrame(bool forceRenderEveryFrame) {
    m_forceRenderEveryFrame = forceRenderEveryFrame;
}

bool Window::forceRenderEveryFrame() const noexcept {
    return m_forceRenderEveryFrame;
}

Rc<Display> Window::display() const {
    NativeWindowHandle handle = getHandle();
    if (!handle)
        return nullptr;
    for (auto display : Display::all())
        if (display->containsWindow(handle))
            return display;

    return nullptr;
}

float Window::convertUnit(Unit destUnit, float value, Unit sourceUnit) const noexcept {
    const bool appScaling = hiDPIMode() == HiDPIMode::ApplicationScaling;

    switch (sourceUnit) {
    case Unit::Screen:
        switch (destUnit) {
        case Unit::Screen:
            return value;
        case Unit::Framebuffer:
            return appScaling ? value : value * m_contentScale;
        case Unit::Content:
            return appScaling ? value / m_contentScale : value;
        }
    case Unit::Framebuffer:
        switch (destUnit) {
        case Unit::Screen:
            return appScaling ? value : value / m_contentScale;
        case Unit::Framebuffer:
            return value;
        case Unit::Content:
            return value / m_contentScale;
        }
    case Unit::Content:
        switch (destUnit) {
        case Unit::Screen:
            return appScaling ? value * m_contentScale : value;
        case Unit::Framebuffer:
            return value * m_contentScale;
        case Unit::Content:
            return value;
        }
    }
    BRISK_UNREACHABLE();
}

PointF Window::convertUnit(Unit destUnit, PointF value, Unit sourceUnit) const noexcept {
    return {
        convertUnit(destUnit, value.x, sourceUnit),
        convertUnit(destUnit, value.y, sourceUnit),
    };
}

SizeF Window::convertUnit(Unit destUnit, SizeF value, Unit sourceUnit) const noexcept {
    return {
        convertUnit(destUnit, value.x, sourceUnit),
        convertUnit(destUnit, value.y, sourceUnit),
    };
}

RectangleF Window::convertUnit(Unit destUnit, RectangleF value, Unit sourceUnit) const noexcept {
    return {
        convertUnit(destUnit, value.x1, sourceUnit),
        convertUnit(destUnit, value.y1, sourceUnit),
        convertUnit(destUnit, value.x2, sourceUnit),
        convertUnit(destUnit, value.y2, sourceUnit),
    };
}

const RenderStat& Window::renderStat() const noexcept {
    return m_renderStat;
}

RenderStat& Window::renderStat() noexcept {
    return m_renderStat;
}

FrameStat& RenderStat::operator[](uint64_t frameIndex) noexcept {
    return m_frames[frameIndex % capacity];
}

const FrameStat& RenderStat::operator[](uint64_t frameIndex) const noexcept {
    return m_frames[frameIndex % capacity];
}

FrameStat RenderStat::sum() const noexcept {
    FrameStat result{};
    for (const FrameStat& entry : m_frames) {
        result.windowUpdate += entry.windowUpdate;
        result.windowPaint += entry.windowPaint;
        result.gpuRender += entry.gpuRender;
        result.fullFrame += entry.fullFrame;
        result.numRenderPasses += entry.numRenderPasses;
        result.numQuads += entry.numQuads;
    }
    return result;
}

std::optional<uint64_t> RenderStat::lastFrame() const noexcept {
    return m_lastFrame == UINT64_MAX ? std::nullopt : std::optional{ m_lastFrame };
}

bool RenderStat::hasFrame(uint64_t frameIndex) const noexcept {
    return m_lastFrame != UINT64_MAX && frameIndex >= m_lastFrame - capacity + 1 && frameIndex <= m_lastFrame;
}

void RenderStat::beginFrame(uint64_t frameIndex) {
    // m_lastFrame + 1 is safe here even if m_lastFrame is UINT64_MAX
    for (uint64_t i = std::max(m_lastFrame + 1, frameIndex - capacity + 1); i <= frameIndex; ++i) {
        m_frames[i % capacity] = FrameStat{};
    }

    m_lastFrame = frameIndex;
}

FrameStat& RenderStat::back() noexcept {
    BRISK_ASSERT(m_lastFrame != UINT64_MAX);
    return m_frames[m_lastFrame % capacity];
}

const FrameStat& RenderStat::back() const noexcept {
    BRISK_ASSERT(m_lastFrame != UINT64_MAX);
    return m_frames[m_lastFrame % capacity];
}

} // namespace Brisk
