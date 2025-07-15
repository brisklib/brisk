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
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include <brisk/graphics/Palette.hpp>

#include <brisk/gui/Styles.hpp>
#include <brisk/gui/Gui.hpp>

namespace Brisk {

TEST_CASE("Rules") {
    if (false) {
        fmt::print("sizeof(Style) = {}\n", sizeof(Style));
        fmt::print("sizeof(Selector) = {}\n", sizeof(Selector));
        fmt::print("sizeof(Stylesheet) = {}\n", sizeof(Stylesheet));
        fmt::print("sizeof(Rules) = {}\n", sizeof(Rules));
        fmt::print("sizeof(Rule) = {}\n", sizeof(Rule));
    }

    CHECK(decltype(Widget::borderColor)::name() == "borderColor"sv);
    CHECK(decltype(Widget::shadowSize)::name() == "shadowSize"sv);
    CHECK(decltype(Widget::opacity)::name() == "opacity"sv);
    CHECK(decltype(Widget::layout)::name() == "layout"sv);
    CHECK(decltype(Widget::tabSize)::name() == "tabSize"sv);

    CHECK(Rule(borderColor = 0xFFFFFF_rgb).name() == "borderColor"sv);
    CHECK(Rule(shadowSize = 2).toString() == "shadowSize: 2px"sv);

    CHECK(Rule(borderColor = 0xFFFFFF_rgb) == Rule(borderColor = 0xFFFFFF_rgb));
    CHECK(Rule(borderColor = 0xFFFFFF_rgb) != Rule(borderColor = 0xDDDDDD_rgb));

    CHECK(Rules{ borderColor = 0xFFFFFF_rgb } == Rules{ borderColor = 0xFFFFFF_rgb });
    CHECK(Rules{ borderColor = 0xFFFFFF_rgb } != Rules{ borderColor = 0xDDDDDD_rgb });

    CHECK(Rules{ shadowSize = 2, shadowSize = 1 } == Rules{ shadowSize = 1 });
    CHECK(Rules{ shadowSize = 1, shadowSize = 2 } == Rules{ shadowSize = 2 });

    CHECK(fmt::to_string(Rules{ shadowSize = 1, opacity = 0.5, layout = Layout::Horizontal }) ==
          "layout: Horizontal; opacity: 0.5; shadowSize: 1px"sv);

    using enum WidgetState;
    CHECK(
        fmt::to_string(Rules{ shadowSize = 1, shadowSize | Hover = 2, shadowSize | Pressed = 3,
                              shadowSize | Selected = 4 }) ==
        "shadowSize: 1px; shadowSize | Hover: 2px; shadowSize | Pressed: 3px; shadowSize | Selected: 4px"sv);

    CHECK(Rules{ shadowSize = 2 }.merge(Rules{ shadowSize = 1 }) == Rules{ shadowSize = 1 });
    CHECK(Rules{ shadowSize = 2 }.merge(Rules{ tabSize = 1 }) == Rules{ shadowSize = 2, tabSize = 1 });
    CHECK(Rules{}.merge(Rules{ shadowSize = 2, tabSize = 1 }) == Rules{ shadowSize = 2, tabSize = 1 });

    Rc<Widget> w = rcnew Widget{};
    Rules{ shadowSize = 2, tabSize = 1 }.applyTo(w.get());
    CHECK(w->tabSize.get() == 1);
    CHECK(w->shadowSize.get() == 2_px);
}

template <std::derived_from<Widget> W>
class WidgetProtected : public W {
public:
    using W::m_height;
    using W::m_type;
    using W::m_width;
    using W::resolveProperties;
    using W::restyleIfRequested;
    using W::setState;
    using W::toggleState;

    uint32_t invalidatedCounter() const noexcept {
        return this->m_invalidatedCounter;
    }
};

template <std::derived_from<Widget> W>
WidgetProtected<W>* unprotect(W* w) {
    return reinterpret_cast<WidgetProtected<W>*>(w);
}

template <std::derived_from<Widget> W>
WidgetProtected<W>* unprotect(std::shared_ptr<W> w) {
    return unprotect(w.get());
}

TEST_CASE("Selectors") {
    using namespace Selectors;

    Rc<Widget> w = rcnew Widget{
        id      = "primary",
        classes = { "success", "large" },

        rcnew Widget{
            classes = { "text" },
        },
    };
    unprotect(w)->m_type = "button";
    auto child           = w->widgets().front();

    CHECK(Type{ "button" }.matches(w.get(), MatchFlags::None));
    CHECK(!Type{ "checkbox" }.matches(w.get(), MatchFlags::None));

    CHECK(Id{ "primary" }.matches(w.get(), MatchFlags::None));
    CHECK(!Id{ "secondary" }.matches(w.get(), MatchFlags::None));

    CHECK(Class{ "success" }.matches(w.get(), MatchFlags::None));
    CHECK(Class{ "large" }.matches(w.get(), MatchFlags::None));
    CHECK(!Class{ "small" }.matches(w.get(), MatchFlags::None));

    CHECK(!(!Class{ "large" }).matches(w.get(), MatchFlags::None));
    CHECK((!Class{ "small" }).matches(w.get(), MatchFlags::None));

    CHECK((Class{ "success" } && Class{ "large" }).matches(w.get(), MatchFlags::None));
    CHECK(!(Class{ "success" } && Class{ "small" }).matches(w.get(), MatchFlags::None));

    CHECK(!Nth{ 0 }.matches(w.get(), MatchFlags::None));

    CHECK(Nth{ 0 }.matches(child.get(), MatchFlags::None));
    CHECK(NthLast{ 0 }.matches(child.get(), MatchFlags::None));
    CHECK(!Nth{ 1 }.matches(child.get(), MatchFlags::None));
    CHECK(!NthLast{ 1 }.matches(child.get(), MatchFlags::None));

    CHECK(Parent{ Id{ "primary" } }.matches(child.get(), MatchFlags::None));
    CHECK((Parent{ Type{ "button" } } && Class{ "text" }).matches(child.get(), MatchFlags::None));

    CHECK(Brisk::Selector{ Type{ "button" } }.matches(w.get(), MatchFlags::None));
    CHECK(!Brisk::Selector{ Type{ "checkbox" } }.matches(w.get(), MatchFlags::None));

    CHECK(Brisk::Selector{ Id{ "primary" } }.matches(w.get(), MatchFlags::None));
    CHECK(!Brisk::Selector{ Id{ "secondary" } }.matches(w.get(), MatchFlags::None));
}

TEST_CASE("Styles") {
    using namespace Selectors;
    using enum WidgetState;
    Rc<Stylesheet> ss = rcnew Stylesheet{
        Style{
            Type{ "button" },
            { padding = Edges{ 20 } },
        },
        Style{
            Type{ "progress" },
            { padding = Edges{ 10 } },
        },
        Style{
            Class{ "success" },
            {
                backgroundColor            = Palette::green,
                backgroundColor | Hover    = Palette::yellow,
                backgroundColor | Pressed  = Palette::red,
                backgroundColor | Disabled = Palette::grey,
            },
        },
        Style{
            Class{ "warning" },
            { backgroundColor = Palette::yellow },
        },
        Style{
            Class{ "danger" },
            { backgroundColor = Palette::red },
        },
        Style{
            Id{ "primary" },
            { shadowSize = 2 },
        },
        Style{
            Id{ "secondary" },
            { shadowSize = 3 },
        },
    };

    Rc<Widget> w1 = rcnew Widget{
        id = "primary",
    };

    CHECK(w1->id.get() == "primary");
    CHECK(w1->shadowSize.get() == Length(0));

    Rc<Widget> w2 = rcnew Widget{
        stylesheet = ss,
        id         = "first",
        id         = "primary",
    };
    unprotect(w2)->restyleIfRequested();

    CHECK(w2->id.get() == "primary");
    CHECK(w2->shadowSize.get() == 2_px);

    w2->id = "secondary";
    unprotect(w2)->restyleIfRequested();

    CHECK(w2->id.get() == "secondary");
    CHECK(w2->shadowSize.get() == 3_px);

    w2->classes = { "warning" };
    unprotect(w2)->restyleIfRequested();

    CHECK(w2->backgroundColor.get() == ColorW(Palette::yellow));

    w2->classes = { "success" };
    unprotect(w2)->restyleIfRequested();

    CHECK(w2->backgroundColor.get() == ColorW(Palette::green));

    unprotect(w2)->toggleState(WidgetState::Hover, true);
    CHECK(w2->backgroundColor.get() == ColorW(Palette::yellow));

    unprotect(w2)->toggleState(WidgetState::Pressed, true);
    CHECK(w2->backgroundColor.get() == ColorW(Palette::red));
}

TEST_CASE("separate SizeL") {

    using namespace Selectors;
    Rc<const Stylesheet> stylesheet = rcnew Stylesheet{
        Style{
            Type{ Widget::widgetType },
            Rules{
                height = 1_em,
            },
        },
    };

    Rc<Widget> w1 = rcnew Widget{
        Arg::stylesheet = stylesheet,
    };

    CHECK(w1->dimensions.get() == SizeL{ undef, undef });
    unprotect(w1)->restyleIfRequested();
    CHECK(w1->dimensions.get() == SizeL{ undef, 1_em });

    Rc<Widget> w2 = rcnew Widget{
        Arg::stylesheet = stylesheet,
        width           = 200,
    };

    CHECK(w2->dimensions.get() == SizeL{ 200_px, undef });
    unprotect(w2)->restyleIfRequested();
    CHECK(w2->dimensions.get() == SizeL{ 200_px, 1_em });
}

TEST_CASE("separate SizeL 2") {
    using namespace Selectors;
    Rc<const Stylesheet> stylesheet = rcnew Stylesheet{
        Style{
            Type{ Widget::widgetType },
            Rules{
                dimensions = 1_em,
            },
        },
    };

    Rc<Widget> w1 = rcnew Widget{
        Arg::stylesheet = stylesheet,
    };

    CHECK(w1->dimensions.get() == SizeL{ undef, undef });
    unprotect(w1)->restyleIfRequested();
    CHECK(w1->dimensions.get() == SizeL{ 1_em, 1_em });

    Rc<Widget> w2 = rcnew Widget{
        Arg::stylesheet = stylesheet,
        width           = 200,
    };

    CHECK(w2->dimensions.get() == SizeL{ 200_px, undef });
    unprotect(w2)->restyleIfRequested();
    CHECK(w2->dimensions.get() == SizeL{ 200_px, 1_em });
}

TEST_CASE("resolving") {
    Rc<Widget> w           = rcnew Widget{};
    w->borderRadius        = 10_px;
    w->borderRadiusTopLeft = 1_px;

    CornersF radius        = w->borderRadius.current();

    CHECK(radius == CornersF{ 1, 10, 10, 10 });
}

TEST_CASE("inherit") {
    Rc<Widget> w1 = rcnew Widget{
        // w1
        fontSize = 20_px,
        rcnew Widget{
            // w2
            fontSize = 200_perc,
            rcnew Widget{
                // w2ch
                // fontSize = inherit
            },
        },
        rcnew Widget{
            // w1ch
            // fontSize = inherit
        },
    };

    Rc<Widget> w2   = w1->widgets().front();

    Rc<Widget> w1ch = w1->widgets().back();
    Rc<Widget> w2ch = w2->widgets().back();

    CHECK(w1->fontSize.get() == 20_px);
    CHECK(w1->fontSize.current() == 20);
    CHECK(w2->fontSize.get() == 200_perc);
    CHECK(w2->fontSize.current() == 40);

    CHECK(w1ch->fontSize.get() == 20_px);
    CHECK(w1ch->fontSize.current() == 20);
    CHECK(w2ch->fontSize.get() == 200_perc);
    CHECK(w2ch->fontSize.current() == 40);
}

TEST_CASE("inherit2") {
    using namespace Selectors;
    Rc<const Stylesheet> stylesheet = rcnew Stylesheet{
        Style{
            Id{ "A" },
            Rules{
                color = Palette::red,
            },
        },
    };

    Rc<Widget> w1 = rcnew Widget{
        Arg::stylesheet = stylesheet,
        id              = "A",

        rcnew Widget{},
    };

    unprotect(w1)->restyleIfRequested();
    CHECK(w1->color.get() == ColorW(Palette::red));
    CHECK(w1->widgets().front()->color.get() == ColorW(Palette::red));
}

TEST_CASE("inherit3") {
    auto w = rcnew Widget{
        color = inherit,
    };

    auto parent = rcnew Widget{
        color = Palette::red,
    };

    CHECK(w->color.get() == ColorW(Palette::white));    // Default
    CHECK(parent->color.get() == ColorW(Palette::red)); // Overridden

    parent->append(w);
    CHECK(w->color.get() == ColorW(Palette::red)); // Inherited
}

TEST_CASE("inherit4") {
    auto w = rcnew Widget{
        color = Palette::red,
        Builder{
            [](Widget* target) {
                target->apply(rcnew Widget{
                    rcnew Widget{},
                });
            },
            BuilderKind::Delayed,
        },
    };
    CHECK(w->color.get() == ColorW(Palette::red)); // Overridden
    REQUIRE(w->widgets().empty());

    unprotect(w)->rebuild(false);

    REQUIRE(!w->widgets().empty());
    REQUIRE(!w->widgets().front()->widgets().empty());

    CHECK(w->widgets().front()->color.get() == ColorW(Palette::red));                    // Inherited
    CHECK(w->widgets().front()->widgets().front()->color.get() == ColorW(Palette::red)); // Inherited

    w->color = Palette::blue;
    CHECK(w->color.get() == ColorW(Palette::blue));

    CHECK(w->widgets().front()->color.get() == ColorW(Palette::blue));                    // Inherited
    CHECK(w->widgets().front()->widgets().front()->color.get() == ColorW(Palette::blue)); // Inherited
}

TEST_CASE("Stylesheet with inheritance") {
    Rc<Widget> w2;
    auto w = rcnew Widget{
        stylesheet =
            rcnew Stylesheet{
                Style{
                    Selectors::Id{ "A" },
                    Rules{
                        color           = Palette::red,
                        backgroundColor = Palette::blue,
                    },
                },
            },
        color = 0x808080_rgb,
        rcnew Widget{
            storeWidget(&w2),
            id = "A",
            rcnew Widget{
                id = "B",
            },
        },
    };
    unprotect(w)->restyleIfRequested();
    CHECK(w2->color.get() == Palette::red);
    CHECK(w2->backgroundColor.get() == Palette::blue);
    CHECK(w2->widgets().front()->color.get() == Palette::red);
    CHECK(w2->widgets().front()->backgroundColor.get() == Palette::transparent);
}

TEST_CASE("Style with states") {
    using namespace Selectors;
    using enum WidgetState;
    Rc<const Stylesheet> stylesheet = rcnew Stylesheet{
        Style{
            Id{ "A" },
            Rules{
                color            = Palette::white,
                color | Selected = Palette::red,
            },
        },
    };

    Rc<Widget> w1 = rcnew Widget{
        Arg::stylesheet = stylesheet,
        id              = "A",
        rcnew Widget{},
    };
    unprotect(w1)->restyleIfRequested();
    CHECK(w1->color.get() == ColorW(Palette::white));
    CHECK(w1->widgets().front()->color.get() == ColorW(Palette::white));
    w1->selected.set(true);
    CHECK(w1->color.get() == ColorW(Palette::red));
    CHECK(w1->widgets().front()->color.get() == ColorW(Palette::red));
}

class Derived : public Widget {
    BRISK_DYNAMIC_CLASS(Derived, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "derived";

    template <WidgetArgument... Args>
    explicit Derived(const Args&... args) : Derived{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

private:
    ColorW m_fillColor;

    explicit Derived(Construction construction, ArgumentsView<Derived> args)
        : Widget{ construction, nullptr } {
        args.apply(this);
    }

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::GuiProp{ &Derived::m_fillColor, Palette::black, AffectPaint, "fillColor" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Derived, ColorW, 0> fillColor;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline PropArgument<decltype(Derived::fillColor)> fillColor{};
}

TEST_CASE("Properties for derived widgets") {
    auto w = rcnew Derived{
        fillColor = Palette::green,
    };

    CHECK(w->fillColor.get() == Palette::green);

    CHECK(unprotect(w)->invalidatedCounter() == 1);

    w->fillColor = Palette::red;

    CHECK(w->fillColor.get() == Palette::red);

    CHECK(unprotect(w)->invalidatedCounter() == 2);
}

TEST_CASE("Stylesheet for derived widgets") {
    auto style = rcnew Stylesheet{
        Style{
            Selectors::Type{ Derived::widgetType },
            Rules{
                fillColor = Palette::magenta,
            },
        },
    };

    auto ww = rcnew Widget{
        stylesheet = style,
        rcnew Derived{},
    };

    unprotect(ww)->restyleIfRequested();

    REQUIRE(dynamicPointerCast<Derived>(ww->widgets().front()));

    CHECK(dynamicPointerCast<Derived>(ww->widgets().front())->fillColor.get() == Palette::magenta);
}
} // namespace Brisk
