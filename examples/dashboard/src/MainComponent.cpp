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
#include "MainComponent.hpp"
#include <brisk/widgets/Widgets.hpp>
#include "AboutComponent.hpp"
#include "SettingsComponent.hpp"
#include "Style.hpp"
#include "View.hpp"
#include "brisk/core/Version.hpp"
#include <brisk/window/OsDialogs.hpp>

namespace App {

void MainComponent::unhandledEvent(Event& event) {
    handleDebugKeystrokes(event);
    handleActionShortcuts(event, {
                                     &m_actionSaveJson,
                                     &m_actionAboutDialog,
                                     &m_actionSettingsDialog,
                                     &m_actionQuit,
                                 });
}

void MainComponent::saveToJson() {
    Json json   = m_viewModel->json();
    auto result = Shell::showSaveDialog({ Shell::FileDialogFilter{ "*.json", "JSON files"_tr } },
                                        defaultFolder(DefaultFolder::Documents));
    if (result) {
        if (auto err = writeJson(*result, json, 4); !err) {
            Shell::showMessage("Cannot save .json file to \"{}\""_trfmt(result->string()));
        } else {
            m_notifications.show(".json file has been saved"_Text);
        }
    }
}

Rc<Widget> MainComponent::build() {
    return rcnew Widget{
        layout = Layout::Vertical,
        globalStyle,

        rcnew Widget{
            layout = Layout::Horizontal,
            rcnew ToggleButton{
                &m_widthGroup,
                value   = Value{ &m_menu },
                classes = { "flat", "slim" },
                rcnew Text{ ICON_menu + " Menu"_tr },
                rcnew Menu{
                    role    = "menu",
                    height  = 100_vh,
                    classes = { "withicons" },
                    visible = Value{ &m_menu },

                    rcnew Item{ m_actionSaveJson },

                    rcnew Item{ m_actionAboutDialog },

                    rcnew Item{ m_actionSettingsDialog },

                    rcnew Spacer{ height = 5_apx, flexGrow = 0 },

                    rcnew Item{ m_actionQuit },

                    rcnew Spacer{ flexGrow = 1 },

                    rcnew Hyperlink{
                        "https://brisklib.com",
                        "Visit brisklib.com"_Text,
                    },
                },
            },

            rcnew Spacer{},

            rcnew Text{
                "Dashboard"_tr,
                fontSize   = 120_perc,
                fontWeight = FontWeight::Bold,
            },

            rcnew Spacer{},

            rcnew Text{
                version,
                &m_widthGroup,
                padding = 8_apx,
            },
        },

        dataView(m_viewModel, Value{ &showPlots }),
        rcnew NotificationContainer(notManaged(&m_notifications)),
    };
}

MainComponent::MainComponent() {
    bindings->connect(Value{ &m_updateTrigger }, Value{ &frameStartTime }.transform([&](double v) {
        return std::round(v * 1000 / m_refreshInterval);
    }));

    bindings->connectBidir(Value{ &refreshInterval }, settings->value("refreshInterval", 1000));
    bindings->connectBidir(Value{ &showPlots }, settings->value("showPlots", true));

    m_viewModel  = rcnew DataSourceViewModel(dataCpuUsage(), Value{ &m_updateTrigger });

    m_actionQuit = {
        .caption  = "Quit",
        .icon     = ICON_door_open,
        .callback = staticLifetime |
                    []() {
                        guiApplication->quit();
                    },
        .shortcut = Shortcut{ KeyModifiers::ControlOrCommand, KeyCode::Q },
    };
    m_actionSaveJson = {
        .caption  = "Save .json",
        .icon     = ICON_save,
        .callback = BindableCallback<>(this, &MainComponent::saveToJson),
        .shortcut = Shortcut{ KeyModifiers::ControlOrCommand, KeyCode::S },
    };
    m_actionAboutDialog = {
        .caption  = "About",
        .callback = lifetime() |
                    [this]() {
                        guiApplication->showModalComponent(rcnew AboutComponent());
                    },
    };
    m_actionSettingsDialog = {
        .caption  = "Settings",
        .callback = lifetime() |
                    [this]() {
                        auto comp             = rcnew SettingsComponent();
                        comp->refreshInterval = refreshInterval;
                        comp->showPlots       = showPlots;
                        guiApplication->showModalComponent(comp);
                        if (comp->result) {
                            refreshInterval = comp->refreshInterval;
                            showPlots       = comp->showPlots;
                        }
                    },
    };
}

void MainComponent::configureWindow(Rc<GuiWindow> window) {
    window->setTitle("Application"_tr);
    window->setSize({ 1200, 900 });
    window->windowFit = WindowFit::MinimumSize;
    window->setStyle(WindowStyle::Normal);
}
} // namespace App
