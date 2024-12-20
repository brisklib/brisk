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

#include "ValueWidget.hpp"

namespace Brisk {

class WIDGET ScrollBar : public ValueWidget {
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "scrollbar";

    template <WidgetArgument... Args>
    explicit ScrollBar(const Args&... args) : ScrollBar(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    void onEvent(Event& event) override;
    Rectangle handleRect() const;
    bool isDragActive() const noexcept;

protected:
    bool m_dragActive     = false;
    float m_savedPosition = 0.f;
    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
    explicit ScrollBar(Construction construction, ArgumentsView<ScrollBar> args);
};

void scrollBarPainter(Canvas& canvas, const Widget& widget);

} // namespace Brisk
