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
#include <brisk/widgets/Widgets.hpp>
#include <catch2/catch_all.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/gui/Icons.hpp>
#include "Catch2Utils.hpp"
#include "../graphics/VisualTests.hpp"
#include <brisk/graphics/Offscreen.hpp>
#include <random>

namespace Brisk {

TEST_CASE("Text") {
    auto w = rcnew Text{
        text = "Initialize",
    };

    CHECK(w->text.get() == "Initialize");
}

class Row : public Widget {
    BRISK_DYNAMIC_CLASS(Row, Widget)
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
    BRISK_DYNAMIC_CLASS(Container, Widget)
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

static void widgetTest(const std::string& name, Rc<Widget> widget, std::initializer_list<Event> events = {},
                       Size size = defaultSize, float pixelRatio = defaultPixelRatio,
                       ColorW winColor = 0x131419_rgb) {
    InputQueue input;
    for (const Event& e : events) {
        input.addEvent(e);
    }
    WidgetTree tree(&input);
    tree.disableTransitions();
    Brisk::pixelRatio() = pixelRatio;
    tree.setViewportRectangle({ Point{}, size });
    tree.setRoot(rcnew Container{
        windowColor = winColor,
        std::move(widget),
    });
    renderTest(name, tree.viewportRectangle().size(), [&](RenderContext& context) {
        Canvas canvas(context);
        tree.update();
        tree.paint(canvas, Palette::black, true);
    });
}

static Event mouseMove(PointF pt) {
    EventMouseMoved event;
    event.point = pt;
    return event;
}

static Event mousePress(PointF pt) {
    EventMouseButtonPressed event;
    event.button    = MouseButton::Left;
    event.downPoint = event.point = pt;
    return event;
}

static Event mouseRelease(PointF pt) {
    EventMouseButtonReleased event;
    event.button = MouseButton::Left;
    event.point  = pt;
    return event;
}

TEST_CASE("Widget Text") {
    widgetTest("widget-text", rcnew Text{ "Text" });
}

TEST_CASE("Widget Button") {
    widgetTest("widget-button", rcnew Button{ rcnew Text{ "Button" } });
}

TEST_CASE("Widget Button Hovered") {
    widgetTest("widget-button-hovered", rcnew Button{ rcnew Text{ "Button" } }, { mouseMove({ 180, 60 }) });
}

TEST_CASE("Widget Button Pressed") {
    widgetTest("widget-button-pressed", rcnew Button{ rcnew Text{ "Button" } }, { mousePress({ 180, 60 }) });
}

TEST_CASE("Widget Button Disabled") {
    widgetTest("widget-button-disabled", rcnew Button{ rcnew Text{ "Button" }, disabled = true });
}

TEST_CASE("Widget ToggleButton") {
    widgetTest("widget-togglebutton", rcnew Row{ rcnew ToggleButton{ rcnew Text{ "On" }, value = true },
                                                 rcnew ToggleButton{ rcnew Text{ "Off" }, value = false } });
}

TEST_CASE("Widget CheckBox") {
    widgetTest("widget-checkbox", rcnew Row{ rcnew CheckBox{ rcnew Text{ "On" }, value = true },
                                             rcnew CheckBox{ rcnew Text{ "Off" }, value = false } });
}

TEST_CASE("Widget Switch") {
    widgetTest("widget-switch", rcnew Row{ rcnew Switch{ rcnew Text{ "On" }, value = true },
                                           rcnew Switch{ rcnew Text{ "Off" }, value = false } });
}

TEST_CASE("Widget RadioButton") {
    widgetTest("widget-radiobutton", rcnew Row{ rcnew RadioButton{ rcnew Text{ "On" }, value = true },
                                                rcnew RadioButton{ rcnew Text{ "Off" }, value = false } });
}

TEST_CASE("Widget Button with color") {
    widgetTest("widget-button-color", rcnew Button{ rcnew Text{ "Button with color applied" },
                                                    Graphene::mainColor = Palette::Standard::amber });
}

TEST_CASE("Widget Button with icon") {
    widgetTest("widget-button-icon", rcnew Button{
                                         rcnew Text{ "Button with icon " ICON_calendar_1 },
                                     });
}

TEST_CASE("Widget Button with emoji") {
    widgetTest("widget-button-emoji", rcnew Button{
                                          rcnew Text{ "Button with emoji 🏆" },
                                      });
}

TEST_CASE("Widget Button with SVG") {
    widgetTest("widget-button-svg",
               rcnew Button{ rcnew SvgImageView{
                   R"SVG(<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128">
    <path d="M106.809 115a13.346 13.346 0 0 1 0-18.356h-80.9a4.71 4.71 0 0 0-4.71 4.71v8.936a4.71 4.71 0 0
    0 4.71 4.71z" fill="#dbedff"/><path fill="#f87c68" d="M42.943
    105.82v15.873l-5.12-5.12-5.12 5.12V105.82h10.24z"/> <path d="M25.906 6.307a4.71 4.71 0 0
    0-4.71 4.71v90.335a4.71 4.71 0 0 1 4.71-4.71h80.9V6.307z" fill="#64d465"/><path
    d="M32.7 6.31v90.33h-6.8a4.712 4.712 0 0 0-4.71 4.71V11.02a4.712 4.712 0 0 1 4.71-4.71z"
    fill="#40c140"/> <path fill="#dbedff" d="M50.454 24.058h38.604v20.653H50.454z"/><path d="M103.15
    105.82a11 11 0 0 0 .13 1.75H32.7a1.75 1.75 0 0 1 0-3.5h70.58a11 11 0 0 0-.13 1.75z"
    fill="#b5dcff"/></svg>)SVG",
                   dimensions = { 24, 24 },
               } });
}

TEST_CASE("Widget ComboBox Text") {
    widgetTest("widget-combobox",
               rcnew ComboBox{
                   value     = 2,
                   alignSelf = Align::FlexStart,
                   marginTop = 12_apx,
                   rcnew Menu{
                       visible = true,
                       rcnew Text{ "Avocado" },
                       rcnew Text{ "Blueberry" },
                       rcnew Text{ "Cherry" },
                       rcnew Text{ "Dragon Fruit" },
                   },
               },
               {}, { 360, 360 });
}

TEST_CASE("Widget ComboBox Color") {
    widgetTest("widget-combobox-color",
               rcnew ComboBox{
                   value     = 1,
                   alignSelf = Align::FlexStart,
                   marginTop = 12_apx,
                   rcnew Menu{
                       visible = true,
                       rcnew ColorView{ Palette::Standard::red },
                       rcnew ColorView{ Palette::Standard::green },
                       rcnew ColorView{ Palette::Standard::blue },
                       rcnew ColorView{ Palette::Standard::yellow },
                   },
               },
               {}, { 360, 360 });
}

TEST_CASE("Widget ComboBox Gradient") {
    widgetTest("widget-combobox-gradient",
               rcnew ComboBox{
                   rcnew Menu{
                       visible  = true,
                       minWidth = 4.8_em,
                       rcnew GradientView{ ColorStopArray{
                           { 0.f, Palette::white },
                           { 1.f, Palette::black },
                       } },
                       rcnew GradientView{ ColorStopArray{
                           { 0.f, Palette::white },
                           { 0.5f, Palette::blue },
                           { 1.f, Palette::black },
                       } },
                       rcnew GradientView{ ColorStopArray{
                           { 0.f, Palette::black },
                           { 0.33f, Palette::red },
                           { 0.67f, Palette::yellow },
                           { 1.f, Palette::white },
                       } },
                   },
               },
               {}, { 360, 360 });
}

TEST_CASE("Widget Knob") {
    widgetTest("widget-knob", rcnew Knob{ value = 0.5f, minimum = 0, maximum = 1 });
}

TEST_CASE("Widget Slider") {
    widgetTest("widget-slider", rcnew Slider{ width = 160_apx, value = 50, minimum = 0, maximum = 100 });
}

TEST_CASE("Widget Shadow") {
    widgetTest("widget-shadow1",
               rcnew Widget{ dimensions = { 240, 240 }, shadowSize = 8.f, shadowColor = Palette::black,
                             backgroundColor = Palette::white },
               {}, { 320, 320 }, 1, Palette::white);
    widgetTest("widget-shadow2",
               rcnew Widget{ dimensions = { 240, 240 }, shadowSize = 8.f, shadowOffset = { 2.f, 2.f },
                             shadowColor = Palette::black, backgroundColor = Palette::white },
               {}, { 320, 320 }, 1, Palette::white);
    widgetTest("widget-shadow3",
               rcnew Widget{ dimensions = { 240, 240 }, borderRadius = 10, shadowSize = 16.f,
                             shadowSpread = 10, shadowColor = Palette::black,
                             backgroundColor = Palette::white },
               {}, { 320, 320 }, 1, Palette::white);
}

using namespace std::literals::chrono_literals;

struct WidgetAnimation {
    WebpAnimationEncoder anim;
    InputQueue input;
    WidgetTree tree{ &input };
    OffscreenCanvas offscreen;
    Rc<Image> pendingFrame;
    std::chrono::milliseconds pendingFrameDuration{};
    int pixelScale;
    int fps;

    WidgetAnimation(Size size, bool transitions = true, int pixelScale = 3, int fps = 30)
        : pixelScale(pixelScale), fps(fps) {
        offscreen = OffscreenCanvas(size * pixelScale, pixelScale, { .subPixelText = false });

        if (!transitions)
            tree.disableTransitions();
        tree.disableRealtimeMode();
        Brisk::pixelRatio() = pixelScale;
        tree.setViewportRectangle({ Point{}, size * pixelScale });
    }

    void frames(std::chrono::milliseconds time) {
        frames(std::max(int64_t(1), int64_t(time.count() * fps / 1000)));
    }

    void frames(int numFrames = 1) {
        std::chrono::milliseconds dur = std::chrono::milliseconds(1000 / fps);
        for (int i = 0; i < numFrames; ++i) {
            tree.update();
            if (!pendingFrame || !tree.paintRect().empty()) {
                flush();
                tree.paint(offscreen.canvas(), Palette::transparent, true);
                pendingFrame = offscreen.render();
            }
            pendingFrameDuration += dur;
            bindings->assign(frameStartTime) += 1.0 / fps;
        }
    }

    void flush() {
        if (pendingFrame) {
            anim.addFrame(std::move(pendingFrame), pendingFrameDuration);
            pendingFrame         = {};
            pendingFrameDuration = {};
        }
    }

    void save(std::string_view name) {
        flush();
        fs::path targetPath = fs::path(PROJECT_BINARY_DIR "/visualTest/") / name;
        fs::create_directories(targetPath.parent_path());
        REQUIRE(writeBytes(targetPath, anim.encode()).has_value());
    }
};

TEST_CASE("Switch animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    bool val = false;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        rcnew Row{ rcnew Switch{ rcnew Text{ "Switch" }, value = Value{ &val } } },
    });
    animation.frames();

    val = true;
    bindings->notify(&val);

    animation.frames(1000ms);

    val = false;
    bindings->notify(&val);

    animation.frames(1000ms);
    animation.save("animation/switch.webp");
}

TEST_CASE("CheckBox animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    bool val = false;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        rcnew Row{ rcnew CheckBox{ rcnew Text{ "CheckBox" }, value = Value{ &val } } },
    });
    animation.frames();

    val = true;
    bindings->notify(&val);

    animation.frames(1000ms);

    val = false;
    bindings->notify(&val);

    animation.frames(1000ms);
    animation.save("animation/checkbox.webp");
}

TEST_CASE("ToggleButton animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    bool val = false;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        rcnew Row{ rcnew ToggleButton{ rcnew Text{ "ToggleButton" }, value = Value{ &val } } },
    });
    animation.frames();

    val = true;
    bindings->notify(&val);

    animation.frames(1000ms);

    val = false;
    bindings->notify(&val);

    animation.frames(1000ms);
    animation.save("animation/togglebutton.webp");
}

TEST_CASE("Slider animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    float val = 0;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        rcnew Row{ rcnew Slider{ width = 80_apx, value = Value{ &val }, minimum = -1, maximum = 1 } },
    });

    for (float x = 0.f; x < 2 * std::numbers::pi; x += 0.1f) {
        val = std::sin(x);
        bindings->notify(&val);
        animation.frames(1);
    }
    animation.save("animation/slider.webp");
}

TEST_CASE("Button states animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    float val = 0;
    BindingRegistration val_r(&val, nullptr);
    Rc<Button> btn;
    animation.tree.setRoot(rcnew Container{
        rcnew Row{ rcnew Button{ storeWidget(&btn), "Button"_Text } },
    });

    animation.input.addEvent(mouseMove(btn->rect().center()));
    dynamicPointerCast<Text>(btn->widgets().front())->text = "Hover";

    animation.frames(1250ms);

    animation.input.addEvent(mousePress(btn->rect().center()));
    dynamicPointerCast<Text>(btn->widgets().front())->text = "Pressed";

    animation.frames(1250ms);

    animation.input.addEvent(mouseRelease(btn->rect().center()));
    dynamicPointerCast<Text>(btn->widgets().front())->text = "Hover";

    animation.frames(1250ms);

    animation.input.addEvent(mouseMove({}));
    dynamicPointerCast<Text>(btn->widgets().front())->text = "Normal";

    animation.frames(1250ms);

    animation.save("animation/button-states.webp");
}

TEST_CASE("Text wordWrap animation") {
    WidgetAnimation animation({ 288, 192 }, true);

    std::ignore = fonts->addFontFromFile("Noto", FontStyle::Normal, FontWeight::Regular,
                                         PROJECT_SOURCE_DIR "/resources/fonts/GoNotoCurrent-Regular.ttf");

    float val;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        alignItems       = Align::Stretch,
        layout           = Layout::Vertical,
        fontFamily       = "Noto",
        contentOverflowX = ContentOverflow::Allow,
        gapRow           = 4_apx,
        padding          = 4_apx,
        rcnew HLayout{
            gapColumn = 4_apx,
            flexGrow  = 1,
            flexBasis = 0,
            rcnew Text{
                "Hello, universe. This is an example of text.",
                wordWrap        = true,
                width           = Value{ &val },
                backgroundColor = 0xFFFFFF'20_rgba,
            },
            rcnew Text{
                "مرحباً يا كون. هذا مثال للنص.",
                textAlign       = TextAlign::End,
                wordWrap        = true,
                flexBasis       = 0,
                flexGrow        = 1,
                backgroundColor = 0xFFFFFF'20_rgba,
            },
        },
        rcnew HLayout{
            gapColumn = 4_apx,
            flexGrow  = 1,
            flexBasis = 0,
            rcnew Text{
                "שלום, יקום. זהו דוגמה לטקסט.",
                textAlign       = TextAlign::End,
                wordWrap        = true,
                width           = Value{ &val },
                backgroundColor = 0xFFFFFF'20_rgba,
            },
            rcnew Text{
                "你好，宇宙。这是一个文本示例。",
                wordWrap        = true,
                flexBasis       = 0,
                flexGrow        = 1,
                backgroundColor = 0xFFFFFF'20_rgba,
            },
        },
    });

    for (int i = 0; i < 80; ++i) {
        val = 144 - 8 + std::sin(i / 40. * std::numbers::pi) * 70.f;
        bindings->notify(&val);
        animation.frames(2);
    }

    animation.save("animation/textwrap.webp");
}

TEST_CASE("TextEditor animation") {
    WidgetAnimation animation({ 180, 60 }, true);

    std::string val;
    BindingRegistration val_r(&val, nullptr);
    animation.tree.setRoot(rcnew Container{
        rcnew Row{
            padding = 4_apx,
            rcnew TextEditor{ text = Value{ &val }, flexGrow = 1, fontSize = 125_perc, autofocus = true },
            flexGrow = 1 },
    });

    std::u32string text = U"Hello, Brisk! 🚀🎨📝 fi Áñ";

    std::mt19937 rnd(123);
    std::uniform_int_distribution<> uniform(75, 150);
    for (char32_t ch : text) {
        animation.input.addEvent(EventCharacterTyped{ .character = ch });
        int32_t delay = uniform(rnd);
        animation.frames(delay * 1ms);
    }
    animation.frames(800ms);
    for (char32_t ch : text) {
        animation.input.addEvent(EventKeyPressed{ EventKey{ {}, KeyCode::Left } });
        animation.frames(40ms);
    }
    animation.frames(600ms);
    animation.input.addEvent(
        EventKeyPressed{ EventKey{ EventInput{ {}, KeyModifiers::ControlOrCommand }, KeyCode::A } });

    animation.frames(600ms);
    animation.input.addEvent(EventKeyPressed{ EventKey{ {}, KeyCode::Del } });
    animation.frames(600ms);

    animation.save("animation/texteditor.webp");
}
} // namespace Brisk
