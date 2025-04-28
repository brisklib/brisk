#include "Dropdowns.hpp"
#include <brisk/gui/Icons.hpp>
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

        rcnew Text{ "PopupButton (widgets/PopupButton.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew PopupButton{
                    rcnew Text{ "Button with menu" },
                    rcnew PopupBox{
                        classes = { "menubox" },
                        rcnew Item{ rcnew Text{ "Item" } },
                        rcnew Item{ rcnew Text{ "Item with icon" }, icon = ICON_award },
                        rcnew Spacer{ height = 6 },
                        rcnew Item{ checked = Value<bool>::mutableValue(true), checkable = true,
                                    rcnew Text{ "Item with checkbox" } },
                    },
                },
                &m_group,
            },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew PopupButton{
                    rcnew Text{ "Button with box" },
                    rcnew PopupBox{
                        layout     = Layout::Vertical,
                        width      = 100_apx,
                        alignItems = AlignItems::Stretch,
                        rcnew ColorView{ Palette::Standard::index(0) },
                        rcnew ColorView{ Palette::Standard::index(1) },
                        rcnew ColorView{ Palette::Standard::index(2) },
                        rcnew ColorView{ Palette::Standard::index(3) },
                        rcnew ColorView{ Palette::Standard::index(4) },
                        rcnew ColorView{ Palette::Standard::index(5) },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "Click outside the box to hide it" },
        },
        rcnew Text{ "ComboBox (widgets/ComboBox.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew ComboBox{
                    value = Value{ &m_month },
                    rcnew ItemList{
                        rcnew Text{ "January" },
                        rcnew Text{ "February" },
                        rcnew Text{ "March" },
                        rcnew Text{ "April" },
                        rcnew Text{ "May" },
                        rcnew Text{ "June" },
                        rcnew Text{ "July" },
                        rcnew Text{ "August" },
                        rcnew Text{ "September" },
                        rcnew Text{ "October" },
                        rcnew Text{ "November" },
                        rcnew Text{ "December" },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "ComboBox with text items" },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew ComboBox{
                    value = Value{ &m_selectedItem },
                    rcnew ItemList{
                        IndexedBuilder([](int index) -> RC<Widget> {
                            if (index > 40)
                                return nullptr;
                            return rcnew Text{ fmt::to_string(index) };
                        }),
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "ComboBox with generated content" },
        },
        rcnew HLayout{
            rcnew Widget{
                rcnew ComboBox{
                    value = Value{ &m_selectedItem2 },
                    rcnew ItemList{
                        rcnew ColorView{ Palette::Standard::index(0) },
                        rcnew ColorView{ Palette::Standard::index(1) },
                        rcnew ColorView{ Palette::Standard::index(2) },
                        rcnew ColorView{ Palette::Standard::index(3) },
                        rcnew ColorView{ Palette::Standard::index(4) },
                        rcnew ColorView{ Palette::Standard::index(5) },
                    },
                },
                &m_group,
            },
            gapColumn = 10_apx,
            rcnew Text{ "ComboBox with widgets" },
        },

        rcnew Text{ "ContextPopup (widgets/ContextPopup.hpp)", classes = { "section-header" } },
        rcnew HLayout{
            rcnew Widget{
                rcnew Widget{
                    dimensions      = { 200_apx, 100_apx },
                    backgroundColor = 0x777777_rgb,
                    rcnew Text{ "Right-click for context menu", wordWrap = true,
                                alignSelf = AlignSelf::Center, color = 0xFFFFFF_rgb,
                                textAlign = TextAlign::Center, fontSize = 200_perc,
                                mouseInteraction = MouseInteraction::Disable, flexGrow = 1 },

                    rcnew ItemList{
                        role    = "menu",
                        classes = { "withicons" },
                        rcnew Item{ icon = ICON_pencil, "First"_Text },
                        rcnew Item{ icon = ICON_eye, "Second"_Text },
                        rcnew Item{ "Third"_Text },
                        rcnew Item{
                            "Fourth (with submenu)"_Text,
                            rcnew ItemList{
                                rcnew Item{ "Submenu item 1"_Text },
                                rcnew Item{ "Submenu item 2"_Text },
                                rcnew Item{ "Submenu item 3"_Text },
                            },
                        },

                    },
                },
                &m_group,
            },
        },

    };
}
} // namespace Brisk
