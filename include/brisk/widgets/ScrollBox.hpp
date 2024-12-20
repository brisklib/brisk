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
#include "ScrollBar.hpp"

namespace Brisk {

class WIDGET ScrollBox : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "scrollbox";

    template <WidgetArgument... Args>
    explicit ScrollBox(Orientation orientation, const Args&... args)
        : ScrollBox(Construction{ widgetType }, orientation, std::tuple{ args... }) {
        endConstruction();
    }

    std::shared_ptr<ScrollBar> scrollBar() const;
    bool scrollable() const;

protected:
    Orientation m_orientation;
    float m_scrollPosition     = 0;
    mutable float m_scrollSize = 0;

    void onEvent(Event& event) override;
    void onLayoutUpdated() override;
    bool setScrollOffset(float value);
    void updateOffsets();
    void createScrollBar();
    void revealChild(Widget* child) override;
    Ptr cloneThis() const override;
    explicit ScrollBox(Construction construction, Orientation orientation, ArgumentsView<ScrollBox> args);

public:
    BRISK_PROPERTIES_BEGIN
    Property<ScrollBox, float, &ScrollBox::m_scrollPosition, nullptr, nullptr, &ScrollBox::updateOffsets>
        scrollPosition;
    Property<ScrollBox, float, &ScrollBox::m_scrollSize> scrollSize;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::PropArg<decltype(ScrollBox::scrollPosition)>> scrollPosition{};
constexpr inline Argument<Tag::PropArg<decltype(ScrollBox::scrollSize)>> scrollSize{};
} // namespace Arg

class WIDGET VScrollBox final : public ScrollBox {
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
