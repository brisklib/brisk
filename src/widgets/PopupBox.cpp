/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
#include <brisk/widgets/PopupBox.hpp>

namespace Brisk {

PopupBox::PopupBox(Construction construction, ArgumentsView<PopupBox> args)
    : AutoScrollable(construction, Orientation::Vertical,
                     std::tuple{
                         Arg::layout          = Layout::Vertical,
                         Arg::placement       = Placement::Absolute,
                         Arg::zorder          = ZOrder::TopMost,
                         Arg::mouseAnywhere   = true,
                         Arg::focusCapture    = true,
                         Arg::alignToViewport = AlignToViewport::XY,
                     }) {
    m_isPopup = true;
    args.apply(this);
}

Widget::Ptr PopupBox::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

void PopupBox::onEvent(Event& event) {
    AutoScrollable::onEvent(event);
    if (auto e = event.as<EventMouseButtonPressed>()) {
        if (!m_rect.contains(e->point)) {
            visible = false;
        } else {
            event.stopPropagation();
        }
    } else if (auto e = event.as<EventMouseButtonReleased>()) {
        if (m_rect.contains(e->point)) {
            event.stopPropagation();
        }
    }
}
} // namespace Brisk
