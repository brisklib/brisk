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
#include <brisk/widgets/ListBox.hpp>

namespace Brisk {

void ListBox::onEvent(Event& event) {
    Base::onEvent(event);
}

void ListBox::onChanged() {
    if (auto selected = findSelected()) {
        selected->isSelected();
    }
}

std::shared_ptr<Item> ListBox::findSelected() const {
    if (!m_constructed)
        return nullptr;
    auto& widgets = this->widgets();
    int value     = std::round(m_value);
    if (value < 0 || value >= widgets.size())
        return nullptr;
    return std::dynamic_pointer_cast<Item>(widgets[value]);
}

void ListBox::append(Widget::Ptr widget) {
    if (dynamic_cast<Item*>(widget.get()))
        Base::append(std::move(widget));
    else
        Base::append(rcnew Item{ std::move(widget) });
}

Widget::Ptr ListBox::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

ListBox::ListBox(Construction construction, ArgumentsView<ListBox> args) : Base(construction, nullptr) {
    m_tabStop  = true;
    m_layout   = Layout::Vertical;
    m_tabGroup = true;
    args.apply(this);
}

} // namespace Brisk
