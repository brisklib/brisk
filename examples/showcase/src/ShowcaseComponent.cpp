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
#include "ShowcaseComponent.hpp"
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/widgets/ListBox.hpp>
#include <brisk/widgets/SpinBox.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/App.hpp>
#include <brisk/widgets/ComboBox.hpp>
#include <brisk/widgets/ImageView.hpp>
#include <brisk/widgets/Table.hpp>
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/Viewport.hpp>
#include <brisk/widgets/Pages.hpp>
#include <brisk/widgets/Menu.hpp>
#include <brisk/widgets/ScrollBox.hpp>
#include <brisk/widgets/PopupDialog.hpp>
#include <brisk/window/OsDialogs.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/core/internal/Initialization.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/window/Clipboard.hpp>
#include <brisk/window/WindowApplication.hpp>
#include <brisk/widgets/Spinner.hpp>
#include <brisk/widgets/Progress.hpp>
#include <brisk/widgets/Spacer.hpp>

namespace Brisk {

static Rc<Stylesheet> mainStylesheet = rcnew Stylesheet{
    Graphene::stylesheet(),
    Style{
        Selectors::Class{ "section-header" },
        Rules{
            fontSize      = 14_px,
            fontFamily    = "@mono",
            color         = 0x5599ff_rgb,
            margin        = { 0, 10_apx },

            borderColor   = 0x5599ff_rgb,
            borderWidth   = { 0, 0, 0, 1_apx },
            paddingBottom = 2_apx,
        },
    },
    Style{
        Selectors::Type{ "imageview" } && !Selectors::Class{ "zoom" },
        Rules{
            placement  = Placement::Normal,
            dimensions = { auto_, auto_ },
            zorder     = ZOrder::Normal,
        },
    },
    Style{
        Selectors::Type{ "imageview" } && Selectors::Class{ "zoom" },
        Rules{
            placement        = Placement::Window,
            dimensions       = { 100_perc, 100_perc },
            absolutePosition = { 0, 0 },
            anchor           = { 0, 0 },
            zorder           = ZOrder::TopMost,
        },
    },
    Style{
        Selectors::Class{ "table-padding-4" } > Selectors::Type{ "tablerow" } >
            Selectors::Type{ "tablecell" },
        Rules{
            padding     = 4_apx,
            borderWidth = 1_apx,
            borderColor = 0x808890_rgb,
        },
    },
};

Rc<Widget> ShowcaseComponent::build() {
    auto notifications = notManaged(&m_notifications);
    return rcnew VLayout{
        flexGrow   = 1,
        stylesheet = mainStylesheet,
        Graphene::darkColors(),

        rcnew HLayout{
            fontSize = 24_dpx,
            rcnew Button{
                padding = 8_dpx,
                rcnew Text{ ICON_zoom_in },
                borderWidth = 1_dpx,
                onClick     = lifetime() |
                          []() {
                              windowApplication->uiScale =
                                  std::exp2(std::round(std::log2(windowApplication->uiScale) * 2 + 1) * 0.5);
                          },
            },
            rcnew Button{
                padding = 8_dpx,
                rcnew Text{ ICON_zoom_out },
                borderWidth = 1_dpx,
                onClick     = lifetime() |
                          []() {
                              windowApplication->uiScale =
                                  std::exp2(std::round(std::log2(windowApplication->uiScale) * 2 - 1) * 0.5);
                          },
            },
            rcnew Button{
                padding = 8_dpx,
                rcnew Text{ ICON_camera },
                borderWidth = 1_dpx,
                onClick     = lifetime() |
                          [this]() {
                              captureScreenshot();
                          },
            },
            rcnew Button{
                padding = 8_dpx,
                rcnew Text{ ICON_sun_moon },
                borderWidth = 1_dpx,
                onClick     = lifetime() |
                          [this]() {
                              m_lightTheme = !m_lightTheme;
                              this->tree().disableTransitions();
                              if (m_lightTheme)
                                  this->tree().root()->apply(Graphene::lightColors());
                              else
                                  this->tree().root()->apply(Graphene::darkColors());
                          },
            },
            rcnew ToggleButton{
                padding = 8_dpx,
                rcnew Text{ ICON_check },
                borderWidth = 1_dpx,
                value       = Value{ &globalEnabled },
            },
        },
        rcnew Pages{
            value  = Value{ &m_activePage },
            layout = Layout::Horizontal,
            Pages::tabs =
                rcnew Tabs{
                    layout = Layout::Vertical,
                },
            rcnew Page{
                "Buttons",
                rcnew VScrollBox{ flexGrow = 1, m_buttons->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{ "Dropdowns",
                        rcnew VScrollBox{ flexGrow = 1,
                                          m_dropdowns->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{
                "Editors",
                rcnew VScrollBox{ flexGrow = 1, m_editors->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{
                "Visual",
                rcnew VScrollBox{ flexGrow = 1, m_visual->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{
                "Layout",
                rcnew VScrollBox{ flexGrow = 1, m_layout->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{
                "Dialogs",
                rcnew VScrollBox{ flexGrow = 1, m_dialogs->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{ "Typography",
                        rcnew VScrollBox{ flexGrow = 1,
                                          m_typography->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{ "Messenger",
                        rcnew VScrollBox{ flexGrow = 1,
                                          m_messenger->build(notifications, Value{ &globalEnabled }) } },
            rcnew Page{
                "Binding",
                rcnew VScrollBox{ flexGrow = 1, m_binding->build(notifications, Value{ &globalEnabled }) } },
            flexGrow = 1,
        },
        flexGrow = 1,
        rcnew NotificationContainer(notifications),
    };
}

ShowcaseComponent::ShowcaseComponent() {}

void ShowcaseComponent::unhandledEvent(Event& event) {
    handleDebugKeystrokes(event);
}

void ShowcaseComponent::configureWindow(Rc<GuiWindow> window) {
    window->setTitle("Brisk Showcase"_tr);
    window->setSize({ 1050, 740 });
    window->setStyle(WindowStyle::Normal);
}

void ShowcaseComponent::saveScreenshot(Rc<Image> image) {
    Bytes bytes = pngEncode(image);
    if (auto file = Shell::showSaveDialog({ Shell::FileDialogFilter{ "*.png", "PNG image"_tr } },
                                          defaultFolder(DefaultFolder::Pictures))) {
        if (auto s = writeBytes(*file, bytes)) {
            m_notifications.show(rcnew Text{ "Screenshot saved successfully"_tr });
        } else {
            Shell::showMessage(fmt::format(fmt::runtime("Unable to save screenshot to {0}: {1}"_tr),
                                           file->string(), s.error()),
                               MessageBoxType::Warning);
        }
    }
}

void ShowcaseComponent::captureScreenshot() {
    if (auto window = this->window()) {
        window->captureFrame(std::bind(&ShowcaseComponent::saveScreenshot, this, std::placeholders::_1));
    }
}
} // namespace Brisk
