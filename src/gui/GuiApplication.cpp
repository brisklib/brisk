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
#include <brisk/gui/GuiApplication.hpp>

namespace Brisk {

Nullable<GuiApplication> guiApplication;

int GuiApplication::run(Rc<Component> mainComponent) {
    return WindowApplication::run(mainComponent->makeWindow());
}

void GuiApplication::modalRun(Rc<Component> modalComponent) {
    return WindowApplication::modalRun(modalComponent->makeWindow());
}

void GuiApplication::addWindow(Rc<Component> component, bool makeVisible) {
    return WindowApplication::addWindow(component->makeWindow(), makeVisible);
}

GuiApplication::~GuiApplication() {
    BRISK_ASSERT(guiApplication.get() == this);
    guiApplication = nullptr;
}

GuiApplication::GuiApplication() {
    BRISK_ASSERT(guiApplication.get() == nullptr);
    guiApplication = this;
}
} // namespace Brisk
