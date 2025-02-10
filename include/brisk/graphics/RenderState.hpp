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
    float angle;        // x
    float borderRadius; // z
    float corners;      // y, static_cast<float>()
    float reserved1;    // w
};

inline GeometryRectangle makeGeometryRect(RectangleF rectangle) {
    return GeometryRectangle{ rectangle };
}

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
    int hpattern;
    int vpattern;
    int scale = 1;
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

namespace Tag {

struct SubpixelMode {
    using Type = Brisk::SubpixelMode;
    static void apply(const Type& value, RenderStateEx& state);
};

struct FillColor {
    using Type = ColorF;
    static void apply(const Type& value, RenderStateEx& state);
};

struct StrokeColor {
    using Type = ColorF;
    static void apply(const Type& value, RenderStateEx& state);
};

struct FillColors {
    using Type = GradientColors;
    static void apply(const Type& value, RenderStateEx& state);
};

struct StrokeColors {
    using Type = GradientColors;
    static void apply(const Type& value, RenderStateEx& state);
};

struct PaintOpacity {
    using Type = float;
    static void apply(const Type& value, RenderStateEx& state);
};

struct ContourSize {
    using Type = float;
    static void apply(const Type& value, RenderStateEx& state);
};

struct ContourColor {
    using Type = ColorF;
    static void apply(const Type& value, RenderStateEx& state);
};

struct StrokeWidth {
    using Type = float;
    static void apply(const Type& value, RenderStateEx& state);
};

struct Multigradient {
    using Type = RC<GradientResource>;
    static void apply(const Type& value, RenderStateEx& state);
};

template <GradientType grad_type>
struct FillGradient {
    using Type = GradientPoints;
    static void apply(const Type& value, RenderStateEx& state);
};

struct Scissor {
    using Type = RectangleF;
    static void apply(const Type& value, RenderStateEx& state);
};

struct Patterns {
    using Type = PatternCodes;
    static void apply(const Type& value, RenderStateEx& state);
};

struct BlurRadius {
    using Type = float;
    static void apply(const Type& value, RenderStateEx& state);
};

struct BlurDirections {
    using Type = int;
    static void apply(const Type& value, RenderStateEx& state);
};

struct TextureChannel {
    using Type = int;
    static void apply(const Type& value, RenderStateEx& state);
};

struct ContourFlags {
    using Type = int;
    static void apply(const Type& value, RenderStateEx& state);
};

struct CoordMatrix {
    using Type = Matrix;
    static void apply(const Type& value, RenderStateEx& state);
};

struct SamplerMode {
    using Type = Brisk::SamplerMode;
    static void apply(const Type& value, RenderStateEx& state);
};

} // namespace Tag

inline namespace Arg {

constexpr inline Argument<Tag::FillColor> fillColor{};
constexpr inline Argument<Tag::StrokeColor> strokeColor{};
constexpr inline Argument<Tag::FillColors> fillColors{};
constexpr inline Argument<Tag::StrokeColors> strokeColors{};
constexpr inline Argument<Tag::StrokeWidth> strokeWidth{};
constexpr inline Argument<Tag::ContourSize> contourSize{};
constexpr inline Argument<Tag::ContourColor> contourColor{};
constexpr inline Argument<Tag::PaintOpacity> paintOpacity{};
constexpr inline Argument<Tag::FillGradient<GradientType::Linear>> linearGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Radial>> radialGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Angle>> angleGradient{};
constexpr inline Argument<Tag::FillGradient<GradientType::Reflected>> reflectedGradient{};
constexpr inline Argument<Tag::Multigradient> multigradient{};
constexpr inline Argument<Tag::Scissor> scissor{};
constexpr inline Argument<Tag::Patterns> patterns{};
constexpr inline Argument<Tag::BlurRadius> blurRadius{};
constexpr inline Argument<Tag::BlurDirections> blurDirections{};
constexpr inline Argument<Tag::TextureChannel> textureChannel{};
constexpr inline Argument<Tag::ContourFlags> contourFlags{};
constexpr inline Argument<Tag::CoordMatrix> coordMatrix{};
constexpr inline Argument<Tag::SamplerMode> samplerMode{};

} // namespace Arg

constexpr int multigradientColorMix      = -10;

using TextureId                          = uint32_t;
constexpr inline TextureId textureIdNone = static_cast<TextureId>(-1);

struct RenderBuffer;

constexpr inline RectangleF noScissors{ -16777216, -16777216, 16777216, 16777216 };

struct RenderState {
    bool operator==(const RenderState& state) const;

public:
    // ---------------- SPECIAL [1] ----------------
    int dataOffset = 0; ///< Offset in data4 for current operation (multiply by 4 to get offset in data1)
    int dataSize   = 0; ///< Data size in floats
    int instances  = 1; ///< Number of quads to render
    int unused     = 0;

public:
    // ---------------- GLOBAL [5] ----------------
    ShaderType shader          = ShaderType::Rectangles; ///< Type of geometry to generate
    TextureId textureId        = textureIdNone;          ///<
    float scissorsBorderRadius = 0.f;                    ///<
    int scissorsCorners        = 0;                      ///<

    Matrix coordMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f }; ///<
    int spriteOversampling    = 1;
    SubpixelMode subpixelMode = SubpixelMode::Off;

    int hpattern              = 0;
    int vpattern              = 0;
    int patternScale          = 1;
    float opacity             = 1.f; ///< Opacity. Defaults to 1

    RectangleF scissor        = noScissors; ///< Clip area in screen space

public:
    // ---------------- texture [4] ----------------

    int32_t multigradient = -1; ///< Gradient (-1 - disabled)
    int blurDirections    = 3;  ///< 0 - disable, 1 - H, 2 - V, 3 - H&V
    int textureChannel    = 0;  ///<
    int clipInScreenspace = 0;

    Matrix textureMatrix{ 1.f, 0.f, 0.f, 1.f, 0.f, 0.f }; ///<
    SamplerMode samplerMode = SamplerMode::Clamp;         ///<
    float blurRadius        = 0.f;                        ///<

public:
    // ---------------- rectangles, arcs [6] ----------------

    ColorF fillColor1     = Palette::white; ///< Fill (brush) color for gradient at 0%
    ColorF fillColor2     = Palette::white; ///< Fill (brush) color for gradient at 100%
    ColorF strokeColor1   = Palette::black; ///< Stroke (pen) color for gradient at 0%
    ColorF strokeColor2   = Palette::black; ///< Stroke (pen) color for gradient at 100%

    PointF gradientPoint1 = { 0.f, 0.f };     ///< 0% Gradient point
    PointF gradientPoint2 = { 100.f, 100.f }; ///< 100% Gradient point

    float strokeWidth     = 1.f; ///< Stroke or shadow width. Defaults to 1. Set to 0 to disable
    GradientType gradient = GradientType::Linear;
    int shadowFlags       = 3; // 1 - inner, 2 - outer
    float reserved5       = 0;

    union {
        Internal::ImageBackend* imageBackend = nullptr;
        uint64_t dummy;
    };

    float reserved6 = 0;
    float reserved7 = 0;

public:
    SIMD<float, 4> padding[16]{ 0, 0, 0, 0, 0, 0, 0 };

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

template <GradientType grad_type>
void Tag::FillGradient<grad_type>::apply(const GradientPoints& value, RenderStateEx& state) {
    state.gradient       = grad_type;
    state.gradientPoint1 = value.point1;
    state.gradientPoint2 = value.point2;
}

static_assert(sizeof(RenderState) % 256 == 0, "sizeof(RenderState) % 256 == 0");

class RenderContext {
public:
    virtual void command(RenderStateEx&& cmd, std::span<const float> data = {}) = 0;

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
