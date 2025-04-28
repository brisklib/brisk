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

#include <brisk/gui/GUI.hpp>

namespace Brisk {

/**
 * @class Item
 * @brief Widget that represents a list item or a menu item.
 *
 * This class encapsulates the behavior of an item widget, which can have an icon,
 * be checkable, and interact with various user events.
 */
class WIDGET Item : public Widget {
    BRISK_DYNAMIC_CLASS(Item, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "item";

    /**
     * @brief Constructor for the Item class.
     *
     * This constructor initializes the Item widget with the given arguments, including sub-widgets.
     *
     * @tparam Args The types of the arguments passed to the constructor.
     * @param args The arguments used for constructing the Item widget.
     */
    template <WidgetArgument... Args>
    explicit Item(const Args&... args) : Item(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    std::string m_icon;
    float m_iconAlignY  = 0.5f;
    bool m_checked      = false;
    bool m_checkable    = false;
    bool m_closesPopup  = true;
    bool m_focusOnHover = false;
    double m_openTime   = HUGE_VAL;
    double m_closeTime  = HUGE_VAL;
    
    bool isTopMenu() const;
    void postPaint(Canvas& canvas) const override;
    void onEvent(Event& event) override;
    virtual void onClicked();
    virtual void onChanged();
    void onHidden() override;
    void onRefresh() override;
    Ptr cloneThis() const override;
    void onChildAdded(Widget* w) override;
    explicit Item(Construction construction, ArgumentsView<Item> args);

public:
    BRISK_PROPERTIES_BEGIN
    /**
     * @brief The icon property of the Item.
     *
     * This property represents the icon displayed in the item.
     */
    Property<Item, std::string, &Item::m_icon> icon;

    /**
     * @brief The checkable property of the Item.
     *
     * This property indicates whether the item contains a checkbox and can be checked/unchecked.
     */
    Property<Item, bool, &Item::m_checkable> checkable;

    /**
     * @brief The checked property of the Item.
     *
     * This property represents the checkbox state of the item. It triggers the @ref onChanged
     * function when the state changes.
     */
    Property<Item, bool, &Item::m_checked, nullptr, nullptr, &Item::onChanged> checked;

    /**
     * @brief The closesPopup property of the Item.
     *
     * If true, clicking the item will close the nearest parent widget that is a popup.
     * This is typically used for menu items.
     */
    Property<Item, bool, &Item::m_closesPopup> closesPopup;

    /**
     * @brief The focusOnHover property of the Item.
     *
     * If true, the widget will take focus on mouse hover, similar to how a menu item behaves.
     */
    Property<Item, bool, &Item::m_focusOnHover> focusOnHover;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::Named<"checked">> checked{};
constexpr inline Argument<Tag::PropArg<decltype(Item::checkable)>> checkable{};
constexpr inline Argument<Tag::PropArg<decltype(Item::closesPopup)>> closesPopup{};
constexpr inline Argument<Tag::PropArg<decltype(Item::focusOnHover)>> focusOnHover{};
constexpr inline Argument<Tag::Named<"icon">> icon{};
} // namespace Arg

template <typename T>
void applier(Item* target, ArgVal<Tag::Named<"checked">, T> value) {
    target->checked = value.value;
}

template <typename T>
void applier(Item* target, ArgVal<Tag::Named<"icon">, T> value) {
    target->icon = value.value;
}

} // namespace Brisk
