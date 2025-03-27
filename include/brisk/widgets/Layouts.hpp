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

class WIDGET HLayout : public Widget {
    BRISK_DYNAMIC_CLASS(HLayout, Widget)
public:
    template <WidgetArgument... Args>
    explicit HLayout(const Args&... args) : HLayout(Construction{ "hlayout" }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    explicit HLayout(Construction construction, ArgumentsView<HLayout> args);
};

class WIDGET VLayout : public Widget {
    BRISK_DYNAMIC_CLASS(VLayout, Widget)
public:
    template <WidgetArgument... Args>
    explicit VLayout(const Args&... args) : VLayout(Construction{ "vlayout" }, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    explicit VLayout(Construction construction, ArgumentsView<VLayout> args);
};

} // namespace Brisk
