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

#include "Widgets.hpp"
#include "ToggleButton.hpp"

namespace Brisk {

class WIDGET TabButton : public ToggleButton {
    BRISK_DYNAMIC_CLASS(TabButton, ToggleButton)
public:
    using Base                                   = ToggleButton;
    constexpr static std::string_view widgetType = "tabbutton";

    template <WidgetArgument... Args>
    explicit TabButton(const Args&... args) : TabButton(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit TabButton(Construction construction, ArgumentsView<TabButton> args);
};

class Pages;
class Page;

class WIDGET Tabs : public Widget {
    BRISK_DYNAMIC_CLASS(Tabs, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "tabs";

    template <WidgetArgument... Args>
    explicit Tabs(const Args&... args) : Tabs{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

protected:
    friend class Pages;
    virtual void clearTabs();
    virtual void createTab(Value<bool> visible, Page* page);
    Ptr cloneThis() const override;
    explicit Tabs(Construction construction, ArgumentsView<Tabs> args);
};

class WIDGET Page : public Widget {
    BRISK_DYNAMIC_CLASS(Page, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "page";

    template <WidgetArgument... Args>
    explicit Page(std::string title, const Args&... args)
        : Page(Construction{ widgetType }, std::move(title), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    std::string m_title;
    friend class Pages;
    Ptr cloneThis() const override;

    explicit Page(Construction construction, std::string title, ArgumentsView<Page> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &Page::m_title, "title" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Page, std::string, 0> title;
    BRISK_PROPERTIES_END
};

class WIDGET Pages : public Widget {
    BRISK_DYNAMIC_CLASS(Pages, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "pages";

    enum {
        Horizontal = -1,
        Vertical   = -2,
    };

    template <WidgetArgument... Args>
    explicit Pages(const Args&... args) : Pages(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    constexpr static WidgetRole<Tabs, "tabs"> tabs{};

protected:
    int m_value = 0;
    void updateTabs();
    void childrenChanged() override;

    void onConstructed() override;

    virtual void onChanged();

    Ptr cloneThis() const override;

    explicit Pages(Construction construction, ArgumentsView<Pages> args);

private:
    void internalChanged();

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &Pages::m_value, &Pages::onChanged, "value" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Pages, int, 0> value;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(Pages* target, ArgVal<Tag::Named<"value">, T> value) {
    target->value = value.value;
}

inline namespace Arg {
#ifndef BRISK__VALUE_ARG_DEFINED
#define BRISK__VALUE_ARG_DEFINED
constexpr inline Argument<Tag::Named<"value">> value{};
#endif
} // namespace Arg

} // namespace Brisk
