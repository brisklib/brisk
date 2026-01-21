#pragma once

#include <cstdint>
#include <string_view>
#include <span>
#include <memory>
#include <cassert>

#include <brisk/core/BasicTypes.hpp>

namespace Brisk::TextLayout {

#ifdef LAYOUT_FLOAT
using LayoutUnit                      = float;

constexpr inline LayoutUnit kZero     = 0.0f;
constexpr inline LayoutUnit kInfinity = std::numeric_limits<LayoutUnit>::max();

constexpr LayoutUnit fromFloat(float value) {
    return value;
}

constexpr float toFloat(LayoutUnit value) {
    return value;
}

constexpr LayoutUnit from26Dot6(int32_t value) {
    return static_cast<float>(value) * (1.0f / 64.0f);
}

constexpr int32_t to26Dot6(LayoutUnit value) {
    return static_cast<int32_t>(value * 64.0f + 0.5f);
}

constexpr LayoutUnit half(LayoutUnit value) {
    return value * 0.5f;
}
#else
using LayoutUnit                      = int32_t;

constexpr inline LayoutUnit kZero     = 0;
constexpr inline LayoutUnit kInfinity = std::numeric_limits<LayoutUnit>::max();

constexpr LayoutUnit fromFloat(float value) {
    return static_cast<int32_t>(value >= 0.0f ? value * 64.0f + 0.5f : value * 64.0f - 0.5f);
}

constexpr float toFloat(LayoutUnit value) {
    return static_cast<float>(value) * (1.0f / 64.0f);
}

constexpr LayoutUnit from26Dot6(int32_t value) {
    return value;
}

constexpr int32_t to26Dot6(LayoutUnit value) {
    return static_cast<int32_t>(value);
}

constexpr LayoutUnit half(LayoutUnit value) {
    return value / 2;
}
#endif

using CodepointIndex = uint32_t;
using GraphemeIndex  = uint32_t;
using GlyphIndex     = uint32_t;
using RunIndex       = uint32_t;
using LineIndex      = uint32_t;
using ParagraphIndex = uint32_t;
using FontRunIndex   = uint32_t;

using CodepointRange = Range<CodepointIndex>;
using GraphemeRange  = Range<GraphemeIndex>;
using GlyphRange     = Range<GlyphIndex>;
using RunRange       = Range<RunIndex>;
using LayoutRange    = Range<LayoutUnit>;

/// Converts a 4-character tag (e.g. "latn") to its big-endian uint32_t representation.
constexpr uint32_t fourCCToUint32(std::string_view cc) {
    assert(cc.size() == 4);
    return (static_cast<uint32_t>(static_cast<uint8_t>(cc[0])) << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(cc[1])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(cc[2])) << 8) |
           static_cast<uint32_t>(static_cast<uint8_t>(cc[3]));
}

struct OpenTypeFeatureFlag {
    uint32_t feature;
    bool enabled;
};

enum class FontStyle : uint8_t {
    Normal = 0,
    Italic = 1,
};

enum class FontWeight : uint16_t {
    Weight100  = 100,
    Weight200  = 200,
    Weight300  = 300,
    Weight400  = 400,
    Weight500  = 500,
    Weight600  = 600,
    Weight700  = 700,
    Weight800  = 800,
    Weight900  = 900,

    Thin       = Weight100,
    ExtraLight = Weight200,
    Light      = Weight300,
    Regular    = Weight400,
    Medium     = Weight500,
    SemiBold   = Weight600,
    Bold       = Weight700,
    ExtraBold  = Weight800,
    Black      = Weight900,
};

enum class TextAlignment : uint8_t {
    Start,
    End,
    Left,
    Right,
    Center,
    Justify,
};

using OpenTypeTag = uint32_t;

/// Glyph hinting policy applied when loading glyphs through FreeType.
enum class Hinting : uint8_t {
    Auto,    ///< Use the backend's default hinting behavior.
    Enable,  ///< Force hinting on.
    Disable, ///< Load glyphs unhinted (FT_LOAD_NO_HINTING).
};

struct FontVariation {
    OpenTypeTag axisTag;
    LayoutUnit value;
};

/**
 * @brief Represents font properties and settings for text rendering.
 */
struct FontDef {
    std::span<const std::string_view> familyNames;
    LayoutUnit fontSize;
    FontStyle style;
    FontWeight weight;
    LayoutUnit lineHeight;    // same unit as fontSize, 0 means auto
    LayoutUnit letterSpacing; // same unit as fontSize, 0 means auto
    LayoutUnit wordSpacing;   // same unit as fontSize, 0 means auto
    LayoutUnit verticalAlign; // same unit as fontSize, 0 means baseline, positive is up, negative is down
    std::span<const OpenTypeFeatureFlag> features;
    std::span<const FontVariation> variations;
    Hinting hinting = Hinting::Auto;
};

struct VerticalMetrics {
    LayoutUnit ascent;
    LayoutUnit descent;
    LayoutUnit lineGap;

    LayoutUnit height() const {
        return ascent + descent + lineGap;
    }
};

/**
 * @brief Additional font metrics used by text decoration and inline layout.
 *
 * Values are scaled to the active font size and expressed in layout units, except
 * for the fields explicitly represented as floats.
 */
struct ExtendedMetrics {
    LayoutUnit spaceAdvanceX{ kZero }; ///< The horizontal advance width for a space character.
    LayoutUnit underlinePosition{ kZero }; ///< The underline offset below the baseline.
    LayoutUnit lineThickness{ kZero }; ///< The thickness of lines, such as for underline or strikethrough.
    LayoutUnit xHeight{ kZero };       ///< The height of the lowercase 'x' character.
    LayoutUnit capitalHeight{ kZero }; ///< The height of uppercase characters.
};
} // namespace Brisk::TextLayout
