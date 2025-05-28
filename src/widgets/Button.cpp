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
#include <brisk/widgets/Button.hpp>

namespace Brisk {

void Button::doClick() {
    m_onClick.trigger();
    onClicked();
}

void Button::onClicked() {}

void Button::onRefresh() {
    if (m_repeatState) {
        int repeats = std::floor((currentTime() - m_repeatState->startTime) / m_repeatInterval);
        if (repeats > m_repeatState->repeats) {
            m_repeatState->repeats = repeats;
            doClick();
        }
    }
}

void Button::onEvent(Event& event) {
    Widget::onEvent(event);
    if (event.pressed()) {
        focus();
        if (std::isfinite(m_repeatDelay) && std::isfinite(m_repeatInterval)) {
            m_repeatState = { currentTime() + m_repeatDelay, 0 };
        }
        if (m_clickEvent == ButtonClickEvent::MouseDown) {
            doClick();
        }
        event.stopPropagation();
    } else if (event.released(m_rect)) {
        if (m_clickEvent == ButtonClickEvent::MouseUp) {
            doClick();
        }
        m_repeatState = std::nullopt;
        event.stopPropagation();
    } else if ((m_keyEvents && ButtonKeyEvents::AcceptsEnter) && event.keyPressed(KeyCode::Enter) ||
               (m_keyEvents && ButtonKeyEvents::AcceptsSpace) && event.keyPressed(KeyCode::Space)) {
        toggleState(WidgetState::Pressed, true);
        doClick();
        event.stopPropagation();
    } else if ((m_keyEvents && ButtonKeyEvents::AcceptsEnter) && event.keyReleased(KeyCode::Enter) ||
               (m_keyEvents && ButtonKeyEvents::AcceptsSpace) && event.keyReleased(KeyCode::Space)) {
        toggleState(WidgetState::Pressed, false);
        event.stopPropagation();
    }
}

Button::Button(Construction construction, ArgumentsView<Button> args) : Widget{ construction, nullptr } {
    m_processClicks = false;
    m_tabStop       = true;
    args.apply(this);
}

Rc<Widget> Button::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
