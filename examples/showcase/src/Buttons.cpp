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
#include "Buttons.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/ToggleButton.hpp>
#include <brisk/widgets/ImageView.hpp>
#include <brisk/widgets/Viewport.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

Rc<Widget> ShowcaseButtons::build(Rc<Notifications> notifications, Value<bool> globalEnabled) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,
        rcnew Text{ "Button (widgets/Button.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew Button{
                    rcnew Text{ "Button 1" },
                    enabled = globalEnabled,
                    onClick = lifetime() |
                              [notifications]() {
                                  notifications->show(rcnew Text{ "Button 1 clicked" });
                              },
                },
                rcnew Button{
                    rcnew Text{ "Disabled Button" },
                    enabled = false,
                    onClick = lifetime() |
                              [notifications]() {
                                  notifications->show(rcnew Text{ "Disabled Button clicked" });
                              },
                },
                &m_group,
            },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Button{
                    rcnew Text{ ICON_settings "  Button with icon" },
                    enabled = globalEnabled,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Icon from icon font" },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Button{
                    rcnew SvgImageView{
                        R"SVG(<svg viewBox="0 -34 1092 1092" class="icon" xmlns="http://www.w3.org/2000/svg">
  <path d="m307 7-17 13a39 39 0 1 1-62 46L14 229l1044 243z" fill="#FCE875"/>
  <path d="M1092 486 0 232 230 58l3 5a33 33 0 1 0 52-39l-4-5 25-19 4 2zM28 226l996 232L307 14l-9 7a45 45 0 0 1-71 54z" fill="#541018"/>
  <path d="M1019 652a88 88 0 0 1 66-85v-78L8 238v378a72 72 0 0 1 0 144v49l1077 208V738a88 88 0 0 1-66-86z" fill="#FFC232"/>
  <path d="M1091 1024 2 814v-60h6a66 66 0 0 0 0-132H2V230l1089 254v88l-5 1a82 82 0 0 0 0 159l5 1zM14 804l1065 206V742a94 94 0 0 1 0-179v-69L14 246v365a78 78 0 0 1 0 154z" fill="#541018"/>
  <path d="M197 473a66 55 90 1 0 110 0 66 55 90 1 0-110 0Z" fill="#F9E769"/>
  <path d="M252 545c-34 0-61-32-61-72s27-71 61-71 61 32 61 71-28 72-61 72zm0-131c-27 0-49 26-49 59s22 60 49 60 49-27 49-60-22-59-49-59z" fill="#541018"/>
  <path d="M469 206a40 32 0 1 0 79 0 40 32 0 1 0-79 0Z" fill="#F2B42C"/>
  <path d="M509 244c-26 0-46-17-46-38s20-38 46-38 45 17 45 38-20 38-45 38zm0-64c-19 0-34 11-34 26s15 26 34 26 33-12 33-26-15-26-33-26z" fill="#541018"/>
  <path d="M109 199a41 32 0 1 0 82 0 41 32 0 1 0-82 0Z" fill="#F2B42C"/>
  <path d="M150 237c-26 0-47-17-47-38s21-37 47-37 47 17 47 37-21 38-47 38zm0-63c-19 0-35 11-35 25s16 26 35 26 35-11 35-26-15-25-35-25z" fill="#541018"/>
  <path d="M932 925a41 41 0 1 0 82 0 41 41 0 1 0-82 0Z" fill="#FFE600"/>
  <path d="M973 972a47 47 0 1 1 47-47 47 47 0 0 1-47 47zm0-83a35 35 0 1 0 35 36 35 35 0 0 0-35-36z" fill="#541018"/>
  <path d="M807 481a58 52 0 1 0 115 0 58 52 0 1 0-115 0Z" fill="#FFE600"/>
  <path d="M865 540c-36 0-64-26-64-59s28-58 64-58 63 26 63 58-28 59-63 59zm0-105c-29 0-52 21-52 46s23 47 52 47 51-21 51-47-23-46-51-46z" fill="#541018"/>
  <path d="M344 690a122 106 0 1 0 244 0 122 106 0 1 0-244 0Z" fill="#F9E769"/>
  <path d="M466 802c-70 0-128-50-128-112s58-112 128-112 127 50 127 112-57 112-127 112zm0-212c-64 0-116 45-116 100s52 100 116 100 116-45 116-100-52-100-116-100z" fill="#541018"/>
</svg>)SVG",
                        dimensions = { 18_apx, 18_apx },
                    },
                    enabled   = globalEnabled,
                    gapColumn = 5_apx,
                    rcnew Text{ "Button with icon" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "SVG icon" },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Button{
                    rcnew Viewport{
                        [](Canvas& canvas, Rectangle rect) {
                            canvas.setFillColor(Palette::Standard::amber);
                            PreparedText text = fonts->prepare(Font{ Font::DefaultPlusIconsEmoji, dp(18) },
                                                               "This text is rendered dynamically.");
                            float x           = fract(currentTime() * 0.1) * text.bounds().width();
                            PointF offset     = text.alignLines(0.f, 0.5f);
                            canvas.fillText(PointF{ -x, float(rect.center().y) } + offset, text);
                            canvas.fillText(PointF{ -x, float(rect.center().y) } + offset +
                                                PointF{ text.bounds().width(), 0.f },
                                            text);
                        },
                        dimensions = { 70_apx, 25_apx },
                    },
                    enabled = globalEnabled,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Button with Viewport displaying rendered content" },
        },
        rcnew VLayout{
            gapRow     = 5_apx,
            alignItems = AlignItems::FlexStart,
            rcnew Button{
                rcnew Text{ "Button with color applied" },
                enabled             = globalEnabled,
                Graphene::mainColor = 0xFF4791_rgb,
            },
            rcnew Button{
                rcnew Text{ "Button with reduced padding" },
                enabled = globalEnabled,
                padding = 4_px,
            },
            rcnew Button{
                rcnew Text{ "Button with flat style" },
                enabled = globalEnabled,
                classes = { "flat" },
            },
            &m_group,
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew Button{
                    rcnew Text{
                        "Hold to repeat action",
                    },
                    enabled        = globalEnabled,
                    repeatDelay    = 0.2,
                    repeatInterval = 0.2,
                    onClick        = lifetime() |
                              [this] {
                                  m_clicked++;
                                  bindings->notify(&m_clicked);
                              },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{
                text = Value{ &m_clicked }.transform([](int n) {
                    return fmt::format("Clicked {} times", n);
                }),
            },
        },
        rcnew HLayout{
            rcnew Widget{
                squircleCorners = false,
                rcnew Button{
                    rcnew Text{ "First" },
                    enabled = globalEnabled,
                    &m_btnGroup,
                    borderRadius = 15_px,
                },
                rcnew Button{
                    rcnew Text{ "Second" },
                    enabled = globalEnabled,
                    &m_btnGroup,
                    borderRadius = 15_px,
                },
                rcnew Button{
                    rcnew Text{ "Third" },
                    enabled = globalEnabled,
                    &m_btnGroup,
                    borderRadius = 15_px,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Grouped buttons share borders" },
        },
        rcnew HLayout{
            rcnew Button{
                gapColumn = 3_apx,
                rcnew Text{ "This button contains" },
                enabled = globalEnabled,
                rcnew Table{
                    classes = { "table-padding-4" },
                    rcnew TableRow{
                        rcnew TableCell{ rcnew Text{ "A" } },
                        rcnew TableCell{ rcnew Text{ "small" } },
                    },
                    rcnew TableRow{
                        rcnew TableCell{ rcnew Text{ "Table" } },
                        rcnew TableCell{ rcnew Text{ "widget" } },
                    },
                },
                rcnew Text{ "inside it" },
            },
        },
        rcnew Text{ "ToggleButton (widgets/ToggleButton.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew ToggleButton{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "ToggleButton 1" },
                },
                &m_group,
            },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew ToggleButton{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "ToggleButton 2" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Shares state with ToggleButton 1" },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew ToggleButton{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "Off" },
                    rcnew Text{ "On" },
                    twoState = true,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Shares state with ToggleButton 1" },
        },
        rcnew Text{ "CheckBox (widgets/CheckBox.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew CheckBox{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "CheckBox" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Shares state with ToggleButton 1" },
        },
        rcnew Text{ "Switch (widgets/Switch.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew Switch{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "Switch" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Shares state with ToggleButton 1" },
        },
        rcnew Text{ "RadioButton (widgets/RadioButton.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew RadioButton{
                    value   = Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "On" },
                },
                gapColumn = 6_apx,
                rcnew RadioButton{
                    value   = !Value{ &m_toggled },
                    enabled = globalEnabled,
                    rcnew Text{ "Off" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Shares state with ToggleButton 1" },
        },
        rcnew Text{ "Hyperlink (widgets/Hyperlink.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew Hyperlink{
                    "https://brisklib.com",
                    rcnew Text{ "Click to visit brisklib.com" },
                    enabled = globalEnabled,
                },
                &m_group,
            },
        },
    };
}
} // namespace Brisk
