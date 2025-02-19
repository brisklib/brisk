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
#include <brisk/widgets/Widgets.hpp>
#include <catch2/catch_all.hpp>
#include <brisk/graphics/Palette.hpp>
#include "Catch2Utils.hpp"
#include "../graphics/VisualTests.hpp"

namespace Brisk {

TEST_CASE("Text") {
    auto w = rcnew Text{
        text = "Initialize",
    };

    CHECK(w->text.get() == "Initialize");
}

TEST_CASE("WidgetRendering") {
    WidgetTree tree;
    tree.setViewportRectangle({ 0, 0, 128, 64 });
    tree.setRoot(rcnew Widget{
        backgroundColor = Palette::Standard::red,
        margin          = { 8_apx },
        justifyContent  = Justify::Center,
        alignContent    = Align::Center,
        rcnew Text{
            "Text",
            color    = Palette::white,
            fontSize = 40.f,
        },
    });
    renderTest("widget-text", tree.viewportRectangle().size(), [&](RenderContext& context) {
        Canvas canvas(context);
        tree.updateAndPaint(canvas, Palette::black, true);
    });
}
} // namespace Brisk
