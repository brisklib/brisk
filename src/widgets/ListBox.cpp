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
#include <brisk/widgets/ListBox.hpp>

namespace Brisk {

void ListBox::onEvent(Event& event) {
    Base::onEvent(event);
    if (event.pressed()) {
        value = -1;
    }
}

void ListBox::onChanged() {}

void ListBox::childrenChanged() {
    Base::childrenChanged();
    if (m_value > widgets().size()) {
        value = -1;
    }
}

std::shared_ptr<Item> ListBox::findSelected() const {
    if (!m_constructed)
        return nullptr;
    auto& widgets = this->widgets();
    int value     = std::round(m_value);
    if (value < 0 || value >= widgets.size())
        return nullptr;
    return dynamicPointerCast<Item>(widgets[value]);
}

void ListBox::append(Rc<Widget> widget) {
    auto item = dynamicPointerCast<Item>(widget);
    if (!item) {
        item = rcnew Item{ std::move(widget) };
    }
    item->closesPopup   = false;
    item->tabStop       = true;
    item->selectOnFocus = true;
    bindings->connectBidir(Value{ &item->selected }, Value{ &value }.equal(this->widgets().size()),
                           BindType::Immediate);
    Base::append(std::move(item));
}

Rc<Widget> ListBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ListBox::ListBox(Construction construction, ArgumentsView<ListBox> args)
    : Base(construction, nullptr) {
    m_layout   = Layout::Vertical;
    m_tabGroup = true;
    args.apply(this);
}

} // namespace Brisk
