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

#include "AutoScrollable.hpp"

namespace Brisk {

class WIDGET PopupBox : public AutoScrollable {
public:
    using Base                                   = AutoScrollable;
    constexpr static std::string_view widgetType = "popupbox";
    using AutoScrollable::apply;

    template <WidgetArgument... Args>
    explicit PopupBox(const Args&... args) : PopupBox(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    void onEvent(Event& event) override;
    Ptr cloneThis() const override;
    explicit PopupBox(Construction construction, ArgumentsView<PopupBox> args);
    using Base::append;
    void append(RC<Widget> widget) override;
};

} // namespace Brisk
