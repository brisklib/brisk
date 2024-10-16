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

#include "ToggleButton.hpp"

namespace Brisk {

class WIDGET Switch : public ToggleButton {
public:
    using Base                                   = ToggleButton;
    constexpr static std::string_view widgetType = "switch";

    template <WidgetArgument... Args>
    explicit Switch(const Args&... args) : Switch(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    optional<float>& animatedValue() const;

protected:
    Ptr cloneThis() override;
    void paint(Canvas& canvas) const override;
    void onEvent(Event& event) override;
    explicit Switch(Construction construction, ArgumentsView<Switch> args);

private:
    mutable optional<float> m_animatedValue;
};

void switchPainter(Canvas& canvas, const Widget& widget);
} // namespace Brisk
