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

#include "Button.hpp"
#include "PopupBox.hpp"

namespace Brisk {

class WIDGET PopupButton : public Button {
public:
    using Base = Button;
    using Button::widgetType;

    template <WidgetArgument... Args>
    explicit PopupButton(const Args&... args)
        : PopupButton(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    void close();
    std::shared_ptr<PopupBox> popupBox() const;

protected:
    void onEvent(Event& event) override;
    using Button::close;
    void onRefresh() override;
    void onChildAdded(Widget* w) override;
    Ptr cloneThis() const override;
    explicit PopupButton(Construction construction, ArgumentsView<PopupButton> args);
};
} // namespace Brisk
