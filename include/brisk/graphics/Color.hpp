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
 */                                                                                                          \
#pragma once

#include "Pixel.hpp"
#include "ColorSpace.hpp"
#include <brisk/core/internal/Optional.hpp>
#include <brisk/core/Reflection.hpp>
#include <fmt/format.h>
#include <brisk/core/SIMD.hpp>

namespace Brisk {

/**
 * @brief Enum representing different gamma transfer functions for color processing.
 *
 * This enum is used to specify the gamma correction mode for color values. It allows selecting
 * between the sRGB transfer function and a mode that dynamically follows the value of the
 * `linearColor` variable.
 */
enum ColorGamma : uint8_t {
    /**
     * @brief Color uses the sRGB transfer function.
     *
     * In this mode, the color values are encoded using the sRGB transfer function.
     */
    sRGB,

    /**
     * @brief Color transfer function is determined by the `linearColor` variable.
     *
     * When set to `Default`, the gamma mode follows the current state of the `linearColor`
     * variable. If `linearColor` is `true`, the linear transfer function is used; otherwise,
     * the sRGB transfer function is applied.
     */
    Default,
};

constexpr ColorGamma pixelTypeToGamma(PixelType type) {
    return type == PixelType::U8Gamma ? ColorGamma::sRGB : ColorGamma::Default;
}

/**
 * @brief A flag that determines whether color values are stored and rendered in linear or gamma-corrected
 * mode.
 *
 * When `linearColor` is set to `true`, the color values in the `ColorF` structure (and for all colors with
 * ColorGamma::Default) are stored in linear color space. Rendering is also performed in linear mode, with
 * proper gamma correction applied across the entire rendering pipeline.
 *
 * When `linearColor` is `false`, the color values are stored and rendered in gamma-corrected space, which is
 * compatible with many other graphics libraries and APIs. This is also called the 'gamma-naive' method.
 *
 * Modifying the `linearColor` variable does not change the values already stored in `ColorF` structures,
 * nor does it change the gamma mode for existing render targets; it only affects future color processing.
 *
 * @note This variable is not thread-safe. Access to `linearColor` is not synchronized, so it is the
 * responsibility of the user to ensure that no other threads are reading or writing to this variable while it
 * is being modified.
 */
inline bool linearColor = false;

namespace Internal {

constexpr PixelType fixPixelType(PixelType type) {
    return type == PixelType::U8Gamma && !linearColor ? PixelType::U8 : type;
}

} // namespace Internal

/**
 * @brief Template struct for representing colors with different value types and gamma settings.
 *
 * The `ColorOf` struct allows for the representation of colors with customizable types (e.g., float, uint8_t)
 * and gamma modes (e.g., linear or sRGB). It provides various utilities for color manipulation, such as
 * gamma conversion, alpha handling, and color space transformations.
 *
 * @tparam T      Type used to store color values (e.g., float, uint8_t).
 * @tparam gamma  Gamma mode for color processing (e.g., sRGB, linear).
 */
template <typename T, ColorGamma gamma>
struct ColorOf;

/** @brief A floating-point color type with the default gamma setting, determined by `linearColor`. */
using ColorF = ColorOf<float, ColorGamma::Default>;

/** @brief A standard 8-bit sRGB color type. */
using Color  = ColorOf<uint8_t, ColorGamma::sRGB>;

/** @brief An extended-range 16-bit sRGB color type with 13-bit precision. */
using ColorW = ColorOf<int16_t, ColorGamma::sRGB>;

namespace Internal {

template <typename T>
constexpr int colorScale() {
    if constexpr (std::is_floating_point_v<T>)
        return 1;
    else if constexpr (std::is_unsigned_v<T>)
        return std::numeric_limits<T>::max();
    else if constexpr (std::is_same_v<T, int16_t>)
        return 8160; // 16-bit Extended range sRGB
}
} // namespace Internal

/**
 * @brief Converts a 32-bit RGBA color to a `Color` instance.
 *
 * This function takes a 32-bit integer in the form 0xRRGGBBAA and converts it into
 * a `Color` object. The memory layout is expected to be ABGR.
 *
 * @param x The 32-bit integer representing the color in 0xRRGGBBAA format.
 * @return The corresponding `Color` object.
 */
constexpr Color rgbaToColor(uint32_t x);

template <typename T, ColorGamma gamma>
struct ColorOf {
    using Tfloat                        = std::common_type_t<T, float>;
    using ColorFloat                    = ColorOf<Tfloat, gamma>;

    /**
     * @brief Maximum value for color components, based on the type `T`.
     */
    constexpr static inline int maximum = Internal::colorScale<T>();

    /**
     * @brief Returns the color space of the current color based on the gamma setting.
     *
     * This function determines whether the color is in sRGB or linear color space, depending on the value
     * of the `gamma` template parameter and the global `linearColor` flag.
     *
     * @return The color space (sRGB or linear).
     */
    static constexpr ColorSpace colorSpace() {
        if constexpr (gamma == ColorGamma::sRGB) {
            return ColorSpace::sRGBGamma;
        } else {
            return defaultColorSpace();
        }
    }

    static ColorSpace defaultColorSpace() {
        return !linearColor ? ColorSpace::sRGBGamma : ColorSpace::sRGBLinear;
    }

    static_assert(std::is_floating_point_v<T> || std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                      std::is_same_v<T, int16_t>,
                  "Incorrect type");

    /**
     * @brief Constructs a grayscale color with an optional alpha value.
     *
     * @param grey  The grayscale intensity value.
     * @param alpha The alpha (opacity) value. Defaults to the maximum value (fully opaque).
     */
    constexpr explicit ColorOf(T grey, T alpha = maximum) : v(grey, grey, grey, alpha) {}

    /**
     * @brief Constructs a color from RGB components, with an optional alpha value.
     *
     * @param r     The red component.
     * @param g     The green component.
     * @param b     The blue component.
     * @param a     The alpha component. Defaults to the maximum value (fully opaque).
     */
    constexpr ColorOf(T r, T g, T b, T a = maximum) : v(r, g, b, a) {}

    // Default copy constructor.
    constexpr ColorOf(const ColorOf&) = default;

    /**
     * @brief Constructs a color from a SIMD vector.
     *
     * @param v A 4-element SIMD vector representing the color components.
     */
    constexpr explicit ColorOf(const SIMD<T, 4>& v) : v(v) {}

    /**
     * @brief Constructs a color from a 3-element SIMD vector and an optional alpha value.
     *
     * @param v A 3-element SIMD vector representing the RGB components.
     * @param a The alpha component. Defaults to the maximum value.
     */
    constexpr explicit ColorOf(const SIMD<T, 3>& v, T a = maximum) : v(concat(v, SIMD<T, 1>(a))) {}

    /**
     * @brief Constructs a color from a 4-channel SIMD vector and an alpha value.
     *
     * This constructor takes a SIMD vector representing the red, green, and blue color channels
     * and an explicit alpha value. The explicit alpha value is combined with the first three channels to
     * create a complete color representation.
     *
     * @param v A SIMD vector containing the red, green, and blue color channel values.
     * @param a The alpha channel value to be combined with the color channels.
     */
    constexpr explicit ColorOf(const SIMD<T, 4>& v, T a) : v(concat(v.template firstn<3>(), SIMD{ a })) {}

    /**
     * @brief Creates a `ColorOf` object from a 32-bit 0xRRGGBBAA color value.
     *
     * This static function converts a 32-bit unsigned integer representing a color in RGBA format
     * into a `ColorOf` object. It first converts the RGBA value into a color and then constructs
     * the `ColorOf` instance using that color.
     *
     * @param rgba A 32-bit unsigned integer representing the color in RGBA format.
     * @return A `ColorOf` object corresponding to the provided RGBA value.
     */
    static constexpr ColorOf fromRGBA(uint32_t rgba) {
        return ColorOf(rgbaToColor(rgba));
    }

    /**
     * @brief Constructs a `ColorOf` object from a `Trichromatic` color source.
     *
     * This constructor initializes a `ColorOf` instance based on a `Trichromatic` color input.
     * It takes the color channels from the source and an optional alpha value, converting the color
     * space as necessary.
     *
     * @param source A `Trichromatic` object representing the source color.
     * @param a     The alpha channel value (default is the maximum value for type `U`).
     */
    ColorOf(Trichromatic source, double a = 1.0,
            ColorConversionMode conversionMode = ColorConversionMode::Clamp) {
        ColorFloat tmp(static_cast<SIMD<Tfloat, 4>>(
            concat(source.convert(colorSpace(), conversionMode).value, SIMD<double, 1>(a))));
        *this = static_cast<ColorOf>(tmp);
    }

    /**
     * @brief Gets the RGB components of the color as a 3-element SIMD vector.
     *
     * @return A SIMD vector representing the RGB components.
     */
    constexpr SIMD<T, 3> simd_rgb() const noexcept {
        return std::bit_cast<SIMD<T, 3>>(v);
    }

    /**
     * @brief Converts the `ColorOf` object to a `Trichromatic` color representation.
     *
     * This conversion operator allows a `ColorOf` instance to be converted into a `Trichromatic`
     * color. Depending on the current color space, it retrieves the RGB values and constructs the
     * appropriate `Trichromatic` object.
     *
     * @tparam Space The color space for the resulting `Trichromatic`.
     * @return A `Trichromatic<U, Space>` object representing the same color.
     */
    constexpr operator Trichromatic() const {
        return Trichromatic(SIMD<double, 3>(ColorFloat(*this).simd_rgb()), colorSpace());
    }

    /**
     * @brief Converts the `ColorOf` object to another `ColorOf` representation with a different color gamma.
     *
     * This conversion operator allows a `ColorOf` instance to be converted to a different `ColorOf`
     * type, potentially changing the gamma correction applied to the color. If the color spaces differ,
     * the color is rescaled and converted appropriately; otherwise, the color is directly converted
     * while preserving the channel values.
     *
     * @tparam U     The type for the color channels in the resulting `ColorOf`.
     * @tparam UGamma The gamma transfer function for the resulting `ColorOf`.
     * @return A `ColorOf<U, UGamma>` object representing the same color.
     */
    template <typename U, ColorGamma UGamma>
    constexpr operator ColorOf<U, UGamma>() const {
        if (colorSpace() != ColorOf<U, UGamma>::colorSpace()) {
            SIMD<Tfloat, 4> tmp = rescale<Tfloat, 1, maximum>(v);
            if (colorSpace() == ColorSpace::sRGBGamma)
                tmp = concat(Internal::srgbGammaToLinear(tmp.firstn(size_constant<3>{})),
                             tmp.shuffle(std::index_sequence<3>{}));
            else
                tmp = concat(Internal::srgbLinearToGamma(tmp.firstn(size_constant<3>{})),
                             tmp.lastn(size_constant<1>{}));
            return ColorOf<U, UGamma>(rescale<U, ColorOf<U, UGamma>::maximum, 1>(tmp));
        } else {
            return ColorOf<U, UGamma>(rescale<U, ColorOf<U, UGamma>::maximum, maximum>(v));
        }
    }

    /**
     * @brief Default constructor for the `ColorOf` struct.
     *
     * Initializes the color channels (red, green, blue, and alpha) to their default values (0).
     */
    constexpr ColorOf() : r{}, g{}, b{}, a{} {}

    /**
     * @brief Lightens the color by a specified factor.
     *
     * This function returns a new `ColorOf` instance with the color channels multiplied by the given
     * factor, resulting in a lighter color. The alpha channel remains unchanged.
     *
     * @param n A factor by which to lighten the color (default is 1.2).
     * @return A `ColorOf` object representing the lightened color.
     */
    constexpr ColorOf lighter(Tfloat lightnessOffset) const noexcept {
        return adjust(lightnessOffset);
    }

    /**
     * @brief Darkens the color by a specified factor.
     *
     * This function returns a new `ColorOf` instance with the color channels divided by the given
     * factor, resulting in a darker color. The alpha channel remains unchanged.
     *
     * @param n A factor by which to darken the color (default is 1.2).
     * @return A `ColorOf` object representing the darkened color.
     */
    constexpr ColorOf darker(Tfloat lightnessOffset) const noexcept {
        return adjust(-lightnessOffset);
    }

    /**
     * @brief Adjusts the color based on lightness and chroma factors.
     *
     * This function converts a given sRGB color to the OKLAB color space, applies the specified
     * lightness offset and chroma multiplier, and returns the adjusted color.
     *
     * @param luminanceOffset The lightness adjustment value (clamped between -100 and +100).
     * @param chromaMultiplier A multiplier for the chroma channels (default is 1.0).
     * @return A `ColorOf` object representing the adjusted color.
     */
    constexpr ColorOf adjust(Tfloat lightnessOffset, Tfloat chromaMultiplier = 1.f) const noexcept {
        Trichromatic lab = Trichromatic(*this).convert(ColorSpace::OKLAB);
        lab[0]           = std::clamp(lab[0] + lightnessOffset, 0., 100.);
        lab[1]           = lab[1] * chromaMultiplier;
        lab[2]           = lab[2] * chromaMultiplier;
        return ColorF(lab, ColorF(alpha).a);
    }

    /**
     * @brief Computes the lightness of the color.
     *
     * This function calculates the lightness of the color based on its red, green, and blue channels.
     *
     * @return The computed lightness value.
     */
    constexpr T lightness() const {
        return computeY<T>(r, g, b);
    }

    /**
     * @brief Desaturates the color by a specified factor.
     *
     * This function returns a new `ColorOf` instance representing a desaturated version of the color,
     * blending the original color with its lightness based on the provided factor.
     *
     * @param t A factor for desaturation (default is 1.0).
     * @return A `ColorOf` object representing the desaturated color.
     */
    constexpr ColorOf desaturate(Tfloat t = 1.0) const {
        const T l = lightness();
        return ColorOf(mix(t, simd_rgb(), concat(SIMD<T, 3>(l))), a);
    }

    /**
     * @brief Normalizes the color values to a unit lightness.
     *
     * This function returns a new `ColorOf` instance with the color channels normalized based on the
     * computed lightness, ensuring the color has a lightness of 1.
     *
     * @return A `ColorOf` object representing the normalized color.
     */
    constexpr ColorOf normalize() const {
        return ColorOf(v / lightness(), a);
    }

    /**
     * @brief Sets the red channel of the color.
     *
     * This function returns a new `ColorOf` instance with the specified red channel value, while
     * preserving the green, blue, and alpha channels.
     *
     * @param r The new red channel value.
     * @return A `ColorOf` object with the updated red channel.
     */
    constexpr ColorOf withRed(T r) const noexcept {
        return ColorOf(blend<1, 0, 0, 0>(v, SIMD<T, 4>(r)));
    }

    /**
     * @brief Sets the green channel of the color.
     *
     * This function returns a new `ColorOf` instance with the specified green channel value, while
     * preserving the red, blue, and alpha channels.
     *
     * @param g The new green channel value.
     * @return A `ColorOf` object with the updated green channel.
     */
    constexpr ColorOf withGreen(T g) const noexcept {
        return ColorOf(blend<0, 1, 0, 0>(v, SIMD<T, 4>(g)));
    }

    /**
     * @brief Sets the blue channel of the color.
     *
     * This function returns a new `ColorOf` instance with the specified blue channel value, while
     * preserving the red, green, and alpha channels.
     *
     * @param b The new blue channel value.
     * @return A `ColorOf` object with the updated blue channel.
     */
    constexpr ColorOf withBlue(T b) const noexcept {
        return ColorOf(blend<0, 0, 1, 0>(v, SIMD<T, 4>(b)));
    }

    /**
     * @brief Sets the red channel of the color using a floating-point value.
     *
     * This function returns a new `ColorOf` instance with the specified floating-point red channel
     * value, while preserving the other channels.
     *
     * @param r The new red channel value as a floating-point number.
     * @return A `ColorOf` object with the updated red channel.
     */
    constexpr ColorOf withRedF(Tfloat r) const noexcept {
        return ColorFloat(*this).withRed(r);
    }

    /**
     * @brief Sets the green channel of the color using a floating-point value.
     *
     * This function returns a new `ColorOf` instance with the specified floating-point green channel
     * value, while preserving the other channels.
     *
     * @param g The new green channel value as a floating-point number.
     * @return A `ColorOf` object with the updated green channel.
     */
    constexpr ColorOf withGreenF(Tfloat g) const noexcept {
        return ColorFloat(*this).withGreen(g);
    }

    /**
     * @brief Sets the blue channel of the color using a floating-point value.
     *
     * This function returns a new `ColorOf` instance with the specified floating-point blue channel
     * value, while preserving the other channels.
     *
     * @param b The new blue channel value as a floating-point number.
     * @return A `ColorOf` object with the updated blue channel.
     */
    constexpr ColorOf withBlueF(Tfloat b) const noexcept {
        return ColorFloat(*this).withBlue(b);
    }

    /**
     * @brief Multiplies the color's alpha channel by a specified factor.
     *
     * This function returns a new `ColorOf` instance with the alpha channel multiplied by the specified
     * factor according to the alpha mode (Straight or Premultiplied).
     *
     * @param a The factor to multiply the alpha channel by.
     * @param mode The alpha blending mode (default is `AlphaMode::Straight`).
     * @return A `ColorOf` object with the modified alpha channel.
     */
    constexpr ColorOf multiplyAlpha(Tfloat a, AlphaMode mode = AlphaMode::Straight) const noexcept {
        if (mode == AlphaMode::Straight)
            return ColorFloat(ColorFloat(*this).simd_rgb(), toFloat(this->a) * a);
        else
            return ColorFloat(ColorFloat(*this).v * a);
    }

    /**
     * @brief Unpremultiplies the color.
     *
     * This function returns a new `ColorOf` instance that is the result of unpremultiplying the color's
     * values. If the alpha channel is zero, the original color is returned.
     *
     * @return A `ColorOf` object representing the unpremultiplied color.
     */
    constexpr ColorOf unpremultiply() const noexcept {
        if (a == 0)
            return *this;
        ColorFloat flt = *this;
        return ColorFloat(flt.v / flt.a, flt.a);
    }

    /**
     * @brief Premultiplies the color.
     *
     * This function returns a new `ColorOf` instance that is the result of premultiplying the color's
     * values by its alpha channel.
     *
     * @return A `ColorOf` object representing the premultiplied color.
     */
    constexpr ColorOf premultiply() const noexcept {
        ColorFloat flt = *this;
        return ColorFloat(flt.v * flt.a, flt.a);
    }

    /**
     * @brief Converts the color between different alpha modes.
     *
     * This function converts the alpha representation between two specified modes (Straight and
     * Premultiplied). If the source mode matches the destination mode, the original color is returned.
     *
     * @param dstMode The desired destination alpha mode.
     * @param srcMode The current source alpha mode.
     * @return A `ColorOf` object converted to the desired mode.
     */
    constexpr ColorOf convertAlpha(AlphaMode dstMode, AlphaMode srcMode) const noexcept {
        if (srcMode == dstMode)
            return *this;
        if (srcMode == AlphaMode::Straight)
            return premultiply();
        else
            return unpremultiply();
    }

    /**
     * @brief Compares two `ColorOf` objects for equality.
     *
     * This operator checks if the color channels of the current instance are equal to those of another
     * `ColorOf` instance.
     *
     * @param c The `ColorOf` object to compare against.
     * @return `true` if the colors are equal, `false` otherwise.
     */
    constexpr bool operator==(const ColorOf& c) const {
        return v == c.v;
    }

    /**
     * @brief Compares two `ColorOf` objects for inequality.
     *
     * This operator checks if the color channels of the current instance are not equal to those of
     * another `ColorOf` instance.
     *
     * @param c The `ColorOf` object to compare against.
     * @return `true` if the colors are not equal, `false` otherwise.
     */
    constexpr bool operator!=(const ColorOf& c) const {
        return !(*this == c);
    }

    using vec_type = SIMD<T, 4>;

    union {
        struct {
            T r;
            T g;
            T b;
            T a;
        };

        struct {
            T red;
            T green;
            T blue;
            T alpha;
        };

        vec_type v;
        T array[4];
    };

    /**
     * @brief Gets the alpha component of the color as a 1-element SIMD vector.
     *
     * @return A SIMD vector representing the alpha component.
     */
    constexpr SIMD<T, 1> simd_a() const noexcept {
        return SIMD{ a };
    }

    template <typename U>
    static auto toFloat(U x) {
        return rescale<Tfloat, 1, maximum, T>(x);
    }

    template <typename U>
    static auto fromFloat(U x) {
        return rescale<T, maximum, 1, Tfloat>(x);
    }

    constexpr static std::tuple reflection{
        ReflectionField{ "r", &ColorOf::r },
        ReflectionField{ "g", &ColorOf::g },
        ReflectionField{ "b", &ColorOf::b },
        ReflectionField{ "a", &ColorOf::a },
    };
};

/**
 * @brief Converts a 32-bit color in 0xAABBGGRR format to `Color`.
 *
 * This function converts a 32-bit integer where the color is stored as 0xAABBGGRR (memory layout is RGBA)
 * into a `Color` object.
 *
 * @param x The 32-bit integer representing the color in 0xAABBGGRR format.
 * @return The corresponding `Color` object.
 */
constexpr BRISK_INLINE Color abgrToColor(uint32_t x) {
    return Color(SIMD<uint8_t, 4>{ std::bit_cast<SIMD<uint8_t, 4>>(x) });
}

/**
 * @brief Converts a 32-bit color in 0xRRGGBBAA format to `Color`.
 *
 * This function converts a 32-bit integer where the color is stored as 0xRRGGBBAA (memory layout is ABGR)
 * into a `Color` object.
 *
 * @param x The 32-bit integer representing the color in 0xRRGGBBAA format.
 * @return The corresponding `Color` object.
 */
constexpr BRISK_INLINE Color rgbaToColor(uint32_t x) {
    return abgrToColor(Internal::byteswap(x));
}

/**
 * @brief Converts a 32-bit color in 0xRRGGBB format to `Color`.
 *
 * This function converts a 32-bit integer where the color is stored as 0xRRGGBB (memory layout is BGR)
 * into a `Color` object.
 *
 * @param x The 32-bit integer representing the color in 0xRRGGBB format.
 * @return The corresponding `Color` object.
 */
constexpr BRISK_INLINE Color rgbToColor(uint32_t x) {
    return rgbaToColor((x << 8) | 0xFF);
}

/**
 * @brief Mixes two colors using linear interpolation.
 *
 * This function interpolates between two `ColorOf` instances based on a given weight `t`, with optional
 * alpha blending modes.
 *
 * @param t      The interpolation weight (0.0 for `a`, 1.0 for `b`).
 * @param a      The first color.
 * @param b      The second color.
 * @param mode   The alpha blending mode. Defaults to `AlphaMode::Straight`.
 *
 * @return The interpolated `ColorOf` object.
 */
template <typename T, ColorGamma G>
inline ColorOf<T, G> mix(float t, const ColorOf<T, G>& a, const ColorOf<T, G>& b,
                         AlphaMode mode = AlphaMode::Straight) {
    if (mode == AlphaMode::Straight)
        return mix(t, a.premultiply(), b.premultiply(), AlphaMode::Premultiplied).unpremultiply();
    else
        return ColorOf<T, G>(mix(t, a.v, b.v));
}

/**
 * @brief Converts a color from sRGB color space to default color space.
 *
 * This function takes a color in the sRGB color space and converts it to the default
 * color space (determined by the `linearColor` variable). The alpha channel is preserved.
 *
 * @tparam T The data type used for color components.
 * @param c The color in sRGB color space.
 * @return A `ColorOf<T, ColorGamma::Default>` object representing the color in default space.
 */
template <typename T>
BRISK_INLINE ColorOf<T, ColorGamma::Default> from_srgb(ColorOf<T, ColorGamma::sRGB> c) {
    return ColorOf<T, ColorGamma::Default>(Internal::srgbGammaToLinear(slice<0, 3>(c.v)), c.a);
}

/**
 * @brief Converts a color from default color space to sRGB color space.
 *
 * This function takes a color in the default color space (determined by the `linearColor` variable) and
 * converts it to the sRGB color space. The alpha channel is preserved.
 *
 * @tparam T The data type used for color components.
 * @param c The color in default color space.
 * @return A `ColorOf<T, ColorGamma::sRGB>` object representing the color in sRGB space.
 */
template <typename T>
BRISK_INLINE ColorOf<T, ColorGamma::sRGB> to_srgb(ColorOf<T, ColorGamma::Default> c) {
    return ColorOf<T, ColorGamma::sRGB>(Internal::srgbLinearToGamma(slice<0, 3>(c.v)), c.a);
}

/**
 * @brief Converts a constant in the form 0xRRGGBBAA (ABGR layout) to a `Color`.
 *
 * This user-defined literal allows for easy conversion from a hexadecimal representation of a
 * color to a `Color` object.
 *
 * @param rgba The unsigned integer representation of the color.
 * @return A `Color` object representing the specified RGBA value.
 */
constexpr Color operator""_rgba(unsigned long long rgba) {
    return rgbaToColor(rgba);
}

/**
 * @brief Converts a constant in the form 0xRRGGBB (BGR layout) to a `Color`.
 *
 * This user-defined literal allows for easy conversion from a hexadecimal representation of a
 * color to a `Color` object.
 *
 * @param rgb The unsigned integer representation of the color.
 * @return A `Color` object representing the specified RGB value.
 */
constexpr Color operator""_rgb(unsigned long long rgb) {
    return rgbToColor(rgb);
}

namespace Internal {
template <typename T, ColorGamma Gamma>
constexpr PixelType pixelTypeFor() {
    if constexpr (std::is_same_v<T, uint8_t>) {
        if constexpr (Gamma == ColorGamma::sRGB)
            return PixelType::U8Gamma;
        else
            return PixelType::U8;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return PixelType::U16;
    } else if constexpr (std::is_same_v<T, float>) {
        return PixelType::F32;
    } else {
        static_assert(sizeof(T) == 0, "Invalid type passed to pixelTypeFor");
    }
}
} // namespace Internal

/**
 * @brief Converts a pixel to a color representation based on its type and format.
 *
 * This function takes a pixel of the specified type and format, converts it into an intermediate RGBA
 * pixel format, and then returns a `ColorOf` instance with the appropriate color space.
 *
 * @tparam typ The pixel type (e.g., integer, float).
 * @tparam fmt The pixel format (e.g., RGB, RGBA).
 * @param pixel The input pixel to convert.
 * @return A `ColorOf<PixelTypeOf<typ>, pixelTypeToGamma(typ)>` object representing the color.
 */
template <PixelType typ, PixelFormat fmt>
BRISK_INLINE ColorOf<PixelTypeOf<typ>, pixelTypeToGamma(typ)> pixelToColor(Pixel<typ, fmt> pixel) noexcept {
    Pixel<typ, PixelFormat::RGBA> rgbaPixel = cvtPixel<PixelFormat::RGBA>(pixel);
    return ColorOf<PixelTypeOf<typ>, pixelTypeToGamma(typ)>{ rgbaPixel.r, rgbaPixel.g, rgbaPixel.b,
                                                             rgbaPixel.a };
}

/**
 * @brief Converts a pixel to a color and stores the result in a pre-existing `ColorOf` object.
 *
 * This overload allows for in-place conversion, storing the result in the provided `result` parameter.
 *
 * @tparam T The data type for color components.
 * @tparam gamma The gamma space of the resulting color.
 * @tparam typ The pixel type.
 * @tparam fmt The pixel format.
 * @param result Reference to a `ColorOf<T, gamma>` where the converted color will be stored.
 * @param pixel The input pixel to convert.
 */
template <typename T, ColorGamma gamma, PixelType typ, PixelFormat fmt>
BRISK_INLINE void pixelToColor(ColorOf<T, gamma>& result, Pixel<typ, fmt> pixel) noexcept {
    result = pixelToColor(pixel);
}

/**
 * @brief Converts a color to a pixel representation based on its data type and gamma.
 *
 * This function converts a `ColorOf` instance into a pixel with the specified format and gamma correction.
 * The result is always in the RGBA pixel format, suitable for further conversion to other formats.
 *
 * @tparam T The data type used for color components.
 * @tparam Gamma The gamma space of the color.
 * @param color The color to convert.
 * @return A `Pixel<Internal::pixelTypeFor<T, Gamma>(), PixelFormat::RGBA>` representing the color as a pixel.
 */
template <typename T, ColorGamma Gamma>
BRISK_INLINE Pixel<Internal::pixelTypeFor<T, Gamma>(), PixelFormat::RGBA> colorToPixel(
    ColorOf<T, Gamma> color) noexcept {
    return Pixel<Internal::pixelTypeFor<T, Gamma>(), PixelFormat::RGBA>{ color.r, color.g, color.b, color.a };
}

/**
 * @brief Converts a color to a pixel and stores the result in a pre-existing `Pixel` object.
 *
 * This overload allows for in-place conversion, storing the result in the provided `result` parameter.
 * The input color is first converted to the intermediate RGBA format before being converted to the final
 * pixel format.
 *
 * @tparam typ The pixel type.
 * @tparam fmt The pixel format.
 * @tparam T The data type used for color components.
 * @tparam Gamma The gamma space of the color.
 * @param result Reference to a `Pixel<typ, fmt>` where the converted pixel will be stored.
 * @param color The color to convert.
 */
template <PixelType typ, PixelFormat fmt, typename T, ColorGamma Gamma>
BRISK_INLINE void colorToPixel(Pixel<typ, fmt>& result, ColorOf<T, Gamma> color) noexcept {
    ColorOf<PixelTypeOf<typ>, pixelTypeToGamma(typ)> tmpColor = color;
    Pixel<typ, PixelFormat::RGBA> rgbaPixel{ tmpColor.r, tmpColor.g, tmpColor.b, tmpColor.a };
    result = cvtPixel<fmt>(rgbaPixel);
}

/**
 * @brief A namespace containing predefined colors for easy access.
 */
namespace Palette {
constexpr inline Color white       = 0xFFFFFF_rgb;     ///< White color.
constexpr inline Color black       = 0x000000_rgb;     ///< Black color.
constexpr inline Color red         = 0xFF0000_rgb;     ///< Red color.
constexpr inline Color green       = 0x00FF00_rgb;     ///< Green color.
constexpr inline Color blue        = 0x0000FF_rgb;     ///< Blue color.
constexpr inline Color yellow      = 0xFFFF00_rgb;     ///< Yellow color.
constexpr inline Color cyan        = 0x00FFFF_rgb;     ///< Cyan color.
constexpr inline Color magenta     = 0xFF00FF_rgb;     ///< Magenta color.
constexpr inline Color transparent = 0x000000'00_rgba; ///< Transparent color.
constexpr inline Color grey        = 0x808080_rgb;     ///< Grey color.
} // namespace Palette

} // namespace Brisk
