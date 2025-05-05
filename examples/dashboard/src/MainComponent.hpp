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
#pragma once

#include <brisk/core/internal/Initialization.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/gui/GuiWindow.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/gui/Component.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <brisk/gui/Icons.hpp>
#include "ViewModel.hpp"
#include <brisk/gui/Action.hpp>
#include <brisk/gui/Groups.hpp>
#include <brisk/widgets/Notifications.hpp>

namespace App {
using namespace Brisk;

class MainComponent final : public Component {
public:
    MainComponent();
    void unhandledEvent(Event& event) final;

    Rc<Widget> build() final;

    void configureWindow(Rc<GuiWindow> window) final;

private:
    bool m_menu = false;
    Rc<DataSourceViewModel> m_viewModel;
    int m_updateTrigger = 0;
    WidthGroup m_widthGroup;
    Notifications m_notifications;
    Action m_actionQuit;
    Action m_actionSaveJson;
    Action m_actionAboutDialog;
    Action m_actionSettingsDialog;
    int m_refreshInterval = 1000;
    bool m_showPlots      = true;
    void saveToJson();

public:
    BRISK_PROPERTIES_BEGIN
    Property<MainComponent, int, &MainComponent::m_refreshInterval> refreshInterval;
    Property<MainComponent, bool, &MainComponent::m_showPlots> showPlots;
    BRISK_PROPERTIES_END
};
} // namespace App
