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
 */                                                                                                          \
#pragma once

#include <brisk/core/BasicTypes.hpp>
#include <fmt/format.h>
#include <brisk/core/Reflection.hpp>
#include <type_traits>
#include <brisk/core/Simd.hpp>
#include <array>

namespace Brisk {

/**
 * @enum ColorSpace
 * @brief Defines a set of color spaces.
 *
 * This enum class represents various color spaces, each of which operates within a
 * defined range of values for its components. These color spaces are used to represent
 * and manipulate colors in different formats, including both linear and gamma-corrected
 * forms, as well as different color representation systems like CIELAB, LMS, etc.
 */
enum class ColorSpace : uint8_t {
    /**
     * @brief sRGB color space in linear format.
     *
     * The sRGBLinear color space operates in the linear range, where all components
     * (R, G, B) have values between 0 and 1.
     */
    sRGBLinear,

    /**
     * @brief sRGB color space in gamma-corrected format.
     *
     * The sRGBGamma color space operates in a gamma-corrected range, where all
     * components (R, G, B) are also between 0 and 1, but corrected for gamma.
     */
    sRGBGamma,

    /**
     * @brief Display P3 color space in linear format.
     *
     * The DisplayP3Linear color space is used with displays supporting P3 gamut,
     * with all components (R, G, B) having values between 0 and 1 in a linear format.
     */
    DisplayP3Linear,

    /**
     * @brief Display P3 color space in gamma-corrected format.
     *
     * The DisplayP3Gamma color space is gamma-corrected, where the P3 display color
     * components (R, G, B) have values between 0 and 1.
     */
    DisplayP3Gamma,

    /**
     * @brief CIE XYZ-D65 color space.
     *
     * The CIEXYZ color space represents color based on the CIE 1931 XYZ color model.
     * The X, Y, and Z components have ranges between 0 and 100.
     */
    CIEXYZ,

    /**
     * @brief CIE LAB color space.
     *
     * The CIELAB color space is used to approximate human vision, with the L component
     * ranging from 0 to 100, and the a and b components ranging from -200 to +200.
     */
    CIELAB,

    /**
     * @brief CIE LCH color space.
     *
     * The CIELCH color space is based on the cylindrical representation of the CIELAB color
     * model. The L component ranges from 0 to 100, the C component from 0 to 100, and
     * the H component from 0 to 360 degrees.
     */
    CIELCH,

    /**
     * @brief OKLAB color space.
     *
     * The OKLAB color space is another perceptually uniform color model, with the L
     * component ranging from 0 to 100, and the a and b components ranging from -200 to +200.
     */
    OKLAB,

    /**
     * @brief OKLCH color space.
     *
     * The OKLCH color space is a cylindrical version of the OKLAB model. The L component
     * ranges from 0 to 100, the C component from 0 to 100, and the H component from
     * 0 to 360 degrees.
     */
    OKLCH,

    /**
     * @brief LMS color space.
     *
     * The LMS color space is based on the response of the human eye's long, medium,
     * and short-wavelength cones. All components (L, M, S) have values between 0 and 1.
     */
    LMS,
};

constexpr auto operator+(ColorSpace colorSpace) noexcept {
    return static_cast<std::underlying_type_t<ColorSpace>>(colorSpace);
}

template <>
inline constexpr std::initializer_list<NameValuePair<ColorSpace>> defaultNames<ColorSpace>{
    { "sRGBLinear", ColorSpace::sRGBLinear },
    { "sRGBGamma", ColorSpace::sRGBGamma },
    { "DisplayP3Linear", ColorSpace::DisplayP3Linear },
    { "DisplayP3Gamma", ColorSpace::DisplayP3Gamma },
    { "CIEXYZ", ColorSpace::CIEXYZ },
    { "CIELAB", ColorSpace::CIELAB },
    { "CIELCH", ColorSpace::CIELCH },
    { "OKLAB", ColorSpace::OKLAB },
    { "OKLCH", ColorSpace::OKLCH },
    { "LMS", ColorSpace::LMS },
};

/**
 * @enum ColorConversionMode
 * @brief Defines the modes for adjusting colors after conversion.
 *
 * This enum class provides different strategies for handling colors that may fall outside
 * the valid range of a given color space during conversion.
 */
enum class ColorConversionMode {
    /**
     * @brief No adjustment to the color.
     *
     * The color is returned as-is, even if its values are out of the acceptable range for
     * the current color space.
     */
    None,

    /**
     * @brief Clamps the color to the valid range.
     *
     * The color is adjusted by clamping each component to the nearest boundary of the valid
     * range for the current color space.
     */
    Clamp,

    /**
     * @brief Adjusts the color to the nearest valid value by reducing chroma.
     *
     * The color is adjusted by reducing its chroma (saturation) to bring it within the
     * valid range of the color space.
     */
    Nearest,
};

struct Trichromatic {

    constexpr Trichromatic() noexcept : value{ 0, 0, 0 }, colorSpace(ColorSpace::sRGBLinear) {}

    constexpr Trichromatic(const Trichromatic&) noexcept            = default;
    constexpr Trichromatic& operator=(const Trichromatic&) noexcept = default;

    constexpr Trichromatic(double c1, double c2, double c3,
                           ColorSpace colorSpace = ColorSpace::sRGBLinear) noexcept
        : value{ c1, c2, c3 }, colorSpace(colorSpace) {}

    constexpr Trichromatic(Simd<double, 3> value, ColorSpace colorSpace = ColorSpace::sRGBLinear) noexcept
        : value{ value }, colorSpace(colorSpace) {}

    Trichromatic convert(ColorSpace destSpace) const noexcept;

    Trichromatic convert(ColorSpace destSpace, ColorConversionMode mode) const noexcept;

    Trichromatic nearestValid() const noexcept;

    Trichromatic clamped() const noexcept;

    // For sRGB and P3 colorspaces checks whether the values are in range [0,1]. For other color spaces always
    // returns true.
    bool inGamut() const noexcept;

    constexpr bool operator==(Trichromatic other) const noexcept {
        return value == other.value;
    }

    constexpr double operator[](size_t index) const noexcept {
        return value[index];
    }

    constexpr double& operator[](size_t index) noexcept {
        return value[index];
    }

    union {
        Simd<double, 3> value;
        std::array<double, 3> array;
    };

    ColorSpace colorSpace = ColorSpace::sRGBLinear;
};

enum class Illuminant {
    D50 = 0,
    D55,
    D65,
    D75,
    E,
};

constexpr std::underlying_type_t<Illuminant> operator+(Illuminant value) noexcept {
    return static_cast<std::underlying_type_t<Illuminant>>(value);
}

namespace Internal {

template <typename T, size_t N>
constexpr BRISK_INLINE Simd<T, N> srgbGammaToLinear(Simd<T, N> x) noexcept {
    static_assert(std::is_floating_point_v<T>);
    Simd<T, N> v = abs(x);
    if (std::is_constant_evaluated()) {
        v = v * (v * (v * T(0.305306011) + T(0.682171111)) + T(0.012522878));
    } else {
        SimdMask<N> m = le(v, Simd<T, N>(0.04045));
        v             = select(m, v * Simd<T, N>(0.07739938080495356),
                               pow((v + T(0.055)) * std::nexttoward(T(0.947867298578199), T(1)), Simd<T, N>(2.4)));
    }
    return copysign(v, x);
}

template <typename T, size_t N>
constexpr BRISK_INLINE Simd<T, N> srgbLinearToGamma(Simd<T, N> x) noexcept {
    static_assert(std::is_floating_point_v<T>);
    Simd<T, N> v = abs(x);
    if (std::is_constant_evaluated()) {
        Simd<T, N> S1 = sqrt(v);
        Simd<T, N> S2 = sqrt(S1);
        Simd<T, N> S3 = sqrt(S2);
        v             = T(0.585122381) * S1 + T(0.783140355) * S2 - T(0.368262736) * S3;
    } else {
        SimdMask<N> m = le(v, Simd<T, N>(T(0.0031308)));
        v             = select(m, v * T(12.92),
                               T(1.055) * pow(v, Simd<T, N>(std::nexttoward(T(0.416666666666667), T(0)))) - T(0.055));
    }
    return copysign(v, x);
}

template <std::floating_point T>
BRISK_INLINE T srgbGammaToLinear(T v) noexcept {
    return srgbGammaToLinear(Simd{ v }).front();
}

template <std::floating_point T>
BRISK_INLINE T srgbLinearToGamma(T v) noexcept {
    return srgbLinearToGamma(Simd{ v }).front();
}

} // namespace Internal

Trichromatic illuminant(Illuminant illum) noexcept;

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::Trichromatic> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::Trichromatic& value, FormatContext& ctx) const {
        return fmt::formatter<std::string>::format(
            fmt::format("{}{{ {:.5f}, {:.5f}, {:.5f} }}", value.colorSpace, value[0], value[1], value[2]),
            ctx);
    }
};
