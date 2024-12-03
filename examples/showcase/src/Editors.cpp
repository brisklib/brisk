#include "Editors.hpp"
#include "brisk/gui/Icons.hpp"
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Slider.hpp>
#include <brisk/widgets/Knob.hpp>
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/SpinBox.hpp>
#include <brisk/widgets/Color.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

RC<Widget> ShowcaseEditors::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        new Text{ "Slider (widgets/Slider.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new Slider{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, width = 250_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        new HLayout{
            new Widget{
                new Slider{ value = Value{ &m_value }, hintFormatter = "x={:.1f}", minimum = 0.f,
                            maximum = 100.f, width = 250_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Value with custom hint" },
        },

        new HLayout{
            new Widget{
                new Slider{ value = Value{ &m_y }, hintFormatter = "y={:.1f}", minimum = 0.f, maximum = 100.f,
                            width = 250_apx, dimensions = { 20_apx, 80_apx } },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        new Text{ "Knob (widgets/Knob.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new Knob{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, dimensions = 30_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        new Text{ "SpinBox (widgets/SpinBox.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new SpinBox{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, width = 90_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        new Text{ "TextEditor (widgets/TextEditor.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new TextEditor(Value{ &m_text }, fontSize = 150_perc, width = 100_perc),
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{
                text = Value{ &m_text }.transform([](std::string s) {
                    return fmt::format("Text: \"{}\"", s);
                }),
            },
        },

        new Text{ "multiline = true", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new TextEditor(Value{ &m_multilineText }, fontSize = 150_perc, height = 5_em,
                               multiline = true, textVerticalAlign = TextAlign::Start, width = 100_perc),
            },
        },

        new Text{ "PasswordEditor (widgets/TextEditor.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new PasswordEditor(Value{ &m_password }, width = 100_perc, fontFamily = Monospace,
                                   passwordChar = Value{ &m_hidePassword }.transform([](bool v) -> char32_t {
                                       return v ? '*' : 0;
                                   })),
                &m_group,
            },
            gapColumn = 10_apx,
            new CheckBox{ value = Value{ &m_hidePassword }, new Text{ "Hide password" } },
        },

        new Text{ "ColorView (widgets/Color.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new ColorView{ Palette::Standard::indigo },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        new Text{ "ColorSliders (widgets/Color.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new ColorSliders{ Value{ &m_color }, false },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        new Text{ "ColorPalette (widgets/Color.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new ColorPalette{ Value{ &m_color } },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        new Text{ "ColorButton (widgets/Color.hpp)", classes = { "section-header" } },

        new HLayout{
            new Widget{
                new ColorButton{ Value{ &m_color }, false },
                &m_group,
            },
            gapColumn = 10_apx,
        },
    };
}
} // namespace Brisk
