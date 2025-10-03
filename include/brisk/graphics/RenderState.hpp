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

#include "Matrix.hpp"
#include "Gradients.hpp"
#include <brisk/core/MetaClass.hpp>
#include <brisk/core/Json.hpp>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/graphics/internal/Sprites.hpp>
#include <brisk/core/internal/Argument.hpp>

namespace Brisk {

namespace Internal {
constexpr inline uint32_t max2DTextureSize = 8192;
constexpr inline float textRectPadding     = 4 / 6.f; // 0.667f;
constexpr inline float textRectOffset      = 2 / 6.f; // 0.333f;
} // namespace Internal

struct GradientColors {
    ColorF color1;
    ColorF color2;
};

struct GradientPoints {
    PointF point1;
    PointF point2;
};

inline bool toJson(Brisk::Json& j, Rectangle r) {
    return Brisk::packArray(j, r.x1, r.y1, r.x2, r.y2);
}

inline bool toJson(Brisk::Json& j, Size s) {
    return Brisk::packArray(j, s.width, s.height);
}

inline bool fromJson(const Brisk::Json& j, Rectangle& r) {
    return Brisk::unpackArray(j, r.x1, r.y1, r.x2, r.y2);
}

inline bool fromJson(const Brisk::Json& j, Size& s) {
    return Brisk::unpackArray(j, s.width, s.height);
}

inline bool toJson(Brisk::Json& j, const ColorF& p) {
    return Brisk::packArray(j, p.r, p.g, p.b, p.a);
}

inline bool fromJson(const Brisk::Json& j, ColorF& p) {
    return Brisk::unpackArray(j, p.r, p.g, p.b, p.a);
}

enum class ShaderType : uint8_t {
    Rectangle = 0,
    Text      = 1, // Gradient or texture
    Shadow    = 2, // Custom paint or texture
    ColorMask = 3, // Gradient or texture
    Blit      = 4, // Texture
    Mask      = 5, // Gradient or texture
};

enum class ShadingType : int {
    Color                   = 0x00,
    SimpleGradientLinear    = 0x01, // apply_simple_gradient(linear_argument())
    SimpleGradientRadial    = 0x11, // apply_simple_gradient(radial_argument())
    SimpleGradientAngle     = 0x21, // apply_simple_gradient(angle_argument())
    SimpleGradientReflected = 0x31, // apply_simple_gradient(reflected_argument())

    GradientLinear          = 0x02, // apply_gradient(linear_argument())
    GradientRadial          = 0x12, // apply_gradient(radial_argument())
    GradientAngle           = 0x22, // apply_gradient(angle_argument())
    GradientReflected       = 0x32, // apply_gradient(reflected_argument())

    Texture                 = 0x03, // texture.sample(uv)

    TonedTextureC0          = 0x04, // apply_gradient(texture.sample(uv)[0])
    TonedTextureC1          = 0x14, // apply_gradient(texture.sample(uv)[1])
    TonedTextureC2          = 0x24, // apply_gradient(texture.sample(uv)[2])
    TonedTextureC3          = 0x34, // apply_gradient(texture.sample(uv)[3])

    MaskShading             = 0x0F,
    MaskArgument            = 0xF0,

};

enum class SubpixelMode : uint8_t {
    Off = 0,
    RGB = 1,
    BGR = 2,
};

enum class SamplerMode : uint8_t {
    Clamp = 0,
    Wrap  = 1,
};

struct GeometryGlyph {
    RectangleF rect;
    SizeF size;
    float sprite;
    float stride;
};

struct GeometryArc {
    PointF center;
    float outerRadius;
    float innerRadius;
    float startAngle;
    float stopAngle;
    float reserved1;
    float reserved2;
};

inline bool toJson(Json& b, const GradientColors& v) {
    return packArray(b, v.color1, v.color2);
}

inline bool fromJson(const Json& b, GradientColors& v) {
    return unpackArray(b, v.color1, v.color2);
}

struct PatternCodes {
    PatternCodes() : value(0) {}

    PatternCodes(uint16_t hpattern, uint16_t vpattern, uint8_t scale = 1) {
        value = hpattern & 0xFFF;
        value |= (vpattern & 0xFFF) << 12;
        value |= (scale & 0xFF) << 24;
    }

    uint32_t value;
};

struct ConstantPerFrame {
    Simd<float, 4> viewport;
    float blueLightFilter;
    float gamma;
    float textRectPadding;
    float textRectOffset;
    int atlasWidth;
};

struct RenderState;
struct RenderStateEx;

struct RenderBuffer;

enum class BlendingMode : uint8_t {
    Normal     = 0u,
    Multiply   = 1u,
    Screen     = 2u,
    Overlay    = 3u,
    Darken     = 4u,
    Lighten    = 5u,
    ColorDodge = 6u,
    ColorBurn  = 7u,
    HardLight  = 8u,
    SoftLight  = 9u,
    Difference = 10u,
    Exclusion  = 11u,
    Hue        = 12u,
    Saturation = 13u,
    Color      = 14u,
    Luminosity = 15u,
    Clip       = 128u,
};

enum class CompositionMode : uint8_t {
    Clear       = 0u,
    Copy        = 1u,
    Dest        = 2u,
    SrcOver     = 3u,
    DestOver    = 4u,
    SrcIn       = 5u,
    DestIn      = 6u,
    SrcOut      = 7u,
    DestOut     = 8u,
    SrcAtop     = 9u,
    DestAtop    = 10u,
    Xor         = 11u,
    Plus        = 12u,
    PlusLighter = 13u,
};

enum class BlendingCompositionMode : uint16_t {
    Normal =
        (static_cast<uint16_t>(BlendingMode::Normal) << 8) | static_cast<uint16_t>(CompositionMode::SrcOver),
};

constexpr BlendingCompositionMode toBlendingCompositionMode(BlendingMode blend,
                                                            CompositionMode comp) noexcept {
    return static_cast<BlendingCompositionMode>((static_cast<uint16_t>(blend) << 8) |
                                                static_cast<uint16_t>(comp));
}

struct RenderState {
    bool operator==(const RenderState& state) const;

public:
    // ---------------- CONTROL ----------------
    int dataOffset = 0; ///< Offset in data4 for current operation (multiply by 4 to get offset in data1)
    int dataSize   = 0; ///< Data size in floats
    int instances  = 1; ///< Number of quads to render
    int unused     = 0;

public:
    // ---------------- SHADER -----------------
    // packed into two uint32_t values:
    ShaderType shader            = ShaderType::Blit; ///< Type of geometry to generate
    bool hasTexture              = false;            ///<
    GradientType gradient        = GradientType::Linear;
    SubpixelMode subpixelMode    = SubpixelMode::RGB;

    uint8_t blurDirections       = 3;                  ///< 0 - disable, 1 - H, 2 - V, 3 - H&V
    uint8_t textureChannel       = 0;                  ///<
    SamplerMode samplerMode      = SamplerMode::Clamp; ///<
    uint8_t spriteOversampling   = 1;

    BlendingCompositionMode mode = BlendingCompositionMode::Normal;
    bool hasBackTexture          = false;
    uint8_t padding1             = 0;

    uint32_t packed3             = 0;

    int32_t gradientIndex        = -1;  ///< Gradient (-1 - disabled)
    float blurRadius             = 0.f; ///<
    uint32_t reserved1           = 0;
    uint32_t reserved2           = 0;

    Matrix coordMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f };       ///<
    Matrix textureMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f };     ///<
    Matrix backTextureMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f }; ///<

    PatternCodes pattern{};
    float opacity         = 1.f; ///< Opacity. Defaults to 1

    ColorF fillColor1     = Palette::white; ///< Fill (brush) color for gradient at 0%
    ColorF fillColor2     = Palette::white; ///< Fill (brush) color for gradient at 100%

    PointF gradientPoint1 = { 0.f, 0.f };     ///< 0% Gradient point
    PointF gradientPoint2 = { 100.f, 100.f }; ///< 100% Gradient point

    Rectangle scissor     = noClipRect;

    union {
        Internal::ImageBackend* sourceImage = nullptr;
        uint64_t dummy1;
    };

    union {
        Internal::ImageBackend* backImage = nullptr;
        uint64_t dummy2;
    };

    Simd<uint32_t, 4> reserved3;
    Simd<uint32_t, 4> reserved4;
    Simd<uint32_t, 4> reserved5;

public:
    bool compare(const RenderState& second) const;
    void premultiply();

    constexpr static size_t compare_offset = 12;
};

static_assert(sizeof(RenderState) % 256 == 0, "sizeof(RenderState) % 256 == 0");

inline bool requiresAtlasOrGradient(std::span<const RenderState> commands) {
    for (const RenderState& cmd : commands) {
        switch (cmd.shader) {
        case ShaderType::Blit:
            break;
        default:
            return true;
        }
    }
    return false;
}

static_assert(std::is_trivially_copy_constructible_v<RenderState>);

struct RenderStateEx;

using RenderStateExArgs = ArgumentsView<RenderStateEx>;

template <typename Tag, typename U>
void applier(RenderStateEx* target, const ArgVal<Tag, U>& arg) {
    Tag::apply(arg.value, *target);
}

using SpriteResources = SmallVector<Rc<SpriteResource>, 1>;

struct RenderStateEx : RenderState {
    explicit RenderStateEx(ShaderType shader, RenderStateExArgs args);

    explicit RenderStateEx(ShaderType shader, int instances, RenderStateExArgs args);

    Rc<Image> sourceImageHandle;
    Rc<Image> backImageHandle;
    Rc<GradientResource> gradientHandle;
    SpriteResources sprites;
};

class RenderContext {
    BRISK_DYNAMIC_CLASS_ROOT(RenderContext)
public:
    virtual void command(RenderStateEx&& cmd, std::span<const uint32_t> data = {}) = 0;

    virtual void setGlobalScissor(Rectangle rect)                                  = 0;

    template <typename T>
    void command(RenderStateEx&& cmd, std::span<T> value) {
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(sizeof(T) % sizeof(uint32_t) == 0);

        command(std::move(cmd), std::span<const uint32_t>{ reinterpret_cast<const uint32_t*>(value.data()),
                                                           value.size() * sizeof(T) / sizeof(uint32_t) });
    }

    virtual int numBatches() const = 0;
};

} // namespace Brisk
