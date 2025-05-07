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
#include <brisk/widgets/Hyperlink.hpp>
#include <brisk/window/OsDialogs.hpp>

namespace Brisk {
void Hyperlink::onEvent(Event& event) {
    Widget::onEvent(event);
    if (event.pressed() || event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        if (std::string_view(m_url).substr(0, 7) == "http://"sv ||
            std::string_view(m_url).substr(0, 8) == "https://"sv)
            Shell::openURLInBrowser(m_url);
        else if (fs::is_directory(m_url))
            Shell::openFolder(m_url);
        event.stopPropagation();
    }
}

Rc<Widget> Hyperlink::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Hyperlink::Hyperlink(Construction construction, std::string url, ArgumentsView<Hyperlink> args)
    : Widget(construction,
             std::tuple{
                 Arg::cursor  = Cursor::Hand,
                 Arg::tabStop = true,
             }),
      m_url(std::move(url)) {
    args.apply(this);
}
} // namespace Brisk
