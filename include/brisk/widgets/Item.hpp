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
#pragma once

#include <brisk/gui/Gui.hpp>
#include <brisk/gui/Action.hpp>

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

    explicit Item(const Action& action);

protected:
    std::string m_icon;
    float m_iconAlignY   = 0.5f;
    bool m_checked       = false;
    bool m_checkable     = false;
    bool m_closesPopup   = true;
    bool m_focusOnHover  = false;
    bool m_selectOnFocus = true;
    double m_openTime    = HUGE_VAL;
    double m_closeTime   = HUGE_VAL;

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

    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &Item::m_icon, "icon" },
            /*1*/ Internal::PropField{ &Item::m_checkable, "checkable" },
            /*2*/ Internal::PropFieldNotify{ &Item::m_checked, &Item::onChanged, "checked" },
            /*3*/ Internal::PropField{ &Item::m_closesPopup, "closesPopup" },
            /*4*/ Internal::PropField{ &Item::m_focusOnHover, "focusOnHover" },
            /*5*/ Internal::PropField{ &Item::m_selectOnFocus, "selectOnFocus" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    /**
     * @brief The icon property of the Item.
     *
     * This property represents the icon displayed in the item.
     */
    Property<Item, std::string, 0> icon;

    /**
     * @brief The checkable property of the Item.
     *
     * This property indicates whether the item contains a checkbox and can be checked/unchecked.
     */
    Property<Item, bool, 1> checkable;

    /**
     * @brief The checked property of the Item.
     *
     * This property represents the checkbox state of the item. It triggers the @ref onChanged
     * function when the state changes.
     */
    Property<Item, bool, 2> checked;

    /**
     * @brief The closesPopup property of the Item.
     *
     * If true, clicking the item will close the nearest parent widget that is a popup.
     * This is typically used for menu items.
     */
    Property<Item, bool, 3> closesPopup;

    /**
     * @brief The focusOnHover property of the Item.
     *
     * If true, the widget will take focus on mouse hover, similar to how a menu item behaves.
     */
    Property<Item, bool, 4> focusOnHover;

    Property<Item, bool, 5> selectOnFocus;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::Named<"checked">> checked{};
constexpr inline PropArgument<decltype(Item::checkable)> checkable{};
constexpr inline PropArgument<decltype(Item::closesPopup)> closesPopup{};
constexpr inline PropArgument<decltype(Item::focusOnHover)> focusOnHover{};
constexpr inline PropArgument<decltype(Item::selectOnFocus)> selectOnFocus{};
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
