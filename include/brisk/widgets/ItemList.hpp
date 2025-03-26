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

#include "AutoScrollable.hpp"

namespace Brisk {

class WIDGET ItemList : public AutoScrollable {
    BRISK_DYNAMIC_CLASS(ItemList, AutoScrollable)
public:
    using Base                                   = AutoScrollable;
    constexpr static std::string_view widgetType = "itemlist";
    using Widget::apply;

    template <WidgetArgument... Args>
    explicit ItemList(const Args&... args) : ItemList(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Trigger<size_t> m_onItemClick;
    Trigger<> m_onBecameVisible;
    void onEvent(Event& event) override;
    void append(RC<Widget> widget) override;
    Ptr cloneThis() const override;
    void close(Widget* sender) override;
    void onVisible() override;

    explicit ItemList(Construction construction, ArgumentsView<ItemList> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<ItemList, Trigger<size_t>, &ItemList::m_onItemClick> onItemClick;
    Property<ItemList, Trigger<>, &ItemList::m_onBecameVisible> onBecameVisible;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::PropArg<decltype(ItemList::onItemClick)>> onItemClick{};
}
} // namespace Brisk
