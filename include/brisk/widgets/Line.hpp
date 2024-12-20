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

#include <brisk/gui/GUI.hpp>

namespace Brisk {

class WIDGET Line : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "line";

    template <WidgetArgument... Args>
    explicit Line(Orientation orientation, const Args&... args)
        : Line(Construction{ widgetType }, orientation, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Orientation m_orientation;
    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
    explicit Line(Construction construction, Orientation orientation, ArgumentsView<Line> args);
};

class WIDGET HLine final : public Line {
public:
    using Base                                   = Line;
    constexpr static std::string_view widgetType = "hline";

    template <WidgetArgument... Args>
    explicit HLine(const Args&... args)
        : Line(Construction{ widgetType }, Orientation::Horizontal, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};

class WIDGET VLine final : public Line {
public:
    using Base                                   = Line;
    constexpr static std::string_view widgetType = "vline";

    template <WidgetArgument... Args>
    explicit VLine(const Args&... args)
        : Line(Construction{ widgetType }, Orientation::Vertical, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};

class WIDGET MenuLine final : public Line {
public:
    using Base                                   = Line;
    constexpr static std::string_view widgetType = "menuline";

    template <WidgetArgument... Args>
    explicit MenuLine(const Args&... args)
        : Line(Construction{ widgetType }, Orientation::Horizontal, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};
} // namespace Brisk
