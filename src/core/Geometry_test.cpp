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

#include <brisk/graphics/Geometry.hpp>

#include "Catch2Utils.hpp"

namespace Brisk {

TEST_CASE("Rectangle basics") {
    CHECK(Rectangle{ 0, 5, 5, 6 }.contains(Point{ 4, 5 }));

    CHECK(!Rectangle{ Point{ 2, 5 }, Size{ 5, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 0, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 5, 0 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ -1, 6 } }.empty());
    CHECK(Rectangle{ Point{ 2, 5 }, Size{ 5, -1 } }.empty());

    CHECK(Rectangle{ 2, 5, 4, 10 }.union_(Rectangle{ 2, 5, 4, 10 }) == Rectangle{ 2, 5, 4, 10 });
    CHECK(Rectangle{ 2, 5, 4, 10 }.union_(Rectangle{ 3, 5, 4, 10 }) == Rectangle{ 2, 5, 4, 10 });

    CHECK(Rectangle{ 2, 5, 10, 20 }.width() == 8);
    CHECK(Rectangle{ 2, 5, 10, 20 }.height() == 15);
    CHECK(Rectangle{ 2, 5, 10, 20 }.area() == 120);
    CHECK(Rectangle{ 2, 5, 10, 20 }.size() == Size{ 8, 15 });
}

TEST_CASE("Rectangle longestSide (regression: longestSize)") {
    // Regression: RectangleOf::longestSide() used to call a non-existent
    // SizeOf::longestSize(), failing to compile when instantiated.
    CHECK(Rectangle{ 2, 5, 10, 20 }.longestSide() == 15);
    CHECK(Rectangle{ 2, 5, 10, 20 }.shortestSide() == 8);
    CHECK(RectangleF{ 2, 5, 10, 20 }.longestSide() == 15.f);
    CHECK(SizeF{ 3, 7 }.longestSide() == 7.f);
    CHECK(SizeF{ 9, 2 }.longestSide() == 9.f);
}

TEST_CASE("Rectangle toNormCoord (regression: division by zero)") {
    // Regression: toNormCoord used to divide by (p1 - p1) == 0 producing NaNs.
    const RectangleF r{ 10.f, 20.f, 30.f, 60.f };
    const PointF pt{ 20.f, 40.f };
    const PointF n = r.toNormCoord(pt);
    CHECK(n.x == Catch::Approx(0.5f));
    CHECK(n.y == Catch::Approx(0.5f));

    const PointF tl = r.toNormCoord(PointF{ 10.f, 20.f });
    CHECK(tl.x == Catch::Approx(0.f));
    CHECK(tl.y == Catch::Approx(0.f));

    const PointF br = r.toNormCoord(PointF{ 30.f, 60.f });
    CHECK(br.x == Catch::Approx(1.f));
    CHECK(br.y == Catch::Approx(1.f));

    // Also works for integer rectangles: normalization must happen in
    // floating point, not in the integer domain.
    const Rectangle ri{ 10, 20, 30, 60 };
    const PointF ni = ri.toNormCoord(Point{ 20, 40 });
    CHECK(ni.x == Catch::Approx(0.5f));
    CHECK(ni.y == Catch::Approx(0.5f));
}

TEST_CASE("Rectangle toNormCoord with fallback") {
    const RectangleF r{ 10.f, 20.f, 30.f, 60.f };
    const PointF inside{ 20.f, 40.f };
    const PointF outside{ 0.f, 0.f };
    const PointF fallback{ -1.f, -2.f };

    const PointF n = r.toNormCoord(inside, fallback);
    CHECK(n.x == Catch::Approx(0.5f));
    CHECK(n.y == Catch::Approx(0.5f));

    const PointF o = r.toNormCoord(outside, fallback);
    CHECK(o == fallback);
}

TEST_CASE("Rectangle applyMargin/applyPadding (regression: parameter shadowing)") {
    // Regression: applyMargin(h, v)/applyPadding(h, v) had a parameter named v
    // shadowing the SIMD vector member, failing to compile when instantiated.
    Rectangle r{ 10, 20, 30, 60 };
    r.applyMargin(2, 3);
    CHECK(r == Rectangle{ 8, 17, 32, 63 });

    Rectangle r2{ 10, 20, 30, 60 };
    r2.applyPadding(2, 3);
    CHECK(r2 == Rectangle{ 12, 23, 28, 57 });

    // Uniform and edges-based overloads still work.
    Rectangle r3{ 10, 20, 30, 60 };
    r3.applyMargin(1);
    CHECK(r3 == Rectangle{ 9, 19, 31, 61 });

    Rectangle r4{ 10, 20, 30, 60 };
    r4.applyPadding(Edges{ 2, 3, 4, 5 });
    CHECK(r4 == Rectangle{ 12, 23, 26, 55 });

    Rectangle r5{ 10, 20, 30, 60 };
    r5.applyMargin(Edges{ 2, 3, 4, 5 });
    CHECK(r5 == Rectangle{ 8, 17, 34, 65 });
}

TEST_CASE("Rectangle applyStart/withStart (regression: Simd + SizeOf)") {
    // Regression: applyStart/withStart used to add a Simd to a SizeOf,
    // failing to compile when instantiated.
    RectangleF r{ 10.f, 20.f, 30.f, 60.f };

    RectangleF a = r;
    a.applyStart(PointF{ 1.f, 2.f });
    CHECK(a == RectangleF{ 1.f, 2.f, 21.f, 42.f });

    RectangleF b = r;
    b.applyStart(1.f, 2.f);
    CHECK(b == RectangleF{ 1.f, 2.f, 21.f, 42.f });

    CHECK(r.withStart(PointF{ 1.f, 2.f }) == RectangleF{ 1.f, 2.f, 21.f, 42.f });
    CHECK(r.withStart(1.f, 2.f) == RectangleF{ 1.f, 2.f, 21.f, 42.f });
}

TEST_CASE("Rectangle applySize (regression: undefined pack)") {
    // Regression: applySize(w, h) used an undefined helper pack(w, h),
    // failing to compile when instantiated.
    RectangleF r{ 10.f, 20.f, 30.f, 60.f };
    r.applySize(5.f, 8.f);
    CHECK(r == RectangleF{ 10.f, 20.f, 15.f, 28.f });

    RectangleF r2{ 10.f, 20.f, 30.f, 60.f };
    r2.applyWidth(100.f);
    CHECK(r2 == RectangleF{ 10.f, 20.f, 110.f, 60.f });

    RectangleF r3{ 10.f, 20.f, 30.f, 60.f };
    r3.applyHeight(100.f);
    CHECK(r3 == RectangleF{ 10.f, 20.f, 30.f, 120.f });
}

TEST_CASE("Rectangle split (regression: Simd<int,2> * Simd<float,2>)") {
    // Regression: split() multiplied Simd<int,2> by Simd<float,2> directly,
    // failing to compile for integer rectangles.
    const Rectangle ri{ 0, 0, 100, 200 };
    const Rectangle a = ri.split(0.5f, 0.5f, 0.25f, 0.25f);
    CHECK(a == Rectangle{ 50, 100, 75, 150 });

    const RectangleF rf{ 0.f, 0.f, 100.f, 200.f };
    const RectangleF b = rf.split(PointF{ 0.5f, 0.5f }, SizeF{ 0.25f, 0.25f });
    CHECK(b == RectangleF{ 50.f, 100.f, 75.f, 150.f });
}

TEST_CASE("Point distanceManhattan (regression: Chebyshev)") {
    // Regression: distanceManhattan used horizontalMax (Chebyshev distance)
    // instead of horizontalSum (Manhattan distance).
    CHECK(PointF{ 0.f, 0.f }.distanceManhattan(PointF{ 3.f, 4.f }) == 7.f);
    CHECK(PointF{ 0.f, 0.f }.distanceManhattan(PointF{ -3.f, 4.f }) == 7.f);
    CHECK(PointF{ 1.f, 1.f }.distanceManhattan(PointF{ 1.f, 1.f }) == 0.f);
    CHECK(Point{ 0, 0 }.distanceManhattan(Point{ 3, 4 }) == 7);
}

TEST_CASE("Polar conversion") {
    using Catch::Approx;

    const PolarF p = PointF(10.f, 0.f);
    CHECK(p.angle == Approx(0.f).margin(0.002f));
    CHECK(p.radius == Approx(10.f).margin(0.002f));

    // Regression: conversion operator used to be non-const, so this
    // conversion on a const point failed to compile.
    const PointF c{ 3.f, 4.f };
    const PolarF pc = c;
    CHECK(pc.radius == Approx(5.f).margin(0.002f));
    CHECK(pc.angle == Approx(std::atan2(4.f, 3.f)).margin(0.002f));

    const PointF back{ pc };
    CHECK(back.x == Approx(3.f).margin(0.002f));
    CHECK(back.y == Approx(4.f).margin(0.002f));
}

TEST_CASE("Element-wise min/max on geometry types") {
    // min/max are now constrained to SimdCompatible element types and must
    // work for both integer and floating-point geometry.
    CHECK(min(Point{ 1, 5 }, Point{ 3, 2 }) == Point{ 1, 2 });
    CHECK(max(Point{ 1, 5 }, Point{ 3, 2 }) == Point{ 3, 5 });
    CHECK(min(PointF{ 1.f, 5.f }, PointF{ 3.f, 2.f }) == PointF{ 1.f, 2.f });
    CHECK(max(PointF{ 1.f, 5.f }, PointF{ 3.f, 2.f }) == PointF{ 3.f, 5.f });

    CHECK(min(Size{ 1, 5 }, Size{ 3, 2 }) == Size{ 1, 2 });
    CHECK(max(Size{ 1, 5 }, Size{ 3, 2 }) == Size{ 3, 5 });
    CHECK(min(SizeF{ 1.f, 5.f }, SizeF{ 3.f, 2.f }) == SizeF{ 1.f, 2.f });
    CHECK(max(SizeF{ 1.f, 5.f }, SizeF{ 3.f, 2.f }) == SizeF{ 3.f, 5.f });

    CHECK(min(Edges{ 1, 5, 3, 2 }, Edges{ 4, 1, 2, 3 }) == Edges{ 1, 1, 2, 2 });
    CHECK(max(Edges{ 1, 5, 3, 2 }, Edges{ 4, 1, 2, 3 }) == Edges{ 4, 5, 3, 3 });
    CHECK(min(EdgesF{ 1.f, 5.f, 3.f, 2.f }, EdgesF{ 4.f, 1.f, 2.f, 3.f }) == EdgesF{ 1.f, 1.f, 2.f, 2.f });
    CHECK(max(EdgesF{ 1.f, 5.f, 3.f, 2.f }, EdgesF{ 4.f, 1.f, 2.f, 3.f }) == EdgesF{ 4.f, 5.f, 3.f, 3.f });
}

TEST_CASE("Broadcast constructor is implicit") {
    // Broadcast constructors remain implicit: scalar -> geometry conversions
    // are used intentionally in GUI code.
    STATIC_REQUIRE(std::is_constructible_v<Size, int>);
    STATIC_REQUIRE(std::is_convertible_v<int, Size>);
    STATIC_REQUIRE(std::is_convertible_v<int, SizeF>);
    STATIC_REQUIRE(std::is_convertible_v<int, Edges>);
    STATIC_REQUIRE(std::is_convertible_v<int, EdgesF>);
    STATIC_REQUIRE(std::is_convertible_v<int, Corners>);
    STATIC_REQUIRE(std::is_convertible_v<int, CornersF>);
    // Note: PointOf has no broadcast (single-scalar) constructor, so
    // scalar -> Point conversion is not possible by design.
    STATIC_REQUIRE(!std::is_convertible_v<float, PointF>);
    STATIC_REQUIRE(!std::is_convertible_v<double, PointOf<double>>);

    CHECK(Size{ 3 } == Size{ 3, 3 });
    CHECK(SizeF{ 3.f } == SizeF{ 3.f, 3.f });
}

TEST_CASE("Rectangle roundOutward/roundInward") {
    CHECK(RectangleF{ 1.2f, 2.4f, 5.6f, 8.9f }.roundOutward() == Rectangle{ 1, 2, 6, 9 });
    CHECK(RectangleF{ 1.2f, 2.4f, 5.6f, 8.9f }.roundInward() == Rectangle{ 2, 3, 5, 8 });
    CHECK(Rectangle{ 1, 2, 6, 9 }.roundOutward() == Rectangle{ 1, 2, 6, 9 });
}

TEST_CASE("Rectangle intersection/intersects") {
    CHECK(Rectangle{ 0, 0, 10, 10 }.intersects(Rectangle{ 5, 5, 15, 15 }));
    CHECK(!Rectangle{ 0, 0, 10, 10 }.intersects(Rectangle{ 20, 20, 30, 30 }));
    CHECK(Rectangle{ 0, 0, 10, 10 }.intersection(Rectangle{ 5, 5, 15, 15 }) == Rectangle{ 5, 5, 10, 10 });
    CHECK(Rectangle{ 0, 0, 10, 10 }.intersection(Rectangle{ 20, 20, 30, 30 }).empty());
}

} // namespace Brisk
