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

#include "Widgets.hpp"
#include "Text.hpp"

namespace Brisk {

class WIDGET UpDownButtons : public Widget {
    BRISK_DYNAMIC_CLASS(UpDownButtons, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "updownbuttons";

    template <WidgetArgument... Args>
    explicit UpDownButtons(const Args&... args)
        : UpDownButtons(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    constexpr static WidgetRole<Button, "up"> up{};
    constexpr static WidgetRole<Button, "down"> down{};

protected:
    void onConstructed() override;
    void onChildAdded(Widget* w) override;
    Ptr cloneThis() const override;
    explicit UpDownButtons(Construction construction, ArgumentsView<UpDownButtons> args);
};

class WIDGET SpinBox : public ValueWidget {
    BRISK_DYNAMIC_CLASS(SpinBox, ValueWidget)
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "spinbox";

    template <WidgetArgument... Args>
    explicit SpinBox(const Args&... args) : SpinBox(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    constexpr static WidgetRole<Text, "display"> display{};
    constexpr static WidgetRole<UpDownButtons, "buttons"> buttons{};

protected:
    ValueFormatter m_valueFormatter;

    void onConstructed() override;
    void onChildAdded(Widget* w) override;
    void onEvent(Event& event) override;
    Ptr cloneThis() const override;
    explicit SpinBox(Construction construction, ArgumentsView<SpinBox> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<SpinBox, ValueFormatter, &SpinBox::m_valueFormatter> valueFormatter;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::PropArg<decltype(SpinBox::valueFormatter)>> valueFormatter{};
}

} // namespace Brisk
