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

#include <brisk/core/Binding.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/graphics/Renderer.hpp>
#include <brisk/core/Time.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <brisk/core/Threading.hpp>
#include <brisk/window/Types.hpp>
#include <mutex>

namespace Brisk {

struct FrameStat {
    std::chrono::nanoseconds windowUpdate;
    std::chrono::nanoseconds windowPaint;
    std::chrono::nanoseconds gpuRender;
    std::chrono::nanoseconds fullFrame;
    uint32_t numRenderPasses;
    uint32_t numQuads;
    // uint64_t uniformTransferred;
    // uint64_t textureTransferred;
};

class RenderStat {
public:
    FrameStat& operator[](uint64_t frameIndex) noexcept;

    const FrameStat& operator[](uint64_t frameIndex) const noexcept;

    FrameStat sum() const noexcept;

    bool hasFrame(uint64_t frameIndex) const noexcept;

    void beginFrame(uint64_t frameIndex);

    std::optional<uint64_t> lastFrame() const noexcept;

    FrameStat& back() noexcept;

    const FrameStat& back() const noexcept;

    constexpr static size_t capacity = 128;

private:
    uint64_t m_lastFrame = UINT64_MAX;
    std::array<FrameStat, capacity> m_frames{};
};

// Binding enabled
extern double frameStartTime;

class Canvas;

class WindowApplication;
class Window;

namespace Internal {

struct DisplaySyncPoint {
    bool active = false;
    Clock::time_point frameStartTime{};
    Clock::duration frameDuration{ 0 };
};

extern std::atomic_bool debugShowRenderTimeline;

struct FrameTimePredictor;

/// Current window instance. Available in UI thread. Set in uiThreadBody
extern Window* currentWindow;
RC<Window> currentWindowPtr();

/// Default value for Window::bufferedRendering
extern constinit bool bufferedRendering;

} // namespace Internal

class PlatformWindow;

class Window : public BindingObject<Window, &mainScheduler>, public OSWindow {
public:
    // All these functions can be called from any thread
    /**
     * @brief Get the position of window in desktop coordinates
     *
     * @remark If the window is not visible, returns the most recent value.
     */
    Point getPosition() const;

    /**
     * @brief Get the size of window in desktop coordinates
     *
     * @remark If the window is not visible, returns the most recent value.
     */
    Size getSize() const;

    /**
     * @brief Get the bounds of window in desktop coordinates,
     *
     * Equals to @c Rectangle(Point(0,0),getSize())
     *
     * @remark If the window is not visible, returns the most recent value.
     */
    Rectangle getBounds() const;

    /**
     * @brief Get the position and size of window in desktop coordinates,
     *
     * Equals to @c Rectangle(getPosition(),getSize())
     *
     * @remark If the window is not visible, returns the most recent value.
     */
    Rectangle getRectangle() const;

    Rectangle getFramebufferBounds() const;

    Size getFramebufferSize() const;

    void setRectangle(Rectangle rect);
    void setPosition(Point pos);
    void setSize(Size size);
    void setMinimumSize(Size size);
    void setFixedSize(Size size);
    Bytes windowPlacement() const;
    void setWindowPlacement(BytesView data);

    void focus();

    float windowPixelRatio() const;
    float canvasPixelRatio() const;

    PointF getMousePosition() const;

    void setTitle(std::string title);
    std::string getTitle() const;

    Window();
    ~Window() override;

    WindowStyle getStyle() const;
    void setStyle(WindowStyle style);

    bool isVisible() const;
    bool isTopMost() const;
    bool isMaximized() const;
    bool isIconified() const;
    bool isFocused() const;
    void show();
    void hide();
    void close();
    void restore();
    void maximize();
    void iconify();

    void setCursor(Cursor cursor);

    PlatformWindow* platformWindow();

    void disableKeyHandling();
    void getHandle(OSWindowHandle& handle) const final;

    void setOwner(RC<Window> window);
    void enterModal();
    void exitModal();

    bool bufferedRendering() const noexcept;
    void setBufferedRendering(bool bufferedRendering);

    void captureFrame(function<void(RC<Image>)> callback);

    RC<WindowRenderTarget> target() const;

    RenderStat& renderStat() noexcept;

    const RenderStat& renderStat() const noexcept;

protected:
    friend class WindowApplication;
    void beforeDestroying();
    virtual void beforeOpeningWindow();
    virtual void attachedToApplication();
    bool m_attached = false;

private:
    void mustBeUIThread() const;

protected:
    // Properties and dimensions
    WindowStyle m_style = WindowStyle::Normal; /// UI-thread
    std::string m_title;                       /// UI-thread
    Size m_minimumSize{ -1, -1 };              /// UI-thread
    Size m_maximumSize{ -1, -1 };              /// UI-thread
    Size m_windowSize{ 640, 480 };             /// UI-thread
    Size m_framebufferSize{ 0, 0 };            /// UI-thread
    Point m_position{ -1, -1 };                /// UI-thread
    Cursor m_cursor = Cursor::Arrow;
    void* m_parent  = nullptr;
    bool m_visible{ true };              /// Desired value. Will be applied to OS window when open
    std::atomic_bool m_closing{ false }; /// If true, application will remove this window from windows list
    // call to change visibility
    void setVisible(bool newVisible);

protected:
    // Input
    KeyModifiers m_mods = KeyModifiers::None; // Reflects OS state. Changed in keyboard callback
    PointF m_mousePoint{ 0, 0 };              // Reflects OS state. Changed in callback
    std::optional<PointF> m_downPoint;        // Reflects OS state. Changed in callback
    double m_firstClickTime = -1.0;
    PointF m_firstClickPos{ -1.f, -1.f };
    bool m_doubleClicked = false;
    bool m_keyHandling   = true;

    void keyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods);
    void charEvent(char32_t character);
    void mouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point);
    void mouseMove(PointF point);
    void wheelEvent(float x, float y);
    void mouseEnter();
    void mouseLeave();
    void filesDropped(std::vector<std::string> files);
    void windowStateChanged(bool isIconified, bool isMaximized);
    void focusChange(bool gained);
    void visibilityChanged(bool newVisible);
    void closeAttempt();
    void windowResized(Size windowSize, Size framebufferSize);
    void windowMoved(Point position);
    virtual void onKeyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods);
    virtual void onCharEvent(char32_t character);
    virtual void onMouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF point,
                              int conseqClicks);
    virtual void onMouseMove(PointF point);
    virtual void onWheelEvent(float x, float y);
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onFilesDropped(std::vector<std::string> files);
    virtual void onWindowStateChanged(bool isIconified, bool isMaximized);
    virtual void onFocusChange(bool gained);
    virtual void onVisibilityChanged(bool newVisible);
    virtual void onWindowResized(Size windowSize, Size framebufferSize);
    virtual void onWindowMoved(Point position);
    virtual CloseAction shouldClose();

protected:
    // Rendering
    RC<WindowRenderTarget> m_target;
    RC<RenderEncoder> m_encoder;
    function<void(RC<Image>)> m_captureCallback;
    RC<ImageRenderTarget> m_bufferedFrameTarget;
    std::chrono::microseconds m_lastFrameRenderTime{ 0 };
    Internal::DisplaySyncPoint m_syncPoint;
    std::atomic_llong m_frameNumber{ 0 };
    std::optional<double> m_nextFrameTime;
    std::unique_ptr<Internal::FrameTimePredictor> m_frameTimePredictor;
    std::mutex m_mutex;
    VisualSettings m_renderSettings{};
    std::atomic_bool m_rendering{ false }; /// true if rendering is active
    std::atomic_bool m_bufferedRendering{ Internal::bufferedRendering };
    RenderStat m_renderStat;
    virtual void update();
    virtual void paint(RenderContext& context, bool fullRepaint);
    virtual void paintImmediate(RenderContext& context);
    virtual void beforeFrame();
    void paintDebug(RenderContext& context);
    void doPaint();
    void initializeRenderer();
    void finalizeRenderer();
    Size framebufferSize() const final;

protected:
    // Modal
    bool m_modal = false;
    WeakRC<Window> m_owner;

protected:
    // DPI
    std::atomic<float> m_windowPixelRatio{ 1.f };
    std::atomic<float> m_canvasPixelRatio{ 1.f };
    std::atomic<float> m_pixelRatioScale{ 1.f };
    std::atomic<bool> m_useMonitorScale{ true };
    int m_syncInterval{ 1 };
    virtual void dpiChanged();
    void windowPixelRatioChanged();
    void determineWindowDPI();

protected:
    friend class PlatformWindow;
    std::unique_ptr<PlatformWindow> m_platformWindow;
    void openWindow();
    void closeWindow();

protected:
    void paintStat(Canvas& canvas, Rectangle rect);
};

inline double currentFramePresentationTime{};

struct ModalMode {
    ModalMode();
    ~ModalMode();

    RC<Window> owner;
};

} // namespace Brisk
