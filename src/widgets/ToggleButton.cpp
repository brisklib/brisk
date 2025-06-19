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
#include <brisk/widgets/ToggleButton.hpp>

namespace Brisk {

void ToggleButton::updateState() {
    if (widgets().size() == 2 && m_twoState) {
        widgets()[0]->visible = !m_value;
        widgets()[1]->visible = m_value;
        toggleState(WidgetState::Selected, false);
    } else {
        toggleState(WidgetState::Selected, m_value);
    }
}

void ToggleButton::onAnimationFrame() {
    Button::onAnimationFrame();
    if (m_interpolatedValueTransition.isActive()) {
        requestAnimationFrame();
    }
    m_interpolatedValueTransition.tick(
        m_interpolatedValue, m_value ? m_interpolatedValueTransitionOn : m_interpolatedValueTransitionOff,
        &easeLinear);
}

void ToggleButton::onClicked() {
    value = !m_value;
    Button::onClicked();
}

void ToggleButton::onChanged() {
    invalidate();
    updateState();
    m_interpolatedValueTransition.set(
        m_interpolatedValue, m_value ? 1 : 0,
        transitionAllowed() ? (m_value ? m_interpolatedValueTransitionOn : m_interpolatedValueTransitionOff)
                            : 0);
    bindings->notify(&m_interpolatedValue);
    if (m_interpolatedValueTransition.isActive())
        requestAnimationFrame();
}

Rc<Widget> ToggleButton::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ToggleButton::ToggleButton(Construction construction, ArgumentsView<ToggleButton> args)
    : Base{ construction, nullptr } {
    args.apply(this);
}

void ToggleButton::onConstructed() {
    Base::onConstructed();
    updateState();
}
} // namespace Brisk
