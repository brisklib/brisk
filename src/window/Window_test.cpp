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
#include <catch2/catch_all.hpp>
#include "../core/Catch2Utils.hpp"
#include <brisk/window/Window.hpp>
#include "PlatformWindow.hpp"

using namespace Brisk;

class TestWindow final : public Window {
public:
    TestWindow() {}

protected:
    double m_time = currentTime() + 1.0;

    void paint(RenderContext& context, bool fullRepaint) override {
        if (currentTime() > m_time) {
            close();
        }
    }
};

#ifdef BRISK_INTERACTIVE_TESTS
TEST_CASE("Window tests") {
    WindowApplication app;
    Rc<TestWindow> w1 = rcnew TestWindow{};

    w1->setSize({ 1000, 100 });
    w1->setStyle(WindowStyle::Undecorated);
    w1->setPosition({ 300, 300 });

    app.addWindow(w1);
    std::ignore = app.run();
}
#endif
