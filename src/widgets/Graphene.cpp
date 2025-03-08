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
#include <brisk/widgets/Graphene.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

namespace Graphene {

constexpr inline double defaultShadowSize = 25;

Rules darkColors() {
    return Rules{
        mainColor = 0x292E38_rgb,      windowColor = 0x131419_rgb, selectedColor = (0x1976E8_rgb).adjust(0),
        linkColor = 0x378AFF_rgb,      editorColor = 0xFDFDFD_rgb, boxRadius = -5.f,
        menuColor = 0xFDFDFD_rgb,      animationSpeed = 1.f,       boxBorderColor = 0x000000'A0_rgba,
        shadeColor = 0x000000'88_rgba, deepColor = 0x000000_rgb,
    };
}

Rules lightColors() {
    return {
        mainColor = 0xEDF1F7_rgb,      windowColor = 0xFAFAFA_rgb, selectedColor = (0x1976E8_rgb).adjust(8),
        linkColor = 0x004DB8_rgb,      editorColor = 0xFDFDFD_rgb, boxRadius = -5.f,
        menuColor = 0xFDFDFD_rgb,      animationSpeed = 1.f,       boxBorderColor = 0x000000'1F_rgba,
        shadeColor = 0x000000'54_rgba, deepColor = 0x8D8D8D_rgb,
    };
}

RC<const Stylesheet> stylesheet() {
    using namespace Selectors;
    using enum WidgetState;

    constexpr Color textLightColor = 0xF0F0F0_rgb;
    constexpr Color textDarkColor  = 0x111111_rgb;

    return rcnew Stylesheet{
        Style{
            Root{},
            {
                backgroundColor    = styleVar<windowColor>,
                color              = textColorFor(styleVar<windowColor>, textLightColor, textDarkColor),
                scrollBarColor     = styleVar<selectedColor>,
                scrollBarThickness = 8_apx,
                scrollBarRadius    = 4_px,
            },
        },
        Style{
            Class{ "toolbar" },
            {
                layout     = Layout::Horizontal,
                alignItems = AlignItems::Stretch,
                gap        = { 8_apx, 8_apx },
            },
        },
        Style{
            Class{ "hotkeyhint" },
            {
                opacity    = 0.75f,
                fontSize   = FontSize::Normal - 1,
                marginLeft = 16_apx,
            },
        },
        Style{
            Type{ CheckBox::widgetType },
            {
                paddingLeft = 18_apx,
            },
        },
        Style{
            Type{ Switch::widgetType },
            {
                paddingLeft   = 28_apx,
                minDimensions = { 40_apx, 20_apx },
            },
        },
        Style{
            Type{ RadioButton::widgetType },
            {
                paddingLeft = 18_apx,
            },
        },
        Style{
            Type{ Dot::widgetType },
            {
                dimensions       = { 4_apx, 4_apx },
                borderRadius     = 100.f,
                margin           = { 3_apx, 3_apx },
                backgroundColor  = Palette::Standard::green,
                placement        = Placement::Absolute,
                zorder           = ZOrder::TopMost,
                anchor           = { 0, 0 },
                absolutePosition = { 0, 0 },
            },
        },
        Style{
            Type{ "itemlist" },
            {
                padding         = { 0, 4_apx },
                backgroundColor = styleVar<menuColor>,
                shadowSize      = defaultShadowSize,
                shadowColor     = styleVar<shadeColor>,
                borderRadius    = styleVar<boxRadius>,
                alignItems      = AlignItems::Stretch,
                color           = textColorFor(styleVar<menuColor>, textLightColor, textDarkColor),
                textAlign       = TextAlign::Start,
            },
        },
        Style{
            Type{ "listbox" },
            {
                padding         = { 0, 4_apx },
                backgroundColor = styleVar<menuColor>,
                borderRadius    = styleVar<boxRadius>,
                alignItems      = AlignItems::Stretch,
                color           = textColorFor(styleVar<menuColor>, textLightColor, textDarkColor),
                textAlign       = TextAlign::Start,
            },
        },
        Style{
            Type{ Knob::widgetType },
            {
                minDimensions                   = { 22_apx, 22_apx },
                borderRadius                    = 50_apx,
                backgroundColor                 = styleVar<deepColor>,
                borderColor                     = styleVar<selectedColor>,
                borderColor | Hover             = adjustColor(styleVar<selectedColor>, +8),
                borderColor | Pressed           = adjustColor(styleVar<selectedColor>, -8),
                borderColorTransition           = scaleValue(styleVar<animationSpeed>, 0.25),
                borderColorTransition | Hover   = scaleValue(styleVar<animationSpeed>, 0.15),
                borderColorTransition | Pressed = scaleValue(styleVar<animationSpeed>, 0.02),
            },
        },
        Style{
            Type{ Item::widgetType },
            {
                padding                              = EdgesL(32_apx, 5_apx, 16_apx, 5_apx),
                textAlign                            = TextAlign::Start,

                backgroundColor                      = Palette::transparent,
                backgroundColor | Selected           = transparency(styleVar<selectedColor>, 0.8f),
                backgroundColor | Selected | Pressed = styleVar<selectedColor>,

                color                                = inherit,
                color | Selected =
                    textColorFor(transparency(styleVar<selectedColor>, 0.8f), textLightColor, textDarkColor),
                color | Selected | Pressed =
                    textColorFor(styleVar<selectedColor>, textLightColor, textDarkColor),
            },
        },
        Style{
            Type{ ItemList::widgetType } > Type{ Item::widgetType },
            {
                padding = { 16_apx, 5_apx },
            },
        },
        Style{
            Type{ Item::widgetType } && Role("selecteditem"),
            {
                backgroundColor                      = Palette::transparent,
                backgroundColor | Selected           = Palette::transparent,
                backgroundColor | Selected | Pressed = Palette::transparent,
                color                                = inherit,
                color | Selected                     = inherit,
                color | Selected | Pressed           = inherit,
                padding                              = { 5_apx, 5_apx },
            },
        },
        Style{
            Type{ Hyperlink::widgetType } > Type{ Text::widgetType },
            {
                colorTransition = scaleValue(styleVar<animationSpeed>, 0.1),
                color           = styleVar<linkColor>,
                color | Hover   = adjustColor(styleVar<linkColor>, +50 * 0.2f /* 1.7f */),
                color | Pressed = adjustColor(styleVar<linkColor>, -30 * 0.2f /* 0.7f */),
            },
        },
        Style{
            Type{ TextEditor::widgetType },
            {
                backgroundColor           = styleVar<editorColor>,
                backgroundColor | Focused = adjustColor(styleVar<editorColor>, +20 * 0.2f /* 1.2f */),
                backgroundColor | Hover   = adjustColor(styleVar<editorColor>, +20 * 0.2f /* 1.2f */),
                backgroundColorTransition = scaleValue(styleVar<animationSpeed>, 0.25),
                backgroundColorTransition | Focused = scaleValue(styleVar<animationSpeed>, 0.05),
                backgroundColorTransition | Hover   = scaleValue(styleVar<animationSpeed>, 0.15),
                borderColor                         = 0xC0C0C0_rgb,
                borderColor | Focused               = 0xE0E0E0_rgb,
                borderColor | Hover                 = 0xE0E0E0_rgb,
                borderRadius                        = scaleValue(styleVar<boxRadius>, 0.5f),
                borderWidth                         = 1.f,
                color                               = Palette::black,
                padding                             = { 7_apx, 6_apx },
                cursor                              = Cursor::IBeam,
                height                              = 1_em,
            },
        },
        Style{
            Type{ "tabbutton" },
            {
                backgroundColor           = Palette::transparent, //
                backgroundColorTransition = scaleValue(styleVar<animationSpeed>, 0.1),
                borderColor               = styleVar<selectedColor>,
                borderRadius              = 0.f, //
                borderWidth               = 0,
                borderWidth | Selected    = { 0, 0, 0, 2_apx },
                color                     = textColorFor(styleVar<mainColor>, textLightColor, textDarkColor),
                color | Hover = adjustColor(textColorFor(styleVar<mainColor>, textLightColor, textDarkColor),
                                            +20 * 0.2f /* 1.2f */),
                color | Pressed = adjustColor(
                    textColorFor(styleVar<mainColor>, textLightColor, textDarkColor), -30 * 0.2f /* 0.7f */),
                color | Selected = adjustColor(
                    textColorFor(styleVar<mainColor>, textLightColor, textDarkColor), +20 * 0.2f /* 1.2f */),
                justifyContent             = Justify::SpaceAround,
                padding                    = { 16_apx, 5_apx, 16_apx, 5_apx },
                padding | Selected         = { 16_apx, 5_apx, 16_apx, 3_apx },
                textAlign                  = TextAlign::Center,
                color | Disabled           = 0x808080_rgb,
                backgroundColor | Disabled = adjustColor(styleVar<mainColor>, +2.f, 0.f),
            },
        },
        Style{
            Type{ "switch" },
            {
                backgroundColor = styleVar<selectedColor>,
                borderColor     = styleVar<boxBorderColor>,
                borderWidth     = 1.f,
            },
        },
        Style{
            Type{ "tabs" },
            {
                backgroundColor = styleVar<windowColor>,
                corners         = +CornerFlags::Top,
                borderRadius    = styleVar<boxRadius>,
                layout          = Layout::Horizontal,
            },
        },
        Style{
            Type{ "hint" },
            {
                backgroundColor  = 0xFFE9AD_rgb,
                padding          = { 6_apx, 3_apx },
                absolutePosition = { 0.f, 100_perc },
                anchor           = { 0.f, 0.f },
                color            = Palette::black,
                textAlign        = TextAlign::Center,
                borderRadius     = scaleValue(styleVar<boxRadius>, 0.5f),
                fontSize         = FontSize::Small,
            },
        },
        Style{
            Type{ "notifications" },
            {
                layout           = Layout::Vertical,
                placement        = Placement::Absolute,
                zorder           = ZOrder::TopMost,
                absolutePosition = { 0_perc, 0 },
                anchor           = { 0_perc, 0_perc },
                height           = 100_perc,
                minWidth         = 360_apx,
                alignItems       = AlignItems::FlexStart,
                gapRow           = 15_apx,
                mouseInteraction = MouseInteraction::Disable,
                justifyContent   = Justify::FlexEnd,
            },
        },
        Style{
            Type{ "notification" },
            {
                backgroundColor  = adjustColor(styleVar<windowColor>, +20 * 0.2f),
                shadowSize       = defaultShadowSize,
                shadowColor      = styleVar<shadeColor>,
                borderRadius     = 5_px,
                minHeight        = 32_apx,
                mouseInteraction = MouseInteraction::Enable,
            },
        },
        Style{
            Class{ "notification-body" },
            {
                padding = { 20_apx, 12_apx, 36_apx, 12_apx },
            },
        },
        Style{
            Type{ Slider::widgetType },
            {
                borderColor                     = styleVar<selectedColor>,
                borderColor | Hover             = adjustColor(styleVar<selectedColor>, +8),
                borderColor | Pressed           = adjustColor(styleVar<selectedColor>, -8),
                borderColorTransition           = scaleValue(styleVar<animationSpeed>, 0.25),
                borderColorTransition | Hover   = scaleValue(styleVar<animationSpeed>, 0.15),
                borderColorTransition | Pressed = scaleValue(styleVar<animationSpeed>, 0.02),
                backgroundColor                 = transparency(styleVar<selectedColor>, 0.65f),
                minDimensions                   = { 15_apx, 15_apx },
            },
        },
        Style{
            Type{ ColorView::widgetType },
            {
                minDimensions = { 1.2_em, 1.2_em },
            },
        },
        Style{
            Type{ ColorView::widgetType } && Class{ "large" },
            {
                minDimensions = { 60_apx, 60_apx },
                margin        = { 0, 0, 8_apx, 8_apx },
                borderRadius  = scaleValue(styleVar<boxRadius>, 2.f),
            },
        },
        Style{
            Type{ "popupbox" },
            {
                padding          = { 7_apx, 5_apx },
                backgroundColor  = styleVar<windowColor>,
                borderRadius     = styleVar<boxRadius>,
                absolutePosition = { 0, 100_perc },
                anchor           = { 0, 0 },
                shadowSize       = defaultShadowSize,
                shadowColor      = styleVar<shadeColor>,
                color            = textColorFor(styleVar<windowColor>, textLightColor, textDarkColor),
                layout           = Layout::Vertical,
            },
        },
        Style{
            Class{ "menubox" } || Role{ "context" } || Role{ "itemlist" },
            {
                padding          = { 0, 4_apx },
                backgroundColor  = styleVar<menuColor>,
                borderRadius     = styleVar<boxRadius>,
                absolutePosition = { 0, 100_perc },
                anchor           = { 0, 0 },
                shadowSize       = defaultShadowSize,
                shadowColor      = styleVar<shadeColor>,
                color            = textColorFor(styleVar<menuColor>, textLightColor, textDarkColor),
            },
        },
        Style{
            Type{ "menuline" },
            {
                padding = EdgesL(32_apx, 50_apx, 16_apx, 5_apx),
                opacity = 0.35,
            },
        },
        Style{
            Type{ "vline" },
            {
                width = 5_apx,
            },
        },
        Style{
            Type{ "hline" },
            {
                height = 5_apx,
            },
        },
        Style{
            Type{ "guide" },
            {
                borderRadius     = styleVar<boxRadius>,
                shadowSize       = defaultShadowSize,
                shadowColor      = styleVar<shadeColor>,
                absolutePosition = { 50_perc, 50_perc },
                anchor           = { 100_perc, 0 },
                fontSize         = FontSize::Bigger,
                backgroundColor  = Palette::Standard::amber,
                padding          = { 24_apx, 8_apx },
                color            = Palette::black,
            },
        },
        Style{
            Type{ "progress" },
            {},
        },
        Style{
            Type{ "progressbar" },
            {
                minDimensions    = { 5_apx, 5_apx },
                placement        = Placement::Absolute,
                absolutePosition = { 0, 0 },
                anchor           = { 0, 0 },
                dimensions       = { 0, 100_perc },
                backgroundColor  = styleVar<selectedColor>,
            },
        },
        Style{
            Type{ "popupdialog" },
            {
                backgroundColor = styleVar<boxBorderColor>,
            },
        },
        Style{
            Class{ "dialog" },
            {
                shadowSize      = defaultShadowSize,
                shadowColor     = styleVar<shadeColor>,
                backgroundColor = styleVar<windowColor>,
                borderRadius    = styleVar<boxRadius>,
                borderColor     = styleVar<mainColor>,
                borderWidth     = 1_apx,
                minWidth        = 180_apx,
            },
        },
        Style{
            Class{ "dialog-title" },
            {
                fontWeight      = FontWeight::Bold,
                textAlign       = TextAlign::Center,
                backgroundColor = styleVar<mainColor>,
            },
        },
        Style{
            Class{ "dialog-body" },
            {
                padding = { 4_apx, 4_apx },
            },
        },
        Style{
            Class{ "dialog-button" },
            {
                marginTop = 4_apx,
            },
        },
        Style{
            Type{ "page" },
            {},
        },
        Style{
            Class{ "header" },
            {
                fontSize     = FontSize::Bigger,
                fontFamily   = Font::DefaultPlusIconsEmoji,
                fontWeight   = FontWeight::Bold,
                color        = ColorF(0.4f, 0.4f),
                marginTop    = 8_apx,
                marginBottom = 6_apx,
                textAlign    = TextAlign::Start,
            },
        },
        Style{
            Class{ "dialog-header" },
            {
                fontFamily = Font::DefaultPlusIconsEmoji,
                fontWeight = FontWeight::Bold,
                padding    = { 8_apx, 4_apx },
            },
        },
        Style{
            Class{ "dialog-title" },
            {
                padding = { 12_apx, 8_apx },
            },
        },
        Style{
            Class{ "dialog-body" },
            {
                padding = { 8_apx, 8_apx },
            },
        },
        Style{
            Type{ SpinBox::widgetType },
            {
                borderColor                = styleVar<boxBorderColor>,
                borderRadius               = styleVar<boxRadius>, //
                borderWidth                = 1.f,
                padding                    = EdgesL{ 0, 0 },
                backgroundColor            = styleVar<mainColor>,
                color                      = textColorFor(styleVar<mainColor>, textLightColor, textDarkColor),
                color | Disabled           = 0x808080_rgb,
                backgroundColor | Disabled = adjustColor(styleVar<mainColor>, +2.f, 0.f),
            },
        },
        Style{
            Type{ SpinBox::widgetType } > Role{ "display" },
            {
                padding = EdgesL{ 5_apx, 5_apx },
            },
        },
        Style{
            Type{ Button::widgetType },
            {
                justifyContent = Justify::SpaceAround,
                textAlign      = TextAlign::Center,
            },
        },
        Style{
            Type{ Button::widgetType } || Type{ ComboBox::widgetType },
            {
                backgroundColorTransition           = scaleValue(styleVar<animationSpeed>, 0.25),
                backgroundColorTransition | Hover   = scaleValue(styleVar<animationSpeed>, 0.15),
                backgroundColorTransition | Pressed = scaleValue(styleVar<animationSpeed>, 0.02),
                borderColor                         = styleVar<boxBorderColor>,
                borderRadius                        = styleVar<boxRadius>, //
                borderWidth                         = 1.f,
                colorTransition                     = scaleValue(styleVar<animationSpeed>, 0.25),
                colorTransition | Hover             = scaleValue(styleVar<animationSpeed>, 0.15),
                colorTransition | Pressed           = scaleValue(styleVar<animationSpeed>, 0.02),
                padding                             = { 16_apx, 5_apx },

                color            = textColorFor(styleVar<mainColor>, textLightColor, textDarkColor),
                color | Selected = textColorFor(styleVar<selectedColor>, textLightColor, textDarkColor),
                color | Disabled = 0x808080_rgb,
                backgroundColor  = styleVar<mainColor>,
                backgroundColor | Hover    = adjustColor(styleVar<mainColor>, +50 * 0.2f /* 1.7f */),
                backgroundColor | Pressed  = adjustColor(styleVar<mainColor>, -40 * 0.2f /* 0.6f */),
                backgroundColor | Selected = styleVar<selectedColor>,
                backgroundColor | Selected | Hover =
                    adjustColor(styleVar<selectedColor>, +50 * 0.2f /* 1.7f */),
                backgroundColor | Selected | Pressed =
                    adjustColor(styleVar<selectedColor>, -40 * 0.2f /* 0.6f */),
                backgroundColor | Disabled = adjustColor(styleVar<mainColor>, +2.f, 0.f),
            },
        },
        Style{
            Type{ UpDownButtons::widgetType } > Type{ Button::widgetType },
            {
                padding         = EdgesL{ 3_apx, 3_apx },
                borderWidth     = 0,
                backgroundColor = Palette::transparent,
            },
        },
        Style{
            Type{ "combobox" },
            {
                padding = { 0, 0 },
            },
        },
        Style{
            Type{ "combobox" } > Type{ "button" },
            {
                padding                              = { 5_apx, 5_apx },
                borderWidth                          = 0.f,
                backgroundColor                      = Palette::transparent,
                backgroundColor | Hover              = Palette::transparent,
                backgroundColor | Pressed            = Palette::transparent,
                backgroundColor | Selected           = Palette::transparent,
                backgroundColor | Selected | Hover   = Palette::transparent,
                backgroundColor | Selected | Pressed = Palette::transparent,
            },
        },
        Style{
            Class{ "slim" },
            {
                padding = { 7_apx, 5_apx },
            },
        },
        Style{
            Class{ "square" },
            {
                padding = { 6_apx, 6_apx },
            },
        },
        Style{
            Class{ "flat" },
            {
                borderWidth               = 0.f,
                backgroundColor           = Palette::transparent,
                backgroundColor | Hover   = 0xe0e0e0'60_rgba,
                backgroundColor | Pressed = 0x000000'50_rgba,
            },
        },
        Style{ Class{ "success" }, { mainColor = Palette::Standard::green } },
        Style{ Class{ "warning" }, { mainColor = Palette::Standard::yellow } },
        Style{ Class{ "danger" }, { mainColor = Palette::Standard::red } },
        Style{ Class{ "info" }, { mainColor = Palette::Standard::blue } },

        Style{ Class{ "white" }, { backgroundColor = Palette::white } },
        Style{ Class{ "black" }, { backgroundColor = Palette::black } },
        Style{ Class{ "red" }, { backgroundColor = Palette::Standard::red } },
        Style{ Class{ "orange" }, { backgroundColor = Palette::Standard::orange } },
        Style{ Class{ "amber" }, { backgroundColor = Palette::Standard::amber } },
        Style{ Class{ "yellow" }, { backgroundColor = Palette::Standard::yellow } },
        Style{ Class{ "lime" }, { backgroundColor = Palette::Standard::lime } },
        Style{ Class{ "green" }, { backgroundColor = Palette::Standard::green } },
        Style{ Class{ "teal" }, { backgroundColor = Palette::Standard::teal } },
        Style{ Class{ "cyan" }, { backgroundColor = Palette::Standard::cyan } },
        Style{ Class{ "blue" }, { backgroundColor = Palette::Standard::blue } },
        Style{ Class{ "indigo" }, { backgroundColor = Palette::Standard::indigo } },
        Style{ Class{ "violet" }, { backgroundColor = Palette::Standard::violet } },
        Style{ Class{ "fuchsia" }, { backgroundColor = Palette::Standard::fuchsia } },
    };
}
} // namespace Graphene

} // namespace Brisk
