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

namespace Brisk {

class WIDGET PopupDialog : public Widget {
    BRISK_DYNAMIC_CLASS(PopupDialog, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "popupdialog";

    template <WidgetArgument... Args>
    explicit PopupDialog(Value<bool> visibleProp, const Args&... args)
        : PopupDialog{ Construction{ widgetType }, std::move(visibleProp), std::tuple{ args... } } {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit PopupDialog(Construction construction, Value<bool> visibleProp, ArgumentsView<Widget> args);
};

class WIDGET PopupOKDialog : public PopupDialog {
    BRISK_DYNAMIC_CLASS(PopupOKDialog, PopupDialog)
public:
    using Base = PopupDialog;

    template <WidgetArgument... Args>
    explicit PopupOKDialog(std::string title, Value<bool> visibleProp, VoidFunc accepted, const Args&... args)
        : PopupOKDialog{ Construction{ widgetType }, std::move(title), std::move(visibleProp),
                         std::move(accepted), std::tuple{ args... } } {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;

    explicit PopupOKDialog(Construction construction, std::string title, Value<bool> visibleProp,
                           VoidFunc accepted, ArgumentsView<Widget> args);
};

} // namespace Brisk
