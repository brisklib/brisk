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
#include "Binding.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

void ShowcaseBinding::onTimer() {
    if (m_buttonPtr) {
        static int n = 0;
        if (++n % 10 == 0)
            m_buttonPtr->apply(Graphene::mainColor = Palette::Standard::index(n / 10));
    }
}

ShowcaseBinding::ShowcaseBinding() {
    bindings->listen(Value{ &frameStartTime }, BindableCallback<>(this, &ShowcaseBinding::onTimer));
}

Rc<Widget> ShowcaseBinding::build(Rc<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,
        rcnew HLayout{
            rcnew Widget{
                layout     = Layout::Vertical,
                alignItems = AlignItems::FlexStart,
                rcnew ToggleButton{ value = Value{ &m_open }, "Open hidden content"_Text,
                                    rcnew Text{ ICON_x }, twoState = true },
                rcnew Widget{
                    visible         = Value{ &m_open },
                    padding         = 16_apx,
                    margin          = 1_apx,
                    backgroundColor = 0x808080'40_rgba,
                    "Hidden content"_Text,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "ToggleButton::value controls Widget::visible"_Text,
        },

        rcnew HLayout{
            rcnew Widget{
                rcnew Widget{
                    gapColumn = 4_apx,
                    rcnew Knob{ value = Value{ &m_value1 }, minimum = 0.f, maximum = 100.f,
                                dimensions = 30_apx },
                    rcnew Slider{ value = Value{ &m_value1 }, minimum = 0.f, maximum = 100.f,
                                  width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value bound to Slider::value (<->)"_Text,
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Widget{
                    gapColumn = 4_apx,
                    rcnew Knob{ value = Value{ &m_value2 }, minimum = 0.f, maximum = 100.f,
                                dimensions = 30_apx },
                    rcnew Slider{ value = Value{ &m_value2 }.readOnly(), minimum = 0.f, maximum = 100.f,
                                  width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value controls Slider::value (->)"_Text,
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Widget{
                    gapColumn = 4_apx,
                    rcnew Knob{ value = Value{ &m_value3 }.readOnly(), minimum = 0.f, maximum = 100.f,
                                dimensions = 30_apx },
                    rcnew Slider{ value = Value{ &m_value3 }, minimum = 0.f, maximum = 100.f,
                                  width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value is controlled by Slider::value (<-)"_Text,
        },

        rcnew HLayout{
            rcnew VLayout{
                rcnew CheckBox{ value = Value{ &m_checkBoxes[0] }, "Monday"_Text },
                rcnew CheckBox{ value = Value{ &m_checkBoxes[1] }, "Tuesday"_Text },
                rcnew CheckBox{ value = Value{ &m_checkBoxes[2] }, "Wednesday"_Text },
                rcnew CheckBox{ value = Value{ &m_checkBoxes[3] }, "Thursday"_Text },
                rcnew CheckBox{ value = Value{ &m_checkBoxes[4] }, "Friday"_Text },
                rcnew Text{
                    "Select two weekdays",
                    visible = transform(
                        [](bool mon, bool tue, bool wed, bool thu, bool fri) {
                            return mon + tue + wed + thu + fri != 2;
                        },
                        Value{ &m_checkBoxes[0] }, Value{ &m_checkBoxes[1] }, Value{ &m_checkBoxes[2] },
                        Value{ &m_checkBoxes[3] }, Value{ &m_checkBoxes[4] }),
                    color = Palette::red,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Text::visible is bound to the number of selected checkboxes"_Text,
        },

        rcnew HLayout{
            rcnew Button{
                rcnew Text{ "Button with color changed from code" },
                storeWidget(&m_buttonPtr),
            },
        },
    };
}
} // namespace Brisk
