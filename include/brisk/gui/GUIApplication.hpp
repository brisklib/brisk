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

#include <brisk/window/WindowApplication.hpp>
#include <brisk/gui/Component.hpp>

namespace Brisk {

class GUIApplication;

extern Nullable<GUIApplication> guiApplication;

class GUIApplication : public WindowApplication {
public:
    using WindowApplication::addWindow;
    using WindowApplication::modalRun;
    using WindowApplication::run;

    [[nodiscard]] int run(RC<Component> mainComponent);

    void modalRun(RC<Component> modalComponent);

    void addWindow(RC<Component> component, bool makeVisible = true);

    template <std::derived_from<Component> TComponent>
    RC<TComponent> showModalComponent(RC<TComponent> component) {
        RC<Window> window = component->makeWindow();
        addWindow(window, false);
        modalRun(window);
        return window;
    }

    template <std::derived_from<Component> TComponent, typename... Args>
    RC<TComponent> showModalComponent(Args&&... args) {
        return showModalComponent(std::make_shared<TComponent>(std::forward<Args>(args)...));
    }

    GUIApplication();

    ~GUIApplication();
};

} // namespace Brisk
