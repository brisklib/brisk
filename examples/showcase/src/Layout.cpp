#include "Layout.hpp"
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

RC<Widget> ShowcaseLayout::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow         = 1,
        padding          = 16_apx,
        gapRow           = 8_apx,
        contentOverflowX = ContentOverflow::Allow,

        rcnew Text{ "flexWrap = Wrap::Wrap", classes = { "section-header" } },
        rcnew HLayout{
            padding         = 16_apx,
            gapRow          = 16_apx,
            gapColumn       = 16_apx,
            backgroundColor = 0x000000_rgb,
            flexWrap        = Wrap::Wrap,
            fontSize        = 28,
            minWidth        = 400_apx,

            Builder([](Widget* target) {
                for (int i = 0; i < 24; ++i) {
                    target->apply(rcnew Widget{
                        dimensions = { 80_apx, 80_apx },
                        rcnew Text{
                            fmt::to_string(i + 1),
                            flexGrow  = 1,
                            alignSelf = AlignSelf::Stretch,
                            textAlign = TextAlign::Center,
                        },
                        backgroundColor = Palette::Standard::index(i),
                    });
                }
            }),
        },

        rcnew HLayout{
            rcnew Widget{
                rcnew Text{
                    "This widget's size is set to half of the window's width and a quarter of its height "
                    "<code>(width = 50_vw, height = 25_vh)</code>",
                    wordWrap    = true,
                    textOptions = TextOptions::HTML,
                    fontSize    = 16_px,
                },
                alignContent    = Align::Center,
                justifyContent  = Justify::Center,
                width           = 50_vw,
                height          = 25_vh,
                backgroundColor = Palette::grey,
            },
        },
    };
}
} // namespace Brisk
