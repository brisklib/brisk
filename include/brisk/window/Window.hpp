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
#include <brisk/window/Display.hpp>
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
extern constinit bool forceRenderEveryFrame;

} // namespace Internal

class PlatformWindow;

/**
 * @enum HiDPIMode
 * @brief Defines modes for handling high-DPI display scaling in window systems.
 *
 * This enum categorizes how window systems manage position, size, and content scaling
 * for displays with varying DPI (dots per inch).
 */
enum class HiDPIMode {
    /**
     * @brief Application-driven scaling using physical pixels.
     *
     * Window position and size are specified in physical pixels.
     * Applications must scale their content to match the display's DPI.
     * The window scaling factor may be 1, 2, or any value in between.
     * Examples include Windows (DPI-aware) and X11.
     */
    ApplicationScaling,

    /**
     * @brief System-driven scaling using logical units.
     *
     * Window position and size are specified in logical units.
     * A logical unit maps to 1x1 physical pixel (standard DPI) or 2x2 physical pixels (Retina, HiDPI).
     * Applications render content at a 1x or 2x scale.
     * The system scales the framebuffer to match the display's DPI.
     * The window scaling factor is typically 1 or 2 (macOS), with other integers possible in Wayland.
     * Examples include macOS and Wayland.
     */
    FramebufferScaling,
};

HiDPIMode hiDPIMode();

class Window : public BindingObject<Window, &mainScheduler>, public OSWindow {
public:
    /**
     * @brief Get the position of window in Screen coordinates
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Point The window's top-left position in screen coordinates.
     */
    Point getPosition() const;

    /**
     * @brief Get the size of window in Screen coordinates
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Size The window's width and height in screen coordinates.
     */
    Size getSize() const;

    /**
     * @brief Get the bounds of window in Screen coordinates
     *
     * Equals to @c Rectangle(Point(0,0),getSize())
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Rectangle The window's bounds, with origin at (0,0) and size from getSize().
     */
    Rectangle getBounds() const;

    /**
     * @brief Get the position and size of window in Screen coordinates
     *
     * Equals to @c Rectangle(getPosition(),getSize())
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Rectangle The window's position and size as a rectangle in screen coordinates.
     */
    Rectangle getRectangle() const;

    /**
     * @brief Get the bounds of window in Framebuffer coordinates
     *
     * Returns the window's bounds scaled to the framebuffer's pixel dimensions, accounting for the display's
     * backing scale factor or DPI.
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Rectangle The window's bounds in framebuffer coordinates.
     */
    Rectangle getFramebufferBounds() const;

    /**
     * @brief Get the size of window in Framebuffer coordinates
     *
     * Returns the window's size scaled to the framebuffer's pixel dimensions, accounting for the display's
     * backing scale factor or DPI.
     *
     * @remark If the window is not visible, returns the most recent value.
     * @return Size The window's width and height in framebuffer coordinates.
     */
    Size getFramebufferSize() const;

    /**
     * @brief Retrieve the display containing the window
     *
     * Returns a reference-counted pointer to the display containing the window.
     *
     * @return RC<Display> The display associated with the window.
     */
    RC<Display> display() const;

    enum class Unit {
        Screen,      ///< Screen coordinates (logical units or pixels, depending on platform).
        Framebuffer, ///< Framebuffer coordinates (physical pixels).
        Content,     ///< Content coordinates.
    };

    /**
     * @brief Convert a scalar value between coordinate units
     *
     * Converts a value from the source unit to the destination unit, accounting for scaling factors such as
     * DPI or backing scale.
     *
     * @param destUnit The target unit (Screen, Framebuffer, or Content).
     * @param value The value to convert.
     * @param sourceUnit The source unit (Screen, Framebuffer, or Content).
     * @return float The converted value.
     */
    float convertUnit(Unit destUnit, float value, Unit sourceUnit) const noexcept;

    /**
     * @brief Convert a 2D point between coordinate units
     *
     * Converts a point from the source unit to the destination unit, accounting for scaling factors such as
     * DPI or backing scale.
     *
     * @param destUnit The target unit (Screen, Framebuffer, or Content).
     * @param value The point to convert.
     * @param sourceUnit The source unit (Screen, Framebuffer, or Content).
     * @return PointF The converted point.
     */
    PointF convertUnit(Unit destUnit, PointF value, Unit sourceUnit) const noexcept;

    /**
     * @brief Convert a 2D size between coordinate units
     *
     * Converts a size from the source unit to the destination unit, accounting for scaling factors such as
     * DPI or backing scale.
     *
     * @param destUnit The target unit (Screen, Framebuffer, or Content).
     * @param value The size to convert.
     * @param sourceUnit The source unit (Screen, Framebuffer, or Content).
     * @return SizeF The converted size.
     */
    SizeF convertUnit(Unit destUnit, SizeF value, Unit sourceUnit) const noexcept;

    /**
     * @brief Convert a rectangle between coordinate units
     *
     * Converts a rectangle from the source unit to the destination unit, accounting for scaling factors such
     * as DPI or backing scale.
     *
     * @param destUnit The target unit (Screen, Framebuffer, or Content).
     * @param value The rectangle to convert.
     * @param sourceUnit The source unit (Screen, Framebuffer, or Content).
     * @return RectangleF The converted rectangle.
     */
    RectangleF convertUnit(Unit destUnit, RectangleF value, Unit sourceUnit) const noexcept;

    /**
     * @brief Set the window's position and size in Screen coordinates
     *
     * Updates the window's position and size to match the specified rectangle.
     *
     * @param rect The rectangle defining the new position and size in screen coordinates.
     */
    void setRectangle(Rectangle rect);

    /**
     * @brief Set the window's position in Screen coordinates
     *
     * Updates the window's top-left position to the specified point, keeping the size unchanged.
     *
     * @param pos The new position in screen coordinates.
     */
    void setPosition(Point pos);

    /**
     * @brief Set the window's size in Screen coordinates
     *
     * Updates the window's width and height to the specified size, keeping the position unchanged.
     *
     * @param size The new size in screen coordinates.
     */
    void setSize(Size size);

    /**
     * @brief Set the window's minimum size in Screen coordinates
     *
     * Specifies the minimum allowable size for the window, constraining resizing operations.
     *
     * @param size The minimum size in screen coordinates.
     */
    void setMinimumSize(Size size);

    /**
     * @brief Set the window's maximum size in Screen coordinates
     *
     * Specifies the maximum allowable size for the window, constraining resizing operations.
     *
     * @param size The maximum size in screen coordinates.
     */
    void setMaximumSize(Size size);

    /**
     * @brief Set the window's minimum and maximum size in Screen coordinates
     *
     * Specifies both the minimum and maximum allowable sizes for the window, constraining resizing
     * operations.
     *
     * @param minSize The minimum size in screen coordinates.
     * @param maxSize The maximum size in screen coordinates.
     */
    void setMinimumMaximumSize(Size minSize, Size maxSize);

    Bytes windowPlacement() const;
    void setWindowPlacement(BytesView data);

    void focus();

    float contentScale() const noexcept;
    float canvasScale() const noexcept;
    float pixelRatio() const noexcept;

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
    OSWindowHandle getHandle() const final;

    void setOwner(RC<Window> window);
    void enterModal();
    void exitModal();

    bool bufferedRendering() const noexcept;
    void setBufferedRendering(bool bufferedRendering);

    bool forceRenderEveryFrame() const noexcept;
    void setForceRenderEveryFrame(bool forceRenderEveryFrame);

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
    std::atomic_bool m_forceRenderEveryFrame{ Internal::forceRenderEveryFrame };
    RenderStat m_renderStat;
    RC<RenderDevice> m_renderDevice;
    RC<RenderDevice> renderDevice();
    virtual bool update();
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
    /**
     * @brief Content scaling factor for rendering, depending on HiDPI mode.
     *
     * For ApplicationScaling mode, this represents the display's DPI divided by the standard DPI
     * (typically 96 DPI), indicating the scaling factor applications should use to adjust content.
     * For FramebufferScaling mode, this represents the backing scale factor, an integer (e.g., 1 for
     * standard displays, 2 for Retina/HiDPI displays) used by the system to map logical units to
     * physical pixels.
     *
     */
    std::atomic<float> m_contentScale{ 1.f };

    /**
     * @brief Additional scaling factor applied to the UI and canvas.
     *
     * This represents a user-defined scale applied to the UI and canvas drawing.
     *
     * Default value is 1.0 (no additional scaling).
     */
    std::atomic<float> m_canvasScale{ 1.f };

    // m_pixelRatio = m_contentScale * m_canvasScale, UI thread
    float m_pixelRatio = 1.f;

    int m_syncInterval{ 1 };
    virtual void pixelRatioChanged();
    void recomputeScales();

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
