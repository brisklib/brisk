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

class WIDGET Viewport : public Widget {
    BRISK_DYNAMIC_CLASS(Viewport, Widget)
public:
    constexpr static std::string_view widgetType = "pages";

    using Renderer                               = function<void(Canvas&, Rectangle)>;
    using Controller                             = function<void(Event&, Rectangle)>;

    template <WidgetArgument... Args>
    explicit Viewport(Renderer renderer, Controller controller, const Args&... args)
        : Viewport(Construction{ widgetType }, std::move(renderer), std::move(controller),
                   std::tuple{ args... }) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit Viewport(Renderer renderer, const Args&... args)
        : Viewport(Construction{ widgetType }, std::move(renderer), nullptr, std::tuple{ args... }) {
        endConstruction();
    }

protected:
    void onAnimationFrame() override;
    void paint(Canvas& canvas) const override;
    void onEvent(Event& event) override;
    explicit Viewport(Construction construction, Renderer renderer, Controller controller,
                      ArgumentsView<Viewport> args);
    Renderer m_renderer;
    Controller m_controller;
};
} // namespace Brisk
