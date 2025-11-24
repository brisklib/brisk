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
#include "PlatformWindow.hpp"
#include <brisk/graphics/Svg.hpp>
#include <brisk/window/Window.hpp>
#include "Cursors.hpp"
#include <brisk/window/Types.hpp>

namespace Brisk {

std::vector<PlatformWindow*> PlatformWindow::platformWindows;

void PlatformWindow::updateSize() {
    mustBeMainThread();
    if (m_iconified)
        return;

    m_window->windowResized(m_windowSize, m_framebufferSize);
}

namespace Internal {

PlatformCursors platformCursors;

void PlatformCursors::registerCursor(Cursor cursor, SvgCursor svgCursor) {
    BRISK_ASSERT(!isSystem(cursor));
    m_svgCursors.insert_or_assign(cursor, std::move(svgCursor));
}

Rc<SystemCursor> PlatformCursors::getCursor(Cursor cursor, float scale_) {
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
        Rc<Image> bmp  = SvgImage(it->second.svg).render(SizeF(SvgCursor::size) * scale);
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
    m_svgCursors.insert_or_assign(Cursor::Grab, SvgCursor{ std::string(cursorGrabSvg), Point{ 12, 12 } });
    m_svgCursors.insert_or_assign(Cursor::GrabDeny,
                                  SvgCursor{ std::string(cursorGrabDenySvg), Point{ 12, 12 } });
    m_svgCursors.insert_or_assign(Cursor::GrabReady,
                                  SvgCursor{ std::string(cursorGrabReadySvg), Point{ 12, 12 } });
}

bool PlatformCursors::isSystem(Cursor cursor) {
    return static_cast<uint32_t>(cursor) & 0x80000000u;
}
} // namespace Internal

bool PlatformWindow::charEvent(char32_t codepoint, bool nonClient) {
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160))
        return false;
    if (!nonClient) {
        return m_window->charEvent(static_cast<char32_t>(codepoint));
    }
    return false;
}

void PlatformWindow::releaseButtonsAndKeys() {
    for (int kc = 0; kc <= +KeyCode::Last; ++kc) {
        if (m_keyState[kc]) {
            std::ignore = keyEvent(static_cast<KeyCode>(kc), keyCodeToScanCode(KeyCode(kc)),
                                   KeyAction::Release, KeyModifiers::None);
        }
    }
    for (int mb = 0; mb <= +MouseButton::Last; ++mb) {
        if (m_mouseState[mb]) {
            std::ignore = mouseEvent(static_cast<MouseButton>(mb), MouseAction::Release, KeyModifiers::None,
                                     PointF(-1, -1));
        }
    }
}

void PlatformWindow::focusChange(bool gained) {
    if (!gained) {
        releaseButtonsAndKeys();
    }

    m_window->focusChange(gained);
}

void PlatformWindow::closeAttempt() {
    m_shouldClose = true;
    m_window->closeAttempt();
}

void PlatformWindow::requestRedraw() {
    m_window->doPaint();
}

bool PlatformWindow::keyEvent(KeyCode key, int scancode, KeyAction action, KeyModifiers mods) {
    if (m_windowStyle && WindowStyle::Disabled)
        return false;

    if (key < KeyCode(0) || key > KeyCode::Last) // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
        return false;

    bool repeated = false;

    if (action == KeyAction::Release && !m_keyState[+key])
        return false;

    if (action == KeyAction::Press && m_keyState[+key])
        repeated = true;

    m_keyState[+key] = action == KeyAction::Press;

    if (repeated)
        action = KeyAction::Repeat;
    return m_window->keyEvent(static_cast<KeyCode>(key), scancode, static_cast<KeyAction>(action),
                              static_cast<KeyModifiers>(mods));
}

bool PlatformWindow::mouseEvent(MouseButton button, MouseAction action, KeyModifiers mods, PointF pos,
                                Window::Unit unit) {
    if (m_windowStyle && WindowStyle::Disabled)
        return false;

    if (button < MouseButton(0) || button > MouseButton::Last)
        return false;

    if (action == MouseAction::Release && !m_mouseState[+button])
        return false;
    if (action == MouseAction::Press && m_mouseState[+button])
        return false;

    m_mouseState[+button] = action == MouseAction::Press;

    return m_window->mouseEvent(button, action, mods,
                                m_window->convertUnit(Window::Unit::Framebuffer, pos, unit));
}

void PlatformWindow::mouseEnterOrLeave(bool enter) {
    if (enter)
        m_window->mouseEnter();
    else
        m_window->mouseLeave();
}

bool PlatformWindow::mouseMove(PointF pos, Window::Unit unit) {
    return m_window->mouseMove(m_window->convertUnit(Window::Unit::Framebuffer, pos, unit));
}

bool PlatformWindow::wheelEvent(float x, float y) {
    return m_window->wheelEvent(x, y);
}

void PlatformWindow::windowStateEvent(WindowState state) {}

void PlatformWindow::windowResized(Size windowSize, Size framebufferSize) {
    mustBeMainThread();
    updateSize();
}

void PlatformWindow::windowMoved(Point position) {
    mustBeMainThread();
    if (!isVisible()) {
        return;
    }
    m_window->windowMoved(position);
}

void PlatformWindow::windowNonClientClicked() {
    mustBeMainThread();
    if (!isVisible()) {
        return;
    }
    m_window->windowNonClientClicked();
}

void PlatformWindow::contentScaleChanged(float xscale, float yscale) {
    mustBeMainThread();
    updateSize();
    std::ignore              = yscale;
    m_window->m_contentScale = xscale;
    m_window->recomputeScales();
}

bool PlatformWindow::filesDropped(std::vector<std::string> files) {
    mustBeMainThread();
    return m_window->filesDropped(std::move(files));
}

void PlatformWindow::windowStateChanged(bool isIconified, bool isMaximized) {
    m_window->windowStateChanged(isIconified, isMaximized);
}

} // namespace Brisk
