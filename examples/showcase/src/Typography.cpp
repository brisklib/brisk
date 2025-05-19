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
#include "Typography.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/window/Clipboard.hpp>

namespace Brisk {

constexpr std::initializer_list<Range<char32_t, true>> emojis{
    { 0x231A, 0x231B },   { 0x23E9, 0x23EC },   { 0x23F0, 0x23F0 },   { 0x23F3, 0x23F3 },
    { 0x25FD, 0x25FE },   { 0x2614, 0x2615 },   { 0x2648, 0x2653 },   { 0x267F, 0x267F },
    { 0x2693, 0x2693 },   { 0x26A1, 0x26A1 },   { 0x26AA, 0x26AB },   { 0x26BD, 0x26BE },
    { 0x26C4, 0x26C5 },   { 0x26CE, 0x26CE },   { 0x26D4, 0x26D4 },   { 0x26EA, 0x26EA },
    { 0x26F2, 0x26F3 },   { 0x26F5, 0x26F5 },   { 0x26FA, 0x26FA },   { 0x26FD, 0x26FD },
    { 0x2705, 0x2705 },   { 0x270A, 0x270B },   { 0x2728, 0x2728 },   { 0x274C, 0x274C },
    { 0x274E, 0x274E },   { 0x2753, 0x2755 },   { 0x2757, 0x2757 },   { 0x2795, 0x2797 },
    { 0x27B0, 0x27B0 },   { 0x27BF, 0x27BF },   { 0x2B1B, 0x2B1C },   { 0x2B50, 0x2B50 },
    { 0x2B55, 0x2B55 },   { 0x1F004, 0x1F004 }, { 0x1F0CF, 0x1F0CF }, { 0x1F18E, 0x1F18E },
    { 0x1F191, 0x1F19A }, { 0x1F201, 0x1F201 }, { 0x1F21A, 0x1F21A }, { 0x1F22F, 0x1F22F },
    { 0x1F232, 0x1F236 }, { 0x1F238, 0x1F23A }, { 0x1F250, 0x1F251 }, { 0x1F300, 0x1F30C },
    { 0x1F30D, 0x1F30E }, { 0x1F30F, 0x1F30F }, { 0x1F310, 0x1F310 }, { 0x1F311, 0x1F311 },
    { 0x1F312, 0x1F312 }, { 0x1F313, 0x1F315 }, { 0x1F316, 0x1F318 }, { 0x1F319, 0x1F319 },
    { 0x1F31A, 0x1F31A }, { 0x1F31B, 0x1F31B }, { 0x1F31C, 0x1F31C }, { 0x1F31D, 0x1F31E },
    { 0x1F31F, 0x1F320 }, { 0x1F32D, 0x1F32F }, { 0x1F330, 0x1F331 }, { 0x1F332, 0x1F333 },
    { 0x1F334, 0x1F335 }, { 0x1F337, 0x1F34A }, { 0x1F34B, 0x1F34B }, { 0x1F34C, 0x1F34F },
    { 0x1F350, 0x1F350 }, { 0x1F351, 0x1F37B }, { 0x1F37C, 0x1F37C }, { 0x1F37E, 0x1F37F },
    { 0x1F380, 0x1F393 }, { 0x1F3A0, 0x1F3C4 }, { 0x1F3C5, 0x1F3C5 }, { 0x1F3C6, 0x1F3C6 },
    { 0x1F3C7, 0x1F3C7 }, { 0x1F3C8, 0x1F3C8 }, { 0x1F3C9, 0x1F3C9 }, { 0x1F3CA, 0x1F3CA },
    { 0x1F3CF, 0x1F3D3 }, { 0x1F3E0, 0x1F3E3 }, { 0x1F3E4, 0x1F3E4 }, { 0x1F3E5, 0x1F3F0 },
    { 0x1F3F4, 0x1F3F4 }, { 0x1F3F8, 0x1F407 }, { 0x1F408, 0x1F408 }, { 0x1F409, 0x1F40B },
    { 0x1F40C, 0x1F40E }, { 0x1F40F, 0x1F410 }, { 0x1F411, 0x1F412 }, { 0x1F413, 0x1F413 },
    { 0x1F414, 0x1F414 }, { 0x1F415, 0x1F415 }, { 0x1F416, 0x1F416 }, { 0x1F417, 0x1F429 },
    { 0x1F42A, 0x1F42A }, { 0x1F42B, 0x1F43E }, { 0x1F440, 0x1F440 }, { 0x1F442, 0x1F464 },
    { 0x1F465, 0x1F465 }, { 0x1F466, 0x1F46B }, { 0x1F46C, 0x1F46D }, { 0x1F46E, 0x1F4AC },
    { 0x1F4AD, 0x1F4AD }, { 0x1F4AE, 0x1F4B5 }, { 0x1F4B6, 0x1F4B7 }, { 0x1F4B8, 0x1F4EB },
    { 0x1F4EC, 0x1F4ED }, { 0x1F4EE, 0x1F4EE }, { 0x1F4EF, 0x1F4EF }, { 0x1F4F0, 0x1F4F4 },
    { 0x1F4F5, 0x1F4F5 }, { 0x1F4F6, 0x1F4F7 }, { 0x1F4F8, 0x1F4F8 }, { 0x1F4F9, 0x1F4FC },
    { 0x1F4FF, 0x1F502 }, { 0x1F503, 0x1F503 }, { 0x1F504, 0x1F507 }, { 0x1F508, 0x1F508 },
    { 0x1F509, 0x1F509 }, { 0x1F50A, 0x1F514 }, { 0x1F515, 0x1F515 }, { 0x1F516, 0x1F52B },
    { 0x1F52C, 0x1F52D }, { 0x1F52E, 0x1F53D }, { 0x1F54B, 0x1F54E }, { 0x1F550, 0x1F55B },
    { 0x1F55C, 0x1F567 }, { 0x1F57A, 0x1F57A }, { 0x1F595, 0x1F596 }, { 0x1F5A4, 0x1F5A4 },
    { 0x1F5FB, 0x1F5FF }, { 0x1F600, 0x1F600 }, { 0x1F601, 0x1F606 }, { 0x1F607, 0x1F608 },
    { 0x1F609, 0x1F60D }, { 0x1F60E, 0x1F60E }, { 0x1F60F, 0x1F60F }, { 0x1F610, 0x1F610 },
    { 0x1F611, 0x1F611 }, { 0x1F612, 0x1F614 }, { 0x1F615, 0x1F615 }, { 0x1F616, 0x1F616 },
    { 0x1F617, 0x1F617 }, { 0x1F618, 0x1F618 }, { 0x1F619, 0x1F619 }, { 0x1F61A, 0x1F61A },
    { 0x1F61B, 0x1F61B }, { 0x1F61C, 0x1F61E }, { 0x1F61F, 0x1F61F }, { 0x1F620, 0x1F625 },
    { 0x1F626, 0x1F627 }, { 0x1F628, 0x1F62B }, { 0x1F62C, 0x1F62C }, { 0x1F62D, 0x1F62D },
    { 0x1F62E, 0x1F62F }, { 0x1F630, 0x1F633 }, { 0x1F634, 0x1F634 }, { 0x1F635, 0x1F635 },
    { 0x1F636, 0x1F636 }, { 0x1F637, 0x1F640 }, { 0x1F641, 0x1F644 }, { 0x1F645, 0x1F64F },
    { 0x1F680, 0x1F680 }, { 0x1F681, 0x1F682 }, { 0x1F683, 0x1F685 }, { 0x1F686, 0x1F686 },
    { 0x1F687, 0x1F687 }, { 0x1F688, 0x1F688 }, { 0x1F689, 0x1F689 }, { 0x1F68A, 0x1F68B },
    { 0x1F68C, 0x1F68C }, { 0x1F68D, 0x1F68D }, { 0x1F68E, 0x1F68E }, { 0x1F68F, 0x1F68F },
    { 0x1F690, 0x1F690 }, { 0x1F691, 0x1F693 }, { 0x1F694, 0x1F694 }, { 0x1F695, 0x1F695 },
    { 0x1F696, 0x1F696 }, { 0x1F697, 0x1F697 }, { 0x1F698, 0x1F698 }, { 0x1F699, 0x1F69A },
    { 0x1F69B, 0x1F6A1 }, { 0x1F6A2, 0x1F6A2 }, { 0x1F6A3, 0x1F6A3 }, { 0x1F6A4, 0x1F6A5 },
    { 0x1F6A6, 0x1F6A6 }, { 0x1F6A7, 0x1F6AD }, { 0x1F6AE, 0x1F6B1 }, { 0x1F6B2, 0x1F6B2 },
    { 0x1F6B3, 0x1F6B5 }, { 0x1F6B6, 0x1F6B6 }, { 0x1F6B7, 0x1F6B8 }, { 0x1F6B9, 0x1F6BE },
    { 0x1F6BF, 0x1F6BF }, { 0x1F6C0, 0x1F6C0 }, { 0x1F6C1, 0x1F6C5 }, { 0x1F6CC, 0x1F6CC },
    { 0x1F6D0, 0x1F6D0 }, { 0x1F6D1, 0x1F6D2 }, { 0x1F6D5, 0x1F6D5 }, { 0x1F6D6, 0x1F6D7 },
    { 0x1F6DC, 0x1F6DC }, { 0x1F6DD, 0x1F6DF }, { 0x1F6EB, 0x1F6EC }, { 0x1F6F4, 0x1F6F6 },
    { 0x1F6F7, 0x1F6F8 }, { 0x1F6F9, 0x1F6F9 }, { 0x1F6FA, 0x1F6FA }, { 0x1F6FB, 0x1F6FC },
    { 0x1F7E0, 0x1F7EB }, { 0x1F7F0, 0x1F7F0 }, { 0x1F90C, 0x1F90C }, { 0x1F90D, 0x1F90F },
    { 0x1F910, 0x1F918 }, { 0x1F919, 0x1F91E }, { 0x1F91F, 0x1F91F }, { 0x1F920, 0x1F927 },
    { 0x1F928, 0x1F92F }, { 0x1F930, 0x1F930 }, { 0x1F931, 0x1F932 }, { 0x1F933, 0x1F93A },
    { 0x1F93C, 0x1F93E }, { 0x1F93F, 0x1F93F }, { 0x1F940, 0x1F945 }, { 0x1F947, 0x1F94B },
    { 0x1F94C, 0x1F94C }, { 0x1F94D, 0x1F94F }, { 0x1F950, 0x1F95E }, { 0x1F95F, 0x1F96B },
    { 0x1F96C, 0x1F970 }, { 0x1F971, 0x1F971 }, { 0x1F972, 0x1F972 }, { 0x1F973, 0x1F976 },
    { 0x1F977, 0x1F978 }, { 0x1F979, 0x1F979 }, { 0x1F97A, 0x1F97A }, { 0x1F97B, 0x1F97B },
    { 0x1F97C, 0x1F97F }, { 0x1F980, 0x1F984 }, { 0x1F985, 0x1F991 }, { 0x1F992, 0x1F997 },
    { 0x1F998, 0x1F9A2 }, { 0x1F9A3, 0x1F9A4 }, { 0x1F9A5, 0x1F9AA }, { 0x1F9AB, 0x1F9AD },
    { 0x1F9AE, 0x1F9AF }, { 0x1F9B0, 0x1F9B9 }, { 0x1F9BA, 0x1F9BF }, { 0x1F9C0, 0x1F9C0 },
    { 0x1F9C1, 0x1F9C2 }, { 0x1F9C3, 0x1F9CA }, { 0x1F9CB, 0x1F9CB }, { 0x1F9CC, 0x1F9CC },
    { 0x1F9CD, 0x1F9CF }, { 0x1F9D0, 0x1F9E6 }, { 0x1F9E7, 0x1F9FF }, { 0x1FA70, 0x1FA73 },
    { 0x1FA74, 0x1FA74 }, { 0x1FA75, 0x1FA77 }, { 0x1FA78, 0x1FA7A }, { 0x1FA7B, 0x1FA7C },
    { 0x1FA80, 0x1FA82 }, { 0x1FA83, 0x1FA86 }, { 0x1FA87, 0x1FA88 }, { 0x1FA89, 0x1FA89 },
    { 0x1FA8F, 0x1FA8F }, { 0x1FA90, 0x1FA95 }, { 0x1FA96, 0x1FAA8 }, { 0x1FAA9, 0x1FAAC },
    { 0x1FAAD, 0x1FAAF }, { 0x1FAB0, 0x1FAB6 }, { 0x1FAB7, 0x1FABA }, { 0x1FABB, 0x1FABD },
    { 0x1FABE, 0x1FABE }, { 0x1FABF, 0x1FABF }, { 0x1FAC0, 0x1FAC2 }, { 0x1FAC3, 0x1FAC5 },
    { 0x1FAC6, 0x1FAC6 }, { 0x1FACE, 0x1FACF }, { 0x1FAD0, 0x1FAD6 }, { 0x1FAD7, 0x1FAD9 },
    { 0x1FADA, 0x1FADB }, { 0x1FADC, 0x1FADC }, { 0x1FADF, 0x1FADF }, { 0x1FAE0, 0x1FAE7 },
    { 0x1FAE8, 0x1FAE8 }, { 0x1FAE9, 0x1FAE9 }, { 0x1FAF0, 0x1FAF6 }, { 0x1FAF7, 0x1FAF8 },
};

constexpr std::initializer_list<char32_t> emojis2{
    0x00A9,  0x00AE,  0x203C,  0x2049,  0x2122,  0x2139,  0x2194,  0x2195,  0x2196,  0x2197,  0x2198,
    0x2199,  0x21A9,  0x21AA,  0x2328,  0x23CF,  0x23ED,  0x23EE,  0x23EF,  0x23F1,  0x23F2,  0x23F8,
    0x23F9,  0x23FA,  0x24C2,  0x25AA,  0x25AB,  0x25B6,  0x25C0,  0x25FB,  0x25FC,  0x2600,  0x2601,
    0x2602,  0x2603,  0x2604,  0x260E,  0x2611,  0x2618,  0x261D,  0x2620,  0x2622,  0x2623,  0x2626,
    0x262A,  0x262E,  0x262F,  0x2638,  0x2639,  0x263A,  0x2640,  0x2642,  0x265F,  0x2660,  0x2663,
    0x2665,  0x2666,  0x2668,  0x267B,  0x267E,  0x2692,  0x2694,  0x2695,  0x2696,  0x2697,  0x2699,
    0x269B,  0x269C,  0x26A0,  0x26A7,  0x26B0,  0x26B1,  0x26C8,  0x26CF,  0x26D1,  0x26D3,  0x26E9,
    0x26F0,  0x26F1,  0x26F4,  0x26F7,  0x26F8,  0x26F9,  0x2702,  0x2708,  0x2709,  0x270C,  0x270D,
    0x270F,  0x2712,  0x2714,  0x2716,  0x271D,  0x2721,  0x2733,  0x2734,  0x2744,  0x2747,  0x2763,
    0x2764,  0x27A1,  0x2934,  0x2935,  0x2B05,  0x2B06,  0x2B07,  0x3030,  0x303D,  0x3297,  0x3299,
    0x1F170, 0x1F171, 0x1F17E, 0x1F17F, 0x1F202, 0x1F237, 0x1F321, 0x1F324, 0x1F325, 0x1F326, 0x1F327,
    0x1F328, 0x1F329, 0x1F32A, 0x1F32B, 0x1F32C, 0x1F336, 0x1F37D, 0x1F396, 0x1F397, 0x1F399, 0x1F39A,
    0x1F39B, 0x1F39E, 0x1F39F, 0x1F3CB, 0x1F3CC, 0x1F3CD, 0x1F3CE, 0x1F3D4, 0x1F3D5, 0x1F3D6, 0x1F3D7,
    0x1F3D8, 0x1F3D9, 0x1F3DA, 0x1F3DB, 0x1F3DC, 0x1F3DD, 0x1F3DE, 0x1F3DF, 0x1F3F3, 0x1F3F5, 0x1F3F7,
    0x1F43F, 0x1F441, 0x1F4FD, 0x1F549, 0x1F54A, 0x1F56F, 0x1F570, 0x1F573, 0x1F574, 0x1F575, 0x1F576,
    0x1F577, 0x1F578, 0x1F579, 0x1F587, 0x1F58A, 0x1F58B, 0x1F58C, 0x1F58D, 0x1F590, 0x1F5A5, 0x1F5A8,
    0x1F5B1, 0x1F5B2, 0x1F5BC, 0x1F5C2, 0x1F5C3, 0x1F5C4, 0x1F5D1, 0x1F5D2, 0x1F5D3, 0x1F5DC, 0x1F5DD,
    0x1F5DE, 0x1F5E1, 0x1F5E3, 0x1F5E8, 0x1F5EF, 0x1F5F3, 0x1F5FA, 0x1F6CB, 0x1F6CD, 0x1F6CE, 0x1F6CF,
    0x1F6E0, 0x1F6E1, 0x1F6E2, 0x1F6E3, 0x1F6E4, 0x1F6E5, 0x1F6E9, 0x1F6F0, 0x1F6F3,
};

Rc<Widget> emojiWidget(std::u32string str) {
    return rcnew Text{
        utf32ToUtf8(str),
        dimensions        = { 40, 40 },
        fontSize          = 28,
        textAlign         = TextAlign::Center,
        textVerticalAlign = TextAlign::Center,
        onClick           = staticLifetime |
                  [str] {
                      std::string text;
                      for (char32_t ch : str) {
                          if (ch > U'\uFFFF')
                              text += fmt::format("\\U{:08X}", uint32_t(ch));
                          else
                              text += fmt::format("\\u{:04X}", uint32_t(ch));
                      }
                      Clipboard::setText(std::move(text));
                  },
    };
}

static Builder emojiBuilder() {
    return Builder([](Widget* target) {
        for (auto rng : emojis) {
            for (char32_t ch = rng.min; ch <= rng.max; ++ch) {
                target->append(emojiWidget({ ch }));
            }
        }
        for (char32_t ch : emojis2) {
            target->append(emojiWidget({ ch, U'\uFE0F' }));
        }
    });
}

static Builder iconsBuilder() {
    return Builder([](Widget* target) {
        constexpr int columns = 16;
        auto iconFontFamily   = Font::Icons;
        int iconFontSize      = 22;
        for (int icon = ICON__first; icon < ICON__last; icon += columns) {
            Rc<HLayout> glyphs = rcnew HLayout{
                rcnew Text{
                    fmt::format("{:04X}", icon),
                    textVerticalAlign = TextAlign::Center,
                    dimensions        = { 36, 36 },
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
                    dimensions        = { 36, 36 },
                    onClick           = staticLifetime |
                              [ch] {
                                  Clipboard::setText(fmt::format("\\u{:04X}", uint32_t(ch)));
                              },
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

Rc<Widget> ShowcaseTypography::build(Rc<Notifications> notifications) {
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
                        auto row = [target, size](std::string name, std::string family, FontWeight weight) {
                            target->apply(rcnew Text{
                                pangram + fmt::format(" [{}, {}px]", name, size),
                                fontFamily = std::move(family),
                                fontWeight = weight,
                                fontSize   = size,
                            });
                        };
                        row("Lato Light", Font::Default, FontWeight::Light);
                        row("Lato Regular", Font::Default, FontWeight::Regular);
                        row("Lato Bold", Font::Default, FontWeight::Bold);
                        row("GoNoto", "Noto", FontWeight::Regular);
                        row("Monospace", Font::Monospace, FontWeight::Regular);
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
                fontFamily     = "Lato",
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

        rcnew Text{ "Emoji", classes = { "section-header" } },

        rcnew HLayout{
            padding    = { 8_apx, 8_apx },
            flexWrap   = Wrap::Wrap,
            maxWidth   = 640_apx,
            gap        = { 10_apx },
            fontFamily = Font::Emoji,

            emojiBuilder(),
        },
    };
}
} // namespace Brisk
