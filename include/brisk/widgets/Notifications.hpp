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
#pragma once

#include "Widgets.hpp"
#include "AutoScrollable.hpp"

namespace Brisk {

class WIDGET NotificationView final : public Widget {
    BRISK_DYNAMIC_CLASS(NotificationView, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "notification";

    template <WidgetArgument... Args>
    explicit NotificationView(double showTime, const Args&... args)
        : Widget{
              Construction{ widgetType },
              std::tuple{
                  Arg::layout = Layout::Vertical,
                  makeCloseButton(),
                  rcnew VLayout{
                      Arg::classes = { "notification-body" },
                      args...,
                  },
              },
          } {
        m_expireTime = frameStartTime + showTime;
        endConstruction();
    }

    bool expired() const;

    void expireNow();

protected:
    RC<Widget> makeCloseButton();
    double m_expireTime;
    void onEvent(Event& event) override;

    Ptr cloneThis() const override;
};

class Notifications {
public:
    template <WidgetArgument... Args>
    void show(const Args&... args) {
        if (m_receiver)
            m_receiver(rcnew NotificationView{ 5.0, args... });
    }

    template <WidgetArgument... Args>
    void show(double showTime, const Args&... args) {
        if (m_receiver)
            m_receiver(rcnew NotificationView{ showTime, args... });
    }

    void setReceiver(Callback<RC<NotificationView>> receiver);

protected:
    Callback<RC<NotificationView>> m_receiver;
};

class WIDGET NotificationContainer final : public AutoScrollable {
    BRISK_DYNAMIC_CLASS(NotificationContainer, AutoScrollable)
public:
    using Base                                   = AutoScrollable;
    constexpr static std::string_view widgetType = "notifications";

    template <WidgetArgument... Args>
    explicit NotificationContainer(RC<Notifications> notifications, const Args&... args)
        : AutoScrollable(Construction{ widgetType }, Orientation::Vertical, std::tuple{ args... }),
          m_notifications(std::move(notifications)) {
        endConstruction();
        m_notifications->setReceiver(std::bind(&NotificationContainer::receive, this, std::placeholders::_1));
    }

protected:
    RC<Notifications> m_notifications;
    void receive(RC<NotificationView> view);
    void onRefresh() override;
    Ptr cloneThis() const override;
};
} // namespace Brisk
