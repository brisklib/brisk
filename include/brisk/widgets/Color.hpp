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

#include "Widgets.hpp"
#include "PopupButton.hpp"

namespace Brisk {

class WIDGET ColorView : public Widget {
    BRISK_DYNAMIC_CLASS(ColorView, Widget)
protected:
    ColorW m_value = Palette::black;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
    ColorView(Construction construction, ColorW color, ArgumentsView<ColorView> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &ColorView::m_value, &ColorView::invalidate, "value" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorView, ColorW, 0> value;
    BRISK_PROPERTIES_END

public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorview";

    template <WidgetArgument... Args>
    explicit ColorView(Value<ColorW> color, const Args&... args)
        : ColorView(Construction{ widgetType }, ColorW{}, std::tuple{ args... }) {
        endConstruction();
        bindings->connect(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorView(ColorW color, const Args&... args)
        : ColorView(Construction{ widgetType }, std::move(color), std::tuple{ args... }) {
        endConstruction();
    }
};

class WIDGET ColorSliders : public Widget {
    BRISK_DYNAMIC_CLASS(ColorSliders, Widget)
protected:
    ColorW m_value = Palette::black;

    Ptr cloneThis() const override;
    explicit ColorSliders(Construction construction, ColorW color, bool alpha,
                          ArgumentsView<ColorSliders> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &ColorSliders::m_value, &ColorSliders::invalidate, "value" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorSliders, ColorW, 0> value;
    BRISK_PROPERTIES_END
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorsliders";

    template <WidgetArgument... Args>
    explicit ColorSliders(Value<ColorW> color, bool alpha, const Args&... args)
        : ColorSliders(Construction{ widgetType }, ColorW{}, alpha, std::tuple{ args... }) {
        endConstruction();
        bindings->connectBidir(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorSliders(ColorW color, bool alpha, const Args&... args)
        : ColorSliders(Construction{ widgetType }, color, alpha, std::tuple{ args... }) {
        endConstruction();
    }
};

class WIDGET ColorPalette final : public Widget {
    BRISK_DYNAMIC_CLASS(ColorPalette, Widget)
protected:
    ColorW m_value = Palette::black;

    Rc<Widget> addColor(ColorW swatch, float brightness = 0.f, float chroma = 1.f);
    Ptr cloneThis() const override;
    explicit ColorPalette(Construction construction, ColorW color, ArgumentsView<ColorPalette> args);

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &ColorPalette::m_value, &ColorPalette::invalidate, "value" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorPalette, ColorW, 0> value;
    BRISK_PROPERTIES_END
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorpalette";

    template <WidgetArgument... Args>
    explicit ColorPalette(Value<ColorW> color, const Args&... args)
        : ColorPalette(Construction{ widgetType }, ColorW{}, std::tuple{ args... }) {
        bindings->connectBidir(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorPalette(ColorW color, const Args&... args)
        : ColorPalette(Construction{ widgetType }, std::move(color), std::tuple{ args... }) {}
};

class WIDGET ColorButton : public PopupButton {
    BRISK_DYNAMIC_CLASS(ColorButton, Widget)
public:
    using Base = PopupButton;
    using Button::widgetType;

    template <WidgetArgument... Args>
    explicit ColorButton(Value<ColorW> prop, bool alpha, const Args&... args)
        : ColorButton(Construction{ widgetType }, std::move(prop), alpha, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit ColorButton(Construction construction, Value<ColorW> prop, bool alpha,
                         ArgumentsView<ColorButton> args);
};

class WIDGET GradientView final : public Widget {
    BRISK_DYNAMIC_CLASS(GradientView, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "gradientitem";

    template <WidgetArgument... Args>
    explicit GradientView(ColorStopArray gradient, const Args&... args)
        : GradientView(Construction{ widgetType }, std::move(gradient), std::tuple{ args... }) {
        endConstruction();
    }

    ColorStopArray gradient;

protected:
    GradientView(Construction construction, ColorStopArray gradient, ArgumentsView<GradientView> args);

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
};

} // namespace Brisk
