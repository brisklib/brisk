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

class WIDGET Slider : public ValueWidget {
public:
    using Base                                   = ValueWidget;
    constexpr static std::string_view widgetType = "slider";

    template <WidgetArgument... Args>
    explicit Slider(const Args&... args) : Slider(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    Rectangle trackRect() const noexcept;
    RectangleF thumbRect() const noexcept;

    Orientation orientation() const noexcept;

protected:
    void paint(Canvas& canvas) const override;
    void onEvent(Event& event) override;
    void onLayoutUpdated() override;
    void onChanged() override;
    Ptr cloneThis() override;
    explicit Slider(Construction construction, ArgumentsView<Slider> args);

private:
    bool m_drag                         = false;
    float savedValue                    = NAN;
    constexpr static int trackThickness = 4;
    constexpr static int thumbRadius    = 5;
    float m_distance                    = NAN;

    Rectangle m_trackRect;
    RectangleF m_thumbRect;
    void updateSliderGeometry();
};

void sliderPainter(Canvas& canvas, const Widget& widget);

} // namespace Brisk
