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
#include <brisk/graphics/Image.hpp>
#include <brisk/core/IO.hpp>
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include "VisualTests.hpp"

namespace Brisk {
TEST_CASE("Image") {
    Image image(Size{ 16, 9 }, ImageFormat::Greyscale_U8Gamma);

    REQUIRE(image.size() == Size{ 16, 9 });

    {
        auto w   = image.mapWrite<ImageFormat::Unknown_U8Gamma>();
        w(0, 0)  = 255;
        w(15, 8) = 1;
    }
    {
        auto r = image.mapRead<ImageFormat::Greyscale_U8Gamma>();
        CHECK(r(0, 0) == PixelGreyscale8{ 255 });
        CHECK(r(15, 8) == PixelGreyscale8{ 1 });
    }

    Image image2(Size{ 16, 9 }, ImageFormat::Greyscale_U8Gamma);
    image2.clear(Color(100));
    {
        auto r = image2.mapRead<ImageFormat::Greyscale_U8Gamma>();
        CHECK(r(0, 0) == PixelGreyscale8{ 100 });
        CHECK(r(15, 8) == PixelGreyscale8{ 100 });
    }

    image2.copyFrom(notManaged(&image));
    {
        auto r = image2.mapRead<ImageFormat::Greyscale_U8Gamma>();
        CHECK(r(0, 0) == PixelGreyscale8{ 255 });
        CHECK(r(15, 8) == PixelGreyscale8{ 1 });
    }
    {
        auto w = image2.mapWrite<ImageFormat::Greyscale_U8Gamma>();
        for (int y = 0; y < w.height(); ++y) {
            for (int x = 0; x < w.width(); ++x) {
                w(x, y) = static_cast<uint8_t>(255.f * x * y / (w.width() - 1) / (w.height() - 1));
            }
        }
    }

    CHECK_THROWS_AS(rcnew Image(Size{ 65536, 65536 }), EArgument);
}
} // namespace Brisk
