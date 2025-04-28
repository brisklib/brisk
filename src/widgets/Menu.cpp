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
#include <brisk/widgets/Menu.hpp>
#include <brisk/widgets/Item.hpp>

namespace Brisk {

void Menu::close(Widget* sender) {
    if (auto index = indexOf(sender)) {
        m_onItemClick.trigger(*index);
    }
    visible = false;
}

void Menu::onEvent(Event& event) {
    AutoScrollable::onEvent(event);
    if (auto e = event.as<EventMouseButtonPressed>()) {
        if (!m_rect.contains(e->point)) {
            visible = false;
        } else {
            event.stopPropagation();
        }
    } else if (auto e = event.as<EventMouseButtonReleased>()) {
        if (m_rect.contains(e->point)) {
            event.stopPropagation();
        }
    } else if (event.keyPressed(KeyCode::Escape)) {
        visible = false;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Left)) {
        if (Item* parentItem = dynamicCast<Item*>(parent())) {
            visible = false;
            event.stopPropagation();
            parentItem->focus(true);
        }
    }
}

void Menu::append(RC<Widget> widget) {
    if (Item* it = dynamicCast<Item*>(widget.get())) {
        it->focusOnHover = true;
        Base::append(std::move(widget));
    } else {
        Base::append(rcnew Item{ std::move(widget), focusOnHover = true });
    }
}

Menu::Menu(Construction construction, ArgumentsView<Menu> args)
    : Base(construction, Orientation::Vertical,
           std::tuple{ Arg::placement = Placement::Absolute, Arg::zorder = ZOrder::TopMost,
                       /* Arg::mouseAnywhere = true, */ Arg::layout =
                           Layout::Vertical, /*  Arg::focusCapture = true, */
                       Arg::alignToViewport = AlignToViewport::XY, Arg::tabGroup = true, Arg::visible = false,
                       /*   Arg::absolutePosition = { 0, 0 } */ }) {
    m_isPopup = true;
    args.apply(this);
}

RC<Widget> Menu::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void Menu::onVisible() {
    Base::onVisible();
    m_onBecameVisible.trigger();
}

void Menu::onHidden() {
    Base::onHidden();
    if (visible)
        visible = false;
}
} // namespace Brisk
