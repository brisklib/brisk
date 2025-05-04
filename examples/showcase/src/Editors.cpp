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
#include "Editors.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Slider.hpp>
#include <brisk/widgets/Knob.hpp>
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/SpinBox.hpp>
#include <brisk/widgets/Color.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

Rc<Widget> ShowcaseEditors::build(Rc<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        rcnew Text{ "Slider (widgets/Slider.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew Slider{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, width = 250_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        rcnew HLayout{
            rcnew Widget{
                rcnew Slider{ value = Value{ &m_value }, hintFormatter = "x={:.1f}", minimum = 0.f,
                              maximum = 100.f, width = 250_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Value with custom hint" },
        },

        rcnew HLayout{
            rcnew Widget{
                rcnew Slider{ value = Value{ &m_y }, hintFormatter = "y={:.1f}", minimum = 0.f,
                              maximum = 100.f, width = 250_apx, dimensions = { 20_apx, 80_apx } },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        rcnew Text{ "Knob (widgets/Knob.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew Knob{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, dimensions = 30_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        rcnew Text{ "SpinBox (widgets/SpinBox.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew SpinBox{ value = Value{ &m_value }, minimum = 0.f, maximum = 100.f, width = 90_apx },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text = Value{ &m_value }.transform([](float v) {
                    return fmt::format("Value: {:.1f}", v);
                }),
            },
        },

        rcnew Text{ "TextEditor (widgets/TextEditor.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew TextEditor(Value{ &m_text }, fontSize = 150_perc, width = 100_perc),
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text = Value{ &m_text }.transform([](std::string s) {
                    return fmt::format("Text: \"{}\"", s);
                }),
            },
        },

        rcnew Text{ "multiline = true", classes = { "section-header" } },

        rcnew TextEditor(Value{ &m_multilineText }, fontSize = 150_perc, height = 5_em, multiline = true,
                         textVerticalAlign = TextAlign::Start, width = auto_),

        rcnew Text{ "PasswordEditor (widgets/TextEditor.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew PasswordEditor(Value{ &m_password }, width = 100_perc, fontFamily = Font::Monospace,
                                     passwordChar =
                                         Value{ &m_hidePassword }.transform([](bool v) -> char32_t {
                                             return v ? '*' : 0;
                                         })),
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew CheckBox{ value = Value{ &m_hidePassword }, rcnew Text{ "Hide password" } },
        },

        rcnew Text{ "Basic HTML", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew TextEditor(Value{ &m_html }, fontSize = 150_perc, width = 100_perc),
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text        = Value{ &m_html },
                textOptions = TextOptions::Html,
            },
        },

        rcnew Text{ "ColorView (widgets/Color.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew ColorView{ Palette::Standard::indigo },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        rcnew Text{ "ColorSliders (widgets/Color.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew ColorSliders{ Value{ &m_color }, false },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        rcnew Text{ "ColorPalette (widgets/Color.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew ColorPalette{ Value{ &m_color } },
                &m_group,
            },
            gapColumn = 10_apx,
        },

        rcnew Text{ "ColorButton (widgets/Color.hpp)", classes = { "section-header" } },

        rcnew HLayout{
            rcnew Widget{
                rcnew ColorButton{ Value{ &m_color }, false },
                &m_group,
            },
            gapColumn = 10_apx,
        },
    };
}
} // namespace Brisk
