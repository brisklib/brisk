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
#pragma once

#include "Widgets.hpp"
#include "PopupButton.hpp"

namespace Brisk {

class WIDGET ColorView : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorview";

    template <WidgetArgument... Args>
    explicit ColorView(Value<ColorF> color, const Args&... args)
        : ColorView(Construction{ widgetType }, ColorF{}, std::tuple{ args... }) {
        endConstruction();
        bindings->connect(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorView(ColorF color, const Args&... args)
        : ColorView(Construction{ widgetType }, std::move(color), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    ColorF m_value = Palette::black;

    void paint(Canvas& canvas) const override;
    Ptr cloneThis() override;
    ColorView(Construction construction, ColorF color, ArgumentsView<ColorView> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorView, ColorF, &ColorView::m_value> value;
    BRISK_PROPERTIES_END
};

class WIDGET ColorSliders : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorsliders";

    template <WidgetArgument... Args>
    explicit ColorSliders(Value<ColorF> color, bool alpha, const Args&... args)
        : ColorSliders(Construction{ widgetType }, ColorF{}, alpha, std::tuple{ args... }) {
        endConstruction();
        bindings->connectBidir(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorSliders(ColorF color, bool alpha, const Args&... args)
        : ColorSliders(Construction{ widgetType }, color, alpha, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    ColorF m_value = Palette::black;

    Ptr cloneThis() override;
    explicit ColorSliders(Construction construction, ColorF color, bool alpha,
                          ArgumentsView<ColorSliders> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorSliders, ColorF, &ColorSliders::m_value> value;
    BRISK_PROPERTIES_END
};

class WIDGET ColorPalette final : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "colorpalette";

    template <WidgetArgument... Args>
    explicit ColorPalette(Value<ColorF> color, const Args&... args)
        : ColorPalette(Construction{ widgetType }, ColorF{}, std::tuple{ args... }) {
        bindings->connectBidir(Value{ &value }, std::move(color));
    }

    template <WidgetArgument... Args>
    explicit ColorPalette(ColorF color, const Args&... args)
        : ColorPalette(Construction{ widgetType }, std::move(color), std::tuple{ args... }) {}

protected:
    ColorF m_value = Palette::black;

    RC<Widget> addColor(const ColorF& swatch, float brightness = 0.f, float chroma = 1.f);
    Ptr cloneThis() override;
    explicit ColorPalette(Construction construction, ColorF color, ArgumentsView<ColorPalette> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<ColorPalette, ColorF, &ColorPalette::m_value> value;
    BRISK_PROPERTIES_END
};

class WIDGET ColorButton : public PopupButton {
public:
    using Base = PopupButton;
    using Button::widgetType;

    template <WidgetArgument... Args>
    explicit ColorButton(Value<ColorF> prop, bool alpha, const Args&... args)
        : ColorButton(Construction{ widgetType }, std::move(prop), alpha, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() override;

    explicit ColorButton(Construction construction, Value<ColorF> prop, bool alpha,
                         ArgumentsView<ColorButton> args);
};

class WIDGET GradientItem final : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "gradientitem";

    template <WidgetArgument... Args>
    explicit GradientItem(RC<GradientResource> gradient, const Args&... args)
        : Widget(Construction{ widgetType }, args...), gradient(gradient) {
        endConstruction();
    }

    RC<GradientResource> gradient = nullptr;

protected:
    void paint(Canvas& canvas) const override;
    Ptr cloneThis() override;
};

} // namespace Brisk
