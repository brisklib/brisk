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

#include "PopupBox.hpp"

namespace Brisk {

class WIDGET ContextPopup : public PopupBox {
    BRISK_DYNAMIC_CLASS(ContextPopup, PopupBox)
public:
    using Base                                   = PopupBox;
    constexpr static std::string_view widgetType = "contextpopup";

    template <WidgetArgument... Args>
    explicit ContextPopup(const Args&... args)
        : ContextPopup(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
    void append(RC<Widget> widget) override;
    explicit ContextPopup(Construction construction, ArgumentsView<ContextPopup> args);
};
} // namespace Brisk
