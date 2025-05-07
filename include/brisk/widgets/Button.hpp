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

enum class ButtonClickEvent {
    MouseDown,
    MouseUp,
};

enum class ButtonKeyEvents {
    None         = 0,
    AcceptsEnter = 1,
    AcceptsSpace = 2,
};

template <>
constexpr inline bool isBitFlags<ButtonKeyEvents> = true;

class WIDGET Button : public Widget {
    BRISK_DYNAMIC_CLASS(Button, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "button";

    template <WidgetArgument... Args>
    explicit Button(const Args&... args) : Button{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

protected:
    double m_repeatDelay          = std::numeric_limits<double>::infinity();
    double m_repeatInterval       = std::numeric_limits<double>::infinity();
    ButtonClickEvent m_clickEvent = ButtonClickEvent::MouseUp;
    ButtonKeyEvents m_keyEvents   = ButtonKeyEvents::AcceptsEnter | ButtonKeyEvents::AcceptsSpace;

    struct RepeatState {
        double startTime;
        int repeats;
    };

    std::optional<RepeatState> m_repeatState;
    void onEvent(Event& event) override;
    void onRefresh() override;
    virtual void onClicked();
    void doClick();
    Ptr cloneThis() const override;
    explicit Button(Construction construction, ArgumentsView<Button> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<Button, double, &Button::m_repeatDelay> repeatDelay;
    Property<Button, double, &Button::m_repeatInterval> repeatInterval;
    Property<Button, ButtonClickEvent, &Button::m_clickEvent> clickEvent;
    Property<Button, ButtonKeyEvents, &Button::m_keyEvents> keyEvents;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::PropArg<decltype(Button::repeatDelay)>> repeatDelay{};
constexpr inline Argument<Tag::PropArg<decltype(Button::repeatInterval)>> repeatInterval{};
constexpr inline Argument<Tag::PropArg<decltype(Button::clickEvent)>> clickEvent{};
constexpr inline Argument<Tag::PropArg<decltype(Button::keyEvents)>> keyEvents{};
} // namespace Arg

} // namespace Brisk
