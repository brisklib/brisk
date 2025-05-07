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
#pragma once
#include <catch2/catch_all.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/graphics/Color.hpp>
#include <brisk/graphics/Renderer.hpp>

namespace Brisk {

[[maybe_unused]] inline float imagePSNR(Rc<Image> img, Rc<Image> ref) {
    auto rimg = img->mapRead<ImageFormat::Unknown_U8Gamma>();
    auto rref = ref->mapRead<ImageFormat::Unknown_U8Gamma>();
    REQUIRE(rimg.components() == rref.components());
    REQUIRE(rimg.width() == rref.width());
    REQUIRE(rimg.height() == rref.height());
    double sumsqr = 0;
    for (int y = 0; y < rimg.height(); ++y) {
        for (int x = 0; x < rimg.memoryWidth(); ++x) {
            sumsqr += std::pow((rimg.line(y)[x] - rref.line(y)[x]), 2);
        }
    }
    double mse  = sumsqr / rimg.memorySize();
    double maxI = 255.f;
    double psnr = 10 * std::log10(maxI * maxI / mse);
    return psnr;
}

[[maybe_unused]] inline float imageMaxDiff(Rc<Image> img, Rc<Image> ref) {
    auto rimg = img->mapRead<ImageFormat::Unknown_U8Gamma>();
    auto rref = ref->mapRead<ImageFormat::Unknown_U8Gamma>();
    REQUIRE(rimg.components() == rref.components());
    REQUIRE(rimg.width() == rref.width());
    REQUIRE(rimg.height() == rref.height());
    double maxDiff = 0;
    for (int y = 0; y < rimg.height(); ++y) {
        for (int x = 0; x < rimg.memoryWidth(); ++x) {
            maxDiff = std::max(maxDiff, std::abs(double(rimg.line(y)[x]) - double(rref.line(y)[x])));
        }
    }
    return maxDiff / 255.0;
}

template <PixelFormat Format = PixelFormat::RGBA, typename Fn>
static void visualTest(const std::string& referenceImageName, Size size, Fn&& fn, float maximumDiff = 0.02f) {
    BRISK_ASSERT(maximumDiff < 1.f);
    INFO(referenceImageName);
    Rc<Image> testImage =
        rcnew Image(size, imageFormat(PixelType::U8Gamma, Format), Color(255, 255, 255, 255));
    bool testOk = false;
    SCOPE_EXIT {
        if (!testOk) {
            fs::create_directories(PROJECT_BINARY_DIR "/visualTest");
            fs::path savePath =
                uniqueFileName(PROJECT_BINARY_DIR "/visualTest/" + referenceImageName + ".png",
                               PROJECT_BINARY_DIR "/visualTest/" + referenceImageName + " {}.png");
            WARN("PNG saved at " << savePath.string());
            {
                auto rw = testImage->mapReadWrite();
                rw.unpremultiplyAlpha();
            }
            REQUIRE(writeBytes(savePath, pngEncode(testImage)));
        }
    };

    fn(testImage);

    fs::path fileName = fs::path(PROJECT_SOURCE_DIR) / "src" / "testdata" / (referenceImageName + ".png");
    auto refImgBytes  = readBytes(fileName);
    CHECK(refImgBytes.has_value());
    if (refImgBytes.has_value()) {
        expected<Rc<Image>, ImageIoError> decodedRefImg =
            pngDecode(*refImgBytes, imageFormat(PixelType::U8Gamma, Format), true);
        REQUIRE(decodedRefImg.has_value());
        REQUIRE((*decodedRefImg)->size() == size);
        REQUIRE((*decodedRefImg)->pixelFormat() == Format);
        Rc<Image> decodedRefImgT = (*decodedRefImg);
        float testDiff           = imageMaxDiff(testImage, decodedRefImgT);
        CHECK(testDiff < maximumDiff);
        testOk = testDiff < maximumDiff;
    }
}

template <typename Fn>
static void visualTestMono(const std::string& referenceImageName, Size size, Fn&& fn,
                           float maximumDiff = 0.02f) {
    visualTest<PixelFormat::Greyscale>(referenceImageName, size, std::forward<Fn>(fn), maximumDiff);
}

inline ColorF defaultBackColor  = Palette::transparent;
inline float defaultMaximumDiff = 0.05f;

template <bool profile = false, bool passRenderTarget = false, typename Fn>
static void renderTest(const std::string& referenceImageName, Size size, Fn&& fn,
                       ColorF backColor = defaultBackColor, float maximumDiff = defaultMaximumDiff,
                       std::initializer_list<RendererBackend> backends = rendererBackends) {

    for (RendererBackend bk : backends) {
        INFO(fmt::to_string(bk));
        expected<Rc<RenderDevice>, RenderDeviceError> device_ =
            createRenderDevice(bk, RendererDeviceSelection::Default);
        REQUIRE(device_.has_value());
        Rc<RenderDevice> device = *device_;
        auto info               = device->info();
        REQUIRE(!info.api.empty());
        REQUIRE(!info.vendor.empty());
        REQUIRE(!info.device.empty());

        Rc<ImageRenderTarget> target = device->createImageTarget(size, PixelType::U8Gamma);

        REQUIRE(!!target.get());
        REQUIRE(target->size() == size);

        Rc<RenderEncoder> encoder = device->createEncoder();
        encoder->setVisualSettings(VisualSettings{ .blueLightFilter = 0, .gamma = 1, .subPixelText = false });

        visualTest<PixelFormat::BGRA>(
            referenceImageName, size,
            [&](Rc<Image> image) {
                if constexpr (profile) {
                    encoder->beginFrame(0);
                }
                if constexpr (passRenderTarget) {
                    fn(encoder, target);
                } else {
                    RenderPipeline pipeline(encoder, target, backColor);

                    fn(static_cast<RenderContext&>(pipeline));

                    pipeline.flush();
                }

                if constexpr (profile) {
                    encoder->endFrame(
                        [referenceImageName](uint64_t frame,
                                             std::span<const std::chrono::nanoseconds> durations) {
                            using dur = std::chrono::duration<double, std::micro>;
                            SmallVector<dur, RenderEncoder::maxDurations> durations_us(durations.size());
                            for (size_t i = 0; i < durations_us.size(); ++i) {
                                durations_us[i] = std::chrono::duration_cast<dur>(durations[i]);
                            }
                            fmt::println("{} GPU execution times: {}\n", referenceImageName,
                                         fmt::join(durations_us, ", "));
                        });
                }

                encoder->wait();
                Rc<Image> out = target->image();
                image->copyFrom(out);
            },
            maximumDiff);
    }
}

} // namespace Brisk
