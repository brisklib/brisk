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
#include <random>

namespace Catch {
namespace Matchers {

template <typename T, Brisk::ColorSpace Space>
class ColorWithinMatcher final : public MatcherBase<Brisk::Trichromatic<T, Space>> {
public:
    ColorWithinMatcher(const Brisk::Trichromatic<T, Space>& target) : m_target(target) {}

    bool match(const Brisk::Trichromatic<T, Space>& matchee) const override {
        auto range                = Brisk::Internal::colorRange<Space, T>();
        Brisk::SIMD<T, 3> target  = m_target.value;
        Brisk::SIMD<T, 3> absdiff = Brisk::abs(matchee.value - target);
        return Brisk::horizontalAll(Brisk::lt(absdiff, range.max * 0.002f));
    }

    std::string describe() const override {
        return "is approx. equal to " + ::Catch::Detail::stringify(m_target);
    }

private:
    Brisk::Trichromatic<T, Space> m_target;
    double m_margin;
};

template <typename T, Brisk::ColorSpace Space>
ColorWithinMatcher(Brisk::Trichromatic<T, Space>) -> ColorWithinMatcher<T, Space>;

} // namespace Matchers
} // namespace Catch

namespace Brisk {

template <typename T, ColorSpace Space1, ColorSpace Space2>
static void checkColor(Trichromatic<T, Space1> color1, Trichromatic<T, Space2> color2) {
    {
        INFO("From " << fmt::to_string(Space1));
        INFO("To " << fmt::to_string(Space2));
        CHECK_THAT((static_cast<Trichromatic<T, Space2>>(color1)),
                   Catch::Matchers::ColorWithinMatcher(color2));
    }
    {
        INFO("From " << fmt::to_string(Space2));
        INFO("To " << fmt::to_string(Space1));
        CHECK_THAT((static_cast<Trichromatic<T, Space1>>(color2)),
                   Catch::Matchers::ColorWithinMatcher(color1));
    }
}

TEST_CASE("ColorSpaces") {
    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorSRGBGamma{ 1.0851f, 0.9769f, 0.9587f });
    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorSRGBLinear{ 1.2048f, 0.9484, 0.9087f });
    checkColor(illuminant(Illuminant::D65), ColorSRGBGamma{ 1.0f, 1.0f, 1.0f });
    checkColor(illuminant(Illuminant::D65), ColorSRGBLinear{ 1.0f, 1.0f, 1.0f });

    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorCIELAB{ 100.000f, 8.539f, 5.594f });
    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorCIELCH{ 100.000f, 10.208f, 33.230f });
    checkColor(illuminant(Illuminant::D65), ColorCIELAB{ 100.000f, 0.0f, 0.0f });
    checkColor(illuminant(Illuminant::D65), ColorCIELCH{ 100.000f, 0.0f, 0.0f });

    checkColor(ColorCIELAB{ 100.000f, 8.539f, 5.594f }, ColorCIELCH{ 100.000f, 10.208f, 33.230f });

    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorLMS{ 1.0519f, 0.9984f, 0.9464f });
    checkColor(illuminant(Illuminant::D65), ColorLMS{ 1.0f, 1.0f, 1.0f });

    checkColor(ColorCIEXYZ{ 100.f, 100.f, 100.f }, ColorOKLAB{ 100.32f, 2.67f, 1.47f });
    checkColor(illuminant(Illuminant::D65), ColorOKLAB{ 100.0f, 0.0f, 0.0f });
    checkColor(ColorCIEXYZ{ 100.f, 0.f, 0.f }, ColorOKLAB{ 45.0, 123.6, -1.902 });
    checkColor(ColorCIEXYZ{ 0.000, 100.000, 0.000 }, ColorOKLAB{ 92.18, -67.11, 26.33 });
    checkColor(ColorCIEXYZ{ 0.000, 0.000, 100.000 }, ColorOKLAB{ 15.26, -141.5, -44.89 });

    checkColor(illuminant(Illuminant::D65), ColorOKLCH{ 100.f, 0.f, 263.368f });

    checkColor(illuminant(Illuminant::D65), ColorDisplayP3Linear{ 1.f, 1.f, 1.f });

    checkColor(ColorDisplayP3Linear{ 1.f, 0.f, 0.f }, ColorCIEXYZ{ 48.657, 22.897, 0 });
    checkColor(ColorDisplayP3Linear{ 0.f, 1.f, 0.f }, ColorCIEXYZ{ 26.567, 69.174, 4.511 });
    checkColor(ColorDisplayP3Linear{ 0.f, 0.f, 1.f }, ColorCIEXYZ{ 19.822, 7.929, 104.394 });

    checkColor(ColorSRGBLinear{ 1, 0, 0 }, ColorCIELCH{ 53.23324, 104.57511, 40.000282 });
    checkColor(ColorSRGBLinear{ 0, 1, 0 }, ColorCIELCH{ 87.73715, 119.7777, 136.01593 });
    checkColor(ColorSRGBLinear{ 0, 0, 1 }, ColorCIELCH{ 32.30301, 133.8152, 306.2873 });

    checkColor(ColorSRGBLinear{ 1, 0, 0 }, ColorOKLCH{ 62.79259, 25.768465, 29.223183 });
    checkColor(ColorSRGBLinear{ 0, 1, 0 }, ColorOKLCH{ 86.64519, 29.48074, 142.51117 });
    checkColor(ColorSRGBLinear{ 0, 0, 1 }, ColorOKLCH{ 45.203295, 31.32954, 264.07294 });

    CHECK_THAT(
        convertColorSpace<ColorSpace::sRGBGamma>(ColorOKLCH{ 38.49, 26.4, 270. }, ColorConversionMode::None),
        Catch::Matchers::ColorWithinMatcher(ColorSRGBGamma{ 0.14073244, -0.06990181, 0.8018577 }));
    CHECK_THAT(
        convertColorSpace<ColorSpace::sRGBGamma>(ColorOKLCH{ 38.49, 26.4, 270. }, ColorConversionMode::Clamp),
        Catch::Matchers::ColorWithinMatcher(ColorSRGBGamma{ 0.14073244, 0, 0.8018577 }));
    CHECK_THAT(convertColorSpace<ColorSpace::sRGBGamma>(ColorOKLCH{ 38.49, 26.4, 270. },
                                                        ColorConversionMode::Nearest),
               Catch::Matchers::ColorWithinMatcher(ColorSRGBGamma{ 0.13672051, 0, 0.7782618 }));

    CHECK_THAT(convertColorSpace<ColorSpace::sRGBGamma>(ColorOKLCH{ 67.42, 39.1, 73.97 },
                                                        ColorConversionMode::Nearest),
               Catch::Matchers::ColorWithinMatcher(ColorSRGBGamma{ 0.79200876, 0.52818274, 0 }));
}

TEST_CASE("Colorspace Gradients") {
    visualTest("oklch-radient", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = convertColorSpace<ColorSpace::sRGBLinear>(
                    ColorOKLCH{
                        100.f * (1.f - float(y) / (wr.height() - 1)),
                        10.f,
                        360.f * (float(x) / (wr.width() - 1) - 0.5f),
                    },
                    ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("cielab-radient", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = convertColorSpace<ColorSpace::sRGBLinear>(
                    ColorCIELAB{
                        50.f,
                        200.f * (float(x) / (wr.width() - 1) - 0.5f),
                        200.f * (float(y) / (wr.height() - 1) - 0.5f),
                    },
                    ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("lms-radient0", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = convertColorSpace<ColorSpace::sRGBLinear>(
                    ColorLMS{
                        0.f,
                        float(x) / (wr.width() - 1),
                        float(y) / (wr.height() - 1),
                    },
                    ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
    visualTest("lms-radient1", { 512, 512 }, [&](RC<Image> image) {
        auto wr = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        for (size_t y = 0; y < wr.height(); ++y) {
            auto line = wr.line(y);
            for (size_t x = 0; x < wr.width(); ++x) {
                ColorF color = convertColorSpace<ColorSpace::sRGBLinear>(
                    ColorLMS{
                        1.f,
                        float(x) / (wr.width() - 1),
                        float(y) / (wr.height() - 1),
                    },
                    ColorConversionMode::Nearest);
                colorToPixel(line[x], color);
            }
        }
    });
}

} // namespace Brisk
