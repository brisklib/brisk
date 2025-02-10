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
#include <brisk/widgets/Notifications.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

void NotificationView::onEvent(Event& event) {
    Widget::onEvent(event);
    if (event.pressed()) {
        expireNow();
    }
}

RC<Widget> NotificationView::makeCloseButton() {
    return rcnew Button{
        rcnew Text{ ICON_x },
        Arg::classes          = { "flat", "slim" },
        Arg::placement        = Placement::Absolute,
        Arg::zorder           = ZOrder::TopMost,
        Arg::absolutePosition = { 100_perc, 50_perc },
        Arg::anchor           = { 100_perc, 50_perc },
        Arg::onClick          = lifetime() |
                       [this]() {
                           expireNow();
                       },
    };
}

void NotificationView::expireNow() {
    m_expireTime = 0.0;
}

bool NotificationView::expired() const {
    return frameStartTime >= m_expireTime;
}

RC<Widget> NotificationView::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void NotificationContainer::onRefresh() {
    removeIf([](Widget* w) {
        auto* notification = dynamic_cast<NotificationView*>(w);
        return notification && notification->expired();
    });
}

RC<Widget> NotificationContainer::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void NotificationContainer::receive(RC<NotificationView> view) {
    apply(std::move(view));
}

void Notifications::setReceiver(Callback<RC<NotificationView>> receiver) {
    m_receiver = std::move(receiver);
}
} // namespace Brisk
