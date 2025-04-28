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

#include <bitset>
#include <brisk/window/Types.hpp>
#include <brisk/window/Window.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/graphics/Geometry.hpp>

namespace Brisk {

KeyCode scanCodeToKeyCode(int scanCode);
int keyCodeToScanCode(KeyCode keyCode);

struct OSWindowHandle;

class Window;

namespace Internal {
struct SystemCursor;

class PlatformCursors {
public:
    PlatformCursors();

    struct CursorKey {
        Cursor cursor;
        int scale;
        constexpr auto operator<=>(const CursorKey&) const noexcept = default;
    };

    void registerCursor(Cursor cursor, SVGCursor svgCursor);
    static bool isSystem(Cursor cursor);
    RC<SystemCursor> getCursor(Cursor cursor, float scale);

private:
    std::map<Cursor, SVGCursor> m_svgCursors;
    std::map<Cursor, RC<SystemCursor>> m_systemCursors;
    std::map<CursorKey, RC<SystemCursor>> m_svgCursorCache;
    void initSystemCursors();
    bool m_systemCursorsInitialized = false;
    static RC<SystemCursor> getSystemCursor(Cursor cursor);
    static RC<SystemCursor> cursorFromImage(const RC<Image>& image, Point point, float scale);
};

extern PlatformCursors platformCursors;
} // namespace Internal

struct PlatformWindowData;

struct MsgParams;

struct DblClickParams {
    double time;
    int distance;
};

class PlatformWindow {
public:
    ~PlatformWindow();

    static void initialize();
    static void finalize();

    static void pollEvents();
    static void waitEvents();
    static void postEmptyEvent();

    constexpr static int32_t dontCare = -1;

    static DblClickParams dblClickParams();

    std::unique_ptr<PlatformWindowData> m_data;
    Window* m_window;
    RC<Internal::SystemCursor> m_cursor;
    float m_scale          = 1.f;
    bool m_visible : 1     = false;
    bool m_shouldClose : 1 = false;
    bool m_iconified : 1   = false;
    bool m_maximized : 1   = false;
    std::bitset<numKeyCodes> m_keyState;
    std::bitset<numMouseButtons> m_mouseState;
    WindowStyle m_windowStyle = WindowStyle::Normal;
    Size m_minSize{ dontCare, dontCare };
    Size m_maxSize{ dontCare, dontCare };
    Size m_windowSize{ dontCare, dontCare };
    Size m_framebufferSize{ dontCare, dontCare };
    Point m_position{ dontCare, dontCare };

    Bytes placement() const;
    void setPlacement(BytesView data);
    explicit PlatformWindow(Window* window, Size windowSize, Point position, WindowStyle style);
    bool createWindow();

    void setTitle(std::string_view title);
    void setSize(Size size);
    void setPosition(Point point);
    void setSizeLimits(Size minSize, Size maxSize);
    void setStyle(WindowStyle style);
    void setOwner(RC<Window> window);
    void releaseButtonsAndKeys();

    OSWindowHandle getHandle() const;
    void setCursor(Cursor cursor);
    void updateSize();
    void iconify();
    void maximize();
    void restore();
    void focus();
    bool isFocused() const;
    bool isIconified() const;
    bool isMaximized() const;
    void updateVisibility();

    void setWindowIcon();
    bool cursorInContentArea() const;
    void updateCursorImage();
    bool isVisible() const;

    void charEvent(char32_t codepoint, bool nonClient);
    void mouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF pos,
                    Window::Unit unit = Window::Unit::Screen);
    void focusChange(bool gained);
    void closeAttempt();
    void keyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods);
    void mouseEnterOrLeave(bool enter);
    void mouseMove(PointF pos, Window::Unit unit = Window::Unit::Screen);
    void wheelEvent(float x, float y);
    void windowStateEvent(WindowState state);
    void windowResized(Size windowSize, Size framebufferSize);
    void windowMoved(Point position);
    void windowNonClientClicked();
    void contentScaleChanged(float xscale, float yscale);
    void filesDropped(std::vector<std::string> files);
    void windowStateChanged(bool isIconified, bool isMaximized);

    long long windowProc(MsgParams params);
    Size toScreenPixels(Size size) const;

    static std::vector<PlatformWindow*> platformWindows;
};

} // namespace Brisk
