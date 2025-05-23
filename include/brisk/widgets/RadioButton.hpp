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

#include "ToggleButton.hpp"

namespace Brisk {

class WIDGET RadioButton : public ToggleButton {
    BRISK_DYNAMIC_CLASS(RadioButton, ToggleButton)
public:
    using Base                                   = ToggleButton;
    constexpr static std::string_view widgetType = "radiobutton";

    template <WidgetArgument... Args>
    explicit RadioButton(const Args&... args)
        : RadioButton(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
    explicit RadioButton(Construction construction, ArgumentsView<RadioButton> args);
};

void radioButtonPainter(Canvas& canvas, const Widget& widget);
} // namespace Brisk
