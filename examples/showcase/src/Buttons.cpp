#include "Buttons.hpp"
#include "brisk/gui/Icons.hpp"
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/ToggleButton.hpp>
#include <brisk/widgets/ImageView.hpp>
#include <brisk/widgets/Viewport.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

RC<Widget> ShowcaseButtons::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,
        new Text{ "Button (widgets/Button.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new Button{
                    new Text{ "Button 1" },
                    onClick = m_lifetime |
                              [notifications]() {
                                  notifications->show(new Text{ "Button 1 clicked" });
                              },
                },
                new Button{
                    new Text{ "Disabled Button" },
                    disabled = true,
                    onClick  = m_lifetime |
                              [notifications]() {
                                  notifications->show(new Text{ "Disabled Button clicked" });
                              },
                },
                &m_group,
            },
        },
        new HLayout{
            new Widget{
                new Button{
                    new Text{ ICON_settings "  Button with icon" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Icon from icon font" },
        },
        new HLayout{
            new Widget{
                new Button{
                    new SVGImageView{
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
                    gapColumn = 5_apx,
                    new Text{ "Button with icon" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "SVG icon" },
        },
        new HLayout{
            new Widget{
                new Button{
                    new Viewport{
                        [](Canvas& canvas, Rectangle rect) {
                            canvas.setFillColor(Palette::Standard::amber);
                            PreparedText text = fonts->prepare(Font{ FontFamily::Default, dp(18) },
                                                               "This text is rendered dynamically.");
                            float x           = fract(currentTime() * 0.1) * text.bounds().width();
                            text.align({ -x, float(rect.center().y) }, 0.f, 0.5f);
                            canvas.fillText(text);
                            text.applyOffset({ text.bounds().width(), 0.f });
                            canvas.fillText(text);
                        },
                        dimensions = { 70_apx, 25_apx },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Button can contain any widget" },
        },
        new HLayout{
            new Widget{
                new Button{
                    new Text{ "Button with color applied" },
                    Graphene::buttonColor = 0xFF4791_rgb,
                },
                &m_group,
            },
        },
        new HLayout{
            new Widget{
                new Button{
                    new Text{
                        "Hold to repeat action",
                    },
                    repeatDelay    = 0.2,
                    repeatInterval = 0.2,
                    onClick        = m_lifetime |
                              [this] {
                                  m_clicked++;
                                  bindings->notify(&m_clicked);
                              },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{
                text = Value{ &m_clicked }.transform([](int n) {
                    return fmt::format("Clicked {} times", n);
                }),
            },
        },
        new Text{ "ToggleButton (widgets/ToggleButton.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new ToggleButton{
                    value = Value{ &m_toggled },
                    new Text{ "ToggleButton 1" },
                },
                &m_group,
            },
        },
        new HLayout{
            new Widget{
                new ToggleButton{
                    value = Value{ &m_toggled },
                    new Text{ "ToggleButton 2" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Shares state with ToggleButton 1" },
        },
        new HLayout{
            new Widget{
                new ToggleButton{
                    value = Value{ &m_toggled },
                    new Text{ "Off" },
                    new Text{ "On" },
                    twoState = true,
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Shares state with ToggleButton 1" },
        },
        new Text{ "CheckBox (widgets/CheckBox.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new CheckBox{
                    value = Value{ &m_toggled },
                    new Text{ "CheckBox" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Shares state with ToggleButton 1" },
        },
        new Text{ "Switch (widgets/Switch.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new Switch{
                    value = Value{ &m_toggled },
                    new Text{ "Switch" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Shares state with ToggleButton 1" },
        },
        new Text{ "RadioButton (widgets/RadioButton.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new RadioButton{
                    value = Value{ &m_toggled },
                    new Text{ "On" },
                },
                gapColumn = 6_apx,
                new RadioButton{
                    value = Value{ &m_toggled }.transform(
                        [](bool v) {
                            return !v;
                        },
                        [](bool v) {
                            return !v;
                        }),
                    new Text{ "Off" },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Shares state with ToggleButton 1" },
        },
        new Text{ "Hyperlink (widgets/Hyperlink.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new Hyperlink{
                    "https://brisklib.com",
                    new Text{ "Click to visit brisklib.com" },
                },
                &m_group,
            },
        },
    };
}
} // namespace Brisk
