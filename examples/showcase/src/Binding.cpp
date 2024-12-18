#include "Binding.hpp"
#include <brisk/gui/Icons.hpp>

namespace Brisk {

RC<Widget> ShowcaseBinding::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,
        new HLayout{
            new Widget{
                layout     = Layout::Vertical,
                alignItems = AlignItems::FlexStart,
                new ToggleButton{ value    = Value{ &m_open }, "Open hidden content"_Text, new Text{ ICON_x },
                                  twoState = true },
                new Widget{
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

        new HLayout{
            new Widget{
                new Widget{
                    gapColumn = 4_apx,
                    new Knob{ value = Value{ &m_value1 }, minimum = 0.f, maximum = 100.f,
                              dimensions = 30_apx },
                    new Slider{ value = Value{ &m_value1 }, minimum = 0.f, maximum = 100.f, width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value bound to Slider::value (<->)"_Text,
        },
        new HLayout{
            new Widget{
                new Widget{
                    gapColumn = 4_apx,
                    new Knob{ value = Value{ &m_value2 }, minimum = 0.f, maximum = 100.f,
                              dimensions = 30_apx },
                    new Slider{ value = Value{ &m_value2 }.readOnly(), minimum = 0.f, maximum = 100.f,
                                width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value controls Slider::value (->)"_Text,
        },
        new HLayout{
            new Widget{
                new Widget{
                    gapColumn = 4_apx,
                    new Knob{ value = Value{ &m_value3 }.readOnly(), minimum = 0.f, maximum = 100.f,
                              dimensions = 30_apx },
                    new Slider{ value = Value{ &m_value3 }, minimum = 0.f, maximum = 100.f, width = 250_apx },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            "Knob::value is controlled by Slider::value (<-)"_Text,
        },

        new HLayout{
            new VLayout{
                new CheckBox{ value = Value{ &m_checkBoxes[0] }, "Monday"_Text },
                new CheckBox{ value = Value{ &m_checkBoxes[1] }, "Tuesday"_Text },
                new CheckBox{ value = Value{ &m_checkBoxes[2] }, "Wednesday"_Text },
                new CheckBox{ value = Value{ &m_checkBoxes[3] }, "Thursday"_Text },
                new CheckBox{ value = Value{ &m_checkBoxes[4] }, "Friday"_Text },
                new Text{
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
    };
}
} // namespace Brisk
