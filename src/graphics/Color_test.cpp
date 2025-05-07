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
#include <brisk/graphics/Color.hpp>
#include <brisk/core/Reflection.hpp>

namespace Brisk {

TEST_CASE("colorGamma") {
    CHECK_THAT(Internal::srgbGammaToLinear(0.f), Catch::Matchers::WithinAbs(0.f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(0.5f), Catch::Matchers::WithinAbs(0.21404f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(1.f), Catch::Matchers::WithinAbs(1.f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(2.f), Catch::Matchers::WithinAbs(4.9538f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(-0.5f), Catch::Matchers::WithinAbs(-0.21404f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(-1.f), Catch::Matchers::WithinAbs(-1.f, 0.001f));
    CHECK_THAT(Internal::srgbGammaToLinear(-2.f), Catch::Matchers::WithinAbs(-4.9538f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(0.f), Catch::Matchers::WithinAbs(0.f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(0.5f), Catch::Matchers::WithinAbs(0.73536f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(1.f), Catch::Matchers::WithinAbs(1.f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(2.f), Catch::Matchers::WithinAbs(1.3532f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(-0.5f), Catch::Matchers::WithinAbs(-0.73536f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(-1.f), Catch::Matchers::WithinAbs(-1.f, 0.001f));
    CHECK_THAT(Internal::srgbLinearToGamma(-2.f), Catch::Matchers::WithinAbs(-1.3532f, 0.001f));
}

TEST_CASE("ColorW") {
    bool linearColor = true;
    std::swap(linearColor, Brisk::linearColor);
    SCOPE_EXIT {
        std::swap(linearColor, Brisk::linearColor);
    };

    CHECK(ColorW(Color(255, 128, 0)).v == ColorW(8160, 4096, 0, 8160).v);
    CHECK_THAT(ColorF(ColorW(32767, -32767, 32767, 8160)).v,
               Catch::Matchers::SimdWithinMatcher(ColorF(25.54848f, -25.54848f, 25.54848f, 1.f).v, 0.001));

    for (int i = 0; i <= 255; i++) {
        Color orig(i, 255);
        Color convf = ColorF(orig);
        CHECK(static_cast<int>(orig.r) == static_cast<int>(convf.r));
        Color convw = ColorW(orig);
        CHECK(static_cast<int>(orig.r) == static_cast<int>(convw.r));
    }
}

TEST_CASE("rgbToColor") {
    CHECK(rgbToColor(0xAABBCC) == Color{ 0xAA, 0xBB, 0xCC, 0xFF });

    CHECK(rgbaToColor(0xAABBCC'DD) == Color{ 0xAA, 0xBB, 0xCC, 0xDD });

    CHECK(abgrToColor(0xDD'CCBBAA) == Color{ 0xAA, 0xBB, 0xCC, 0xDD });

    CHECK(0xAABBCC_rgb == Color{ 0xAA, 0xBB, 0xCC, 0xFF });

    CHECK(0xAABBCC'DD_rgba == Color{ 0xAA, 0xBB, 0xCC, 0xDD });

    CHECK(Color{ 0xAA, 0xBB, 0xCC, 0xDD }.withRed(0x33) == Color{ 0x33, 0xBB, 0xCC, 0xDD });
    CHECK(Color{ 0xAA, 0xBB, 0xCC, 0xDD }.withGreen(0x33) == Color{ 0xAA, 0x33, 0xCC, 0xDD });
    CHECK(Color{ 0xAA, 0xBB, 0xCC, 0xDD }.withBlue(0x33) == Color{ 0xAA, 0xBB, 0x33, 0xDD });

    CHECK(Color{ 0xAA, 0xBB, 0xCC, 0xDD }.multiplyAlpha(0.5f, AlphaMode::Premultiplied) ==
          Color{ 0xAA / 2, 0xBB / 2 + 1, 0xCC / 2, 0xDD / 2 + 1 });
}
} // namespace Brisk
