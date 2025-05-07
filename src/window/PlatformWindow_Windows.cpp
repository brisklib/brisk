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
#define BRISK_ALLOW_OS_HEADERS 1
#define OEMRESOURCE

#define NOMINMAX 1
#include <windows.h>
#include <windowsx.h>
#include "ShellScalingApi.h"
#include <dwmapi.h>

#include <brisk/core/Encoding.hpp>
#include "PlatformWindow.hpp"
#include <brisk/window/Window.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/core/Time.hpp>
#include <brisk/graphics/NativeWindowHandle.hpp>
#include <brisk/core/System.hpp>

#include <brisk/core/platform/SystemWindows.hpp>

#include <brisk/window/Display.hpp>
#include <brisk/window/Types.hpp>

namespace Brisk {

static const wchar_t* propKey = L"CC";

static struct {
    ATOM helperWindowClass  = {};
    HWND helperWindowHandle = {};
    ATOM mainWindowClass    = {};
} staticData;

static LRESULT CALLBACK helperWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DISPLAYCHANGE:
        Internal::updateDisplays();
        break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

struct PlatformWindowData {
    HWND hWnd{};
    WCHAR highSurrogate{};
    Point mousePos{ -1, -1 };
    bool cursorTracked = false;
};

namespace Internal {

struct SystemCursor {
    HCURSOR cursor;

    ~SystemCursor() {
        DestroyIcon((HICON)cursor);
    }
};
} // namespace Internal

// Retrieves and translates modifier keys
static KeyModifiers getKeyMods() {
    KeyModifiers mods = KeyModifiers::None;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        mods |= KeyModifiers::Shift;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        mods |= KeyModifiers::Control;
    if (GetKeyState(VK_MENU) & 0x8000)
        mods |= KeyModifiers::Alt;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000)
        mods |= KeyModifiers::Super;
    if (GetKeyState(VK_CAPITAL) & 1)
        mods |= KeyModifiers::CapsLock;
    if (GetKeyState(VK_NUMLOCK) & 1)
        mods |= KeyModifiers::NumLock;
    return mods;
}

static DWORD getWindowStyle(WindowStyle style) {
    DWORD result = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX;
    if (style && WindowStyle::Undecorated)
        result |= WS_POPUP;
    else
        result |= WS_CAPTION;
    if (style && WindowStyle::Resizable)
        result |= WS_MAXIMIZEBOX | WS_THICKFRAME;

    return result;
}

static DWORD getWindowExStyle(WindowStyle style) {
    DWORD result = 0;

    if (style && WindowStyle::TopMost)
        result |= WS_EX_TOPMOST;

    if (style && WindowStyle::ToolWindow)
        result |= WS_EX_TOOLWINDOW;
    else
        result |= WS_EX_APPWINDOW;

    return result;
}

struct MsgParams {
    UINT uMsg;
    WPARAM wParam;
    LPARAM lParam;
};

long long PlatformWindow::windowProc(MsgParams params) {
    UINT uMsg     = params.uMsg;
    WPARAM wParam = params.wParam;
    LPARAM lParam = params.lParam;

    switch (uMsg) {
    case WM_SETFOCUS: {
        focusChange(true);
        return 0;
    }
    case WM_KILLFOCUS: {
        focusChange(false);
        return 0;
    }
    case WM_CLOSE: {
        closeAttempt();
        return 0;
    }

    case WM_CHAR:
    case WM_SYSCHAR: {
        if (wParam >= 0xd800 && wParam <= 0xdbff) {
            m_data->highSurrogate = (WCHAR)wParam;
        } else {
            uint32_t codepoint = 0;

            if (wParam >= 0xdc00 && wParam <= 0xdfff) {
                if (m_data->highSurrogate) {
                    codepoint += (m_data->highSurrogate - 0xd800) << 10;
                    codepoint += (WCHAR)wParam - 0xdc00;
                    codepoint += 0x10000;
                }
            } else {
                codepoint = (WCHAR)wParam;
            }
            m_data->highSurrogate = 0;

            charEvent(static_cast<char32_t>(codepoint), uMsg == WM_SYSCHAR);
        }

        return 0;
    }

    case WM_UNICHAR: {
        if (wParam == UNICODE_NOCHAR) {
            // WM_UNICHAR is not sent by Windows, but is sent by some
            // third-party input method engine
            // Returning TRUE here announces support for this message
            return TRUE;
        }

        charEvent(static_cast<char32_t>(wParam), false);
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {

        int scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
        if (!scancode) {
            // NOTE: Some synthetic key messages have a scancode of zero
            // HACK: Map the virtual key back to a usable scancode
            scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
        }

        // HACK: Alt+PrtSc has a different scancode than just PrtSc
        if (scancode == 0x54)
            scancode = 0x137;

        // HACK: Ctrl+Pause has a different scancode than just Pause
        if (scancode == 0x146)
            scancode = 0x45;

        // HACK: CJK IME sets the extended bit for right Shift
        if (scancode == 0x136)
            scancode = 0x36;

        KeyCode key = scanCodeToKeyCode(scancode);

        // The Ctrl keys require special handling
        if (wParam == VK_CONTROL) {
            if (HIWORD(lParam) & KF_EXTENDED) {
                // Right side keys have the extended key bit set
                key = KeyCode::RightControl;
            } else {
                // NOTE: Alt Gr sends Left Ctrl followed by Right Alt
                // HACK: We only want one event for Alt Gr, so if we detect
                //       this sequence we discard this Left Ctrl message now
                //       and later report Right Alt normally
                MSG next;
                const DWORD time = GetMessageTime();

                if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE)) {
                    if (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN ||
                        next.message == WM_KEYUP || next.message == WM_SYSKEYUP) {
                        if (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) &&
                            next.time == time) {
                            // Next message is Right Alt down so discard this
                            break;
                        }
                    }
                }

                // This is a regular Left Ctrl message
                key = KeyCode::LeftControl;
            }
        } else if (wParam == VK_PROCESSKEY) {
            // IME notifies that keys have been filtered by setting the
            // virtual key-code to VK_PROCESSKEY
            break;
        }
        const KeyAction action  = (HIWORD(lParam) & KF_UP) ? KeyAction::Release : KeyAction::Press;
        const KeyModifiers mods = getKeyMods();

        if (action == KeyAction::Release && wParam == VK_SHIFT) {
            // HACK: Release both Shift keys on Shift up event, as when both
            //       are pressed the first release does not emit any event
            // NOTE: The other half of this is in pollEvents
            keyEvent(KeyCode::LeftShift, scancode, action, mods);
            keyEvent(KeyCode::RightShift, scancode, action, mods);
        } else if (wParam == VK_SNAPSHOT) {
            // HACK: Key down is not reported for the Print Screen key
            keyEvent(key, scancode, KeyAction::Press, mods);
            keyEvent(key, scancode, KeyAction::Release, mods);
        } else {
            keyEvent(key, scancode, action, mods);
        }

        break;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP: {
        MouseButton button;
        MouseAction action;

        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
            button = MouseButton::Left;
        else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
            button = MouseButton::Right;
        else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
            button = MouseButton::Middle;
        else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
            button = MouseButton::Btn4;
        else
            button = MouseButton::Btn5;

        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN ||
            uMsg == WM_XBUTTONDOWN) {
            action = MouseAction::Press;
        } else
            action = MouseAction::Release;

        if (!m_mouseState.any())
            SetCapture(m_data->hWnd);

        mouseEvent(button, action, getKeyMods(), m_data->mousePos);

        if (!m_mouseState.any())
            ReleaseCapture();

        if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP)
            return TRUE;

        return 0;
    }

    case WM_MOUSEMOVE: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);

        if (!m_data->cursorTracked) {
            TRACKMOUSEEVENT tme{ .cbSize = sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags   = TME_LEAVE;
            tme.hwndTrack = m_data->hWnd;
            TrackMouseEvent(&tme);

            m_data->cursorTracked = true;
            mouseEnterOrLeave(true);
        }

        m_data->mousePos = { x, y };

        mouseMove(m_data->mousePos);

        return 0;
    }
    case WM_MOUSELEAVE: {
        m_data->cursorTracked = false;
        mouseEnterOrLeave(false);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        wheelEvent(0.f, (SHORT)HIWORD(wParam) / (float)WHEEL_DELTA);
        return 0;
    }
    case WM_MOUSEHWHEEL: {
        // NOTE: The X-axis is inverted for consistency with macOS and X11
        wheelEvent(-((SHORT)HIWORD(wParam) / (float)WHEEL_DELTA), 0.f);
        return 0;
    }

    case WM_SIZE: {
        const Size newSize   = Size(LOWORD(lParam), HIWORD(lParam));
        const bool iconified = wParam == SIZE_MINIMIZED;
        const bool maximized = wParam == SIZE_MAXIMIZED || (m_maximized && wParam != SIZE_RESTORED);

        if (iconified != m_iconified || maximized != m_maximized)
            windowStateChanged(iconified, maximized);

        if (!iconified && newSize != m_windowSize) {
            m_windowSize      = newSize;
            m_framebufferSize = newSize;
            windowResized(m_windowSize, m_framebufferSize);
        }

        m_iconified = iconified;
        m_maximized = maximized;
        return 0;
    }

    case WM_MOVE: {
        // NOTE: This cannot use LOWORD/HIWORD recommended by MSDN, as
        // those macros do not handle negative window positions correctly

        m_position = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        windowMoved(m_position);
        return 0;
    }

    case WM_GETMINMAXINFO: {
        RECT frame               = { 0 };
        MINMAXINFO* mmi          = (MINMAXINFO*)lParam;
        const DWORD style        = getWindowStyle(m_windowStyle);
        const DWORD exStyle      = getWindowExStyle(m_windowStyle);

        const Size scaledMinSize = m_minSize;
        const Size scaledMaxSize = m_maxSize;

        if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
            AdjustWindowRectExForDpi(&frame, style, FALSE, exStyle, GetDpiForWindow(m_data->hWnd));
        } else {
            AdjustWindowRectEx(&frame, style, FALSE, exStyle);
        }

        if (m_minSize.width != dontCare)
            mmi->ptMinTrackSize.x = scaledMinSize.width + frame.right - frame.left;
        if (m_minSize.height != dontCare)
            mmi->ptMinTrackSize.y = scaledMinSize.height + frame.bottom - frame.top;

        if (m_maxSize.width != dontCare)
            mmi->ptMaxTrackSize.x = scaledMaxSize.width + frame.right - frame.left;
        if (m_maxSize.height != dontCare)
            mmi->ptMaxTrackSize.y = scaledMaxSize.height + frame.bottom - frame.top;

        if (m_windowStyle && WindowStyle::Undecorated) {
            const HMONITOR mh = MonitorFromWindow(m_data->hWnd, MONITOR_DEFAULTTONEAREST);

            MONITORINFO mi{ .cbSize = sizeof(MONITORINFO) };
            GetMonitorInfoW(mh, &mi);

            mmi->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
            mmi->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
            mmi->ptMaxSize.x     = mi.rcWork.right - mi.rcWork.left;
            mmi->ptMaxSize.y     = mi.rcWork.bottom - mi.rcWork.top;
        }
        return 0;
    }
    case WM_PAINT: {
        break;
    }
    case WM_ERASEBKGND: {
        return TRUE;
    }
    case WM_NCACTIVATE:
    case WM_NCPAINT: {
        // Prevent title bar from being drawn after restoring a minimized
        // undecorated window
        if (m_windowStyle && WindowStyle::Undecorated)
            return TRUE;

        break;
    }

    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCXBUTTONDOWN: {
        windowNonClientClicked();
        break;
    }

    case WM_GETDPISCALEDSIZE: {
        break;
    }

    case WM_DPICHANGED: {
        const float xscale = HIWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;
        const float yscale = LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;

        m_scale            = std::max(xscale, yscale);

        // Resize windowed mode windows that either permit rescaling or that
        // need it to compensate for non-client area scaling
        RECT* suggested    = (RECT*)lParam;
        SetWindowPos(m_data->hWnd, HWND_TOP, suggested->left, suggested->top,
                     suggested->right - suggested->left, suggested->bottom - suggested->top,
                     SWP_NOACTIVATE | SWP_NOZORDER);

        contentScaleChanged(xscale, yscale);
        break;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            if (m_cursor)
                SetCursor(m_cursor->cursor);
            else
                SetCursor(LoadCursorW(NULL, IDC_ARROW));
            return TRUE;
        }
        break;
    }

    case WM_DROPFILES: {
        HDROP drop      = (HDROP)wParam;

        const int count = DragQueryFileW(drop, 0xffffffff, NULL, 0);
        std::vector<std::string> paths;

        POINT pt;
        // Move the mouse to the position of the drop
        DragQueryPoint(drop, &pt);

        m_data->mousePos = { pt.x, pt.y };
        mouseMove(m_data->mousePos);

        for (int i = 0; i < count; i++) {
            std::wstring ws;
            ws.resize(DragQueryFileW(drop, i, NULL, 0) + 1);

            ws.resize(DragQueryFileW(drop, i, ws.data(), ws.size()));
            paths.push_back(wcsToUtf8(ws));
        }

        filesDropped(std::move(paths));

        DragFinish(drop);
        return 0;
    }
    }
    return DefWindowProcW(m_data->hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PlatformWindow* window = reinterpret_cast<PlatformWindow*>(GetPropW(hWnd, propKey));
    if (!window) {
        if (uMsg == WM_NCCREATE) {
            if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
                EnableNonClientDpiScaling(hWnd);
            }
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return window->windowProc({ uMsg, wParam, lParam });
}

static bool createHelperWindow() {
    WNDCLASSEXW wc{ .cbSize = sizeof(wc) };
    wc.style                     = CS_OWNDC;
    wc.lpfnWndProc               = (WNDPROC)helperWindowProc;
    wc.hInstance                 = winInstance;
    wc.lpszClassName             = L"Brisk Helper";

    staticData.helperWindowClass = RegisterClassExW(&wc);
    if (!staticData.helperWindowClass) {
        return false;
    }

    staticData.helperWindowHandle = CreateWindowExW(
        WS_EX_OVERLAPPEDWINDOW, MAKEINTATOM(staticData.helperWindowClass), L"Brisk message window",
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, 1, 1, nullptr, nullptr, winInstance, nullptr);

    if (!staticData.helperWindowHandle) {
        return false;
    }

    // HACK: The command to the first ShowWindow call is ignored if the parent
    //       process passed along a STARTUPINFO, so clear that with a no-op call
    ShowWindow(staticData.helperWindowHandle, SW_HIDE);

    MSG msg;
    while (PeekMessageW(&msg, staticData.helperWindowHandle, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return true;
}

/*static*/ void PlatformWindow::initialize() {
    if (isOsWindows10(Windows10Version::CreatorsUpdate))
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    else
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    Internal::updateDisplays();
    createHelperWindow();
}

/*static*/ void PlatformWindow::finalize() {
    DestroyWindow(staticData.helperWindowHandle);
}

void PlatformWindow::setWindowIcon() {
    HICON hIcon = LoadIconW(winInstance, MAKEINTRESOURCE(1));
    SendMessageW(m_data->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessageW(m_data->hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

NativeWindowHandle PlatformWindow::getHandle() const {
    return NativeWindowHandle(m_data->hWnd);
}

Bytes PlatformWindow::placement() const {
    HWND h                    = m_data->hWnd;
    WINDOWPLACEMENT placement = { sizeof(WINDOWPLACEMENT) };
    if (GetWindowPlacement(h, &placement)) {
        Bytes data(sizeof(WINDOWPLACEMENT));
        memcpy(data.data(), &placement, sizeof(WINDOWPLACEMENT));
        return data;
    }
    return {};
}

void PlatformWindow::setPlacement(BytesView data) {
    HWND h = m_data->hWnd;
    if (data.size() == sizeof(WINDOWPLACEMENT)) {
        WINDOWPLACEMENT placement;
        memcpy(&placement, data.data(), sizeof(WINDOWPLACEMENT));
        if (placement.length == sizeof(WINDOWPLACEMENT))
            SetWindowPlacement(h, &placement);
    }
}

void PlatformWindow::setOwner(Rc<Window> window) {
    if (window && window->m_platformWindow) {
        SetWindowLongPtrW(m_data->hWnd, GWLP_HWNDPARENT, (LONG_PTR)window->m_platformWindow->m_data->hWnd);
    } else {
        SetWindowLongPtrW(m_data->hWnd, GWLP_HWNDPARENT, 0);
    }
}

bool PlatformWindow::createWindow() {
    if (!staticData.mainWindowClass) {
        WNDCLASSEXW wc   = { sizeof(wc) };
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc   = &Brisk::windowProc;
        wc.hInstance     = winInstance;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.lpszClassName = L"Brisk";
        // Load default icon
        wc.hIcon = (HICON)LoadImageW(NULL, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

        staticData.mainWindowClass = RegisterClassExW(&wc);
        if (!staticData.mainWindowClass) {
            BRISK_SOFT_ASSERT_MSG("Win32: Failed to register window class", false);
            return false;
        }
    }

    Size size        = max(m_windowSize, Size{ 1, 1 });
    Point initialPos = m_position;

    HMONITOR primary = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    SizeOf<UINT> primaryDpi;
    GetDpiForMonitor(primary, MDT_EFFECTIVE_DPI, &primaryDpi.x, &primaryDpi.y);
    m_scale       = primaryDpi.longestSide() / static_cast<float>(USER_DEFAULT_SCREEN_DPI);

    RECT rect     = { 0, 0, size.width, size.height };

    DWORD style   = getWindowStyle(m_windowStyle);
    DWORD exStyle = getWindowExStyle(m_windowStyle);

    if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
        AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, primaryDpi.x);
    } else {
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    }

    std::wstring wideTitle = utf8ToWcs(m_window->m_title);
    m_data->hWnd = CreateWindowExW(exStyle, MAKEINTATOM(staticData.mainWindowClass), wideTitle.c_str(), style,
                                   initialPos.x == dontCare ? CW_USEDEFAULT : initialPos.x,
                                   initialPos.y == dontCare ? CW_USEDEFAULT : initialPos.y,
                                   rect.right - rect.left, rect.bottom - rect.top,
                                   nullptr, // No parent window
                                   nullptr, // No window menu
                                   winInstance, (LPVOID)this);

    SetPropW(m_data->hWnd, propKey, this);

#ifndef WM_COPYGLOBALDATA
#define WM_COPYGLOBALDATA 0x0049
#endif

    ChangeWindowMessageFilterEx(m_data->hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(m_data->hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
    ChangeWindowMessageFilterEx(m_data->hWnd, WM_COPYGLOBALDATA, MSGFLT_ALLOW, nullptr);

    const HMONITOR mh = MonitorFromWindow(m_data->hWnd, MONITOR_DEFAULTTONEAREST);

    SizeOf<UINT> dpi;
    GetDpiForMonitor(mh, MDT_EFFECTIVE_DPI, &dpi.x, &dpi.y);
    float newScale = dpi.longestSide() / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
    if (newScale != m_scale) {
        m_scale = newScale;
        // Adjust window rect to account for DPI scaling of the window frame and
        // (if enabled) DPI scaling of the content area
        // This cannot be done until we know what monitor the window was placed on
        // Only update the restored window rect as the window may be maximized
        rect    = { 0, 0, size.width, size.height };

        if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
            AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, GetDpiForWindow(m_data->hWnd));
        } else {
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        }

        WINDOWPLACEMENT wp = { .length = sizeof(wp) };
        GetWindowPlacement(m_data->hWnd, &wp);
        OffsetRect(&rect, wp.rcNormalPosition.left - rect.left, wp.rcNormalPosition.top - rect.top);

        wp.rcNormalPosition = rect;
        wp.showCmd          = SW_HIDE;
        SetWindowPlacement(m_data->hWnd, &wp);
    }

    DragAcceptFiles(m_data->hWnd, TRUE);

    RECT clientRect{};
    GetClientRect(m_data->hWnd, &clientRect);
    m_windowSize      = { clientRect.right, clientRect.bottom };
    m_framebufferSize = m_windowSize;

    return true;
}

PlatformWindow::~PlatformWindow() {
    mustBeMainThread();

    RemovePropW(m_data->hWnd, propKey);
    DestroyWindow(m_data->hWnd);
}

PlatformWindow::PlatformWindow(Window* window, Size windowSize, Point position, WindowStyle style)
    : m_data(new PlatformWindowData{}), m_window(window), m_windowStyle(style), m_windowSize(windowSize),
      m_position(position) {
    mustBeMainThread();
    BRISK_ASSERT(m_window);

    bool created = createWindow();
    BRISK_SOFT_ASSERT(created);
    if (!created)
        return;

    setWindowIcon();
    updateSize();
    contentScaleChanged(m_scale, m_scale);
}

void PlatformWindow::setTitle(std::string_view title) {
    SetWindowTextW(m_data->hWnd, utf8ToWcs(title).c_str());
}

void PlatformWindow::setSize(Size size) {
    RECT rect = { 0, 0, size.x, size.y };

    if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(m_windowStyle), FALSE, getWindowExStyle(m_windowStyle),
                                 GetDpiForWindow(m_data->hWnd));
    } else {
        AdjustWindowRectEx(&rect, getWindowStyle(m_windowStyle), FALSE, getWindowExStyle(m_windowStyle));
    }

    SetWindowPos(m_data->hWnd, HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
}

void PlatformWindow::setPosition(Point point) {
    RECT rect = { point.x, point.y, point.x, point.y };

    if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
        AdjustWindowRectExForDpi(&rect, getWindowStyle(m_windowStyle), FALSE, getWindowExStyle(m_windowStyle),
                                 GetDpiForWindow(m_data->hWnd));
    } else {
        AdjustWindowRectEx(&rect, getWindowStyle(m_windowStyle), FALSE, getWindowExStyle(m_windowStyle));
    }

    SetWindowPos(m_data->hWnd, nullptr, rect.left, rect.top, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void PlatformWindow::setSizeLimits(Size minSize, Size maxSize) {
    m_minSize = minSize;
    m_maxSize = maxSize;
    if (m_minSize == Size{ dontCare, dontCare } && m_maxSize == Size{ dontCare, dontCare })
        return;

    RECT area;
    GetWindowRect(m_data->hWnd, &area);
    MoveWindow(m_data->hWnd, area.left, area.top, area.right - area.left, area.bottom - area.top, TRUE);
}

void PlatformWindow::setStyle(WindowStyle windowStyle) {
    if ((windowStyle && WindowStyle::Disabled) && !(m_windowStyle && WindowStyle::Disabled)) {
        // Release all keyboard keys and mouse buttons if the window becomes disabled
        releaseButtonsAndKeys();
    }

    m_windowStyle = windowStyle;

    // Decorated and Resizeable
    RECT rect;
    DWORD style = GetWindowLongW(m_data->hWnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP);
    style |= getWindowStyle(m_windowStyle);

    GetClientRect(m_data->hWnd, &rect);

    if (isOsWindows10(Windows10Version::AnniversaryUpdate)) {
        AdjustWindowRectExForDpi(&rect, style, FALSE, getWindowExStyle(m_windowStyle),
                                 GetDpiForWindow(m_data->hWnd));
    } else {
        AdjustWindowRectEx(&rect, style, FALSE, getWindowExStyle(m_windowStyle));
    }

    ClientToScreen(m_data->hWnd, (POINT*)&rect.left);
    ClientToScreen(m_data->hWnd, (POINT*)&rect.right);
    SetWindowLongW(m_data->hWnd, GWL_STYLE, style);
    SetWindowPos(m_data->hWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);

    // TopMost
    const HWND after = m_windowStyle && WindowStyle::TopMost ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(m_data->hWnd, after, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

    // Disabled
    EnableWindow(m_data->hWnd, m_windowStyle && WindowStyle::Disabled ? FALSE : TRUE);
}

// Returns whether the cursor is in the content area of the specified window
//
bool PlatformWindow::cursorInContentArea() const {
    RECT area;
    POINT pos;

    if (!GetCursorPos(&pos))
        return false;

    if (WindowFromPoint(pos) != m_data->hWnd)
        return false;

    GetClientRect(m_data->hWnd, &area);
    ClientToScreen(m_data->hWnd, (POINT*)&area.left);
    ClientToScreen(m_data->hWnd, (POINT*)&area.right);

    return PtInRect(&area, pos);
}

namespace Internal {

static HICON createIcon(const ImageAccess<ImageFormat::RGBA, AccessMode::R>& image, int xhot, int yhot,
                        bool icon) {
    int i;
    HDC dc;
    HICON handle;
    HBITMAP color, mask;
    BITMAPV5HEADER bi;
    ICONINFO ii;
    unsigned char* target       = NULL;
    const unsigned char* source = (const unsigned char*)image.data();

    ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size        = sizeof(bi);
    bi.bV5Width       = image.width();
    bi.bV5Height      = -image.height();
    bi.bV5Planes      = 1;
    bi.bV5BitCount    = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask     = 0x00ff0000;
    bi.bV5GreenMask   = 0x0000ff00;
    bi.bV5BlueMask    = 0x000000ff;
    bi.bV5AlphaMask   = 0xff000000;

    dc                = GetDC(NULL);
    color = CreateDIBSection(dc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&target, NULL, (DWORD)0);
    ReleaseDC(NULL, dc);

    if (!color) {
        BRISK_SOFT_ASSERT_MSG("Win32: Failed to create RGBA bitmap", false);
        return NULL;
    }

    mask = CreateBitmap(image.width(), image.height(), 1, 1, NULL);
    if (!mask) {
        BRISK_SOFT_ASSERT_MSG("Win32: Failed to create mask bitmap", false);
        DeleteObject(color);
        return NULL;
    }

    for (i = 0; i < image.width() * image.height(); i++) {
        target[0] = source[2];
        target[1] = source[1];
        target[2] = source[0];
        target[3] = source[3];
        target += 4;
        source += 4;
    }

    ZeroMemory(&ii, sizeof(ii));
    ii.fIcon    = icon;
    ii.xHotspot = xhot;
    ii.yHotspot = yhot;
    ii.hbmMask  = mask;
    ii.hbmColor = color;

    handle      = CreateIconIndirect(&ii);

    DeleteObject(color);
    DeleteObject(mask);

    if (!handle) {
        if (icon) {
            BRISK_SOFT_ASSERT_MSG("Win32: Failed to create icon", false);
        } else {
            BRISK_SOFT_ASSERT_MSG("Win32: Failed to create cursor", false);
        }
    }

    return handle;
}

Rc<SystemCursor> PlatformCursors::cursorFromImage(const Rc<Image>& image, Point point, float scale) {
    return rcnew SystemCursor{ createIcon(image->mapRead<ImageFormat::RGBA>(), point.x, point.y, true) };
}

Rc<SystemCursor> PlatformCursors::getSystemCursor(Cursor shape) {
    int id = 0;

    switch (shape) {
    case Cursor::Arrow:
        id = OCR_NORMAL;
        break;
    case Cursor::IBeam:
        id = OCR_IBEAM;
        break;
    case Cursor::Crosshair:
        id = OCR_CROSS;
        break;
    case Cursor::Hand:
        id = OCR_HAND;
        break;
    case Cursor::HResize:
    case Cursor::EWResize:
        id = OCR_SIZEWE;
        break;
    case Cursor::NSResize:
    case Cursor::VResize:
        id = OCR_SIZENS;
        break;
    case Cursor::NWSEResize:
        id = OCR_SIZENWSE;
        break;
    case Cursor::NESWResize:
        id = OCR_SIZENESW;
        break;
    case Cursor::AllResize:
        id = OCR_SIZEALL;
        break;
    case Cursor::NotAllowed:
        id = OCR_NO;
        break;
    default:
        return nullptr;
    }

    HCURSOR cur =
        (HCURSOR)LoadImageW(NULL, MAKEINTRESOURCEW(id), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
    return rcnew SystemCursor{ cur };
}

} // namespace Internal

void PlatformWindow::setCursor(Cursor cursor) {
    m_cursor = Internal::platformCursors.getCursor(cursor, m_scale);

    if (cursor != Cursor::NotSet)
        SetCursor(m_cursor->cursor);
    else
        SetCursor(LoadCursorW(NULL, IDC_ARROW));
}

bool PlatformWindow::isVisible() const {
    return IsWindowVisible(m_data->hWnd);
}

void PlatformWindow::iconify() {
    ShowWindow(m_data->hWnd, SW_MINIMIZE);
}

void PlatformWindow::maximize() {
    if (IsWindowVisible(m_data->hWnd)) {
        ShowWindow(m_data->hWnd, SW_MAXIMIZE);
    }
}

void PlatformWindow::restore() {
    ShowWindow(m_data->hWnd, SW_RESTORE);
}

void PlatformWindow::focus() {
    BringWindowToTop(m_data->hWnd);
    SetForegroundWindow(m_data->hWnd);
    SetFocus(m_data->hWnd);
}

bool PlatformWindow::isFocused() const {
    return m_data->hWnd == GetActiveWindow();
}

bool PlatformWindow::isIconified() const {
    return IsIconic(m_data->hWnd);
}

bool PlatformWindow::isMaximized() const {
    return IsZoomed(m_data->hWnd);
}

void PlatformWindow::updateVisibility() {
    bool visible = m_window->m_visible;
    if (visible) {
        ShowWindow(m_data->hWnd, SW_SHOWNA);
        focus();
    } else {
        ShowWindow(m_data->hWnd, SW_HIDE);
    }
}

/* static */ void PlatformWindow::pollEvents() {
    MSG msg;
    HWND handle;
    PlatformWindow* window;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            // NOTE: Other processes may post WM_QUIT, for example Task Manager
            // HACK: Treat WM_QUIT as a close on all windows
            windowApplication->quit(0);
        } else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // HACK: Release modifier keys that the system did not emit KEYUP for
    // NOTE: Shift keys on Windows tend to "stick" when both are pressed as
    //       no key up message is generated by the first key release
    // NOTE: Windows key is not reported as released by the Win+V hotkey
    //       Other Win hotkeys are handled implicitly by focusChange
    //       because they change the input focus
    // NOTE: The other half of this is in the WM_*KEY* handler in windowProc
    handle = GetActiveWindow();
    if (handle) {
        window = (PlatformWindow*)GetPropW(handle, propKey);
        if (window) {
            const struct {
                int k;
                KeyCode c;
            } keys[4] = { { VK_LSHIFT, KeyCode::LeftShift },
                          { VK_RSHIFT, KeyCode::RightShift },
                          { VK_LWIN, KeyCode::LeftSuper },
                          { VK_RWIN, KeyCode::RightSuper } };

            for (int i = 0; i < 4; i++) {
                const int vk       = keys[i].k;
                const KeyCode key  = keys[i].c;
                const int scancode = keyCodeToScanCode(key);

                if ((GetKeyState(vk) & 0x8000))
                    continue;
                if (!window->m_keyState[+key])
                    continue;

                window->keyEvent(key, scancode, KeyAction::Release, getKeyMods());
            }
        }
    }
}

/* static */ void PlatformWindow::waitEvents() {
    WaitMessage();
    pollEvents();
}

/* static */ void PlatformWindow::postEmptyEvent() {
    PostMessageW(staticData.helperWindowHandle, WM_NULL, 0, 0);
}

/* static */ DblClickParams PlatformWindow::dblClickParams() {
    return { GetDoubleClickTime() / 1000.0,
             (GetSystemMetrics(SM_CXDOUBLECLK) + GetSystemMetrics(SM_CYDOUBLECLK)) / 2 / 2 };
}

HiDPIMode hiDPIMode() {
    return HiDPIMode::ApplicationScaling;
}

} // namespace Brisk
