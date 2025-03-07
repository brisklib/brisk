#pragma once
#include "Renderer.hpp"
#include "Fonts.hpp"
#include "Path.hpp"
#include <brisk/core/internal/Span.hpp>

namespace Brisk {

float& pixelRatio() noexcept;

template <typename T>
inline float dp(T value) noexcept {
    return static_cast<float>(value) * pixelRatio();
}

template <typename T>
inline int idp(T value) noexcept {
    return static_cast<int>(std::round(static_cast<float>(value) * pixelRatio()));
}

template <typename T>
inline float invertdp(T value) noexcept {
    return static_cast<float>(value) / pixelRatio();
}

template <typename T>
inline int invertidp(T value) noexcept {
    return static_cast<int>(std::round(static_cast<float>(value) / pixelRatio()));
}

inline float operator""_dp(long double value) noexcept {
    return dp(value);
}

inline int operator""_idp(long double value) noexcept {
    return idp(value);
}

inline float operator""_dp(unsigned long long value) noexcept {
    return dp(value);
}

inline int operator""_idp(unsigned long long value) noexcept {
    return idp(value);
}

inline float scalePixels(float x) noexcept {
    return dp(x);
}

inline int scalePixels(int x) noexcept {
    return idp(x);
}

inline PointF scalePixels(PointF x) noexcept {
    return { dp(x.x), dp(x.y) };
}

inline Point scalePixels(Point x) noexcept {
    return { idp(x.x), idp(x.y) };
}

inline SizeF scalePixels(SizeF x) noexcept {
    return { dp(x.x), dp(x.y) };
}

inline Size scalePixels(Size x) noexcept {
    return { idp(x.x), idp(x.y) };
}

inline EdgesF scalePixels(const EdgesF& x) noexcept {
    return { dp(x.x1), dp(x.y1), dp(x.x2), dp(x.y2) };
}

inline Edges scalePixels(const Edges& x) noexcept {
    return { idp(x.x1), idp(x.y1), idp(x.x2), idp(x.y2) };
}

inline Font scalePixels(Font x) noexcept {
    Font result          = std::move(x);
    result.fontSize      = dp(result.fontSize);
    result.letterSpacing = dp(result.letterSpacing);
    result.wordSpacing   = dp(result.wordSpacing);
    return result;
}

inline float unscalePixels(float x) noexcept {
    return invertdp(x);
}

inline int unscalePixels(int x) noexcept {
    return invertidp(x);
}

inline PointF unscalePixels(PointF x) noexcept {
    return { invertdp(x.x), invertdp(x.y) };
}

inline Point unscalePixels(Point x) noexcept {
    return { invertidp(x.x), invertidp(x.y) };
}

inline SizeF unscalePixels(SizeF x) noexcept {
    return { invertdp(x.x), invertdp(x.y) };
}

inline Size unscalePixels(Size x) noexcept {
    return { invertidp(x.x), invertidp(x.y) };
}

inline EdgesF unscalePixels(const EdgesF& x) noexcept {
    return { invertdp(x.x1), invertdp(x.y1), invertdp(x.x2), invertdp(x.y2) };
}

inline Edges unscalePixels(const Edges& x) noexcept {
    return { invertidp(x.x1), invertidp(x.y1), invertidp(x.x2), invertidp(x.y2) };
}

inline Font unscalePixels(Font x) noexcept {
    Font result          = std::move(x);
    result.fontSize      = invertdp(result.fontSize);
    result.letterSpacing = invertdp(result.letterSpacing);
    result.wordSpacing   = invertdp(result.wordSpacing);
    return result;
}

enum class LineEnd {
    Butt,
    Square,
    Round,
};

using GeometryGlyphs = std::vector<GeometryGlyph>;

namespace Internal {
GeometryGlyphs pathLayout(SpriteResources& sprites, const RasterizedPath& path);
}

class Canvas;

class RawCanvas {
public:
    explicit RawCanvas(RenderContext& context);

    RectangleF align(RectangleF rect) const;
    PointF align(PointF v) const;

    RawCanvas& drawLine(PointF p1, PointF p2, float thickness, LineEnd end, RenderStateExArgs args);
    RawCanvas& drawText(PointF pos, const PreparedText& run, Range<uint32_t> selection,
                        RenderStateExArgs args);
    RawCanvas& drawRectangle(RectangleF rect, float borderRadius, float angle, RenderStateExArgs args);
    RawCanvas& drawRectangle(const GeometryRectangle& rect, RenderStateExArgs args);
    RawCanvas& drawShadow(RectangleF rect, float borderRadius, float angle, RenderStateExArgs args);
    RawCanvas& drawEllipse(RectangleF rect, float angle, RenderStateExArgs args);
    RawCanvas& drawArc(PointF center, float outerRadius, float innerRadius, float startAngle, float endEngle,
                       RenderStateExArgs args);
    RawCanvas& drawTexture(RectangleF rect, RC<Image> tex, const Matrix& matrix, RenderStateExArgs args);
    RawCanvas& drawText(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                        RenderStateExArgs args);
    RawCanvas& drawMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                        RenderStateExArgs args);
    RawCanvas& drawColorMask(SpriteResources sprites, std::span<const GeometryGlyph> glyphs,
                             RenderStateExArgs args);

    RawCanvas& drawLine(PointF p1, PointF p2, float thickness, ColorW color, LineEnd end = LineEnd::Butt);

    template <typename... Args>
    RawCanvas& drawLine(PointF p1, PointF p2, float thickness, LineEnd end, const Args&... args) {
        return drawLine(p1, p2, thickness, end, RenderStateExArgs{ std::make_tuple(args...) });
    }

    /// @brief Draw rounded rectangle
    template <typename... Args>
    RawCanvas& drawRectangle(RectangleF rect, float borderRadius, float angle, const Args&... args) {
        return drawRectangle(rect, borderRadius, angle, RenderStateExArgs{ std::make_tuple(args...) });
    }

    template <typename... Args>
    RawCanvas& drawRectangle(const GeometryRectangle& rect, const Args&... args) {
        return drawRectangle(rect, RenderStateExArgs{ std::make_tuple(args...) });
    }

    /// @brief Draw rounded rectangle
    template <typename... Args>
    RawCanvas& drawShadow(RectangleF rect, float borderRadius, float angle, const Args&... args) {
        return drawShadow(rect, borderRadius, angle, RenderStateExArgs{ std::make_tuple(args...) });
    }

    /// @brief Draw ellipse
    template <typename... Args>
    RawCanvas& drawEllipse(RectangleF rect, float angle, const Args&... args) {
        return drawEllipse(rect, angle, RenderStateExArgs{ std::make_tuple(args...) });
    }

    /// @brief Draw ellipse
    template <typename... Args>
    RawCanvas& drawArc(PointF center, float outerRadius, float innerRadius, float startAngle, float endEngle,
                       const Args&... args) {
        return drawArc(center, outerRadius, innerRadius, startAngle, endEngle,
                       RenderStateExArgs{ std::make_tuple(args...) });
    }

    template <typename... Args>
    RawCanvas& drawText(PointF pos, const PreparedText& run, const Args&... args) {
        return drawText(pos, run, {}, RenderStateExArgs{ std::make_tuple(args...) });
    }

    template <typename... Args>
    RawCanvas& drawText(PointF pos, const PreparedText& run, Range<uint32_t> selection, const Args&... args) {
        return drawText(pos, run, selection, RenderStateExArgs{ std::make_tuple(args...) });
    }

    /// Draw text at the given point
    RawCanvas& drawText(PointF pos, const TextWithOptions& text, const Font& f, ColorW textColor);

    /// Draw text aligned inside the given rectangle
    RawCanvas& drawText(RectangleF rect, float x_alignment, float y_alignment, const TextWithOptions& text,
                        const Font& f, ColorW textColor);

    /// Draw text aligned around the given point
    RawCanvas& drawText(PointF pos, float x_alignment, float y_alignment, const TextWithOptions& text,
                        const Font& f, ColorW textColor);

    template <typename... Args>
    RawCanvas& drawTexture(RectangleF rect, RC<Image> tex, const Matrix& matrix, const Args&... args) {
        return drawTexture(rect, std::move(tex), matrix, RenderStateExArgs{ std::make_tuple(args...) });
    }

    struct State {
        RectangleF scissors        = noScissors;
        float scissorsBorderRadius = 0.f;
        int scissorsCorners        = 15;
        PointF offset              = PointF{};
    };

    struct Save {
        Save(const Save&)            = delete;
        Save(Save&&)                 = delete;
        Save& operator=(const Save&) = delete;
        Save& operator=(Save&&)      = delete;

        explicit Save(RawCanvas& canvas) noexcept : canvas(canvas) {
            savedState = canvas.m_state;
        }

        ~Save() noexcept {
            canvas.m_state = savedState;
        }

        State* operator->() {
            return &canvas.m_state;
        }

        void intersectScissors(RectangleF scissors) {
            canvas.m_state.scissors = canvas.m_state.scissors.intersection(scissors);
        }

        RawCanvas& canvas;
        State savedState;
    };

    Save save() {
        return Save{ *this };
    }

    const State& state() const {
        return m_state;
    }

    RenderContext& renderContext() {
        return m_context;
    }

protected:
    friend class Canvas;
    RenderContext& m_context;
    State m_state;
    void prepareStateInplace(RenderStateEx& state);
    RenderStateEx prepareState(RenderStateEx&& state);
};
} // namespace Brisk
