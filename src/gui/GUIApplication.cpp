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
#include <brisk/gui/GUIApplication.hpp>

namespace Brisk {
    
GUIApplication* guiApplication;

int GUIApplication::run(RC<Component> mainComponent) {
    return WindowApplication::run(mainComponent->makeWindow());
}

void GUIApplication::modalRun(RC<Component> modalComponent) {
    return WindowApplication::modalRun(modalComponent->makeWindow());
}

void GUIApplication::addWindow(RC<Component> component, bool makeVisible) {
    return WindowApplication::addWindow(component->makeWindow(), makeVisible);
}

GUIApplication::~GUIApplication() {
    BRISK_ASSERT(guiApplication == this);
    guiApplication = nullptr;
}

GUIApplication::GUIApplication() {
    BRISK_ASSERT(guiApplication == nullptr);
    guiApplication = this;
}
} // namespace Brisk
