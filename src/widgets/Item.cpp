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
#include "brisk/widgets/Spacer.hpp"
#include <brisk/widgets/Item.hpp>
#include <brisk/widgets/Menu.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/core/Localization.hpp>

namespace Brisk {

void Item::postPaint(Canvas& canvas) const {
    Base::postPaint(canvas);
    std::string icon = m_checked ? ICON_check : m_icon;
    if (!icon.empty()) {
        canvas.setFillColor(m_color.current);
        canvas.setFont(font()(dp(FontSize::Normal)));
        canvas.fillText(
            icon, m_rect.alignedRect({ m_clientRect.x1 - m_rect.x1, m_rect.height() }, { 0.f, m_iconAlignY }),
            PointF(0.5f, 0.5f));
    }
    if (!isTopMenu() && this->find<Menu>()) {
        canvas.setFillColor(m_color.current);
        canvas.setFont(font()(dp(FontSize::Normal)));
        canvas.fillText(ICON_chevron_right,
                        m_rect.alignedRect({ m_clientRect.x1 - m_rect.x1, m_rect.height() }, { 1.f, 0.5f }),
                        PointF(0.5f, 0.5f));
    }
}

bool Item::isTopMenu() const {
    return m_parent && m_parent->role.get() == "mainmenu";
}

void Item::onChildAdded(Widget* w) {
    Base::onChildAdded(w);
    if (isOf<Menu>(w)) {
        addClass("nested");
    }
}

void Item::onEvent(Event& event) {
    Base::onEvent(event);
    if (isDisabled())
        return;
    if (event.pressed(m_rect)) {
        if (!isFocused())
            focus();
        event.stopPropagation();
    }
    if (event.released(m_rect) || event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        if (m_checkable) {
            checked = !checked;
        }
        onClicked();
        m_onClick.trigger();
        if (auto sublist = this->find<Menu>()) {
            if (isTopMenu()) {
                if (sublist->visible)
                    sublist->visible = false;
                else
                    sublist->visible = true;
            } else {
                sublist->visible = true;
                if (event.as<EventKeyPressed>().has_value())
                    sublist->focus(true);
            }
        } else if (m_closesPopup) {
            closeMenuChain();
        }

        event.stopPropagation();
    }
    if (!isTopMenu()) {
        if (auto sublist = this->find<Menu>()) {
            if (event.as<EventMouseMoved>()) {
                if (std::isinf(m_openTime)) {
                    m_openTime = currentTime() + 0.3;
                }
                m_closeTime = HUGE_VAL;
            }
            if (event.keyPressed(KeyCode::Right)) {
                sublist->visible = true;
                sublist->focus(true);
                event.stopPropagation();
            }
        }
    }

    if (m_focusOnHover) {
        if (auto e = event.as<EventMouseMoved>()) {
            selected = true;
            focus();
        } else if (auto e = event.as<EventMouseExited>()) {
            selected = false;
        }
    }
    if (m_selectOnFocus) {
        if (event.focused()) {
            selected = true;
        } else if (event.blurred()) {
            selected = false;
        }
    }
}

Rc<Widget> Item::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Item::Item(Construction construction, ArgumentsView<Item> args)
    : Base(construction, std::tuple{ Arg::tabStop = true }) {
    m_processClicks = false;
    args.apply(this);
}

Item::Item(const Action& action)
    : Item(Construction{ widgetType }, std::tuple{
                                           rcnew Text{ locale->translate(action.caption) },
                                           Arg::icon    = action.icon,
                                           Arg::onClick = action.callback,
                                       }) {
    endConstruction();
    if (action.shortcut != Shortcut{}) {
        apply(rcnew Spacer{});
        apply(rcnew ShortcutHint{ action.shortcut });
    }
}

void Item::onChanged() {}

void Item::onClicked() {}

void Item::onHidden() {
    toggleState(WidgetState::Selected, false);
}

void Item::onRefresh() {
    Base::onRefresh();
    if (!isVisible()) {
        return;
    }
    if (currentTime() > m_openTime) {
        if (auto sublist = this->find<Menu>()) {
            sublist->visible = true;
            m_openTime       = HUGE_VAL;
        }
    }
    if (std::isinf(m_closeTime)) {
        if (auto sublist = this->find<Menu>(); sublist && sublist->visible) {
            if (auto inputQueue = this->inputQueue()) {
                if (auto focused = inputQueue->focused.lock()) {
                    if (!focused->hasParent(this, true)) {
                        m_closeTime = currentTime() + 0.3;
                    }
                }
            }
        }
    }
    if (currentTime() > m_closeTime) {
        if (auto sublist = this->find<Menu>()) {
            sublist->visible = false;
            m_closeTime      = HUGE_VAL;
        }
    }
}
} // namespace Brisk
