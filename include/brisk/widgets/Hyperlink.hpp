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

#include <brisk/gui/Gui.hpp>

namespace Brisk {
class WIDGET Hyperlink : public Widget {
    BRISK_DYNAMIC_CLASS(Hyperlink, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "hyperlink";

    template <WidgetArgument... Args>
    explicit Hyperlink(std::string url, const Args&... args)
        : Hyperlink(Construction{ widgetType }, std::move(url), std::tuple{ args... }) {
        endConstruction();
    }

protected:
    std::string m_url;
    void onEvent(Event& event) override;
    Ptr cloneThis() const override;
    explicit Hyperlink(Construction construction, std::string url, ArgumentsView<Hyperlink> args);
};
} // namespace Brisk
