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
#pragma once

#include "ValueWidget.hpp"
#include "Item.hpp"

namespace Brisk {

class WIDGET ListBox : public ValueWidget {
    BRISK_DYNAMIC_CLASS(ListBox, ValueWidget)
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "listbox";

    template <WidgetArgument... Args>
    explicit ListBox(const Args&... args) : ListBox(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    void onEvent(Event& event) override;
    void onChanged() override;
    std::shared_ptr<Item> findSelected() const;
    void append(Rc<Widget> widget) override;
    Ptr cloneThis() const override;
    explicit ListBox(Construction construction, ArgumentsView<ListBox> args);
};

} // namespace Brisk
