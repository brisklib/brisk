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

#include <brisk/gui/Gui.hpp>

namespace Brisk {

class WIDGET AutoScrollable : public Widget {
    BRISK_DYNAMIC_CLASS(AutoScrollable, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "autoscrollable";
    using Widget::apply;

    template <WidgetArgument... Args>
    explicit AutoScrollable(Orientation orientation, const Args&... args)
        : AutoScrollable(Construction{ widgetType }, orientation, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Orientation m_orientation;

    bool m_enableAutoScroll = true;

    WidgetState m_chevron1  = WidgetState::None;
    WidgetState m_chevron2  = WidgetState::None;
    PeriodicTimer upTimer{ false };
    PeriodicTimer downTimer{ false };
    float upPause    = 0.75f;
    float downPause  = 0.75f;
    int m_offset     = 0;
    int m_scrollSize = 0;
    void onLayoutUpdated() override;

    void postPaint(Canvas& canvas) const override;
    void onAnimationFrame() override;
    Rectangle chevronRect(LogicalDirection direction) const;
    void onEvent(Event& event) override;
    Ptr cloneThis() const override;
    explicit AutoScrollable(Construction construction, Orientation orientation,
                            ArgumentsView<AutoScrollable> args);
};
} // namespace Brisk
