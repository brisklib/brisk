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

#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/Stream.hpp>
#include <brisk/core/Hash.hpp>
#include <mutex>
#include "Color.hpp"
#include "brisk/core/internal/SmallVector.hpp"
#include "internal/OpenType.hpp"
#include "Image.hpp"
#include <brisk/core/IO.hpp>
#include "internal/Sprites.hpp"
#include "I18n.hpp"

namespace Brisk {

class EUnicode : public ELogic {
public:
    using ELogic::ELogic;
};

class EFreeType : public ELogic {
public:
    using ELogic::ELogic;
};

using GlyphID = uint32_t;

enum class LayoutOptions : uint32_t {
    Default      = 0,
    SingleLine   = 1,
    WrapAnywhere = 2,
    HTML         = 4,
};

BRISK_FLAGS(LayoutOptions)

struct OpenTypeFeatureFlag {
    OpenTypeFeature feature;
    bool enabled;
    auto operator<=>(const OpenTypeFeatureFlag& b) const noexcept = default;
};

enum class FontStyle : uint8_t {
    Normal,
    Italic = 1,
};

BRISK_FLAGS(FontStyle)

template <>
inline constexpr std::initializer_list<NameValuePair<FontStyle>> defaultNames<FontStyle>{
    { "Normal", FontStyle::Normal },
    { "Italic", FontStyle::Italic },
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

BRISK_FLAGS(FontWeight)

template <>
inline constexpr std::initializer_list<NameValuePair<FontWeight>> defaultNames<FontWeight>{
    { "Thin", FontWeight::Thin },     { "ExtraLight", FontWeight::ExtraLight },
    { "Light", FontWeight::Light },   { "Regular", FontWeight::Regular },
    { "Medium", FontWeight::Medium }, { "SemiBold", FontWeight::SemiBold },
    { "Bold", FontWeight::Bold },     { "ExtraBold", FontWeight::ExtraBold },
    { "Black", FontWeight::Black },
};

enum class TextDecoration : uint8_t {
    None        = 0,
    Underline   = 1,
    Overline    = 2,
    LineThrough = 4,
};

template <>
inline constexpr std::initializer_list<NameValuePair<TextDecoration>> defaultNames<TextDecoration>{
    { "None", TextDecoration::None },
    { "Underline", TextDecoration::Underline },
    { "Overline", TextDecoration::Overline },
    { "LineThrough", TextDecoration::LineThrough },
};

BRISK_FLAGS(TextDecoration)

class FontManager;

/**
 * @struct FontMetrics
 * @brief Represents metrics for a font, providing details about its dimensions and spacing.
 */
struct FontMetrics {
    float size;          ///< The size of the font in points.
    float ascender;      ///< The ascender height, always positive and points upwards.
    float descender;     ///< The descender height, always negative and points downwards.
    float height;        ///< The total height of the font, including ascender, descender, and line gap.
    float spaceAdvanceX; ///< The horizontal advance width for a space character.
    float lineThickness; ///< The thickness of lines, such as for underline or strikethrough.
    float xHeight;       ///< The height of the lowercase 'x' character.
    float capitalHeight; ///< The height of uppercase characters.

    /**
     * @brief Computes the line gap, the vertical space between lines of text.
     * @return The line gap value.
     */
    float linegap() const noexcept;

    /**
     * @brief Computes the vertical bounds of the font.
     * @return The total vertical bounds (ascender - descender).
     */
    float vertBounds() const noexcept;

    /**
     * @brief Computes the offset for an underline relative to the baseline.
     * @return The underline offset.
     */
    float underlineOffset() const noexcept;

    /**
     * @brief Computes the offset for an overline relative to the baseline.
     * @return The overline offset.
     */
    float overlineOffset() const noexcept;

    /**
     * @brief Computes the offset for a line through the text.
     * @return The line-through offset.
     */
    float lineThroughOffset() const noexcept;

    /**
     * @brief Compares two FontMetrics objects for equality.
     * @param b The FontMetrics object to compare against.
     * @return True if the objects are equal, false otherwise.
     */
    bool operator==(const FontMetrics& b) const noexcept = default;

    inline static const std::tuple Reflection            = {
        ReflectionField{ "size", &FontMetrics::size },
        ReflectionField{ "ascender", &FontMetrics::ascender },
        ReflectionField{ "descender", &FontMetrics::descender },
        ReflectionField{ "height", &FontMetrics::height },
        ReflectionField{ "spaceAdvanceX", &FontMetrics::spaceAdvanceX },
        ReflectionField{ "lineThickness", &FontMetrics::lineThickness },
        ReflectionField{ "xHeight", &FontMetrics::xHeight },
        ReflectionField{ "capitalHeight", &FontMetrics::capitalHeight },
    };
};

struct GlyphRun;

namespace Internal {

struct FontFace;
struct GlyphData;
struct TextRun;

/**
 * @brief Represents a segment of text with uniform properties such as direction and font face.
 */
struct TextRun {
    /**
     * @brief The direction of the text in this run (e.g., left-to-right or right-to-left).
     */
    TextDirection direction;

    /**
     * @brief The position of the first character of the text run.
     */
    uint32_t begin;

    /**
     * @brief The position just beyond the last character of the text run.
     */
    uint32_t end;

    /**
     * @brief The visual order of the text run.
     *
     * This indicates the position of the run when rendered visually.
     */
    uint32_t visualOrder;

    uint32_t fontIndex;

    /**
     * @brief Pointer to the font face associated with the text run.
     */
    FontFace* face;

    /**
     * @brief Equality comparison operator for `TextRun`.
     */
    bool operator==(const TextRun&) const noexcept = default;

    inline static const std::tuple Reflection      = {
        ReflectionField{ "direction", &TextRun::direction },
        ReflectionField{ "begin", &TextRun::begin },
        ReflectionField{ "end", &TextRun::end },
        ReflectionField{ "visualOrder", &TextRun::visualOrder },
    };
};

/**
 * @enum GlyphFlags
 * @brief Flags used to define various properties of glyphs.
 *
 * This enum class defines a set of flags used to represent
 * different characteristics of glyphs, which are typically
 * used in text rendering or processing.
 */
enum class GlyphFlags : uint8_t {
    /**
     * @brief No special properties.
     *
     * The glyph has no special attributes or flags.
     */
    None                  = 0,

    /**
     * @brief Glyph can be safely broken across lines.
     *
     * Indicates that this glyph can be safely broken
     * when wrapping text across multiple lines.
     */
    SafeToBreak           = 1,

    /**
     * @brief Glyph is at a line break position.
     *
     * Marks this glyph as occurring at a line break
     * (e.g., the glyph is at the end of a line or paragraph).
     */
    AtLineBreak           = 2,

    /**
     * @brief Glyph represents a control character.
     *
     * This flag indicates that the glyph is a control character,
     * such as a non-printing character used for text formatting or control.
     */
    IsControl             = 4,

    /**
     * @brief Glyph is printable.
     *
     * Indicates that this glyph is part of the visible text
     * and can be printed or displayed on screen.
     */
    IsPrintable           = 8,

    /**
     * @brief Glyph is compacted whitespace.
     *
     * Denotes that the glyph represents whitespace at a line break
     * which doesn't extend the visual line bounds. This is commonly
     * used to indicate collapsed or compacted whitespace characters.
     */
    IsCompactedWhitespace = 16,
};

BRISK_FLAGS(GlyphFlags)

using FTFixed = int32_t;

/**
 * @brief Represents an individual glyph with its properties and positioning information.
 */
struct Glyph {
    /**
     * @brief The glyph ID, identifying the specific glyph in the font.
     */
    uint32_t glyph      = UINT32_MAX;

    /**
     * @brief The Unicode codepoint represented by this glyph.
     */
    char32_t codepoint  = UINT32_MAX;

    /**
     * @brief Position of the glyph relative to its parent context.
     */
    PointF pos          = { -1.f, -1.f };

    /**
     * @brief The position of the left caret for the glyph.
     */
    float left_caret    = -1.f;

    /**
     * @brief The position of the right caret for the glyph.
     */
    float right_caret   = -1.f;

    /**
     * @brief The index of the first character in the cluster associated with the glyph.
     */
    uint32_t begin_char = UINT32_MAX;

    /**
     * @brief The index of the first character after the cluster associated with the glyph.
     * This marks the position immediately following the last character in the cluster.
     */
    uint32_t end_char   = UINT32_MAX;

    Range<uint32_t> charRange() const {
        return { begin_char, end_char };
    }

    InclusiveRange<float> caretRange() const {
        return { left_caret, right_caret };
    }

    /**
     * @brief The text direction for this glyph (e.g., left-to-right or right-to-left).
     */
    TextDirection dir = TextDirection::LTR;

    /**
     * @brief Flags providing additional properties or states of the glyph.
     */
    GlyphFlags flags  = GlyphFlags::None;

    /**
     * @brief Computes the caret position for the glyph based on the text direction.
     *
     * @param inverse If true, calculates the caret in the opposite direction.
     * @return float The computed caret position.
     *
     * @note This is an internal method and should not be used directly.
     */
    float caretForDirection(bool inverse) const;

    /**
     * @brief Loads and renders the glyph into `SpriteResource`.
     *
     * @param run The glyph run containing this glyph.
     * @return optional<GlyphData> The loaded glyph data if available, or an empty value otherwise.
     *
     * @note This is an internal method and should not be used directly.
     */
    optional<GlyphData> load(const GlyphRun& run) const;
};

/**
 * @brief Contains detailed data about a single glyph, including its metrics and rendered sprite.
 */
struct GlyphData {
    /**
     * @brief The size of the glyph in its rendered form.
     */
    Size size;

    /**
     * @brief A reference-counted resource pointing to the sprite used to render the glyph.
     */
    RC<SpriteResource> sprite;

    /**
     * @brief The horizontal offset from the glyph's origin to the start of its shape.
     *
     * Known as the "left bearing," this value determines the space between the glyph's origin and
     * the leftmost edge of its bounding box.
     */
    float offset_x;

    /**
     * @brief The vertical offset from the glyph's origin to the top of its shape.
     *
     * Known as the "top bearing," this value is positive for upwards Y-coordinates and determines
     * the space between the glyph's baseline and the topmost edge of its bounding box.
     */
    int offset_y;

    /**
     * @brief The horizontal distance to advance to the next glyph's origin.
     *
     * This is the space the glyph occupies horizontally, including the glyph itself and any trailing
     * whitespace.
     */
    float advance_x;
};

using GlyphList = SmallVector<Glyph, 1>;

} // namespace Internal

/**
 * @brief Specifies the types of bounds that can be calculated for a glyph run.
 */
enum class GlyphRunBounds {
    /**
     * @brief Bounds that consider all glyphs in the run, including whitespace.
     */
    Text,

    /**
     * @brief Bounds that exclude whitespace at line breaks.
     */
    Alignment,

    /**
     * @brief Bounds that consider only printable glyphs in the run.
     */
    Printable,
};

struct AscenderDescender {
    float ascender;  // always positive
    float descender; // always positive

    float height() const noexcept {
        return ascender + descender;
    }

    friend AscenderDescender max(AscenderDescender a, AscenderDescender b) noexcept {
        return { std::max(a.ascender, b.ascender), std::max(a.descender, b.descender) };
    }

    bool operator==(const AscenderDescender&) const noexcept = default;
};

/**
 * @brief Represents a sequence of glyphs along with their associated properties.
 *
 * The `GlyphRun` struct encapsulates the properties and operations of a run of glyphs,
 * including positioning, font information, and range calculations.
 */
struct GlyphRun {
    /**
     * @brief List of glyphs contained in this run in visual order (left-to-right).
     */
    Internal::GlyphList glyphs;

    /**
     * @brief Pointer to the font face associated with the glyph run.
     */
    Internal::FontFace* face;

    /**
     * @brief Font size of the glyph run.
     */
    float fontSize   = 0;

    float tabWidth   = 0;
    float lineHeight = 0;

    /**
     * @brief Metrics of the font used in the glyph run.
     */
    FontMetrics metrics;

    /**
     * @brief Text decoration applied to the glyph run (e.g., underline, strikethrough).
     */
    TextDecoration decoration = TextDecoration::None;

    /**
     * @brief Text direction of the glyph run (left-to-right or right-to-left).
     */
    TextDirection direction;

    /**
     * @brief Indicates whether the horizontal ranges are valid and up-to-date.
     */
    mutable bool rangesValid = false;

    /**
     * @brief Horizontal range of all glyphs in the run.
     */
    mutable InclusiveRange<float> textHRange;

    /**
     * @brief Horizontal range of glyphs, excluding whitespace at line breaks.
     */
    mutable InclusiveRange<float> alignmentHRange;

    /**
     * @brief Horizontal range of printable glyphs in the run.
     */
    mutable InclusiveRange<float> printableHRange;

    /**
     * @brief Visual order of the glyph run within the text.
     */
    int32_t visualOrder;

    /**
     * @brief Vertical offset of the glyph run relative to the text baseline.
     */
    float verticalAlign;

    /**
     * @brief Position of the left-most point of the glyph run at the text baseline.
     */
    PointF position;

    std::optional<Color> color;

    InclusiveRange<float> textVRange() const;

    AscenderDescender ascDesc() const;

    float firstCaret() const noexcept;

    float lastCaret() const noexcept;

    /**
     * @brief Returns the bounds of the glyph run.
     *
     * @param boundsType Specifies the type of bounds to compute.
     * @return RectangleF The computed bounds of the glyph run.
     */
    RectangleF bounds(GlyphRunBounds boundsType) const;

    /**
     * @brief Returns the size of the glyph run.
     *
     * @param boundsType Specifies the type of bounds to consider for size calculation.
     * @return SizeF The size of the glyph run.
     */
    SizeF size(GlyphRunBounds boundsType) const;

    /**
     * @brief Marks the horizontal ranges of the glyph run as invalid.
     *
     * Call this function whenever glyph modifications require recalculating ranges.
     */
    void invalidateRanges();

    /**
     * @brief Updates the horizontal ranges of the glyph run.
     *
     * Ensures the ranges (e.g., `textHRange`, `alignmentHRange`, `printableHRange`)
     * are valid and accurate.
     */
    void updateRanges() const;

    /**
     * @brief Returns the widest glyph run that fits within the specified width and removes those glyphs.
     *
     * @param width The maximum width available for the glyph run.
     * @param allowEmpty A boolean flag that determines whether an empty glyph run is allowed as a result.
     * @return GlyphRun The widest glyph run that fits within the specified width.
     */
    GlyphRun breakAt(float width, bool allowEmpty, bool wrapAnywhere) &;

    /**
     * @brief Retrieves the glyph flags associated with the glyph run.
     *
     * @return Internal::GlyphFlags The flags indicating properties or states of the glyphs.
     */
    Internal::GlyphFlags flags() const;

    /**
     * @brief Retrieves the character range covered by the glyph run.
     *
     * @return Range<uint32_t> The range of characters covered by this glyph run.
     */
    Range<uint32_t> charRange() const;
};

using GlyphRuns = SmallVector<GlyphRun, 1>;

struct Font;

/**
 * @brief Represents text that has been processed and prepared for rendering or layout.
 *
 * The `PreparedText` struct manages glyph runs, logical and visual orders, grapheme boundaries,
 * caret positions, and alignment for efficient text layout and rendering.
 */
struct PreparedText {
    /**
     * @brief The glyph runs associated with the text, stored in logical order.
     */
    GlyphRuns runs;

    /**
     * @brief The visual order of the glyph runs.
     *
     * Each entry corresponds to an index in `runs`, and `visualOrder.size()` must equal `runs.size()`.
     */
    std::vector<uint32_t> visualOrder;

    /**
     * @brief Layout options applied during text preparation.
     */
    LayoutOptions options = LayoutOptions::Default;

    /**
     * @brief Caret offsets (grapheme boundaries) within the text.
     *
     * Each entry indicates the a grapheme boundary, providing a mapping between graphemes and characters.
     */
    std::vector<uint32_t> graphemeBoundaries;

    /**
     * @brief The caret positions for each grapheme boundary.
     *
     * This vector is populated by `updateCaretData` and contains one entry per grapheme boundary.
     */
    std::vector<float> caretPositions;

    /**
     * @brief The ranges of horizontal positions for each grapheme.
     *
     * This vector is populated by `updateCaretData` and contains one entry per grapheme.
     */
    std::vector<InclusiveRange<float>> ranges;

    bool hasCaretData() const noexcept;

    /**
     * @brief Represents a line of glyphs, including its range and metrics.
     */
    struct GlyphLine {
        /**
         * @brief Range of runs (sequence of glyphs sharing the same style) in the line.
         *
         * This range is empty if the line contains no runs.
         */
        Range<uint32_t> runRange{ UINT32_MAX, 0 };

        /**
         * @brief Range of grapheme boundaries (caret positions) in the line.
         *
         * This range is always non-empty, even if the line contains no visible content.
         */
        Range<uint32_t> graphemeRange{ UINT32_MAX, 0 };

        /**
         * @brief The ascender and descender metrics for the line.
         *
         * Contains the maximum ascender and descender values for the glyphs in the line.
         */
        AscenderDescender ascDesc{ 0, 0 };

        /**
         * @brief The baseline position for the line.
         *
         * Represents the vertical offset between this line's baseline and the baseline of the first line.
         */
        float baseline = 0.f;

        /**
         * @brief Checks if the line is empty.
         *
         * A line is considered empty if it contains no runs.
         *
         * @return `true` if the line is empty, `false` otherwise.
         */
        bool empty() const noexcept {
            return runRange.empty();
        }
    };

    /**
     * @brief A collection of GlyphLine objects, representing multiple lines of text.
     */
    std::vector<GlyphLine> lines;

    /**
     * @brief Updates the caret positions and horizontal ranges for graphemes.
     *
     * This function calculates and fills the `caretPositions` and `ranges` fields based on the current text
     * properties.
     */
    void updateCaretData();

    /**
     * @brief Maps a point to the nearest grapheme boundary.
     *
     * Determines the grapheme boundary index corresponding to the provided point in text layout space.
     *
     * @param pt The point in text layout space.
     * @return uint32_t The index of the nearest grapheme boundary.
     */
    uint32_t caretToGrapheme(PointF pt) const;

    uint32_t caretToGrapheme(uint32_t line, float x) const;

    /**
     * @brief Maps a grapheme boundary index to its caret position.
     *
     * Calculates the position of the caret corresponding to the given grapheme boundary in text layout space.
     *
     * @param graphemeIndex The index of the grapheme boundary.
     * @return PointF The caret position for the specified grapheme.
     */
    PointF graphemeToCaret(uint32_t graphemeIndex) const;

    uint32_t graphemeToLine(uint32_t graphemeIndex) const;

    /**
     * @brief Maps a vertical position to the nearest text line.
     *
     * Determines the line index corresponding to the given vertical position in text layout space.
     *
     * @param y The vertical position in text layout space.
     * @return int32_t The index of the nearest line, or -1 if the position is outside the layout.
     */
    int32_t yToLine(float y) const;

    /**
     * @brief Retrieves a glyph run in visual order.
     *
     * @param index The index in visual order to retrieve.
     * @return const GlyphRun& The glyph run at the specified visual order index.
     */
    const GlyphRun& runVisual(uint32_t index) const;

    /**
     * @brief Retrieves a modifiable glyph run in visual order.
     *
     * @param index The index in visual order to retrieve.
     * @return GlyphRun& The modifiable glyph run at the specified visual order index.
     */
    GlyphRun& runVisual(uint32_t index);

    /**
     * @brief Calculates the bounds of the text based on the specified bounds type.
     *
     * @param boundsType The type of bounds to calculate (e.g., text, alignment, printable).
     * @return RectangleF The calculated bounds of the text.
     */
    RectangleF bounds(GlyphRunBounds boundsType = GlyphRunBounds::Alignment) const;

    /**
     * @brief Wraps text to fit within the given width, modifying the current object.
     *
     * Wraps lines of text to fit within the specified `maxWidth`. If `wrapAnywhere` is true,
     * the text can break between any graphemes. Otherwise, breaks occur at word boundaries.
     *
     * @param maxWidth Maximum allowed width for the text.
     * @param wrapAnywhere If true, allows breaking between any graphemes; otherwise, breaks at word
     * boundaries.
     * @return PreparedText The modified object with wrapped lines.
     */
    PreparedText wrap(float maxWidth, bool wrapAnywhere = false) &&;

    /**
     * @brief Wraps text to fit within the given width, returning a copy.
     *
     * Wraps lines of text so that they fit within the specified `maxWidth`. If `wrapAnywhere` is true,
     * the text can break between any graphemes. Otherwise, breaks occur at word boundaries.
     *
     * @param maxWidth Maximum allowed width for the text.
     * @param wrapAnywhere If true, allows breaking between any graphemes; otherwise, breaks at word
     * boundaries.
     * @return PreparedText A copy with wrapped lines.
     */
    PreparedText wrap(float maxWidth, bool wrapAnywhere = false) const&;

    /**
     * @brief Aligns text lines horizontally and vertically.
     *
     * Adjusts the horizontal offsets of each line based on `alignment_x` and returns a PointF with overall
     * horizontal and vertical offsets. Apply the returned offset when painting PreparedText for proper
     * vertical alignment.
     *
     * @param alignment_x Horizontal alignment factor (0: left, 0.5: center, 1: right).
     * @param alignment_y Vertical alignment factor (0: top, 0.5: center, 1: bottom).
     * @return PointF Offsets for alignment.
     */
    PointF alignLines(float alignment_x, float alignment_y = 0.f);

    PointF alignLines(PointF alignment);

    /**
     * @brief Converts a character index to its corresponding grapheme index.
     *
     * @param charIndex The character index to convert.
     * @return uint32_t The corresponding grapheme index.
     */
    uint32_t characterToGrapheme(uint32_t charIndex) const;

    /**
     * @brief Converts a grapheme index to its corresponding character index.
     *
     * @param graphemeIndex The grapheme index to convert.
     * @return uint32_t The corresponding character index.
     */
    uint32_t graphemeToCharacter(uint32_t graphemeIndex) const;

    /**
     * @brief Retrieves the range of characters corresponding to a grapheme index.
     *
     * @param graphemeIndex The grapheme index to query.
     * @return Range<uint32_t> The range of character indices covered by the grapheme.
     */
    Range<uint32_t> graphemeToCharacters(uint32_t graphemeIndex) const;
};

/**
 * @brief A collection of OpenType feature flags.
 */
using OpenTypeFeatureFlags = inline_vector<OpenTypeFeatureFlag, 7>;

/**
 * @brief Represents font properties and settings for text rendering.
 */
struct Font {
    const static std::string Default;
    const static std::string Monospace;
    const static std::string Icons;
    const static std::string Emoji;

    const static std::string DefaultPlusIcons;
    const static std::string DefaultPlusIconsEmoji;

    std::string fontFamily        = DefaultPlusIconsEmoji; ///< The font family.
    float fontSize                = 10.f;                  ///< The size of the font in points.
    FontStyle style               = FontStyle::Normal;     ///< The style of the font (e.g., normal, italic).
    FontWeight weight             = FontWeight::Regular;   ///< The weight of the font (e.g., regular, bold).
    TextDecoration textDecoration = TextDecoration::None;  ///< Text decoration (e.g., underline, none).
    float lineHeight              = 1.2f;                  ///< Line height as a multiplier.
    float tabWidth                = 8.f;                   ///< Tab width in space units.
    float letterSpacing           = 0.f;                   ///< Additional space between letters.
    float wordSpacing             = 0.f;                   ///< Additional space between words.
    float verticalAlign           = 0.f;                   ///< Vertical alignment offset.
    OpenTypeFeatureFlags features{};                       ///< OpenType features for advanced text styling.

    inline static const std::tuple Reflection = {
        ReflectionField{ "fontFamily", &Font::fontFamily },
        ReflectionField{ "fontSize", &Font::fontSize },
        ReflectionField{ "style", &Font::style },
        ReflectionField{ "weight", &Font::weight },
        ReflectionField{ "textDecoration", &Font::textDecoration },
        ReflectionField{ "lineHeight", &Font::lineHeight },
        ReflectionField{ "tabWidth", &Font::tabWidth },
        ReflectionField{ "letterSpacing", &Font::letterSpacing },
        ReflectionField{ "wordSpacing", &Font::wordSpacing },
        ReflectionField{ "verticalAlign", &Font::verticalAlign },
        ReflectionField{ "features", &Font::features },
    };

    /**
     * @brief Creates a copy of the font with a new font family.
     * @param fontFamily The new font family.
     * @return Font A copy with the updated font family.
     */
    Font operator()(std::string fontFamily) const;

    /**
     * @brief Creates a copy of the font with a new font size.
     * @param fontSize The new font size.
     * @return Font A copy with the updated font size.
     */
    Font operator()(float fontSize) const;

    /**
     * @brief Creates a copy of the font with a new style.
     * @param style The new font style.
     * @return Font A copy with the updated font style.
     */
    Font operator()(FontStyle style) const;

    /**
     * @brief Creates a copy of the font with a new weight.
     * @param weight The new font weight.
     * @return Font A copy with the updated font weight.
     */
    Font operator()(FontWeight weight) const;

    auto operator<=>(const Font& b) const noexcept = default;
};

/**
 * @brief Combines font style and weight for simplified handling.
 */
struct FontStyleAndWeight {
    FontStyle style   = FontStyle::Normal;   ///< The font style (e.g., normal, italic).
    FontWeight weight = FontWeight::Regular; ///< The font weight (e.g., regular, bold).

    /**
     * @brief Compares two FontStyleAndWeight objects for equality.
     */
    bool operator==(const FontStyleAndWeight& b) const noexcept = default;
};

struct FontAndColor {
    Font font;
    std::optional<Color> color;
    bool operator==(const FontAndColor&) const noexcept = default;
};

namespace Internal {

enum class FontFormatFlags : uint32_t {
    None           = 0,
    Family         = 1 << 0,
    Size           = 1 << 1,
    Style          = 1 << 2,
    Weight         = 1 << 3,
    Color          = 1 << 4,
    TextDecoration = 1 << 5,

    SizeIsRelative = 1 << 6,
};

BRISK_FLAGS(FontFormatFlags)

struct RichText {
    std::vector<FontAndColor> fonts;
    std::vector<uint32_t> offsets;
    std::vector<FontFormatFlags> flags;

    bool empty() const noexcept {
        return fonts.empty();
    }

    void setBaseFont(const Font& font);

    static std::optional<std::pair<std::u32string, RichText>> fromHtml(std::string_view html);

    bool operator==(const RichText& other) const noexcept = default;
};

} // namespace Internal

struct TextWithOptions {
    std::u32string text;
    LayoutOptions options;
    TextDirection defaultDirection;
    Internal::RichText richText;

    constexpr static std::tuple Reflection{
        ReflectionField{ "text", &TextWithOptions::text },
        ReflectionField{ "options", &TextWithOptions::options },
        ReflectionField{ "defaultDirection", &TextWithOptions::defaultDirection },
    };

    TextWithOptions(std::string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u16string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u32string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u32string text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);

    template <std::convertible_to<std::string_view> T>
    TextWithOptions(T&& text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::string_view(text), options, defaultDirection) {}

    template <std::convertible_to<std::u16string_view> T>
    TextWithOptions(T&& text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::u16string_view(text), options, defaultDirection) {}

    template <std::convertible_to<std::u32string_view> T>
    TextWithOptions(T&& text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::u32string_view(text), options, defaultDirection) {}

    bool operator==(const TextWithOptions& other) const noexcept = default;
};

constexpr inline size_t maxFontsInMergedFonts = 4;

class FontError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum class TestRenderFlags {
    None        = 0,
    TextBounds  = 1,
    GlyphBounds = 2,
    Fade        = 4,
};

BRISK_FLAGS(TestRenderFlags)

enum class FontFlags {
    Default          = 0,
    DisableKerning   = 1,
    DisableHinting   = 2,
    DisableLigatures = 4,
};

BRISK_FLAGS(FontFlags)

struct OSFont {
    std::string family;
    FontStyle style;
    FontWeight weight;
    std::string styleName;
    fs::path path;
};

namespace Internal {
using ShapingCacheKey = std::tuple<Font, TextWithOptions>;
}
} // namespace Brisk

namespace Brisk {

class FontManager final {
public:
    explicit FontManager(std::recursive_mutex* mutex, int hscale, uint32_t cacheTimeMs);
    ~FontManager();

    void addFontAlias(std::string_view newFontFamily, std::string_view existingFontFamily);
    void addFont(std::string fontFamily, FontStyle style, FontWeight weight, bytes_view data,
                 bool makeCopy = true, FontFlags flags = FontFlags::Default);
    [[nodiscard]] bool addFontByName(std::string fontFamily, std::string_view fontName);
    [[nodiscard]] bool addSystemFont(std::string fontFamily);
    [[nodiscard]] status<IOError> addFontFromFile(std::string fontFamily, FontStyle style, FontWeight weight,
                                                  const fs::path& path);
    [[nodiscard]] std::vector<OSFont> installedFonts(bool rescan = false) const;
    std::vector<FontStyleAndWeight> fontFamilyStyles(std::string_view fontFamily) const;

    FontMetrics metrics(const Font& font) const;
    bool hasCodepoint(const Font& font, char32_t codepoint) const;
    PreparedText prepare(const Font& font, const TextWithOptions& text, float width = HUGE_VALF) const;
    RectangleF bounds(const Font& font, const TextWithOptions& text,
                      GlyphRunBounds boundsType = GlyphRunBounds::Alignment) const;
    PreparedText prepare(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                         std::span<const uint32_t> offsets, float width = HUGE_VALF) const;
    RectangleF bounds(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                      std::span<const uint32_t> offsets,
                      GlyphRunBounds boundsType = GlyphRunBounds::Alignment) const;

    using FontKey = std::tuple<std::string, FontStyle, FontWeight>;

    FontKey faceToKey(Internal::FontFace* face) const;
    void testRender(RC<Image> image, const PreparedText& run, Point origin,
                    TestRenderFlags flags = TestRenderFlags::None, std::initializer_list<int> xlines = {},
                    std::initializer_list<int> ylines = {}) const;

    int hscale() const {
        return m_hscale;
    }

    void garbageCollectCache();

private:
    friend struct Internal::FontFace;
    friend struct Font;
    std::map<FontKey, std::shared_ptr<Internal::FontFace>> m_fonts;
    void* m_ft_library;
    mutable std::recursive_mutex* m_lock;

    struct ShapeCacheEntry {
        PreparedText runs;
        uint64_t counter;
    };

    mutable std::unordered_map<Internal::ShapingCacheKey, ShapeCacheEntry, FastHash> m_shapeCache;
    mutable uint64_t m_cacheCounter = 0;
    int m_hscale;
    uint32_t m_cacheTimeMs;
    std::vector<std::string_view> fontList(std::string_view ff) const;
    mutable std::vector<OSFont> m_osFonts;
    Internal::FontFace* lookup(const Font& font) const;
    Internal::FontFace* findFontByKey(FontKey fontKey) const;
    std::pair<Internal::FontFace*, GlyphID> lookupCodepoint(const Font& font, char32_t codepoint,
                                                            bool fallbackToUndef) const;
    FontMetrics getMetrics(const Font& font) const;
    static RectangleF glyphBounds(const Internal::Glyph& g, const Internal::GlyphData& d);
    PreparedText shapeRuns(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                           std::span<const uint32_t> offsets,
                           const std::vector<Internal::TextRun>& textRuns) const;
    std::vector<Internal::TextRun> assignFontsToTextRuns(
        std::u32string_view text, std::span<const FontAndColor> fonts, std::span<const uint32_t> offsets,
        const std::vector<Internal::TextRun>& textRuns) const;
    std::vector<Internal::TextRun> splitControls(std::u32string_view text,
                                                 const std::vector<Internal::TextRun>& textRuns) const;
    PreparedText doPrepare(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                           std::span<const uint32_t> offsets, float width = HUGE_VALF) const;
    PreparedText doShapeCached(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                               std::span<const uint32_t> offsets) const;
    PreparedText doShape(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                         std::span<const uint32_t> offsets) const;
};

extern std::optional<FontManager> fonts;

inline std::vector<uint32_t> textBreakPositions(std::u32string_view text, TextBreakMode mode) {
    std::vector<uint32_t> result(1, 0);
    RC<Internal::TextBreakIterator> iter = Internal::textBreakIterator(text, mode);
    while (auto p = iter->next()) {
        result.push_back(*p);
    }
    return result;
}

namespace Internal {
/**
 * @brief Splits a string of text into multiple `TextRun` objects based on directionality.
 *
 * This function analyzes the given text and segments it into runs of uniform properties. It takes into
 * account the specified default text direction and optionally applies visual order for bidirectional text.
 *
 * @param text The text to be split into text runs.
 * @param defaultDirection The default text direction to use if no explicit directionality is detected.
 * @param visualOrder If `true`, the resulting text runs will be reordered to match the visual order of the
 * text.
 * @return std::vector<TextRun> A vector of `TextRun` objects representing the segmented text.
 */
std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection);

inline std::vector<TextRun> toVisualOrder(std::vector<TextRun> textRuns) {
    std::stable_sort(textRuns.begin(), textRuns.end(), [](const TextRun& a, const TextRun& b) {
        return a.visualOrder < b.visualOrder;
    });
    return textRuns;
}

} // namespace Internal

/**
 * @brief Indicates whether the ICU library is available for full Unicode support.
 *
 * When `icuAvailable` is `true`, the font functions will have full Unicode support
 * for Bidirectional (BiDi) text processing (using splitTextRuns) and grapheme/line
 * breaking functionality (textBreakPositions).
 *
 */
extern bool icuAvailable;

} // namespace Brisk
