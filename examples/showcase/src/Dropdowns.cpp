#include "Dropdowns.hpp"
#include "brisk/gui/Icons.hpp"
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Item.hpp>
#include <brisk/widgets/PopupBox.hpp>
#include <brisk/widgets/PopupButton.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

RC<Widget> ShowcaseDropdowns::build(RC<Notifications> notifications) {
    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        new Text{ "PopupButton (widgets/PopupButton.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new PopupButton{
                    new Text{ "Button with menu" },
                    new PopupBox{
                        classes = { "menubox" },
                        new Item{ new Text{ "Item" } },
                        new Item{ new Text{ "Item with icon" }, icon = ICON_award },
                        new Spacer{ height = 6 },
                        new Item{ checked = Value<bool>::mutableValue(true), checkable = true,
                                  new Text{ "Item with checkbox" } },
                    },
                },
                &m_group,
            },
        },
        new HLayout{
            new Widget{
                new PopupButton{
                    new Text{ "Button with box" },
                    new PopupBox{
                        layout     = Layout::Vertical,
                        width      = 100_apx,
                        alignItems = AlignItems::Stretch,
                        new ColorView{ Palette::Standard::index(0) },
                        new ColorView{ Palette::Standard::index(1) },
                        new ColorView{ Palette::Standard::index(2) },
                        new ColorView{ Palette::Standard::index(3) },
                        new ColorView{ Palette::Standard::index(4) },
                        new ColorView{ Palette::Standard::index(5) },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "Click outside the box to hide it" },
        },
        new Text{ "ComboBox (widgets/ComboBox.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new ComboBox{
                    value = Value{ &m_month },
                    new ItemList{
                        new Text{ "January" },
                        new Text{ "February" },
                        new Text{ "March" },
                        new Text{ "April" },
                        new Text{ "May" },
                        new Text{ "June" },
                        new Text{ "July" },
                        new Text{ "August" },
                        new Text{ "September" },
                        new Text{ "October" },
                        new Text{ "November" },
                        new Text{ "December" },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "ComboBox with text items" },
        },
        new HLayout{
            new Widget{
                new ComboBox{
                    value = Value{ &m_selectedItem },
                    new ItemList{
                        IndexedBuilder([](int index) -> Widget* {
                            if (index > 40)
                                return nullptr;
                            return new Text{ fmt::to_string(index) };
                        }),
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "ComboBox with generated content" },
        },
        new HLayout{
            new Widget{
                new ComboBox{
                    value = Value{ &m_selectedItem2 },
                    new ItemList{
                        new ColorView{ Palette::Standard::index(0) },
                        new ColorView{ Palette::Standard::index(1) },
                        new ColorView{ Palette::Standard::index(2) },
                        new ColorView{ Palette::Standard::index(3) },
                        new ColorView{ Palette::Standard::index(4) },
                        new ColorView{ Palette::Standard::index(5) },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            new Text{ "ComboBox with widgets" },
        },

        new Text{ "ContextPopup (widgets/ContextPopup.hpp)", classes = { "section-header" } },
        new HLayout{
            new Widget{
                new Widget{
                    dimensions      = { 200_apx, 100_apx },
                    backgroundColor = 0x777777_rgb,
                    new Text{ "Right-click for context menu", wordWrap = true, alignSelf = AlignSelf::Center,
                              color = 0xFFFFFF_rgb, textAlign = TextAlign::Center, fontSize = 200_perc,
                              mouseInteraction = MouseInteraction::Disable, flexGrow = 1 },

                    new ContextPopup{
                        role     = "context",
                        tabGroup = true,
                        new Item{ icon = ICON_pencil, new Text{ "First" } },
                        new Item{ icon = ICON_eye, new Text{ "Second" } },
                        new Item{ new Text{ "Third" } },
                        new Item{ new Text{ "Fourth" } },
                    },
                },
                &m_group,
            },
        },

    };
}
} // namespace Brisk
