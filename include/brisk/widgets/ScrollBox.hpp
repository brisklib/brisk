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

#include <brisk/gui/Gui.hpp>

namespace Brisk {

class WIDGET ScrollBox : public Widget {
    BRISK_DYNAMIC_CLASS(ScrollBox, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "scrollbox";

    template <WidgetArgument... Args>
    explicit ScrollBox(Orientation orientation, const Args&... args)
        : ScrollBox(Construction{ widgetType }, orientation, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
    explicit ScrollBox(Construction construction, Orientation orientation, ArgumentsView<ScrollBox> args);
};

class WIDGET VScrollBox final : public ScrollBox {
    BRISK_DYNAMIC_CLASS(VScrollBox, ScrollBox)
public:
    using Base = ScrollBox;
    using ScrollBox::widgetType;

    template <WidgetArgument... Args>
    explicit VScrollBox(const Args&... args)
        : ScrollBox(Construction{ widgetType }, Orientation::Vertical, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};

class WIDGET HScrollBox final : public ScrollBox {
    BRISK_DYNAMIC_CLASS(HScrollBox, ScrollBox)
public:
    using Base = ScrollBox;
    using ScrollBox::widgetType;

    template <WidgetArgument... Args>
    explicit HScrollBox(const Args&... args)
        : ScrollBox(Construction{ widgetType }, Orientation::Horizontal, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};

} // namespace Brisk
