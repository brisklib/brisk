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
#include "AboutComponent.hpp"
#include "Style.hpp"
#include <brisk/gui/Icons.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/widgets/Widgets.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/core/App.hpp>
#include <brisk/core/Version.hpp>
#include <brisk/window/Clipboard.hpp>

namespace App {

static Rc<Widget> copyableText(std::string_view text) {
    return rcnew Widget{
        rcnew Text{ text, Arg::wordWrap = true, maxWidth = 240, fontFamily = Font::Monospace },
        rcnew Button{
            classes = { "flat", "slim" },
            ICON_copy ""_Text,
            onClick = staticLifetime |
                      [text = std::string(text)]() {
                          Clipboard::setText(text);
                      },
        },
    };
}

Rc<Widget> AboutComponent::build() {
    return rcnew Widget{
        padding    = 16_apx,
        layout     = Layout::Vertical,
        alignItems = Align::Center,

        globalStyle,
        rcnew ImageView{ Resources::loadCached("icon.png"), dimensions = { 120_apx, 120_apx } },
        rcnew Text{ "<big>{}</big> by <big>{}</big>"_fmt(appMetadata.name, appMetadata.vendor),
                    textOptions = TextOptions::Html },
        copyableText(version),
        copyableText(buildInfo),
        dialogButtons(DialogButtons::OK),
    };
}

void AboutComponent::configureWindow(Rc<GuiWindow> window) {
    DialogComponent::configureWindow(window);
    window->setTitle("About"_tr);
}
} // namespace App
