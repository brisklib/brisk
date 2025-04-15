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
#include <catch2/catch_all.hpp>
#include "Catch2Utils.hpp"
#include <brisk/graphics/ColorSpace.hpp>
#include <brisk/core/Reflection.hpp>
#include "VisualTests.hpp"

namespace Brisk {

constexpr Range<SIMD<double, 3>, true> colorRange(ColorSpace Space) {
    if (Space == ColorSpace::CIEXYZ)
        return {
            SIMD<double, 3>{ (0), (0), (0) },
            SIMD<double, 3>{ (100), (100), (100) },
        };
    else if (Space == ColorSpace::CIELAB || Space == ColorSpace::OKLAB)
        return {
            SIMD<double, 3>{ (0), (-200), (-200) },
            SIMD<double, 3>{ (100), (200), (200) },
        };
    else if (Space == ColorSpace::CIELCH || Space == ColorSpace::OKLCH)
        return {
            SIMD<double, 3>{ (0), (0), (0) },
            SIMD<double, 3>{ (100), (100), (360) },
        };
    else // sRGB, DisplayP3, LMS
        return {
            SIMD<double, 3>{ (0), (0), (0) }, //
            SIMD<double, 3>{ (1), (1), (1) },
        };
}
} // namespace Brisk

namespace Catch {
namespace Matchers {

class ColorWithinMatcher final : public MatcherBase<Brisk::Trichromatic> {
public:
    ColorWithinMatcher(const Brisk::Trichromatic& target) : m_target(target) {}

    bool match(const Brisk::Trichromatic& matchee) const override {
        BRISK_ASSERT(matchee.colorSpace == m_target.colorSpace);
        auto range                     = Brisk::colorRange(matchee.colorSpace);
        Brisk::SIMD<double, 3> target  = m_target.value;
        Brisk::SIMD<double, 3> absdiff = Brisk::abs(matchee.value - target);
        return Brisk::horizontalAll(Brisk::lt(absdiff, range.max * 0.002));
    }

    std::string describe() const override {
        return "is approx. equal to " + ::Catch::Detail::stringify(m_target);
    }

private:
    Brisk::Trichromatic m_target;
    double m_margin;
};

} // namespace Matchers
} // namespace Catch

namespace Brisk {

static void checkColor(Trichromatic color1, Trichromatic color2) {
    {
        INFO("From " << fmt::to_string(color1.colorSpace));
        INFO("To " << fmt::to_string(color2.colorSpace));
        CHECK_THAT((color1.convert(color2.colorSpace)), Catch::Matchers::ColorWithinMatcher(color2));
    }
    {
        INFO("From " << fmt::to_string(color2.colorSpace));
        INFO("To " << fmt::to_string(color1.colorSpace));
        CHECK_THAT((color2.convert(color1.colorSpace)), Catch::Matchers::ColorWithinMatcher(color1));
    }
}

TEST_CASE("ColorSpaces") {
    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 1.0851, 0.9769, 0.9587, ColorSpace::sRGBGamma });
    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 1.2048, 0.9484, 0.9087, ColorSpace::sRGBLinear });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 1.0, 1.0, 1.0, ColorSpace::sRGBGamma });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 1.0, 1.0, 1.0, ColorSpace::sRGBLinear });

    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 100.000, 8.539, 5.594, ColorSpace::CIELAB });
    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 100.000, 10.208, 33.230, ColorSpace::CIELCH });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 100.000, 0.0, 0.0, ColorSpace::CIELAB });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 100.000, 0.0, 0.0, ColorSpace::CIELCH });

    checkColor(Trichromatic{ 100.000, 8.539, 5.594, ColorSpace::CIELAB },
               Trichromatic{ 100.000, 10.208, 33.230, ColorSpace::CIELCH });

    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 1.0519, 0.9984, 0.9464, ColorSpace::LMS });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 1.0, 1.0, 1.0, ColorSpace::LMS });

    checkColor(Trichromatic{ 100., 100., 100., ColorSpace::CIEXYZ },
               Trichromatic{ 100.32, 2.67, 1.47, ColorSpace::OKLAB });
    checkColor(illuminant(Illuminant::D65), Trichromatic{ 100.0, 0.0, 0.0, ColorSpace::OKLAB });
    checkColor(Trichromatic{ 100., 0., 0., ColorSpace::CIEXYZ },
               Trichromatic{ 45.0, 123.6, -1.902, ColorSpace::OKLAB });
    checkColor(Trichromatic{ 0.000, 100.000, 0.000, ColorSpace::CIEXYZ },
               Trichromatic{ 92.18, -67.11, 26.33, ColorSpace::OKLAB });
    checkColor(Trichromatic{ 0.000, 0.000, 100.000, ColorSpace::CIEXYZ },
               Trichromatic{ 15.26, -141.5, -44.89, ColorSpace::OKLAB });

    checkColor(illuminant(Illuminant::D65), Trichromatic{ 100., 0., 263.368, ColorSpace::OKLCH });

    checkColor(illuminant(Illuminant::D65), Trichromatic{ 1., 1., 1., ColorSpace::DisplayP3Linear });

    checkColor(Trichromatic{ 1., 0., 0., ColorSpace::DisplayP3Linear },
               Trichromatic{ 48.657, 22.897, 0., ColorSpace::CIEXYZ });
    checkColor(Trichromatic{ 0., 1., 0., ColorSpace::DisplayP3Linear },
               Trichromatic{ 26.567, 69.174, 4.511, ColorSpace::CIEXYZ });
    checkColor(Trichromatic{ 0., 0., 1., ColorSpace::DisplayP3Linear },
               Trichromatic{ 19.822, 7.929, 104.394, ColorSpace::CIEXYZ });

    checkColor(Trichromatic{ 1., 0., 0., ColorSpace::sRGBLinear },
               Trichromatic{ 53.23324, 104.57511, 40.000282, ColorSpace::CIELCH });
    checkColor(Trichromatic{ 0., 1., 0., ColorSpace::sRGBLinear },
               Trichromatic{ 87.73715, 119.7777, 136.01593, ColorSpace::CIELCH });
    checkColor(Trichromatic{ 0., 0., 1., ColorSpace::sRGBLinear },
               Trichromatic{ 32.30301, 133.8152, 306.2873, ColorSpace::CIELCH });

    checkColor(Trichromatic{ 1., 0., 0., ColorSpace::sRGBLinear },
               Trichromatic{ 62.79259, 25.768465, 29.223183, ColorSpace::OKLCH });
    checkColor(Trichromatic{ 0., 1., 0., ColorSpace::sRGBLinear },
               Trichromatic{ 86.64519, 29.48074, 142.51117, ColorSpace::OKLCH });
    checkColor(Trichromatic{ 0., 0., 1., ColorSpace::sRGBLinear },
               Trichromatic{ 45.203295, 31.32954, 264.07294, ColorSpace::OKLCH });

    CHECK_THAT((Trichromatic{ 38.49, 26.4, 270., ColorSpace::OKLCH }.convert(ColorSpace::sRGBGamma,
                                                                             ColorConversionMode::None)),
               Catch::Matchers::ColorWithinMatcher(
                   Trichromatic{ 0.14073244, -0.06990181, 0.8018577, ColorSpace::sRGBGamma }));
    CHECK_THAT((Trichromatic{ 38.49, 26.4, 270., ColorSpace::OKLCH }.convert(ColorSpace::sRGBGamma,
                                                                             ColorConversionMode::Clamp)),
               Catch::Matchers::ColorWithinMatcher(
                   Trichromatic{ 0.14073244, 0., 0.8018577, ColorSpace::sRGBGamma }));
    CHECK_THAT((Trichromatic{ 38.49, 26.4, 270., ColorSpace::OKLCH }.convert(ColorSpace::sRGBGamma,
                                                                             ColorConversionMode::Nearest)),
               Catch::Matchers::ColorWithinMatcher(
                   Trichromatic{ 0.13672051, 0., 0.7782618, ColorSpace::sRGBGamma }));

    CHECK_THAT((Trichromatic{ 67.42, 39.1, 73.97, ColorSpace::OKLCH }.convert(ColorSpace::sRGBGamma,
                                                                              ColorConversionMode::Nearest)),
               Catch::Matchers::ColorWithinMatcher(
                   Trichromatic{ 0.79200876, 0.52818274, 0., ColorSpace::sRGBGamma }));
}

TEST_CASE("Colorspace Gradients") {

    visualTest("oklch-gradient-nearest", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color(Trichromatic{ 100. * (1. - double(y) / (wr.height() - 1)), 10.,
                                           360. * (double(x) / (wr.width() - 1) - 0.5), ColorSpace::OKLCH },
                             1.0, ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("cielab-gradient-nearest", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color(Trichromatic{ 50., 200. * (double(x) / (wr.width() - 1) - 0.5),
                                           200. * (double(y) / (wr.height() - 1) - 0.5), ColorSpace::CIELAB },
                             1.0, ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("cielab-gradient-clamp", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color(Trichromatic{ 50., 200. * (double(x) / (wr.width() - 1) - 0.5),
                                           200. * (double(y) / (wr.height() - 1) - 0.5), ColorSpace::CIELAB },
                             1.0, ColorConversionMode::Clamp);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("lms-radient0", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = Trichromatic{ 0., double(x) / (wr.width() - 1), double(y) / (wr.height() - 1),
                                             ColorSpace::LMS }
                                   .convert(ColorSpace::sRGBLinear, ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("lms-radient1", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = Trichromatic{ 1., double(x) / (wr.width() - 1), double(y) / (wr.height() - 1),
                                             ColorSpace::LMS }
                                   .convert(ColorSpace::sRGBLinear, ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
}

} // namespace Brisk
