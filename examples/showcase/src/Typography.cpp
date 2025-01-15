#include "Typography.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/window/Clipboard.hpp>

namespace Brisk {

static Builder iconsBuilder() {
    return Builder([](Widget* target) {
        constexpr int columns = 16;
        auto iconFontFamily   = GoNoto;
        int iconFontSize      = 25;
        for (int icon = ICON__first; icon < ICON__last; icon += columns) {
            RC<HLayout> glyphs = rcnew HLayout{
                rcnew Text{
                    fmt::format("{:04X}", icon),
                    textVerticalAlign = TextAlign::Center,
                    dimensions        = { 60, 50 },
                },
            };
            for (int c = 0; c < columns; c++) {
                char32_t ch    = icon + c;
                std::string u8 = utf32ToUtf8(std::u32string(1, ch));
                glyphs->apply(rcnew Text{
                    u8,
                    classes           = { "icon" },
                    textAlign         = TextAlign::Center,
                    textVerticalAlign = TextAlign::Center,
                    fontFamily        = iconFontFamily,
                    fontSize          = iconFontSize,
                    dimensions        = { 50, 50 },
                    onClick           = listener(
                        [ch] {
                            copyTextToClipboard(fmt::format("\\u{:04X}", uint32_t(ch)));
                        },
                        &staticBinding),
                });
            }
            target->apply(std::move(glyphs));
        }
    });
}

const std::string pangram = "The quick brown fox jumps over the lazy dog 0123456789";

static const NameValueOrderedList<TextDecoration> textDecorationList{
    { "None", TextDecoration::None },
    { "Underline", TextDecoration::Underline },
    { "Overline", TextDecoration::Overline },
    { "LineThrough", TextDecoration::LineThrough },
};

RC<Widget> ShowcaseTypography::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        rcnew Text{ "Fonts", classes = { "section-header" } },

        rcnew HScrollBox{
            rcnew VLayout{
                flexGrow = 1,
                Builder([](Widget* target) {
                    for (int i = 0; i < 7; ++i) {
                        int size = 8 + i * 4;
                        auto row = [target, size](std::string name, FontFamily family, FontWeight weight) {
                            target->apply(rcnew Text{
                                pangram + fmt::format(" [{}, {}px]", name, size),
                                fontFamily = family,
                                fontWeight = weight,
                                fontSize   = size,
                            });
                        };
                        row("Lato Light", Lato, FontWeight::Light);
                        row("Lato Regular", Lato, FontWeight::Regular);
                        row("Lato Bold", Lato, FontWeight::Bold);
                        row("GoNoto", GoNoto, FontWeight::Regular);
                        row("Monospace", Monospace, FontWeight::Regular);
                        target->apply(rcnew Spacer{ height = 12_apx });
                    }
                }),
            },
        },

        rcnew Text{ "Font properties", classes = { "section-header" } },

        rcnew VLayout{
            rcnew Text{
                "gΥφ fi fl3.14 1/3 LT",
                fontSize       = 40,
                fontFamily     = Lato,
                fontFeatures   = Value{ &m_fontFeatures },
                letterSpacing  = Value{ &m_letterSpacing },
                wordSpacing    = Value{ &m_wordSpacing },
                textDecoration = Value{ &m_textDecoration },
            },
            rcnew HLayout{
                Builder{
                    [this](Widget* target) {
                        for (int i = 0; i < m_fontFeatures.size(); ++i) {
                            target->apply(rcnew VLayout{
                                rcnew Text{ fmt::to_string(m_fontFeatures[i].feature) },
                                rcnew Switch{ value = Value{ &m_fontFeatures[i].enabled } },
                            });
                        }
                    },
                },
            },
            rcnew Text{ "Text decoration" },
            rcnew ComboBox{
                Value{ &m_textDecoration },
                notManaged(&textDecorationList),
                width = 200_apx,
            },
            rcnew Text{ "Letter spacing" },
            rcnew Slider{ value = Value{ &m_letterSpacing }, minimum = 0.f, maximum = 10.f, width = 200_apx },
            rcnew Text{ "Word spacing" },
            rcnew Slider{ value = Value{ &m_wordSpacing }, minimum = 0.f, maximum = 10.f, width = 200_apx },
        },

        rcnew Text{ "Icons (gui/Icons.hpp)", classes = { "section-header" } },

        rcnew VLayout{
            padding = { 8_apx, 8_apx },

            iconsBuilder(),
        },
    };
}
} // namespace Brisk
