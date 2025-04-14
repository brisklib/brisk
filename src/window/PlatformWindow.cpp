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
#include "PlatformWindow.hpp"
#include <brisk/graphics/SVG.hpp>
#include <brisk/window/Window.hpp>
#include "Cursors.hpp"
#include <brisk/window/Types.hpp>

namespace Brisk {

std::vector<PlatformWindow*> PlatformWindow::platformWindows;

void PlatformWindow::updateSize() {
    mustBeMainThread();
    if (m_iconified)
        return;

    uiScheduler->dispatch([window = m_window, size = m_windowSize, framebufferSize = m_framebufferSize] {
        window->windowResized(size, framebufferSize);
    });
}

namespace Internal {

PlatformCursors platformCursors;

void PlatformCursors::registerCursor(Cursor cursor, SVGCursor svgCursor) {
    BRISK_ASSERT(!isSystem(cursor));
    m_svgCursors.insert_or_assign(cursor, std::move(svgCursor));
}

RC<SystemCursor> PlatformCursors::getCursor(Cursor cursor, float scale_) {
    if (isSystem(cursor)) {
        initSystemCursors();
        return m_systemCursors.at(cursor);
    }
    CursorKey key{ cursor, static_cast<int>(std::lround(4 * scale_)) };
    float scale = key.scale * 0.25f;

    if (auto it = m_svgCursorCache.find(key); it != m_svgCursorCache.end()) {
        return it->second;
    }
    if (auto it = m_svgCursors.find(cursor); it != m_svgCursors.end()) {
        RC<Image> bmp  = SVGImage(it->second.svg).render(SizeF(SVGCursor::size) * scale);
        auto svgCursor = cursorFromImage(
            bmp, Point(std::lround(it->second.hotspot.x * scale), std::lround(it->second.hotspot.y * scale)),
            scale);
        m_svgCursorCache.insert_or_assign(key, svgCursor);
        return svgCursor;
    }

    return nullptr;
}

void PlatformCursors::initSystemCursors() {
    if (m_systemCursorsInitialized)
        return;
    m_systemCursors.insert_or_assign(Cursor::Arrow, getSystemCursor(Cursor::Arrow));
    m_systemCursors.insert_or_assign(Cursor::IBeam, getSystemCursor(Cursor::IBeam));
    m_systemCursors.insert_or_assign(Cursor::Crosshair, getSystemCursor(Cursor::Crosshair));
    m_systemCursors.insert_or_assign(Cursor::Hand, getSystemCursor(Cursor::Hand));
    m_systemCursors.insert_or_assign(Cursor::HResize, getSystemCursor(Cursor::HResize));
    m_systemCursors.insert_or_assign(Cursor::VResize, getSystemCursor(Cursor::VResize));
    m_systemCursors.insert_or_assign(Cursor::NSResize, getSystemCursor(Cursor::NSResize));
    m_systemCursors.insert_or_assign(Cursor::EWResize, getSystemCursor(Cursor::EWResize));
    m_systemCursors.insert_or_assign(Cursor::NESWResize, getSystemCursor(Cursor::NESWResize));
    m_systemCursors.insert_or_assign(Cursor::NWSEResize, getSystemCursor(Cursor::NWSEResize));
    m_systemCursorsInitialized = true;
}

PlatformCursors::PlatformCursors() {
    m_svgCursors.insert_or_assign(Cursor::Grab, SVGCursor{ std::string(cursorGrabSvg), Point{ 12, 12 } });
    m_svgCursors.insert_or_assign(Cursor::GrabDeny,
                                  SVGCursor{ std::string(cursorGrabDenySvg), Point{ 12, 12 } });
    m_svgCursors.insert_or_assign(Cursor::GrabReady,
                                  SVGCursor{ std::string(cursorGrabReadySvg), Point{ 12, 12 } });
}

bool PlatformCursors::isSystem(Cursor cursor) {
    return static_cast<uint32_t>(cursor) & 0x80000000u;
}
} // namespace Internal

void PlatformWindow::charEvent(char32_t codepoint, bool nonClient) {
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
        return;
    if (!nonClient) {
        uiScheduler->dispatch([window = m_window, codepoint] {
            window->charEvent(static_cast<char32_t>(codepoint));
        });
    }
}

void PlatformWindow::releaseButtonsAndKeys() {
    for (int kc = 0; kc <= +KeyCode::Last; ++kc) {
        if (m_keyState[kc]) {
            keyEvent(static_cast<KeyCode>(kc), keyCodeToScanCode(KeyCode(kc)), KeyAction::Release,
                     KeyModifiers::None);
        }
    }
    for (int mb = 0; mb <= +MouseButton::Last; ++mb) {
        if (m_mouseState[mb]) {
            mouseEvent(static_cast<MouseButton>(mb), MouseAction::Release, KeyModifiers::None,
                       PointF(-1, -1));
        }
    }
}

void PlatformWindow::focusChange(bool gained) {
    if (!gained) {
        releaseButtonsAndKeys();
    }

    uiScheduler->dispatch([window = m_window, gained] {
        window->focusChange(gained);
    });
}

void PlatformWindow::closeAttempt() {
    m_shouldClose = true;
    uiScheduler->dispatch([window = m_window] {
        window->closeAttempt();
    });
}

void PlatformWindow::keyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    if (m_windowStyle && WindowStyle::Disabled)
        return;

    if (key < KeyCode(0) || key > KeyCode::Last) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        return;

    bool repeated = false;

    if (action == KeyAction::Release && !m_keyState[+key])
        return;

    if (action == KeyAction::Press && m_keyState[+key])
        repeated = true;

    m_keyState[+key] = action == KeyAction::Press;

    if (repeated)
        action = KeyAction::Repeat;
    uiScheduler->dispatch([window = m_window, key, scancode, action, mods] {
        window->keyEvent(static_cast<KeyCode>(key), scancode, static_cast<KeyAction>(action),
                         static_cast<KeyModifiers>(mods));
    });
}

void PlatformWindow::mouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF pos) {
    if (m_windowStyle && WindowStyle::Disabled)
        return;

    if (button < MouseButton(0) || button > MouseButton::Last)
        return;

    if (action == MouseAction::Release && !m_mouseState[+button])
        return;
    if (action == MouseAction::Press && m_mouseState[+button])
        return;

    m_mouseState[+button] = action == MouseAction::Press;

    uiScheduler->dispatch([window = m_window, button, action, mods, pos] {
        window->mouseEvent(button, action, mods, pos);
    });
}

void PlatformWindow::mouseEnterOrLeave(bool enter) {
    uiScheduler->dispatch([window = m_window, enter] {
        if (enter)
            window->mouseEnter();
        else
            window->mouseLeave();
    });
}

void PlatformWindow::mouseMove(PointF pos) {
    uiScheduler->dispatch([window = m_window, pos] {
        window->mouseMove(pos);
    });
}

void PlatformWindow::wheelEvent(float x, float y) {
    uiScheduler->dispatch([window = m_window, x, y] {
        window->wheelEvent(x, y);
    });
}

void PlatformWindow::windowStateEvent(WindowState state) {}

void PlatformWindow::windowResized(Size windowSize, Size framebufferSize) {
    updateSize();
}

void PlatformWindow::windowMoved(Point position) {
    if (!isVisible()) {
        return;
    }
    uiScheduler->dispatch([window = m_window, position] {
        window->windowMoved(position);
    });
}

void PlatformWindow::contentScaleChanged(float xscale, float yscale) {
    updateSize();
    std::ignore = yscale;
    uiScheduler->dispatch([platformWindow = this, window = m_window, xscale] {
        window->m_windowPixelRatio = xscale;
        window->determineWindowDPI();
        window->windowPixelRatioChanged();
        mainScheduler->dispatchAndWait([platformWindow] {
            platformWindow->updateSize();
        });
    });
}

void PlatformWindow::filesDropped(std::vector<std::string> files) {
    uiScheduler->dispatch([window = m_window, files = std::move(files)] {
        window->filesDropped(std::move(files));
    });
}

void PlatformWindow::windowStateChanged(bool isIconified, bool isMaximized) {
    uiScheduler->dispatch([window = m_window, isIconified, isMaximized] {
        window->windowStateChanged(isIconified, isMaximized);
    });
}

PointF PlatformWindow::mapFramebuffer(PointF pos) {
    return pos * SizeF(m_framebufferSize) / SizeF(m_windowSize);
}

Size PlatformWindow::scaleSize(Size size) const {
    if ((m_windowStyle && WindowStyle::ExactSize))
        return size;
    return { size.x >= 0 ? static_cast<int>(size.x * m_scale) : size.x,
             size.y >= 0 ? static_cast<int>(size.y * m_scale) : size.y };
}

} // namespace Brisk
