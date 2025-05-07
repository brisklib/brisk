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
#include <brisk/graphics/Pixel.hpp>
#include <brisk/graphics/Color.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"

namespace Brisk {
using Catch::Approx;

TEST_CASE("computeY") {
    CHECK(computeY<uint8_t>(0, 0, 0) == 0);
    CHECK(computeY<uint8_t>(255, 0, 0) == 76);
    CHECK(computeY<uint8_t>(0, 255, 0) == 149);
    CHECK(computeY<uint8_t>(0, 0, 255) == 28);
    CHECK(computeY<uint8_t>(255, 255, 255) == 255);

    CHECK(computeY<float>(0, 0, 0) == 0);
    CHECK(Approx(computeY<float>(1.f, 0, 0)).margin(0.002f) == 0.299f);
    CHECK(Approx(computeY<float>(0, 1.f, 0)).margin(0.002f) == 0.587f);
    CHECK(Approx(computeY<float>(0, 0, 1.f)).margin(0.002f) == 0.114f);
    CHECK(Approx(computeY<float>(1.f, 1.f, 1.f)).margin(0.002f) == 1.f);
}

TEST_CASE("Pixel") {
    const auto pix  = PixelRGBA8{ 11, 22, 33, 255 };
    const auto pixf = PixelRGBA<PixelType::F32>{ 0.1f, 0.2f, 0.3f, 1.f };
    CHECK(cvtPixel<PixelFormat::RGB>(pix) == PixelRGB8{ 11, 22, 33 });
    CHECK(cvtPixel<PixelFormat::RGBA>(pix) == PixelRGBA8{ 11, 22, 33, 255 });
    CHECK(cvtPixel<PixelFormat::ARGB>(pix) == PixelARGB8{ 255, 11, 22, 33 });
    CHECK(cvtPixel<PixelFormat::BGR>(pix) == PixelBGR8{ 33, 22, 11 });
    CHECK(cvtPixel<PixelFormat::BGRA>(pix) == PixelBGRA8{ 33, 22, 11, 255 });
    CHECK(cvtPixel<PixelFormat::ABGR>(pix) == PixelABGR8{ 255, 33, 22, 11 });
    CHECK(cvtPixel<PixelFormat::GreyscaleAlpha>(pix) == PixelGreyscaleAlpha8{ 19, 255 });
    CHECK(cvtPixel<PixelFormat::Greyscale>(pix) == PixelGreyscale8{ 19 });
    CHECK(cvtPixel<PixelFormat::Alpha>(pix) == PixelAlpha8{ 255 });

    CHECK(cvtPixel<PixelFormat::RGBA>(PixelAlpha8{ 255 }) == PixelRGBA8{ 0, 0, 0, 255 });
    CHECK(cvtPixel<PixelFormat::RGBA>(PixelGreyscale8{ 111 }) == PixelRGBA8{ 111, 111, 111, 255 });

    CHECK(pixelToColor(pix) == Color{ 11, 22, 33, 255 });
    CHECK(pixelToColor(pixf) == ColorF{ 0.1f, 0.2f, 0.3f, 1.f });

    CHECK(colorToPixel(Color{ 11, 22, 33, 255 }) == pix);
    CHECK(colorToPixel(ColorF{ 0.1f, 0.2f, 0.3f, 1.f }) == pixf);
}

TEST_CASE("PixelType,PixelFormat") {
    CHECK(fmt::to_string(PixelType::F32) == "F32");
    CHECK(fmt::to_string(PixelType::U8Gamma) == "U8Gamma");

    CHECK(fmt::to_string(PixelFormat::RGBA) == "RGBA");
    CHECK(fmt::to_string(PixelFormat::Alpha) == "Alpha");
}
} // namespace Brisk
