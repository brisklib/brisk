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
#include <brisk/gui/Component.hpp>

namespace Brisk {

RC<GUIWindow> Component::window() {
    return m_window.lock();
}

WidgetTree& Component::tree() {
    return window()->tree();
}

RC<Widget> Component::build() {
    return rcnew Widget{
        flexGrow = 1,
    };
}

RC<GUIWindow> Component::makeWindow() {
    if (auto window = m_window.lock())
        return window;
    RC<GUIWindow> window = createWindow();
    m_window             = window;
    configureWindow(window);
    return window;
}

RC<GUIWindow> Component::createWindow() {
    return rcnew GUIWindow{ shared_from_this() };
}

void Component::configureWindow(RC<GUIWindow> window) {
    RC<Window> curWindow = Internal::currentWindowPtr();

    window->setTitle("Component"_tr);
    window->setSize({ 1050, 740 });
    window->setStyle(WindowStyle::Normal);
}

void Component::unhandledEvent(Event& event) {}

void Component::onScaleChanged() {}

void Component::beforeFrame() {}

void Component::closeWindow() {
    if (auto win = m_window.lock())
        win->close();
}

} // namespace Brisk
