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

#include <cstdint>
#include <brisk/core/Reflection.hpp>
#include <brisk/core/Reflection.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Simd.hpp>

namespace Brisk {

/**
 * @enum PixelType
 * @brief Enumeration representing different pixel data types.
 */
enum class PixelType : uint8_t {
    U8,             ///< 8-bit unsigned integer.
    U8Gamma,        ///< 8-bit unsigned integer with gamma correction.
    U16,            ///< 16-bit unsigned integer.
    F32,            ///< 32-bit floating point.
    Last    = F32,  ///< Last valid pixel type, same as F32.
    Unknown = 0xFF, ///< Unknown pixel type.
};

/**
 * @brief Converts a gamma-corrected pixel type to its non-gamma equivalent.
 *
 * @param type The pixel type to convert.
 * @return The non-gamma equivalent pixel type.
 */
constexpr PixelType noGamma(PixelType type) {
    return type == PixelType::U8Gamma ? PixelType::U8 : type;
}

/**
 * @brief Default names for each pixel type.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<PixelType>> defaultNames<PixelType>{
    { "U8", PixelType::U8 },
    { "U8Gamma", PixelType::U8Gamma },
    { "U16", PixelType::U16 },
    { "F32", PixelType::F32 },
};

/**
 * @brief Converts a PixelType to its corresponding uint8_t value.
 *
 * @param fmt The pixel type to convert.
 * @return The corresponding uint8_t value.
 */
constexpr uint8_t operator+(PixelType fmt) noexcept {
    return static_cast<uint8_t>(fmt);
}

/**
 * @brief Returns the size in bytes of a given PixelType.
 *
 * @param type The pixel type whose size is to be determined.
 * @return The size in bytes.
 */
constexpr int32_t pixelTypeSize(PixelType type) noexcept {
    switch (type) {
    case PixelType::U8:
        return sizeof(uint8_t);
    case PixelType::U8Gamma:
        return sizeof(uint8_t);
    case PixelType::U16:
        return sizeof(uint16_t);
    case PixelType::F32:
        return sizeof(float);
    default:
        return 0;
    }
}

/**
 * @typedef UntypedPixel
 * @brief Represents an untyped pixel using std::byte.
 */
using UntypedPixel = std::byte;

namespace Internal {
template <PixelType T>
struct PixelTypeT;

template <>
struct PixelTypeT<PixelType::Unknown> {
    using Type = UntypedPixel;
};

template <>
struct PixelTypeT<PixelType::U8> {
    using Type = uint8_t;
};

template <>
struct PixelTypeT<PixelType::U8Gamma> {
    using Type = uint8_t;
};

template <>
struct PixelTypeT<PixelType::U16> {
    using Type = uint16_t;
};

template <>
struct PixelTypeT<PixelType::F32> {
    using Type = float;
};
} // namespace Internal

/**
 * @brief Type alias to get the pixel type corresponding to a PixelType enum.
 */
template <PixelType Type>
using PixelTypeOf = typename Internal::PixelTypeT<Type>::Type;

/**
 * @enum AlphaMode
 * @brief Enumeration representing different alpha modes.
 */
enum class AlphaMode : uint8_t {
    Straight,      ///< Straight alpha.
    Premultiplied, ///< Premultiplied alpha.
};

/**
 * @enum PixelFormat
 * @brief Enumeration representing different pixel formats.
 */
enum class PixelFormat : uint8_t {
    //                    Components  [0]     [1]     [2]     [3]
    RGB,             //   3           Red     Green   Blue    -
    RGBA,            //   4           Red     Green   Blue    Alpha
    ARGB,            //   4           Alpha   Red     Green   Blue
    BGR,             //   3           Blue    Green   Red     -
    BGRA,            //   4           Blue    Green   Red     Alpha
    ABGR,            //   4           Alpha   Blue    Green   Red
    GreyscaleAlpha,  //   2           Grey    Alpha
    Greyscale,       //   1           Grey    -       -       -
    Alpha,           //   1           Alpha   -       -       -
    Last    = Alpha, ///< Last valid pixel format, same as Alpha.
    Unknown = 0xFF,  ///< Unknown pixel format.
    Raw     = 0xFE,  ///< Raw pixel format.
};

/**
 * @brief Maps the number of components to the corresponding pixel format.
 *
 * @param comp Number of components.
 * @return The corresponding PixelFormat.
 */
constexpr PixelFormat componentsToFormat(int comp) {
    return staticMap(comp,                           //
                     1, PixelFormat::Greyscale,      //
                     2, PixelFormat::GreyscaleAlpha, //
                     3, PixelFormat::RGB,            //
                     4, PixelFormat::RGBA,           //
                     PixelFormat::Unknown);
}

/**
 * @brief Default names for each pixel format.
 */
template <>
inline constexpr std::initializer_list<NameValuePair<PixelFormat>> defaultNames<PixelFormat>{
    { "RGB", PixelFormat::RGB },
    { "RGBA", PixelFormat::RGBA },
    { "ARGB", PixelFormat::ARGB },
    { "BGR", PixelFormat::BGR },
    { "BGRA", PixelFormat::BGRA },
    { "ABGR", PixelFormat::ABGR },
    { "GreyscaleAlpha", PixelFormat::GreyscaleAlpha },
    { "Greyscale", PixelFormat::Greyscale },
    { "Alpha", PixelFormat::Alpha },
};

/**
 * @brief Converts a PixelFormat to its corresponding uint8_t value.
 *
 * @param fmt The pixel format to convert.
 * @return The corresponding uint8_t value.
 */
constexpr uint8_t operator+(PixelFormat fmt) noexcept {
    return static_cast<uint8_t>(fmt);
}

/**
 * @enum PixelFlagColor
 * @brief Enumeration representing pixel color flags.
 */
enum class PixelFlagColor : uint8_t {
    None,      ///< No color flag.
    RGB,       ///< RGB color flag.
    Greyscale, ///< Greyscale color flag.
};

/**
 * @enum PixelFlagAlpha
 * @brief Enumeration representing pixel alpha flags.
 */
enum class PixelFlagAlpha : uint8_t {
    None,       ///< No alpha flag.
    AlphaFirst, ///< Alpha component is first.
    AlphaLast,  ///< Alpha component is last.
};

/**
 * @struct PixelFormatDesc
 * @brief Structure describing pixel format properties.
 */
struct PixelFormatDesc {
    uint8_t components;   ///< Number of components in the pixel format.
    PixelFlagColor color; ///< Color flag for the pixel format.
    PixelFlagAlpha alpha; ///< Alpha flag for the pixel format.
    bool reversed;        ///< Indicates if the pixel format is reversed.
};

/**
 * @brief Array of descriptions for various pixel formats.
 */
constexpr inline PixelFormatDesc pixelFormatDesc[]{
    /* RGB   */ { 3, PixelFlagColor::RGB, PixelFlagAlpha::None, false },
    /* RGBA  */ { 4, PixelFlagColor::RGB, PixelFlagAlpha::AlphaLast, false },
    /* ARGB  */ { 4, PixelFlagColor::RGB, PixelFlagAlpha::AlphaFirst, false },
    /* BGR   */ { 3, PixelFlagColor::RGB, PixelFlagAlpha::None, true },
    /* BGRA  */ { 4, PixelFlagColor::RGB, PixelFlagAlpha::AlphaLast, true },
    /* ABGR  */ { 4, PixelFlagColor::RGB, PixelFlagAlpha::AlphaFirst, true },
    /* GreyA */ { 2, PixelFlagColor::Greyscale, PixelFlagAlpha::AlphaLast, false },
    /* Grey  */ { 1, PixelFlagColor::Greyscale, PixelFlagAlpha::None, false },
    /* A     */ { 1, PixelFlagColor::None, PixelFlagAlpha::AlphaFirst, false },
};

/**
 * @brief Returns the number of components in a given PixelFormat.
 *
 * @param fmt The pixel format.
 * @return The number of components.
 */
constexpr int pixelComponents(PixelFormat fmt) noexcept {
    return pixelFormatDesc[+fmt].components;
}

/**
 * @brief Returns the color flag for a given PixelFormat.
 *
 * @param fmt The pixel format.
 * @return The color flag.
 */
constexpr PixelFlagColor pixelColor(PixelFormat fmt) noexcept {
    return pixelFormatDesc[+fmt].color;
}

/**
 * @brief Returns the alpha flag for a given PixelFormat.
 *
 * @param fmt The pixel format.
 * @return The alpha flag.
 */
constexpr PixelFlagAlpha pixelAlpha(PixelFormat fmt) noexcept {
    return pixelFormatDesc[+fmt].alpha;
}

/**
 * @brief Checks if a given PixelFormat is reversed.
 *
 * @param fmt The pixel format.
 * @return True if reversed, false otherwise.
 */
constexpr bool pixelIsReversed(PixelFormat fmt) noexcept {
    return pixelFormatDesc[+fmt].reversed;
}

/**
 * @brief Calculates the total size in bytes for a pixel of given type and format.
 *
 * @param type The pixel type.
 * @param format The pixel format.
 * @return The total size in bytes.
 */
constexpr int32_t pixelSize(PixelType type, PixelFormat format) noexcept {
    return pixelTypeSize(type) * pixelComponents(format);
}

/**
 * @brief Template variable to hold the maximum alpha value for different types.
 */
template <SimdCompatible T>
[[maybe_unused]] constexpr inline T alpha = std::numeric_limits<T>::max();

/**
 * @brief Specialization for float type, setting alpha to 1.0f.
 */
template <>
[[maybe_unused]] constexpr inline float alpha<float> = 1.f;

/**
 * @brief Specialization for double type, setting alpha to 1.0.
 */
template <>
[[maybe_unused]] constexpr inline float alpha<double> = 1.;

template <PixelType Type, PixelFormat fmt>
struct PixelStruct;

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::RGBA> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T r, g, b, a;
        };

        T c[4];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T r, T g, T b, T a) noexcept : r(r), g(g), b(b), a(a) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "r", &PixelStruct::r },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "b", &PixelStruct::b },
        ReflectionField{ "a", &PixelStruct::a },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::ARGB> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T a, r, g, b;
        };

        T c[4];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T a, T r, T g, T b) noexcept : a(a), r(r), g(g), b(b) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "a", &PixelStruct::a },
        ReflectionField{ "r", &PixelStruct::r },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "b", &PixelStruct::b },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::BGRA> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T b, g, r, a;
        };

        T c[4];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T b, T g, T r, T a) noexcept : b(b), g(g), r(r), a(a) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "b", &PixelStruct::b },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "r", &PixelStruct::r },
        ReflectionField{ "a", &PixelStruct::a },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::ABGR> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T a, b, g, r;
        };

        T c[4];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T a, T b, T g, T r) noexcept : a(a), b(b), g(g), r(r) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "a", &PixelStruct::a },
        ReflectionField{ "b", &PixelStruct::b },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "r", &PixelStruct::r },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::RGB> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T r, g, b;
        };

        T c[3];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T r, T g, T b) noexcept : r(r), g(g), b(b) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "r", &PixelStruct::r },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "b", &PixelStruct::b },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::BGR> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T b, g, r;
        };

        T c[3];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T b, T g, T r) noexcept : b(b), g(g), r(r) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "b", &PixelStruct::b },
        ReflectionField{ "g", &PixelStruct::g },
        ReflectionField{ "r", &PixelStruct::r },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::Greyscale> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T grey;
        };

        T c[1];
    };

    operator T() const noexcept {
        return grey;
    }

    operator T&() noexcept {
        return grey;
    }

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T grey) noexcept : grey(grey) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "grey", &PixelStruct::grey },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::GreyscaleAlpha> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T grey;
            T a;
        };

        T c[2];
    };

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T grey, T a) noexcept : grey(grey), a(a) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "grey", &PixelStruct::grey },
        ReflectionField{ "a", &PixelStruct::a },
    };
};

template <PixelType Type>
struct PixelStruct<Type, PixelFormat::Alpha> {
    using T = PixelTypeOf<Type>;

    union {
        struct {
            T a;
        };

        T c[1];
    };

    operator T() const noexcept {
        return a;
    }

    operator T&() noexcept {
        return a;
    }

    constexpr PixelStruct() noexcept = default;

    constexpr PixelStruct(T a) noexcept : a(a) {}

    inline static const std::tuple reflection = {
        ReflectionField{ "a", &PixelStruct::a },
    };
};

template <PixelType typ, PixelFormat fmt>
struct Pixel;

template <PixelFormat dstFmt, PixelType typ, PixelFormat srcFmt>
Pixel<typ, dstFmt> cvtPixel(Pixel<typ, srcFmt> src) noexcept;

template <PixelType typ, PixelFormat fmt>
struct Pixel : public PixelStruct<typ, fmt> {

    using T                             = PixelTypeOf<typ>;

    constexpr static PixelFormat format = fmt;

    T& operator[](size_t n) noexcept {
        return this->c[n];
    }

    const T& operator[](size_t n) const noexcept {
        return this->c[n];
    }

    using PixelStruct<typ, fmt>::PixelStruct;

    constexpr Pixel() noexcept                        = default;
    constexpr Pixel(const Pixel&) noexcept            = default;
    constexpr Pixel& operator=(const Pixel&) noexcept = default;

    template <PixelFormat srcFmt>
    Pixel(Pixel<typ, srcFmt> pixel) noexcept {
        *this = cvtPixel<fmt>(pixel);
    }

    explicit Pixel(const Simd<T, pixelComponents(fmt)>& v) noexcept {
        std::memcpy(this->c, &v, std::size(this->c) * sizeof(T));
    }

    constexpr bool operator==(const Pixel& other) const noexcept {
        return memcmp(this->c, other.c, std::size(this->c) * sizeof(T)) == 0;
    }

    constexpr bool operator!=(const Pixel& other) const noexcept {
        return !operator==(other);
    }
};

template <PixelType typ>
using PixelRGB = Pixel<typ, PixelFormat::RGB>;
template <PixelType typ>
using PixelRGBA = Pixel<typ, PixelFormat::RGBA>;
template <PixelType typ>
using PixelARGB = Pixel<typ, PixelFormat::ARGB>;
template <PixelType typ>
using PixelBGR = Pixel<typ, PixelFormat::BGR>;
template <PixelType typ>
using PixelBGRA = Pixel<typ, PixelFormat::BGRA>;
template <PixelType typ>
using PixelABGR = Pixel<typ, PixelFormat::ABGR>;
template <PixelType typ>
using PixelGreyscaleAlpha = Pixel<typ, PixelFormat::GreyscaleAlpha>;
template <PixelType typ>
using PixelGreyscale = Pixel<typ, PixelFormat::Greyscale>;
template <PixelType typ>
using PixelAlpha           = Pixel<typ, PixelFormat::Alpha>;

using PixelRGB8            = PixelRGB<PixelType::U8Gamma>;
using PixelRGBA8           = PixelRGBA<PixelType::U8Gamma>;
using PixelARGB8           = PixelARGB<PixelType::U8Gamma>;
using PixelBGR8            = PixelBGR<PixelType::U8Gamma>;
using PixelBGRA8           = PixelBGRA<PixelType::U8Gamma>;
using PixelABGR8           = PixelABGR<PixelType::U8Gamma>;
using PixelGreyscale8      = PixelGreyscale<PixelType::U8Gamma>;
using PixelGreyscaleAlpha8 = PixelGreyscaleAlpha<PixelType::U8Gamma>;
using PixelAlpha8          = PixelAlpha<PixelType::U8Gamma>;

/**
 * @brief Computes the luminance (Y) component from RGB values using the BT.601 formula.
 *
 * This function computes the luminance of a pixel based on its red, green, and blue components.
 * The formula used is Y = 0.299 * R + 0.587 * G + (1 - 0.299 - 0.587) * B.
 *
 * @tparam T The data type of the color components (e.g., float, uint8_t).
 * @param r The red component.
 * @param g The green component.
 * @param b The blue component.
 * @return The computed luminance value.
 */
template <typename T>
BRISK_INLINE T computeY(T r, T g, T b) noexcept {
    // BT.601
    constexpr double Kred   = 0.299;
    constexpr double Kgreen = 0.587;
    if constexpr (std::is_floating_point_v<T>) {
        constexpr T Kr = static_cast<T>(Kred);
        constexpr T Kg = static_cast<T>(Kgreen);
        constexpr T Kb = T(1) - Kr - Kg;
        return Kr * r + Kg * g + Kb * b;
    } else {
        constexpr uint32_t scale = 1u << std::numeric_limits<T>::digits;
        constexpr T Kr           = static_cast<T>(Kred * scale + 0.5);
        constexpr T Kg           = static_cast<T>(Kgreen * scale + 0.5);
        constexpr T Kb           = scale - (Kr + Kg);
        return T((Kr * r + Kg * g + Kb * b) >> std::numeric_limits<T>::digits);
    }
}

/**
 * @brief Multiplies the color components of a pixel by a given alpha value.
 *
 * This function adjusts the pixel color components based on the alpha value,
 * effectively blending the pixel with transparency.
 *
 * @tparam fmt The pixel format.
 * @tparam T The data type of the color components (e.g., float, uint8_t).
 * @param src The source pixel to be adjusted.
 * @param a The alpha value to multiply by.
 * @return The modified pixel with adjusted color components.
 */
template <PixelFormat fmt, PixelType typ>
BRISK_INLINE Pixel<typ, fmt> mulAlpha(Pixel<typ, fmt> src, PixelTypeOf<typ> a) noexcept {
    using T = PixelTypeOf<typ>;
    if constexpr (typ == PixelType::F32) {
        if constexpr (pixelColor(fmt) == PixelFlagColor::RGB) {
            src.r *= a;
            src.g *= a;
            src.b *= a;
        } else {
            src.grey *= a;
        }
        return src;
    } else {
        if constexpr (pixelColor(fmt) == PixelFlagColor::RGB) {
            src.r = uint32_t(src.r) * a / alpha<T>;
            src.g = uint32_t(src.g) * a / alpha<T>;
            src.b = uint32_t(src.b) * a / alpha<T>;
        } else {
            src.grey = uint32_t(src.grey) * a / alpha<T>;
        }
        return src;
    }
}

/**
 * @brief Converts a pixel from one format to another.
 *
 * This function handles conversions between various pixel formats,
 * including color to greyscale and vice versa.
 *
 * @tparam dstFmt The destination pixel format.
 * @tparam T The data type of the color components (e.g., float, uint8_t).
 * @tparam srcFmt The source pixel format.
 * @param src The source pixel to be converted.
 * @return The converted pixel in the destination format.
 */
template <PixelFormat dstFmt, PixelType typ, PixelFormat srcFmt>
BRISK_INLINE Pixel<typ, dstFmt> cvtPixel(Pixel<typ, srcFmt> src) noexcept {
    using T = PixelTypeOf<typ>;
    Pixel<typ, dstFmt> dst;
    if constexpr (pixelColor(dstFmt) == PixelFlagColor::RGB && pixelColor(srcFmt) == PixelFlagColor::RGB) {
        // Color -> Color
        dst.r = src.r;
        dst.g = src.g;
        dst.b = src.b;
    } else if constexpr (pixelColor(dstFmt) == PixelFlagColor::RGB &&
                         pixelColor(srcFmt) == PixelFlagColor::Greyscale) {
        // Greyscale -> Color
        dst.r = src.grey;
        dst.g = src.grey;
        dst.b = src.grey;
    } else if constexpr (pixelColor(dstFmt) == PixelFlagColor::Greyscale &&
                         pixelColor(srcFmt) == PixelFlagColor::RGB) {
        // Color -> Greyscale
        dst.grey = computeY<T>(src.r, src.g, src.b);
    } else if constexpr (pixelColor(dstFmt) == PixelFlagColor::Greyscale &&
                         pixelColor(srcFmt) == PixelFlagColor::Greyscale) {
        // Greyscale -> Greyscale
        dst.grey = src.grey;
    } else if constexpr (pixelColor(srcFmt) == PixelFlagColor::None) {
        // Handle case where source pixel has no color information.
        if constexpr (pixelColor(dstFmt) == PixelFlagColor::RGB) {
            dst.r = dst.g = dst.b = 0; // Set RGB components to zero.
        } else if constexpr (pixelColor(dstFmt) == PixelFlagColor::Greyscale) {
            dst.grey = 0; // Set grey component to zero.
        }
    }

    // Handle alpha channel
    if constexpr (pixelAlpha(dstFmt) != PixelFlagAlpha::None) {
        if constexpr (pixelAlpha(srcFmt) != PixelFlagAlpha::None) {
            dst.a = src.a;
        } else {
            dst.a = alpha<T>;
        }
    } else {
        if constexpr (pixelAlpha(srcFmt) != PixelFlagAlpha::None) {
            // multiply by alpha if destination has no alpha but source does
            dst = mulAlpha(dst, src.a);
        }
    }
    return dst;
}

/**
 * @brief Macro to assist with pixel type handling.
 *
 * This macro provides a way to handle operations for different pixel types in a concise manner.
 *
 * @param typ The pixel type.
 * @param ... The operation(s) to perform for the specified pixel type.
 */
#define DO_PIX_TYP_HLP(typ, ...)                                                                             \
    {                                                                                                        \
        constexpr PixelType TYP = typ;                                                                       \
        __VA_ARGS__                                                                                          \
    }

/**
 * @brief Macro to define behavior based on pixel type.
 *
 * This macro utilizes a switch statement to execute specific operations based on the provided pixel type.
 *
 * @param typ The pixel type.
 * @param ... The operation(s) to perform for each case of the specified pixel type.
 */
#define DO_PIX_TYP(typ, ...)                                                                                 \
    {                                                                                                        \
        switch (typ) {                                                                                       \
        case PixelType::U8:                                                                                  \
            DO_PIX_TYP_HLP(PixelType::U8, __VA_ARGS__);                                                      \
        case PixelType::U8Gamma:                                                                             \
            DO_PIX_TYP_HLP(PixelType::U8Gamma, __VA_ARGS__);                                                 \
        case PixelType::U16:                                                                                 \
            DO_PIX_TYP_HLP(PixelType::U16, __VA_ARGS__);                                                     \
        case PixelType::F32:                                                                                 \
            DO_PIX_TYP_HLP(PixelType::F32, __VA_ARGS__);                                                     \
        default:                                                                                             \
            BRISK_UNREACHABLE();                                                                             \
        }                                                                                                    \
    }

/**
 * @brief Macro to assist with pixel format handling.
 *
 * This macro provides a way to handle operations for different pixel formats in a concise manner.
 *
 * @param fmt The pixel format.
 * @param ... The operation(s) to perform for the specified pixel format.
 */
#define DO_PIX_FMT_HLP(fmt, ...)                                                                             \
    {                                                                                                        \
        constexpr PixelFormat FMT = fmt;                                                                     \
        __VA_ARGS__                                                                                          \
    }

/**
 * @brief Macro to define behavior based on pixel format.
 *
 * This macro utilizes a switch statement to execute specific operations based on the provided pixel format.
 *
 * @param fmt The pixel format.
 * @param ... The operation(s) to perform for each case of the specified pixel format.
 */
#define DO_PIX_FMT(fmt, ...)                                                                                 \
    {                                                                                                        \
        switch (fmt) {                                                                                       \
        case PixelFormat::RGB:                                                                               \
            DO_PIX_FMT_HLP(PixelFormat::RGB, __VA_ARGS__);                                                   \
        case PixelFormat::RGBA:                                                                              \
            DO_PIX_FMT_HLP(PixelFormat::RGBA, __VA_ARGS__);                                                  \
        case PixelFormat::ARGB:                                                                              \
            DO_PIX_FMT_HLP(PixelFormat::ARGB, __VA_ARGS__);                                                  \
        case PixelFormat::BGR:                                                                               \
            DO_PIX_FMT_HLP(PixelFormat::BGR, __VA_ARGS__);                                                   \
        case PixelFormat::BGRA:                                                                              \
            DO_PIX_FMT_HLP(PixelFormat::BGRA, __VA_ARGS__);                                                  \
        case PixelFormat::ABGR:                                                                              \
            DO_PIX_FMT_HLP(PixelFormat::ABGR, __VA_ARGS__);                                                  \
        case PixelFormat::GreyscaleAlpha:                                                                    \
            DO_PIX_FMT_HLP(PixelFormat::GreyscaleAlpha, __VA_ARGS__);                                        \
        case PixelFormat::Greyscale:                                                                         \
            DO_PIX_FMT_HLP(PixelFormat::Greyscale, __VA_ARGS__);                                             \
        case PixelFormat::Alpha:                                                                             \
            DO_PIX_FMT_HLP(PixelFormat::Alpha, __VA_ARGS__);                                                 \
        default:                                                                                             \
            BRISK_UNREACHABLE();                                                                             \
        }                                                                                                    \
    }

/**
 * @brief Converts a pixel to a specified destination format.
 *
 * This function handles the conversion of a pixel to a different format
 * and stores the result in the provided destination pointer.
 *
 * @tparam srcFmt The source pixel format.
 * @tparam T The data type of the color components (e.g., float, uint8_t).
 * @param dst Pointer to the destination pixel data.
 * @param src The source pixel to convert.
 * @param dstFmt The destination pixel format.
 */
template <PixelFormat srcFmt, PixelType typ, typename T = PixelTypeOf<typ>>
void cvtPixelTo(T* dst, Pixel<typ, srcFmt> src, PixelFormat dstFmt) noexcept {
    DO_PIX_FMT(dstFmt, *reinterpret_cast<Pixel<typ, FMT>*>(dst) = cvtPixel<FMT>(src); break;)
}

/**
 * @brief Converts pixel data from a source format to a destination pixel structure.
 *
 * This function handles the conversion of pixel data from a raw format
 * to a specified destination pixel format.
 *
 * @tparam dstFmt The destination pixel format.
 * @tparam T The data type of the color components (e.g., float, uint8_t).
 * @param dst The destination pixel structure to store the converted data.
 * @param src Pointer to the source pixel data.
 * @param srcFmt The source pixel format.
 */
template <PixelFormat dstFmt, PixelType typ, typename T = PixelTypeOf<typ>>
void cvtPixelFrom(Pixel<typ, dstFmt>& dst, const T* src, PixelFormat srcFmt) noexcept {
    DO_PIX_FMT(srcFmt, dst = cvtPixel<dstFmt>(*reinterpret_cast<const Pixel<typ, FMT>*>(src)); break;)
}

} // namespace Brisk
