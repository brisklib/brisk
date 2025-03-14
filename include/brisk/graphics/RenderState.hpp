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
#pragma once

#include "Matrix.hpp"
#include "Gradients.hpp"
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

enum class ShaderType : int {
    Rectangles, // Gradient or texture
    Arcs,       // Gradient or texture
    Text,       // Gradient or texture
    Shadow,     // Custom paint or texture
    Mask,       // Gradient or texture
    ColorMask,  // Gradient or texture
};

struct GeometryGlyph {
    RectangleF rect;
    SizeF size;
    float sprite;
    float stride;
};

struct GeometryRectangle {
    RectangleF rectangle;
    CornersF borderRadii;
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

enum class SubpixelMode : int32_t {
    Off = 0,
    RGB = 1,
    // BGR = 2,
};

struct ConstantPerFrame {
    SIMD<float, 4> viewport;
    float blueLightFilter;
    float gamma;
    float textRectPadding;
    float textRectOffset;
    int atlasWidth;
};

struct RenderState;
struct RenderStateEx;

enum class SamplerMode : int32_t {
    Clamp = 0,
    Wrap  = 1,
};

struct Quad3 {
    std::array<PointF, 3> points;
    constexpr Quad3()             = default;
    constexpr Quad3(const Quad3&) = default;

    constexpr Quad3(PointF p1, PointF p2, PointF p3) : points{ p1, p2, p3 } {}

    template <typename T>
    constexpr Quad3(RectangleOf<T> rect)
        : points{
              rect.p1,
              PointF(rect.p2.x, rect.p1.y),
              rect.p2,
          } {}
};

constexpr int multigradientColorMix      = -10;

using TextureId                          = uint32_t;
constexpr inline TextureId textureIdNone = static_cast<TextureId>(-1);

struct RenderBuffer;

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
    ShaderType shader   = ShaderType::Rectangles; ///< Type of geometry to generate
    TextureId textureId = textureIdNone;          ///<

    Quad3 scissorQuad   = noClipRect;

    Matrix coordMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f }; ///<
    int spriteOversampling    = 1;
    SubpixelMode subpixelMode = SubpixelMode::Off;

    PatternCodes pattern{};
    int reserved1         = 0;
    int reserved2         = 0;
    float opacity         = 1.f; ///< Opacity. Defaults to 1

    int32_t multigradient = -1; ///< Gradient (-1 - disabled)
    int blurDirections    = 3;  ///< 0 - disable, 1 - H, 2 - V, 3 - H&V
    int textureChannel    = 0;  ///<
    int reserved3         = 0;

    Matrix textureMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f }; ///<
    SamplerMode samplerMode = SamplerMode::Clamp;         ///<
    float blurRadius        = 0.f;                        ///<

    ColorF fillColor1       = Palette::white; ///< Fill (brush) color for gradient at 0%
    ColorF fillColor2       = Palette::white; ///< Fill (brush) color for gradient at 100%
    ColorF strokeColor1     = Palette::black; ///< Stroke (pen) color for gradient at 0%
    ColorF strokeColor2     = Palette::black; ///< Stroke (pen) color for gradient at 100%

    PointF gradientPoint1   = { 0.f, 0.f };     ///< 0% Gradient point
    PointF gradientPoint2   = { 100.f, 100.f }; ///< 100% Gradient point

    float strokeWidth       = 1.f; ///< Stroke or shadow width. Defaults to 1. Set to 0 to disable
    GradientType gradient   = GradientType::Linear;

    union {
        Internal::ImageBackend* imageBackend = nullptr;
        uint64_t dummy;
    };

    Rectangle shaderClip = noClipRect;

public:
    bool compare(const RenderState& second) const;
    void premultiply();

    constexpr static size_t compare_offset = 12;
};

static_assert(std::is_trivially_copy_constructible_v<RenderState>);

struct RenderStateEx;

using RenderStateExArgs = ArgumentsView<RenderStateEx>;

template <typename Tag, typename U>
void applier(RenderStateEx* target, const ArgVal<Tag, U>& arg) {
    Tag::apply(arg.value, *target);
}

using SpriteResources = SmallVector<RC<SpriteResource>, 1>;

struct RenderStateEx : RenderState {
    explicit RenderStateEx(ShaderType shader, RenderStateExArgs args) {
        this->shader = shader;
        args.apply(this);
    }

    explicit RenderStateEx(ShaderType shader, int instances, RenderStateExArgs args) {
        this->instances = instances;
        this->shader    = shader;
        args.apply(this);
    }

    RC<Image> imageHandle;
    RC<GradientResource> gradientHandle;
    SpriteResources sprites;
};

static_assert(sizeof(RenderState) % 256 == 0, "sizeof(RenderState) % 256 == 0");

class RenderContext {
public:
    virtual void command(RenderStateEx&& cmd, std::span<const float> data = {}) = 0;

    virtual void setClipRect(Rectangle clipRect)                                = 0;

    template <typename T>
    void command(RenderStateEx&& cmd, std::span<T> value) {
        static_assert(std::is_trivially_copy_constructible_v<T>);
        static_assert(sizeof(T) % sizeof(float) == 0);

        command(std::move(cmd), std::span<const float>{ reinterpret_cast<const float*>(value.data()),
                                                        value.size() * sizeof(T) / sizeof(float) });
    }

    virtual int numBatches() const = 0;
};

} // namespace Brisk
