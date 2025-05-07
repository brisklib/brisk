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
#include <brisk/widgets/PopupButton.hpp>

namespace Brisk {

std::shared_ptr<PopupBox> PopupButton::popupBox() const {
    return find<PopupBox>(MatchAny{});
}

void PopupButton::onChildAdded(Widget* w) {
    Button::onChildAdded(w);
    auto popupBox = this->popupBox();
    if (!popupBox)
        return;
    popupBox->visible = false;
    bindings->listen(
        Value{ &popupBox->visible }, lifetime() | [this](bool visible) {
            toggleState(WidgetState::ForcePressed, visible);
        });
}

void PopupButton::onEvent(Event& event) {
    auto popupBox = this->popupBox();
    Widget::onEvent(event); // Skip Button::onEvent
    if (event.pressed()) {
        focus();
        auto passedThroughBy = inputQueue()->passedThroughBy.lock();
        if (passedThroughBy != popupBox)
            popupBox->visible = true;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        popupBox->visible = !popupBox->visible;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Escape)) {
        popupBox->visible = false;
        event.stopPropagation();
    }
}

void PopupButton::onRefresh() {}

void PopupButton::close() {
    auto popupBox = this->popupBox();
    if (!popupBox)
        return;
    popupBox->visible = false;
}

PopupButton::PopupButton(Construction construction, ArgumentsView<PopupButton> args)
    : Button(construction, nullptr) {
    args.apply(this);
}

Rc<Widget> PopupButton::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
