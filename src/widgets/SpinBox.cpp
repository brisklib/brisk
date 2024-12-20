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
#include <brisk/widgets/SpinBox.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

void SpinBox::onChildAdded(Widget* w) {
    Base::onChildAdded(w);
    if (auto* text = display.matchesType(w)) {
        text->flexGrow = 1;
        text->text     = Value{ &this->value }.transform(m_valueFormatter);
    }
    if (auto* btns = buttons.matchesType(w)) {
        btns->flexGrow = 0;
        ArgumentsView<Button>{
            std::tuple{ Arg::onClick = listener(
                            [this]() {
                                increment();
                            },
                            this) }
        }.apply(UpDownButtons::up.get(btns).get());
        ArgumentsView<Button>{
            std::tuple{ Arg::onClick = listener(
                            [this]() {
                                decrement();
                            },
                            this) }
        }.apply(UpDownButtons::down.get(btns).get());
    }
}

void SpinBox::onConstructed() {
    m_layout = Layout::Horizontal;

    display.create(this);
    buttons.create(this);
    Base::onConstructed();
}

void SpinBox::onEvent(Event& event) {
    Base::onEvent(event);
    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        value = m_value + delta;
        event.stopPropagation();
        return;
    }
    if (event.keyPressed(KeyCode::Up)) {
        increment();
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Down)) {
        decrement();
        event.stopPropagation();
    }
}

SpinBox::SpinBox(Construction construction, ArgumentsView<SpinBox> args) : Base(construction, nullptr) {
    layout  = Layout::Horizontal;
    tabStop = true;
    args.apply(this);
}

Widget::Ptr SpinBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

UpDownButtons::UpDownButtons(Construction construction, ArgumentsView<UpDownButtons> args)
    : Base(construction, nullptr) {
    layout = Layout::Vertical;
    args.apply(this);
}

void UpDownButtons::onChildAdded(Widget* w) {
    Base::onChildAdded(w);

    if (Button* btn = dynamic_cast<Button*>(w)) {
        if (!up.get(this)) {
            btn->role = up.role();
        } else {
            btn->role = down.role();
        }
        btn->tabStop        = false;
        btn->flexGrow       = 1;
        btn->clickEvent     = ButtonClickEvent::MouseDown;
        btn->repeatDelay    = 0.5;
        btn->repeatInterval = 0.25;
    }
}

void UpDownButtons::onConstructed() {
    if (!down.get(this) && !up.get(this)) {
        apply(rcnew Button{ rcnew Text{ ICON_chevron_up } });
        apply(rcnew Button{ rcnew Text{ ICON_chevron_down } });
    }
    Base::onConstructed();
}

Widget::Ptr UpDownButtons::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}
} // namespace Brisk
