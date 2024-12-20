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

class WIDGET CheckBox : public ToggleButton {
public:
    using Base                                   = ToggleButton;
    constexpr static std::string_view widgetType = "checkbox";

    template <WidgetArgument... Args>
    explicit CheckBox(const Args&... args) : CheckBox(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
    void paint(Canvas& canvas) const override;

    CheckBox(Construction construction, ArgumentsView<CheckBox> args);
};

void checkBoxPainter(Canvas& canvas, const Widget& widget);
} // namespace Brisk
