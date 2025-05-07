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

#include "Gui.hpp"

namespace Brisk {

struct SizeGroup : public WidgetGroup {
public:
    constexpr SizeGroup(Orientation orientation) : orientation(orientation) {}

    ~SizeGroup() override = default;

    Orientation orientation;

    void beforeLayout(bool dirty) final;
};

struct WidthGroup final : public SizeGroup {
    constexpr WidthGroup() : SizeGroup(Orientation::Horizontal) {}

    ~WidthGroup() override = default;
};

struct HeightGroup final : public SizeGroup {
    constexpr HeightGroup() : SizeGroup(Orientation::Vertical) {}

    ~HeightGroup() override = default;
};

struct VisualGroup : public WidgetGroup {
    constexpr VisualGroup(Orientation orientation) : orientation(orientation) {}

    ~VisualGroup() override = default;

    Orientation orientation;

    void beforeFrame() final;
};

struct HorizontalVisualGroup final : public VisualGroup {
    constexpr HorizontalVisualGroup() : VisualGroup(Orientation::Horizontal) {}

    ~HorizontalVisualGroup() override = default;
};

struct VerticalVisualGroup final : public VisualGroup {
    constexpr VerticalVisualGroup() : VisualGroup(Orientation::Vertical) {}

    ~VerticalVisualGroup() override = default;
};

template <typename WidgetGroup>
class WIDGET WidgetWithGroup final : public Widget {
    BRISK_DYNAMIC_CLASS(WidgetWithGroup, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "widgetwithgroup";
    using Widget::apply;

    template <WidgetArgument... Args>
    explicit WidgetWithGroup(const Args&... args)
        : Widget{ Construction{ widgetType }, std::tuple{ args... } } {
        endConstruction();
    }

    ~WidgetWithGroup() override = default;

    void append(Rc<Widget> widget) override {
        if (widget)
            widget->apply(&group);
        Widget::append(std::move(widget));
    }

protected:
    Ptr cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

    WidgetGroup group;
};
} // namespace Brisk
