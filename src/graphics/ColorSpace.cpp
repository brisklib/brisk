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
#include <brisk/graphics/ColorSpace.hpp>
#include <cfloat>

namespace Brisk {

static Simd<double, 3> convertColorSpace(ColorSpace dest, const Simd<double, 3>& value, ColorSpace source);

inline double wrapHue(double value) {
    value -= 360.0 * static_cast<long long>(value / 360.0);
    return value < 0.0 ? value + 360.0 : value;
}

inline Simd<double, 3> wrapHue(Simd<double, 3> value) {
    value[2] = wrapHue(value[2]);
    return value;
}

Trichromatic Trichromatic::convert(ColorSpace destSpace) const {
    if (colorSpace == destSpace)
        return *this;
    return Trichromatic(convertColorSpace(destSpace, value, colorSpace), destSpace);
}

Trichromatic Trichromatic::convert(ColorSpace destSpace, ColorConversionMode mode) const {
    Trichromatic result = convert(destSpace);
    if (result.inGamut())
        return result.clamped();

    switch (mode) {
    case ColorConversionMode::Clamp:
        return result.clamped();
    case ColorConversionMode::Nearest:
        return result.nearestValid();
    default:
        return result;
    }
}

Trichromatic Trichromatic::nearestValid() const {
    switch (colorSpace) {
    case ColorSpace::CIELAB: // clamp L to 0-100
    case ColorSpace::CIELCH: // clamp L to 0-100, wrap H to 0-360
    case ColorSpace::OKLAB:  // clamp L to 0-100
    case ColorSpace::OKLCH:  // clamp L to 0-100, wrap H to 0-360
        return clamped();
    case ColorSpace::CIEXYZ: // unbound
        return *this;
    default:
        break;
    }
    // RGB, DisplayP3, LMS: range 0-1

    constexpr ColorSpace refSpace = ColorSpace::OKLCH;

    Trichromatic lch              = convert(refSpace);
    if (lch[0] <= 0.)
        return Trichromatic(0., 0., 0., colorSpace);
    if (lch[0] >= 100.)
        return Trichromatic(1., 1., 1., colorSpace);

    if (inGamut())
        return clamped();

    double lowest  = 0;
    double highest = lch[1];
    Trichromatic result;
    for (int i = 0; i < 12; ++i) {
        double middle = (lowest + highest) * 0.5;
        result        = Trichromatic{ lch[0], middle, lch[2], refSpace }.convert(colorSpace);
        if (result.inGamut()) {
            lowest = middle;
        } else {
            highest = middle;
        }
    }
    return result.clamped();
}

Trichromatic Trichromatic::clamped() const {
    switch (colorSpace) {
    case ColorSpace::sRGBLinear:
    case ColorSpace::sRGBGamma:
    case ColorSpace::DisplayP3Linear:
    case ColorSpace::DisplayP3Gamma:
    case ColorSpace::LMS:
        return Trichromatic{ clamp(value, Simd<double, 3>(0), Simd<double, 3>(1)), colorSpace };
    case ColorSpace::CIELAB:
    case ColorSpace::OKLAB:
        return Trichromatic{ clamp(value, Simd<double, 3>(0, DBL_MIN, DBL_MIN),
                                   Simd<double, 3>(100, DBL_MAX, DBL_MAX)),
                             colorSpace };
    case ColorSpace::CIELCH:
    case ColorSpace::OKLCH:
        return Trichromatic{ wrapHue(clamp(value, Simd<double, 3>(0, DBL_MIN, DBL_MIN),
                                           Simd<double, 3>(100, DBL_MAX, DBL_MAX))),
                             colorSpace };
    case ColorSpace::CIEXYZ:
        return Trichromatic{ max(value, Simd<double, 3>(0, 0, 0)), colorSpace };
    default:
        BRISK_UNREACHABLE();
    }
}

bool Trichromatic::inGamut() const {
    switch (colorSpace) {
    case ColorSpace::sRGBLinear:
    case ColorSpace::sRGBGamma:
    case ColorSpace::DisplayP3Linear:
    case ColorSpace::DisplayP3Gamma:
    case ColorSpace::LMS:
        return horizontalMin(value) > -0.001 && horizontalMax(value) < 1.001;
    default:
        return true;
    }
}

const static Simd<double, 3> illuminants[] = {
    { 96.422, 100.000, 82.521 },  // D50 illuminant, 2° observer
    { 95.682, 100.000, 92.149 },  // D55 illuminant, 2° observer
    { 95.047, 100.000, 108.883 }, // D65 illuminant, 2° observer
    { 94.972, 100.000, 122.638 }, // D75 illuminant, 2° observer
    { 100, 100, 100 },            // E illuminant, 2° observer
};

Trichromatic illuminant(Illuminant illum) {
    Trichromatic result;
    result.colorSpace = ColorSpace::CIEXYZ;
    result.value      = illuminants[+illum];
    return result;
}

static Simd<double, 3> lchToLab(Simd<double, 3> value) {
    return Simd<double, 3>{
        value[0],
        std::cos(value[2] * deg2rad<double>) * value[1],
        std::sin(value[2] * deg2rad<double>) * value[1],
    };
}

static Simd<double, 3> labToLch(Simd<double, 3> value) {
    return Simd<double, 3>{ value[0], std::hypot(value[1], value[2]),
                            wrapHue(std::atan2(value[2], value[1]) * rad2deg<double>) };
}

using Internal::srgbGammaToLinear;
using Internal::srgbLinearToGamma;

static Simd<double, 3> convertColorSpace(const ColorSpace dest, const Simd<double, 3>& sourceValue,
                                         ColorSpace source) {
    Simd<double, 3> value = sourceValue;
    for (;;) {
        if (source == dest)
            return value;
        switch (source) {
        case ColorSpace::sRGBGamma:
            value  = srgbGammaToLinear(value);
            source = ColorSpace::sRGBLinear;
            break;
        case ColorSpace::DisplayP3Gamma:
            value  = srgbGammaToLinear(value);
            source = ColorSpace::DisplayP3Linear;
            break;
        case ColorSpace::OKLCH:
            value  = lchToLab(value);
            source = ColorSpace::OKLAB;
            break;
        case ColorSpace::CIELCH:
            value  = lchToLab(value);
            source = ColorSpace::CIELAB;
            break;
        case ColorSpace::sRGBLinear:
            if (dest == ColorSpace::sRGBGamma) {
                value  = srgbLinearToGamma(value);
                source = ColorSpace::sRGBGamma;
            } else {
                value = value[0] * Simd<double, 3>{ (41.24), (21.26), (01.93) } +
                        value[1] * Simd<double, 3>{ (35.76), (71.52), (11.92) } +
                        value[2] * Simd<double, 3>{ (18.05), (07.22), (95.05) };
                source = ColorSpace::CIEXYZ;
            }
            break;
        case ColorSpace::DisplayP3Linear:
            if (dest == ColorSpace::DisplayP3Gamma) {
                value  = srgbLinearToGamma(value);
                source = ColorSpace::DisplayP3Gamma;
            } else {
                value = value[0] * Simd<double, 3>{ 48.6571, 22.8975, 0.0000 } +
                        value[1] * Simd<double, 3>{ 26.5668, 69.1739, 4.5113 } +
                        value[2] * Simd<double, 3>{ 19.8217, 7.9287, 104.3944 };
                source = ColorSpace::CIEXYZ;
            }
            break;
        case ColorSpace::CIELAB:
            if (dest == ColorSpace::CIELCH) {
                value  = labToLch(value);
                source = ColorSpace::CIELCH;
            } else {
                const double y             = (value[0] + 16.0) / 116.0;
                const Simd<double, 3> w    = { value[1] / (500.0) + y, y, y - value[2] / (200.0) };
                const Simd<double, 3> cube = w * w * w;
                value                      = select(gt(cube, Simd<double, 3>((216.0 / 24389.0))), cube,
                                                    (w - (16.0 / 116.0)) / ((24389.0 / 27.0 / 116.0)));
                value                      = value * illuminants[+Illuminant::D65];
                source                     = ColorSpace::CIEXYZ;
            }
            break;
        case ColorSpace::OKLAB:
            if (dest == ColorSpace::OKLCH) {
                value  = labToLch(value);
                source = ColorSpace::OKLCH;
            } else {
                value = value[0] * (0.01) +
                        value[1] * Simd<double, 3>{ (0.003963377774), (-0.001055613458), (-0.000894841775) } +
                        value[2] * Simd<double, 3>{ (0.002158037573), (-0.000638541728), (-0.012914855480) };
                value  = value * value * value;
                source = ColorSpace::LMS;
            }
            break;
        case ColorSpace::CIEXYZ:
            switch (dest) {
            case ColorSpace::sRGBLinear:
            case ColorSpace::sRGBGamma:
                value = value[0] * Simd<double, 3>{ (+0.032406), (-0.009689), (+0.000557) } +
                        value[1] * Simd<double, 3>{ (-0.015372), (+0.018758), (-0.002040) } +
                        value[2] * Simd<double, 3>{ (-0.004986), (+0.000415), (+0.010570) };
                source = ColorSpace::sRGBLinear;
                break;

            case ColorSpace::DisplayP3Linear:
            case ColorSpace::DisplayP3Gamma:
                value = value[0] * Simd<double, 3>{ 0.02493498, -0.0082949, 0.00035846 } +
                        value[1] * Simd<double, 3>{ -0.00931385, 0.01762664, -0.00076172 } +
                        value[2] * Simd<double, 3>{ -0.0040271, 0.00023625, 0.00956885 };
                source = ColorSpace::DisplayP3Linear;
                break;

            case ColorSpace::CIELAB:
            case ColorSpace::CIELCH: {
                value                   = value / illuminants[+Illuminant::D65];
                const Simd<double, 3> w = select(gt(value, Simd<double, 3>((0.008856))), cbrt(value),
                                                 ((7.787) * value) + (16.0) / (116.0));

                value                   = Simd<double, 3>{ ((116.0) * w[1]) - (16.0), (500.0) * (w[0] - w[1]),
                                                           (200.0) * (w[1] - w[2]) };
                source                  = ColorSpace::CIELAB;
            } break;

            case ColorSpace::OKLAB:
            case ColorSpace::OKLCH:
            case ColorSpace::LMS:
                value =
                    value[0] * Simd<double, 3>{ (+0.008189330101), (+0.000329845436), (+0.000482003018) } +
                    value[1] * Simd<double, 3>{ (+0.003618667424), (+0.009293118715), (+0.002643662691) } +
                    value[2] * Simd<double, 3>{ (-0.001288597137), (+0.000361456387), (+0.006338517070) };
                source = ColorSpace::LMS;
                break;
            default:
                break;
            }
            break;
        case ColorSpace::LMS:
            if (dest == ColorSpace::OKLCH || dest == ColorSpace::OKLAB) {
                value = cbrt(value);
                value = value[0] * Simd<double, 3>{ (21.04542553), (197.79984951), (2.59040371) } +
                        value[1] * Simd<double, 3>{ (79.36177850), (-242.85922050), (78.27717662) } +
                        value[2] * Simd<double, 3>{ (-0.40720468), (45.05937099), (-80.86757660) };
                source = ColorSpace::OKLAB;
            } else {
                value = value[0] * Simd<double, 3>{ (+122.70138511), (-4.05801784), (-7.63812845) } +
                        value[1] * Simd<double, 3>{ (-55.77999806), (+111.22568696), (-42.14819784) } +
                        value[2] * Simd<double, 3>{ (+28.12561490), (-7.16766787), (+158.61632204) };
                source = ColorSpace::CIEXYZ;
            }
            break;
        }
    }
}

} // namespace Brisk
