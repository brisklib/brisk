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

namespace Brisk {

class WIDGET Menu : public Widget {
    BRISK_DYNAMIC_CLASS(Menu, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "menu";
    using Widget::apply;

    template <WidgetArgument... Args>
    explicit Menu(const Args&... args) : Menu(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Trigger<size_t> m_onItemClick;
    Trigger<> m_onBecameVisible;
    void onEvent(Event& event) override;
    void append(Rc<Widget> widget) override;
    Ptr cloneThis() const override;
    void close(Widget* sender) override;
    void onVisible() override;
    void onHidden() override;

    explicit Menu(Construction construction, ArgumentsView<Menu> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropField{ &Menu::m_onItemClick, "onItemClick" },
            /*1*/ Internal::PropField{ &Menu::m_onBecameVisible, "onBecameVisible" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Menu, Trigger<size_t>, 0> onItemClick;
    Property<Menu, Trigger<>, 1> onBecameVisible;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline PropArgument<decltype(Menu::onItemClick)> onItemClick{};
}

} // namespace Brisk
