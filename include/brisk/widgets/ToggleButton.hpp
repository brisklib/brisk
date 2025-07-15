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

#include "Button.hpp"

namespace Brisk {

class WIDGET ToggleButton : public Button {
    BRISK_DYNAMIC_CLASS(ToggleButton, Button)
public:
    using Base = Button;
    using Base::widgetType;

    template <WidgetArgument... Args>
    explicit ToggleButton(const Args&... args)
        : ToggleButton(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    bool m_twoState                        = false;
    bool m_value                           = false;
    float m_interpolatedValue              = 0.f;
    float m_interpolatedValueTransitionOn  = 0.15f;
    float m_interpolatedValueTransitionOff = 0.15f;
    Internal::Transition2<float> m_interpolatedValueTransition{ m_interpolatedValue };
    void onAnimationFrame() override;
    void onClicked() override;
    void updateState();
    virtual void onChanged();
    Ptr cloneThis() const override;
    void onConstructed() override;
    explicit ToggleButton(Construction construction, ArgumentsView<ToggleButton> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &ToggleButton::m_value, &ToggleButton::onChanged, "value" },
            /*1*/ Internal::PropField{ &ToggleButton::m_twoState, "twoState" },
            /*2*/ Internal::PropField{ &ToggleButton::m_interpolatedValue, "interpolatedValue" },
            /*3*/
            Internal::PropField{ &ToggleButton::m_interpolatedValueTransitionOn,
                                 "interpolatedValueTransitionOn" },
            /*4*/
            Internal::PropField{ &ToggleButton::m_interpolatedValueTransitionOff,
                                 "interpolatedValueTransitionOff" }
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ToggleButton, bool, 0> value;
    Property<ToggleButton, bool, 1> twoState;
    Property<ToggleButton, const float, 2> interpolatedValue;
    Property<ToggleButton, float, 3> interpolatedValueTransitionOn;
    Property<ToggleButton, float, 4> interpolatedValueTransitionOff;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(ToggleButton* target, ArgVal<Tag::Named<"value">, T> value) {
    target->value = value.value;
}

inline namespace Arg {
#ifndef BRISK__VALUE_ARG_DEFINED
#define BRISK__VALUE_ARG_DEFINED
constexpr inline Argument<Tag::Named<"value">> value{};
#endif
constexpr inline PropArgument<decltype(ToggleButton::twoState)> twoState{};
} // namespace Arg

} // namespace Brisk
