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
#include <brisk/widgets/Color.hpp>
#include <brisk/widgets/Slider.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

enum class ColorComp {
    R,
    G,
    B,
    A,
};

constexpr auto operator+(ColorComp value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

template <ColorComp comp>
static Value<float> colorSubvalue(Value<ColorF> color) {
    return color.transform(
        [](ColorF color) -> float {
            if (linearColor)
                return Internal::srgbLinearToGamma(color.array[+comp]);
            else
                return color.array[+comp];
        },
        [](ColorF color, float value) -> ColorF {
            if (linearColor)
                color.array[+comp] = Internal::srgbGammaToLinear(value);
            else
                color.array[+comp] = value;
            return color;
        });
}

ColorSliders::ColorSliders(Construction construction, ColorF color, bool alpha,
                           ArgumentsView<ColorSliders> args)
    : Widget(construction, nullptr), m_value(color) {
    args.apply(this);
    m_layout = Layout::Vertical;

    apply(rcnew Slider{
        Arg::value           = colorSubvalue<ColorComp::R>(Value{ &value }),
        Arg::minDimensions   = { 100, 16 },
        Arg::borderColor     = 0xd80000_rgb,
        Arg::backgroundColor = 0xd80000_rgb,
        Arg::flexGrow        = 1,
        Arg::minimum         = 0,
        Arg::maximum         = 1,
    });
    apply(rcnew Slider{
        Arg::value           = colorSubvalue<ColorComp::G>(Value{ &value }),
        Arg::minDimensions   = { 100, 16 },
        Arg::borderColor     = 0x00b600_rgb,
        Arg::backgroundColor = 0x00b600_rgb,
        Arg::flexGrow        = 1,
        Arg::minimum         = 0,
        Arg::maximum         = 1,
    });
    apply(rcnew Slider{
        Arg::value           = colorSubvalue<ColorComp::B>(Value{ &value }),
        Arg::minDimensions   = { 100, 16 },
        Arg::borderColor     = 0x0000e8_rgb,
        Arg::backgroundColor = 0x0000e8_rgb,
        Arg::flexGrow        = 1,
        Arg::minimum         = 0,
        Arg::maximum         = 1,
    });
    if (alpha) {
        apply(rcnew Slider{
            Arg::value           = colorSubvalue<ColorComp::A>(Value{ &value }),
            Arg::minDimensions   = { 100, 16 },
            Arg::borderColor     = 0xF3F3F3_rgb,
            Arg::backgroundColor = 0xF3F3F3_rgb,
            Arg::flexGrow        = 1,
            Arg::minimum         = 0,
            Arg::maximum         = 1,
        });
    }
}

void ColorView::paint(Canvas& canvas_) const {
    RawCanvas& canvas = canvas_.raw();
    float radius      = horizontalAbsMax(m_borderRadius.resolved.v);
    if (radius == 0)
        radius = 0.001f;
    canvas.drawRectangle(m_rect, radius, 0.f, fillColor = m_value, strokeWidth = 0);
}

#ifdef OKLAB_PALETTE

inline ColorF okLabColor(float hue, float tone) {
    return convertColorSpace<ColorSpace::sRGBGamma>(
        ColorOKLCH(tone * 80.f + 10.f, (1.f - std::abs(tone - 0.5f)) * 50.f, hue * 360.f),
        ColorConversionMode::Nearest);
}

struct OKLabPalette {
    OKLabPalette(int hues, int tones) : hues(hues), tones(tones), colors(hues * tones) {
        for (int hue = 0; hue < hues; ++hue) {
            for (int tone = 0; tone < tones; ++tone) {
                operator()(hue, tone) =
                    okLabColor(static_cast<float>(hue) / hues, static_cast<float>(tone) / (tones - 1));
            }
        }
    }

    int hues;
    int tones;

    ColorF operator()(int hue, int tone) const {
        return colors[hue * tones + tone];
    }

    ColorF& operator()(int hue, int tone) {
        return colors[hue * tones + tone];
    }

    std::vector<ColorF> colors;
};

#endif

ColorPalette::ColorPalette(Construction construction, ColorF color, ArgumentsView<ColorPalette> args)
    : Widget{ construction, nullptr }, m_value(color) {
    args.apply(this);

    m_layout = Layout::Vertical;
#ifdef OKLAB_PALETTE
    static OKLabPalette palette{ 12, 7 };

    for (int tone = 0; tone < palette.tones; ++tone) {
        RC<HLayout> row = rcnew HLayout{
            addColor(ColorOf<float, true>(static_cast<float>(tone) / (palette.tones - 1))),
        };
        for (int hue = 0; hue < palette.hues; ++hue) {
            row->apply(addColor(palette(hue, tone)));
        }
        apply(std::move(row));
    }
#else
    int i   = 0;
    using P = std::pair<float, float>;
    for (std::pair<float, float> p : {
             P{ -67.f, 0.8f },
             P{ -45.f, 0.9f },
             P{ -25.f, 0.95f },
             P{ 0.f, 1.f },
             P{ +25.f, 1.0f },
             P{ +50.f, 1.0f },
             P{ +75.f, 1.0f },
         }) {
        const float v = p.first;
        const float c = p.second;
        apply(rcnew HLayout{
            addColor(ColorOf<float, ColorGamma::sRGB>(i++ / 6.f)), //
            addColor(Palette::Standard::red, v, c), addColor(Palette::Standard::orange, v, c),
            addColor(Palette::Standard::amber, v, c), addColor(Palette::Standard::yellow, v, c),
            addColor(Palette::Standard::lime, v, c), addColor(Palette::Standard::green, v, c),
            addColor(Palette::Standard::teal, v, c), addColor(Palette::Standard::cyan, v, c),
            addColor(Palette::Standard::blue, v, c), addColor(Palette::Standard::indigo, v, c),
            addColor(Palette::Standard::violet, v, c), addColor(Palette::Standard::fuchsia, v, c) });
    }
#endif

    endConstruction();
}

static ColorF adjustSwatch(ColorF color, float lightnessOffset, float chromaMultiplier) {
    ColorOKLAB lab  = static_cast<ColorOKLAB>(color);
    lightnessOffset = std::clamp(lightnessOffset, -100.f, +100.f);
    float lumScale  = 1.f - std::abs(lightnessOffset) / 100.f;
    if (lightnessOffset > 0) // 0..100 ->
        lab[0] = 100.f - (100.f - lab[0]) * lumScale;
    else if (lightnessOffset < 0)
        lab[0] = lab[0] * lumScale;
    lab[1] = lab[1] * chromaMultiplier;
    lab[2] = lab[2] * chromaMultiplier;
    return ColorF(lab, color.alpha);
}

RC<Widget> ColorPalette::addColor(const ColorF& swatch, float brightness, float chroma) {
    ColorF c = adjustSwatch(swatch, brightness, chroma);
    return rcnew Button{
        rcnew Widget{
            rcnew ColorView{
                c,
            },
        },
        Arg::borderRadius = 0,
        Arg::padding      = 0,
        Arg::margin       = 2,
        Arg::onClick      = listener(
            [this, c]() BRISK_INLINE_LAMBDA {
                value = c;
            },
            this),
    };
}

RC<Widget> ColorButton::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> ColorPalette::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> ColorSliders::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RC<Widget> ColorView::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

ColorView::ColorView(Construction construction, ColorF color, ArgumentsView<ColorView> args)
    : Widget(construction, nullptr), m_value(color) {
    args.apply(this);
}

ColorButton::ColorButton(Construction construction, Value<ColorF> prop, bool alpha,
                         ArgumentsView<ColorButton> args)
    : PopupButton(construction, nullptr) {
    args.apply(this);
    m_layout = Layout::Vertical;

    apply(rcnew ColorView{ prop });
    apply(rcnew PopupBox{ rcnew HLayout{ rcnew ColorView{ prop, Arg::classes = { "large" } },
                                         rcnew ColorSliders{ prop, alpha, Arg::flexGrow = 1 } },
                          rcnew ColorPalette(prop) });
}

void GradientItem::paint(Canvas& canvas_) const {
    RawCanvas& canvas = canvas_.raw();
    canvas.drawRectangle(
        m_rect, 0.f, 0.f, fillColors = GradientColors{ Palette::red, Palette::yellow },
        linearGradient = GradientPoints{ RectangleF(m_rect).at(0.f, 0.5f), RectangleF(m_rect).at(1.f, 0.5f) },
        strokeWidth = 0, multigradient = gradient);
}

RC<Widget> GradientItem::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}
} // namespace Brisk
