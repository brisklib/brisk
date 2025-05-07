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
#include "Catch2Utils.hpp"

#include <brisk/graphics/Geometry.hpp>

namespace Brisk {
TEST_CASE("Rectangle") {
    CHECK(Rectangle{ 0, 5, 5, 6 }.contains(Point{ 4, 5 }));

    CHECK(!Rectangle{ Point{ 2, 5 }, Size{ 5, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 0, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 5, 0 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ -1, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 5, -1 } }.empty());

    CHECK(Rectangle{ 2, 5, 4, 10 }.union_(Rectangle{ 2, 5, 4, 10 }) == Rectangle{ 2, 5, 4, 10 });
    CHECK(Rectangle{ 2, 5, 4, 10 }.union_(Rectangle{ 3, 5, 4, 10 }) == Rectangle{ 2, 5, 4, 10 });
}

using Catch::Approx;

TEST_CASE("Polar") {
    CHECK(Approx(PolarF(PointF(10.f, 0.f)).angle).margin(0.002f) == 0.0f);
    CHECK(Approx(PolarF(PointF(10.f, 0.f)).radius).margin(0.002f) == 10.0f);
}
} // namespace Brisk
