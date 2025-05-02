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
#include <brisk/window/Types.hpp>
#include "PlatformWindow.hpp"

#include <brisk/core/Log.hpp>

#include <brisk/core/Utilities.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/window/Display.hpp>
#include <brisk/graphics/OsWindowHandle.hpp>
#include <brisk/window/Window.hpp>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11 1
#define GLFW_EXPOSE_NATIVE_WAYLAND 1
#include <GLFW/glfw3native.h>
#undef None

namespace Brisk {

static HiDPIMode currentHiDPIMode = HiDPIMode::ApplicationScaling;

struct PlatformWindowData {
    GLFWwindow* win = nullptr;
};

/*static*/ void PlatformWindow::initialize() {
    if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
        LOG_INFO(window, "Wayland is supported, enabling it");
    } else {
        LOG_INFO(window, "Wayland is not supported, using x11");
    }

    glfwInit();

    int platform = glfwGetPlatform();
    if (platform == GLFW_PLATFORM_WAYLAND) {
        LOG_INFO(window, "Using: Wayland");
        currentHiDPIMode = HiDPIMode::FramebufferScaling;
    } else {
        LOG_INFO(window, "Using: X11");
        currentHiDPIMode = HiDPIMode::ApplicationScaling;
    }
    Internal::updateDisplays();
}

/*static*/ void PlatformWindow::finalize() {
    glfwTerminate();
}

void PlatformWindow::setWindowIcon() {
    //
}

OsWindowHandle PlatformWindow::getHandle() const {
    return OsWindowHandle(m_data->win);
}

Bytes PlatformWindow::placement() const {
    return {};
}

void PlatformWindow::setPlacement(BytesView data) {}

void PlatformWindow::setOwner(Rc<Window> window) {}

bool PlatformWindow::createWindow() {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    LOG_INFO(window, "GLFW {}", glfwGetVersionString());

    glfwWindowHintString(GLFW_WAYLAND_APP_ID, (appMetadata.name + "Brisk").c_str());
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "Brisk");
    glfwWindowHintString(GLFW_X11_INSTANCE_NAME, appMetadata.name.c_str());

    glfwWindowHint(GLFW_DECORATED, m_windowStyle && WindowStyle::Undecorated ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, m_windowStyle && WindowStyle::TopMost ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, m_windowStyle && WindowStyle::Resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);

    Size size   = max(m_windowSize, Size{ 1, 1 });

    m_data->win = glfwCreateWindow(size.width, size.height, m_window->m_title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_data->win, this);
    glfwSetWindowSize(m_data->win, size.width, size.height);
    if (m_position.x != dontCare && m_position.y != dontCare) {
        glfwSetWindowPos(m_data->win, m_position.x, m_position.y);
    }

    SizeF contentScale{};
    glfwGetWindowContentScale(m_data->win, &contentScale.x, &contentScale.y);
    m_scale = contentScale.longestSide();

    glfwSetWindowCloseCallback(m_data->win, [](GLFWwindow* gw) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->closeAttempt();
    });
    glfwSetWindowPosCallback(m_data->win, nullptr);
    glfwSetWindowFocusCallback(m_data->win, [](GLFWwindow* gw, int gained) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->focusChange(gained);
    });
    glfwSetWindowIconifyCallback(m_data->win, nullptr);
    glfwSetWindowMaximizeCallback(m_data->win, nullptr);
    glfwSetWindowRefreshCallback(m_data->win, nullptr);
    glfwSetWindowContentScaleCallback(m_data->win, [](GLFWwindow* gw, float scalex, float scaley) {
        auto* window    = reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw));
        window->m_scale = std::max(scalex, scaley);
        window->contentScaleChanged(scalex, scaley);
        glfwGetWindowSize(gw, &window->m_windowSize.x, &window->m_windowSize.y);
        glfwGetFramebufferSize(gw, &window->m_framebufferSize.x, &window->m_framebufferSize.y);
        window->windowResized(window->m_windowSize, window->m_framebufferSize);
    });
    glfwSetWindowSizeCallback(m_data->win, [](GLFWwindow* gw, int width, int height) {
        auto* window         = reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw));
        window->m_windowSize = { width, height };
        glfwGetFramebufferSize(gw, &window->m_framebufferSize.x, &window->m_framebufferSize.y);
        window->windowResized(window->m_windowSize, window->m_framebufferSize);
    });
    glfwSetFramebufferSizeCallback(m_data->win, [](GLFWwindow* gw, int width, int height) {
        auto* window              = reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw));
        window->m_framebufferSize = { width, height };
        glfwGetWindowSize(gw, &window->m_windowSize.x, &window->m_windowSize.y);
        window->windowResized(window->m_windowSize, window->m_framebufferSize);
    });
    glfwSetKeyCallback(m_data->win, [](GLFWwindow* gw, int key, int scancode, int action, int mods) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))
            ->keyEvent(static_cast<KeyCode>(key), scancode, static_cast<KeyAction>(action),
                       static_cast<KeyModifiers>(mods));
    });
    glfwSetCharCallback(m_data->win, [](GLFWwindow* gw, unsigned int codepoint) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->charEvent(codepoint, false);
    });
    glfwSetCursorPosCallback(m_data->win, [](GLFWwindow* gw, double xpos, double ypos) {
        PointOf<double> cur{ xpos, ypos };
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->mouseMove(cur, Window::Unit::Screen);
    });
    glfwSetMouseButtonCallback(m_data->win, [](GLFWwindow* gw, int button, int action, int mods) {
        PointOf<double> cur;
        glfwGetCursorPos(gw, &cur.x, &cur.y);
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))
            ->mouseEvent(static_cast<MouseButton>(button), static_cast<MouseAction>(action),
                         static_cast<KeyModifiers>(mods), cur, Window::Unit::Screen);
    });
    glfwSetCursorEnterCallback(m_data->win, [](GLFWwindow* gw, int entered) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->mouseEnterOrLeave(entered);
    });
    glfwSetScrollCallback(m_data->win, [](GLFWwindow* gw, double xoffset, double yoffset) {
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->wheelEvent(xoffset, yoffset);
    });
    glfwSetDropCallback(m_data->win, [](GLFWwindow* gw, int path_count, const char* paths[]) {
        std::vector<std::string> files(paths, paths + path_count);
        reinterpret_cast<PlatformWindow*>(glfwGetWindowUserPointer(gw))->filesDropped(std::move(files));
    });

    glfwGetWindowPos(m_data->win, &m_position.x, &m_position.y);
    glfwGetWindowSize(m_data->win, &m_windowSize.x, &m_windowSize.y);
    glfwGetFramebufferSize(m_data->win, &m_framebufferSize.x, &m_framebufferSize.y);

    return true;
}

PlatformWindow::~PlatformWindow() {
    std::erase(platformWindows, this);

    glfwDestroyWindow(m_data->win);
    m_data->win = nullptr;
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

    platformWindows.push_back(this);
}

void PlatformWindow::setTitle(std::string_view title) {
    glfwSetWindowTitle(m_data->win, std::string(m_window->m_title).c_str());
}

void PlatformWindow::setSize(Size size) {
    glfwSetWindowSize(m_data->win, size.width, size.height);
}

void PlatformWindow::setPosition(Point point) {
    glfwSetWindowPos(m_data->win, point.x, point.y);
}

static_assert(PlatformWindow::dontCare == GLFW_DONT_CARE);

void PlatformWindow::setSizeLimits(Size minSize, Size maxSize) {
    glfwSetWindowSizeLimits(m_data->win, minSize.x, minSize.y, maxSize.x, maxSize.y);
}

void PlatformWindow::setStyle(WindowStyle windowStyle) {
    if ((windowStyle && WindowStyle::Disabled) && !(m_windowStyle && WindowStyle::Disabled)) {
        // Release all keyboard keys and mouse buttons if the window becomes disabled
        releaseButtonsAndKeys();
    }
    m_windowStyle = windowStyle;
    glfwWindowHint(GLFW_DECORATED, m_windowStyle && WindowStyle::Undecorated ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, m_windowStyle && WindowStyle::TopMost ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, m_windowStyle && WindowStyle::Resizable ? GLFW_TRUE : GLFW_FALSE);
}

bool PlatformWindow::cursorInContentArea() const {
    PointOf<double> cur;
    PointOf<int> pos;
    SizeOf<int> size;
    glfwGetCursorPos(m_data->win, &cur.x, &cur.y);
    glfwGetWindowSize(m_data->win, &size.x, &size.y);
    glfwGetWindowPos(m_data->win, &pos.x, &pos.y);
    return RectangleF(pos, size).contains(PointF(cur.x));
}

namespace Internal {

struct SystemCursor {
    GLFWcursor* cursor;

    ~SystemCursor() {
        glfwDestroyCursor(cursor);
    }
};

Rc<SystemCursor> PlatformCursors::cursorFromImage(const Rc<Image>& image, Point point, float scale) {
    GLFWimage img;
    auto rd    = image->mapRead();
    img.width  = rd.width();
    img.height = rd.height();
    Bytes data;
    if (rd.byteStride() == rd.width() * 4) {
        img.pixels = (unsigned char*)rd.data();
    } else {
        rd.writeTo(data);
        img.pixels = (unsigned char*)data.data();
    }
    return rcnew SystemCursor(glfwCreateCursor(&img, point.x, point.y));
}

static GLFWcursor* loadGLFWCursor(Cursor shape) {
    switch (shape) {
    case Cursor::Arrow:
        return glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    case Cursor::IBeam:
        return glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    case Cursor::Crosshair:
        return glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    case Cursor::Hand:
        return glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    case Cursor::HResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    case Cursor::VResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    case Cursor::NSResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    case Cursor::EWResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    case Cursor::NESWResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    case Cursor::NWSEResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    case Cursor::AllResize:
        return glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    case Cursor::NotAllowed:
        return glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
    default:
        return glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    }
}

Rc<SystemCursor> PlatformCursors::getSystemCursor(Cursor shape) {
    GLFWcursor* cur = loadGLFWCursor(shape);
    if (cur) {
        return rcnew SystemCursor{ cur };
    }
    return nullptr;
}
} // namespace Internal

void PlatformWindow::setCursor(Cursor cursor) {
    m_cursor = Internal::platformCursors.getCursor(cursor, m_scale);
    if (m_cursor)
        glfwSetCursor(m_data->win, m_cursor->cursor);
}

bool PlatformWindow::isVisible() const {
    return glfwGetWindowAttrib(m_data->win, GLFW_VISIBLE);
}

void PlatformWindow::iconify() {
    glfwIconifyWindow(m_data->win);
}

void PlatformWindow::maximize() {
    glfwMaximizeWindow(m_data->win);
}

void PlatformWindow::restore() {
    glfwRestoreWindow(m_data->win);
}

void PlatformWindow::focus() {
    glfwFocusWindow(m_data->win);
}

bool PlatformWindow::isFocused() const {
    return glfwGetWindowAttrib(m_data->win, GLFW_FOCUSED);
}

bool PlatformWindow::isIconified() const {
    return glfwGetWindowAttrib(m_data->win, GLFW_ICONIFIED);
}

bool PlatformWindow::isMaximized() const {
    return glfwGetWindowAttrib(m_data->win, GLFW_MAXIMIZED);
}

void PlatformWindow::updateVisibility() {
    bool visible = m_window->m_visible;
    if (visible) {
        glfwShowWindow(m_data->win);
        glfwFocusWindow(m_data->win);
    } else {
        glfwHideWindow(m_data->win);
    }
}

/* static */ void PlatformWindow::pollEvents() {
    glfwPollEvents();
}

/* static */ void PlatformWindow::waitEvents() {
    glfwWaitEvents();
}

/* static */ void PlatformWindow::postEmptyEvent() {
    glfwPostEmptyEvent();
}

/* static */ DblClickParams PlatformWindow::dblClickParams() {
    return { 0.5, 2 };
}

HiDPIMode hiDPIMode() {
    return currentHiDPIMode;
}

} // namespace Brisk
