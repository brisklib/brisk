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
}

void PopupButton::onClicked() {
    auto popupBox = this->popupBox();
    if (!popupBox)
        return;
    if (inputQueue->passedThroughBy.lock() != popupBox)
        popupBox->visible = true;
}

void PopupButton::onRefresh() {
    auto popupBox = this->popupBox();
    toggleState(WidgetState::Selected, popupBox && popupBox->visible.get());
}

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

RC<Widget> PopupButton::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
