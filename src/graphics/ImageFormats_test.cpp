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
#include <brisk/graphics/ImageFormats.hpp>
#include "VisualTests.hpp"
#include "Catch2Utils.hpp"

namespace Brisk {

static void testImageCodec(std::string sample, Rc<Image> reference, ImageCodec codec, std::string ext) {
    INFO("sample=" << sample);
    INFO("codec=" << fmt::to_string(codec));
    INFO("format=" << fmt::to_string(reference->format()));

    fs::path fileName = fs::path(PROJECT_SOURCE_DIR) / "src" / "testdata" / (sample + "." + ext);
    auto bytes        = readBytes(fileName);
    REQUIRE(!!bytes);

    auto img = imageDecode(codec, *bytes, reference->format());
    REQUIRE(img.has_value());

    CHECK(imagePSNR(*img, reference) > 40);

    auto bytes2 = imageEncode(codec, reference, 99, ColorSubsampling::S444);

    auto img2   = imageDecode(codec, bytes2, reference->format());
    REQUIRE(img2.has_value());
    CHECK(imagePSNR(*img2, reference) > 40);
}

template <PixelFormat Format>
static void testImageSample(std::string sample, Size size) {
    auto raw = readBytes(fs::path(PROJECT_SOURCE_DIR) / "src" / "testdata" / (sample + ".raw"));
    REQUIRE(!!raw);
    Image reference(raw->data(), size, size.width * pixelComponents(Format),
                    imageFormat(PixelType::U8Gamma, Format));

    // SECTION("bmp") {
    //     testImageCodec(sample, reference, ImageCodec::BMP, "bmp");
    // }
    SECTION("png") {
        testImageCodec(sample, notManaged(&reference), ImageCodec::PNG, "png");
    }
    if constexpr (Format != PixelFormat::RGBA) {
        SECTION("jpg") {
            testImageCodec(sample, notManaged(&reference), ImageCodec::JPEG, "jpg");
        }
    }
}

TEST_CASE("ImageFormats(RGB)") {
    testImageSample<PixelFormat::RGB>("16616460-rgb", { 320, 213 });
}

TEST_CASE("ImageFormats(RGBA)") {
    testImageSample<PixelFormat::RGBA>("16616460-rgba", { 320, 213 });
}

TEST_CASE("ImageFormats(Greyscale)") {
    testImageSample<PixelFormat::Greyscale>("16616460-mono", { 320, 213 });
}
} // namespace Brisk
