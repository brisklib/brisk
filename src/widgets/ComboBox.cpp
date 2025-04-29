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
#include <brisk/widgets/ComboBox.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/gui/Styles.hpp>

namespace Brisk {

void ComboBox::onChildAdded(Widget* w) {
    Base::onChildAdded(w);

    if (Menu* itemlist = this->itemlist.matchesType(w)) {
        itemlist->onItemClick = lifetime() | [this](size_t index) BRISK_INLINE_LAMBDA {
            value = index;
        };
        itemlist->setMenuRoot();

        itemlist->onBecameVisible = lifetime() | [this]() BRISK_INLINE_LAMBDA {
            if (auto item = findSelected()) {
                item->focus();
            }
        };
        itemlist->absolutePosition = { 0, 100_perc };
        itemlist->anchor           = { 0_px, 0_px };
        bindings->listen(
            Value{ &itemlist->visible }, lifetime() | [this](bool visible) {
                toggleState(WidgetState::ForcePressed, visible);
            });
    }
    if (Item* selectedItem = this->selecteditem.matchesType(w)) {
        selectedItem->flexGrow         = 1;
        selectedItem->tabStop          = false;
        selectedItem->mouseInteraction = MouseInteraction::Disable;
    }
    if (ToggleButton* unroll = this->unroll.matchesType(w)) {
        unroll->tabStop          = false;
        unroll->mouseInteraction = MouseInteraction::Disable;
        unroll->borderWidth      = 0;
        unroll->flexGrow         = 0;
    }
}

void ComboBox::onConstructed() {
    m_layout = Layout::Horizontal;

    itemlist.create(this);
    selecteditem.create(this);

    if (!unroll.get(this)) {
        apply(rcnew ToggleButton{
            Arg::value = Value{ &itemlist.get(this)->visible },
            rcnew Text{ ICON_chevron_down },
            rcnew Text{ ICON_chevron_up },
            Arg::role     = unroll.role(),
            Arg::twoState = true,
        });
    }
    Base::onConstructed();
}

std::shared_ptr<Item> ComboBox::findSelected() const {
    if (!m_constructed)
        return nullptr;
    auto menu     = this->itemlist.get(this);
    auto& widgets = menu->widgets();
    int value     = std::round(m_value);
    if (value < 0 || value >= widgets.size())
        return nullptr;
    return dynamicPointerCast<Item>(widgets[value]);
}

void ComboBox::onEvent(Event& event) {
    auto menu = this->itemlist.get(this);
    Base::onEvent(event);
    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        int val = std::clamp(int(m_value - delta), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    } else if (event.pressed()) {
        focus();
        auto passedThroughBy = inputQueue()->passedThroughBy.lock();
        if (passedThroughBy != menu)
            menu->visible = true;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        menu->visible = !menu->visible;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Escape)) {
        menu->visible = false;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Up)) {
        int val = std::clamp(int(m_value - 1), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Down)) {
        int val = std::clamp(int(m_value + 1), 0, int(menu->widgets().size() - 1));
        value   = val;
        event.stopPropagation();
    }
}

void ComboBox::onChanged() {
    if (!m_constructed)
        return;
    auto w = findSelected();
    std::shared_ptr<Item> cloned;
    if (w) {
        cloned = dynamicPointerCast<Item>(w->clone());
    } else {
        cloned = std::shared_ptr<Item>(new Item{});
    }
    cloned->role     = selecteditem.role();
    cloned->flexGrow = 1;
    cloned->tabStop  = false;
    bool replaced    = replace(selecteditem.get(this), cloned, false);
    BRISK_ASSERT(replaced);
}

Rc<Widget> ComboBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ComboBox::ComboBox(Construction construction, ArgumentsView<ComboBox> args)
    : Base(construction, nullptr) {
    m_tabStop       = true;
    m_processClicks = false;
    args.apply(this);
}
} // namespace Brisk
