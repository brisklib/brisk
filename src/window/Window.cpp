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
#include <numeric>
#include <spdlog/fmt/fmt.h>

#include <brisk/core/Encoding.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/SVG.hpp>
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

constinit bool bufferedRendering = true;

std::atomic_bool debugShowRenderTimeline{ false };
Window* currentWindow = nullptr;

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
    RenderDeviceInfo info = (*getRenderDevice())->info();
    std::string status    = fmt::format("Brisk {} Window {}x{} Pixel {:.2f} Renderer {} GPU {}",
                                        BRISK_VERSION
#ifdef BRISK_DEBUG
                                     " [Debug]"
#else
                                     " [Optimized]"
#endif
                                     ,
                                     m_framebufferSize.width, m_framebufferSize.height,
                                     m_canvasPixelRatio.load(), info.api, info.device

    );

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
        context.setClipRect(rect);
        paintStat(canvas, rect);
        BRISK_ASSERT(canvas.rasterizedPaths() == 0);
    }
}

void Window::doPaint() {
    BRISK_ASSERT(m_encoder);
    PerformanceDuration start_time = perfNow();
    ObjCPool pool;
    high_res_clock::time_point renderStart;

    renderStart                  = high_res_clock::now();

    pixelRatio()                 = m_canvasPixelRatio;

    currentFramePresentationTime = m_nextFrameTime.value_or(currentTime());

    RC<RenderTarget> target      = m_target;
    bool bufferedRendering       = m_bufferedRendering || !m_captureCallback.empty();
    bool textureReset            = false;
    Size targetSize              = m_target->size();
    if (targetSize.area() <= 0) {
        return;
    }

    if (bufferedRendering) {
        auto device = getRenderDevice();
        if (!m_bufferedFrameTarget) {
            m_bufferedFrameTarget = (*device)->createImageTarget(targetSize);
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

    uint64_t frameNumber = m_frameNumber.load(std::memory_order_relaxed);

    m_renderStat.beginFrame(frameNumber);

    m_encoder->beginFrame(frameNumber);

    beforeFrame();

    PerformanceDuration gpuDuration = PerformanceDuration(0);
    {
        {
            Stopwatch perfUpdate(m_renderStat.back().windowUpdate);
            update();
        }
        if (!m_encoder || !m_target) // The window stopped rendering
            return;
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
        RenderPipeline pipeline2(m_encoder, m_target, Palette::transparent, noClipRect);
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

    m_lastFrameRenderTime =
        std::chrono::duration_cast<std::chrono::microseconds>(high_res_clock::now() - renderStart);

    {
        m_target->present();
        m_renderStat.back().fullFrame = std::chrono::duration_cast<std::chrono::nanoseconds>(
            FractionalSeconds{ m_frameTimePredictor->markFrameTime() });
    }
    if (m_captureCallback) {
        BRISK_ASSERT(bufferedRendering);
        m_encoder->wait();
        auto captureCallback    = std::move(m_captureCallback);
        m_captureCallback       = nullptr;
        RC<Image> capturedFrame = m_bufferedFrameTarget->image();

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
    auto wk   = weak_from_this();
    mainScheduler->dispatch([wk]() {
        if (auto lk = std::static_pointer_cast<Window>(wk.lock()))
            lk->closeWindow(); // destroys m_platformWindow
    });
}

OSWindowHandle Window::getHandle() const {
    if (!m_platformWindow)
        return OSWindowHandle{};
    return m_platformWindow->getHandle();
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

void Window::update() {}

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
    mustBeUIThread();
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

RC<WindowRenderTarget> Window::target() const {
    return m_target;
}

void Window::setBufferedRendering(bool bufferedRendering) {
    m_bufferedRendering = bufferedRendering;
}

bool Window::bufferedRendering() const noexcept {
    return m_bufferedRendering;
}

RC<Display> Window::display() const {
    OSWindowHandle handle = getHandle();
    if (!handle)
        return nullptr;
    for (auto display : Display::all())
        if (display->containsWindow(handle))
            return display;

    return nullptr;
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
