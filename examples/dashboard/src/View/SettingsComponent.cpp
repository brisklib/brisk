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
#include "SettingsComponent.hpp"
#include "Style.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Version.hpp>
#include <brisk/window/Clipboard.hpp>

namespace App {

static const NameValueOrderedList<int> refreshIntervals{
    { "0.25s", 250 },
    { "0.5s", 500 },
    { "1s", 1000 },
    { "2s", 2000 },
};

Rc<Widget> SettingsComponent::build() {
    return rcnew Widget{
        padding    = 16_apx,
        layout     = Layout::Vertical,
        alignItems = Align::FlexStart,
        globalStyle,

        "Refresh interval"_Text,

        rcnew ComboBox{
            Value{ &refreshInterval },
            notManaged(&refreshIntervals),
        },

        rcnew CheckBox{ value = Value{ &showPlots }, "Show plots"_Text },

        minDimensions = { 320, 200 },

        rcnew Spacer{},

        dialogButtons(DialogButtons::OKCancel),
    };
}

void SettingsComponent::configureWindow(Rc<GuiWindow> window) {
    DialogComponent::configureWindow(window);
    window->setTitle("Settings"_tr);
}
} // namespace App
