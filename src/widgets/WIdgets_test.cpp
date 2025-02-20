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
#include <brisk/widgets/Widgets.hpp>
#include <catch2/catch_all.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/gui/Icons.hpp>
#include "Catch2Utils.hpp"
#include "../graphics/VisualTests.hpp"

namespace Brisk {

TEST_CASE("Text") {
    auto w = rcnew Text{
        text = "Initialize",
    };

    CHECK(w->text.get() == "Initialize");
}

class Row : public Widget {
public:
    template <WidgetArgument... Args>
    Row(const Args&... args)
        : Widget{
              Arg::layout    = Layout::Horizontal,
              Arg::gapColumn = 8_apx,
              args...,
          } {}
};

class Container : public Widget {
public:
    template <WidgetArgument... Args>
    Container(const Args&... args)
        : Widget{
              Arg::stylesheet = Graphene::stylesheet(),
              Graphene::darkColors(),
              Arg::justifyContent = Justify::Center,
              Arg::alignItems     = Align::Center,
              args...,
          } {}
};

static Size defaultSize{ 360, 120 };
static float defaultPixelRatio = 2.f;

static void widgetTest(const std::string& name, RC<Widget> widget, std::initializer_list<Event> events = {},
                       Size size = defaultSize, float pixelRatio = defaultPixelRatio) {
    InputQueue input;
    InputQueueScope inputScope(&input);
    for (const Event& e : events) {
        input.addEvent(e);
    }
    WidgetTree tree;
    tree.disableTransitions();
    Brisk::pixelRatio() = pixelRatio;
    tree.setViewportRectangle({ Point{}, size });
    tree.setRoot(rcnew Container{
        std::move(widget),
    });
    renderTest(name, tree.viewportRectangle().size(), [&](RenderContext& context) {
        Canvas canvas(context);
        tree.updateAndPaint(canvas, Palette::black, true);
    });
}

static Event mouseMove(float x, float y) {
    EventMouseMoved event;
    event.point = { x, y };
    return event;
}

static Event mousePress(float x, float y) {
    EventMouseButtonPressed event;
    event.button    = MouseButton::Left;
    event.downPoint = event.point = { x, y };
    return event;
}

TEST_CASE("WidgetRendering") {
    widgetTest("widget-text", rcnew Text{ "Text" });
    widgetTest("widget-button", rcnew Button{ rcnew Text{ "Button" } });

    widgetTest("widget-button-hovered", rcnew Button{ rcnew Text{ "Button" } }, { mouseMove(180, 60) });
    widgetTest("widget-button-pressed", rcnew Button{ rcnew Text{ "Button" } }, { mousePress(180, 60) });
    widgetTest("widget-button-disabled", rcnew Button{ rcnew Text{ "Button" }, disabled = true });

    widgetTest("widget-togglebutton", rcnew Row{ rcnew ToggleButton{ rcnew Text{ "On" }, value = true },
                                                 rcnew ToggleButton{ rcnew Text{ "Off" }, value = false } });
    widgetTest("widget-checkbox", rcnew Row{ rcnew CheckBox{ rcnew Text{ "On" }, value = true },
                                             rcnew CheckBox{ rcnew Text{ "Off" }, value = false } });
    widgetTest("widget-switch", rcnew Row{ rcnew Switch{ rcnew Text{ "On" }, value = true },
                                           rcnew Switch{ rcnew Text{ "Off" }, value = false } });
    widgetTest("widget-radiobutton", rcnew Row{ rcnew RadioButton{ rcnew Text{ "On" }, value = true },
                                                rcnew RadioButton{ rcnew Text{ "Off" }, value = false } });

    widgetTest("widget-button-color", rcnew Button{ rcnew Text{ "Button with color applied" },
                                                    Graphene::buttonColor = Palette::Standard::amber });
    widgetTest("widget-button-icon", rcnew Button{
                                         rcnew Text{ "Button with icon " ICON_calendar_1 },
                                     });
    widgetTest("widget-button-emoji", rcnew Button{
                                          rcnew Text{ "Button with emoji 🏆" },
                                      });
    widgetTest("widget-button-svg",
               rcnew Button{ rcnew SVGImageView{
                   R"SVG(<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128">
<path d="M106.809 115a13.346 13.346 0 0 1 0-18.356h-80.9a4.71 4.71 0 0 0-4.71 4.71v8.936a4.71 4.71 0 0 0 4.71 4.71z" fill="#dbedff"/><path fill="#f87c68" d="M42.943 105.82v15.873l-5.12-5.12-5.12 5.12V105.82h10.24z"/>
<path d="M25.906 6.307a4.71 4.71 0 0 0-4.71 4.71v90.335a4.71 4.71 0 0 1 4.71-4.71h80.9V6.307z" fill="#64d465"/><path d="M32.7 6.31v90.33h-6.8a4.712 4.712 0 0 0-4.71 4.71V11.02a4.712 4.712 0 0 1 4.71-4.71z" fill="#40c140"/>
<path fill="#dbedff" d="M50.454 24.058h38.604v20.653H50.454z"/><path d="M103.15 105.82a11 11 0 0 0 .13 1.75H32.7a1.75 1.75 0 0 1 0-3.5h70.58a11 11 0 0 0-.13 1.75z" fill="#b5dcff"/></svg>)SVG",
                   dimensions = { 24, 24 },
               } });

    widgetTest("widget-combobox",
               rcnew ComboBox{
                   value     = 2,
                   alignSelf = Align::FlexStart,
                   marginTop = 12_apx,
                   rcnew ItemList{
                       visible = true,
                       rcnew Text{ "Avocado" },
                       rcnew Text{ "Blueberry" },
                       rcnew Text{ "Cherry" },
                       rcnew Text{ "Dragon Fruit" },
                   },
               },
               {}, { 360, 360 });
    widgetTest("widget-combobox-color",
               rcnew ComboBox{
                   value     = 1,
                   alignSelf = Align::FlexStart,
                   marginTop = 12_apx,
                   rcnew ItemList{
                       visible = true,
                       rcnew ColorView{ Palette::Standard::red },
                       rcnew ColorView{ Palette::Standard::green },
                       rcnew ColorView{ Palette::Standard::blue },
                       rcnew ColorView{ Palette::Standard::yellow },
                   },
               },
               {}, { 360, 360 });

    widgetTest("widget-knob", rcnew Knob{ value = 0.5f, minimum = 0, maximum = 1 });
    widgetTest("widget-slider", rcnew Slider{ width = 160_apx, value = 50, minimum = 0, maximum = 100 });
}
} // namespace Brisk
