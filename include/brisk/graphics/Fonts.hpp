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

#include <memory>
#include <mutex>
#include <optional>
#include <span>

#include <brisk/core/Hash.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Stream.hpp>
#include <brisk/core/internal/FunctionRef.hpp>
#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/internal/SmallVector.hpp>

#include "Color.hpp"
#include "Image.hpp"
#include "internal/OpenType.hpp"
#include "internal/Sprites.hpp"

namespace Brisk {

enum class TextDirection : uint8_t {
    LTR,
    RTL,
};

template <>
inline constexpr std::initializer_list<NameValuePair<TextDirection>> defaultNames<TextDirection>{
    { "LTR", TextDirection::LTR },
    { "RTL", TextDirection::RTL },
};

class EFreeType : public ELogic {
public:
    using ELogic::ELogic;
};

enum class TextOptions : uint32_t {
    Default      = 0,
    SingleLine   = 1,
    WrapAnywhere = 2,
    Html         = 4,
};

template <>
constexpr inline bool isBitFlags<TextOptions> = true;

struct OpenTypeFeatureFlag {
    OpenTypeFeature feature;
    bool enabled;
    auto operator<=>(const OpenTypeFeatureFlag& b) const noexcept = default;
};

inline std::string format_as(const OpenTypeFeatureFlag& val) {
    return fmt::format("{}: {}", val.feature, val.enabled ? "on" : "off");
}

enum class FontStyle : uint8_t {
    Normal = 0,
    Italic = 1,
};

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

template <>
constexpr inline bool isBitFlags<FontWeight> = true;

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

template <>
constexpr inline bool isBitFlags<TextDecoration> = true;

class FontManager;
class ShapedText;
class TextLayout;
struct TextSelectionRect;

namespace Internal {
struct TextEngineState;
} // namespace Internal

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

    inline static const std::tuple reflection            = {
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

/** @brief Horizontal alignment used when producing a `TextLayout`. */
enum class TextLayoutAlignment : uint8_t {
    Start,  ///< Align to the start edge of each paragraph, respecting its direction.
    End,    ///< Align to the end edge of each paragraph, respecting its direction.
    Left,   ///< Align to the physical left edge.
    Right,  ///< Align to the physical right edge.
    Center, ///< Center each line in the available layout width.
};

/**
 * @brief Options controlling conversion of shaped text into a `TextLayout`.
 */
struct TextLayoutOptions {
    /// Maximum line width in pixels. Finite positive values enable wrapping; zero does not impose a
    /// width limit. The resulting coordinates are intrinsic and can be translated by the caller.
    float maxLineWidth            = HUGE_VALF;
    /// Additional indentation applied to the first line of each paragraph, in pixels.
    float firstLineIndent         = 0.f;
    /// Absolute tab-stop interval in pixels. Zero uses the interval configured by the shaped text.
    float tabWidth                = 0.f;
    /// Horizontal alignment of each laid-out line.
    TextLayoutAlignment alignment = TextLayoutAlignment::Start;
    /// If true, permits line breaks at grapheme boundaries where ordinary word wrapping would not.
    bool allowBreakAnywhere       = false;
};

/** @brief Selects which visual side owns a caret at an ambiguous boundary. */
enum class CaretAffinity : uint8_t {
    Upstream,   ///< Associate the caret with the preceding visual position.
    Downstream, ///< Associate the caret with the following visual position.
};

/** @brief A caret location expressed as a grapheme boundary and visual affinity. */
struct CaretIndex {
    uint32_t grapheme      = 0;                         ///< Grapheme boundary index.
    CaretAffinity affinity = CaretAffinity::Downstream; ///< Visual side at the boundary.
};

/** @brief A caret location expressed in layout coordinates. */
struct CaretPosition {
    float x           = 0.f; ///< Horizontal caret position in layout coordinates.
    uint32_t line     = 0;   ///< Zero-based line index.
    uint8_t bidiLevel = 0;   ///< Bidirectional embedding level at the caret.
};

/** @brief One rectangle segment belonging to a text selection. */
struct TextSelectionRect {
    uint32_t line = 0;   ///< Zero-based line index.
    float x0      = 0.f; ///< Start of the selected segment in layout coordinates.
    float x1      = 0.f; ///< End of the selected segment in layout coordinates.
};

/**
 * @brief Character and grapheme range for one paragraph in `ShapedText`.
 */
struct ShapedParagraph {
    Range<uint32_t> characterRange{ 0, 0 };       ///< Half-open character/codepoint range.
    Range<uint32_t> graphemeRange{ 0, 0 };        ///< Half-open grapheme range.
    TextDirection direction = TextDirection::LTR; ///< Base direction of the paragraph.
};

/** @brief Describes why a line ends. */
enum class TextLayoutLineEnd : uint8_t {
    SoftWrap,       ///< The line ended because of the available width.
    MandatoryBreak, ///< The line ended at an explicit line-break character.
    ParagraphEnd,   ///< The line ended at the end of a paragraph.
};

/** @brief Metrics and source ranges for one line in a `TextLayout`. */
struct TextLine {
    Range<uint32_t> characterRange{ 0, 0 };                    ///< Half-open character/codepoint range.
    Range<uint32_t> graphemeRange{ 0, 0 };                     ///< Half-open grapheme range.
    TextDirection direction = TextDirection::LTR;              ///< Base direction of the line.
    TextLayoutLineEnd end   = TextLayoutLineEnd::ParagraphEnd; ///< Reason the line ends.
    float baseline          = 0.f;                             ///< Baseline y-coordinate.
    float ascender          = 0.f;                             ///< Distance above the baseline.
    float descender         = 0.f; ///< Distance below the baseline; normally negative.
    float leading           = 0.f; ///< Extra line spacing distributed around the line.
    float width             = 0.f; ///< Width including trailing whitespace.
    float trimmedWidth      = 0.f; ///< Width excluding trailing whitespace.
};

class TextLayout;

struct PimplAccessor;

/**
 * @brief Immutable, width-independent representation of shaped text.
 *
 * ShapedText stores the source-to-grapheme mappings and paragraph information
 * needed for text editing, cursor movement, selection, and layout creation.
 * It has value semantics: copies share the immutable shaped data through a
 * `std::shared_ptr`-backed store, while each value can be used independently.
 * Use layout() to produce a width-dependent TextLayout for measurement or drawing.
 */
class ShapedText {
public:
    struct Impl;

    /// Constructs an empty shaped-text value.
    ShapedText();
    ~ShapedText();

    /// Copies a shaped-text value and its source mappings.
    ShapedText(const ShapedText&);
    ShapedText& operator=(const ShapedText&);
    /// Transfers a shaped-text value into a new object.
    ShapedText(ShapedText&&) noexcept;
    ShapedText& operator=(ShapedText&&) noexcept;

    /// @return `true` if no text was shaped, otherwise `false`.
    [[nodiscard]] bool empty() const noexcept;
    /// @return Number of source Unicode codepoints represented by the text.
    [[nodiscard]] uint32_t characterCount() const noexcept;
    /// @return Number of graphemes represented by the source text.
    [[nodiscard]] uint32_t graphemeCount() const noexcept;
    /// @return Source character boundaries for graphemes, including the initial and final boundary.
    [[nodiscard]] std::span<const uint32_t> graphemeBoundaries() const noexcept;
    /// @return Number of paragraphs in the shaped text.
    [[nodiscard]] size_t paragraphCount() const noexcept;
    /// @param index Zero-based paragraph index.
    /// @return Paragraph ranges, or an empty value if `index` is out of range.
    [[nodiscard]] ShapedParagraph paragraph(size_t index) const noexcept;

    /// @param character Source character/codepoint boundary, clamped to `characterCount()`.
    /// @return Grapheme boundary containing the specified character boundary.
    [[nodiscard]] uint32_t characterToGrapheme(uint32_t character) const noexcept;
    /// @param grapheme Grapheme boundary, clamped to `graphemeCount()`.
    /// @return Source character/codepoint boundary corresponding to the grapheme boundary.
    [[nodiscard]] uint32_t graphemeToCharacter(uint32_t grapheme) const noexcept;
    /// @param grapheme Grapheme index, clamped to the last grapheme.
    /// @return Half-open source character range covered by the grapheme.
    [[nodiscard]] Range<uint32_t> graphemeToCharacters(uint32_t grapheme) const noexcept;
    /// @param grapheme Grapheme index to inspect.
    /// @return `true` if the grapheme is a paragraph separator rather than paragraph content.
    [[nodiscard]] bool isParagraphSeparator(uint32_t grapheme) const noexcept;
    /// @param character Source character/codepoint boundary.
    /// @param affinity Visual side to associate with the resulting caret.
    /// @return Affinity-aware caret index for the boundary.
    [[nodiscard]] CaretIndex caretFromCharacter(
        uint32_t character, CaretAffinity affinity = CaretAffinity::Downstream) const noexcept;
    /// @param caret Grapheme boundary and visual affinity.
    /// @return Source character/codepoint boundary represented by the caret.
    [[nodiscard]] uint32_t characterFromCaret(CaretIndex caret) const noexcept;

    /// Creates a width-dependent layout using the supplied wrapping and alignment options.
    /// @param options Wrapping, indentation, tab-stop, and alignment settings.
    /// @return Immutable layout suitable for measurement, interaction, and rendering.
    [[nodiscard]] TextLayout layout(const TextLayoutOptions& options = {}) const;

private:
    explicit ShapedText(std::shared_ptr<const Impl> impl);
    friend struct PimplAccessor;

    std::shared_ptr<const Impl> m_impl;

    friend class FontManager;
};

/**
 * @brief Immutable width-dependent layout with measurement and interaction operations.
 *
 * TextLayout contains the lines, metrics, bounds, and source mappings resulting
 * from laying out a ShapedText with specific width and alignment options.
 * It supports caret positioning, hit-testing, vertical caret movement, and
 * selection-rectangle generation in layout coordinates.
 * It has value semantics: copies share the immutable layout data through a
 * `std::shared_ptr`-backed store and remain safe to retain independently.
 */
class TextLayout {
public:
    struct Impl;
    /// Constructs an empty text layout.
    TextLayout();
    ~TextLayout();

    /// Copies a text layout and its interaction data.
    TextLayout(const TextLayout&);
    TextLayout& operator=(const TextLayout&);
    /// Transfers a text layout into a new object.
    TextLayout(TextLayout&&) noexcept;
    TextLayout& operator=(TextLayout&&) noexcept;

    /// @return `true` if the layout contains no lines, otherwise `false`.
    [[nodiscard]] bool empty() const noexcept;
    /// @return Number of laid-out lines, including empty trailing lines.
    [[nodiscard]] size_t lineCount() const noexcept;
    /// @param index Zero-based line index.
    /// @return Line metrics and source ranges, or an empty value if `index` is out of range.
    [[nodiscard]] TextLine line(size_t index) const noexcept;
    /// @return Bounds including trailing whitespace and the full line extents.
    [[nodiscard]] RectangleF bounds() const noexcept;
    /// @return Bounds excluding trailing whitespace while retaining line extents.
    [[nodiscard]] RectangleF trimmedBounds() const noexcept;

    /// @param index Grapheme boundary and affinity to locate.
    /// @return Caret position in layout coordinates.
    [[nodiscard]] CaretPosition caretPosition(CaretIndex index) const noexcept;
    /// @param index Grapheme boundary and affinity to locate.
    /// @return Zero-based line containing the caret; returns zero for an empty layout.
    [[nodiscard]] size_t lineForCaret(CaretIndex index) const noexcept;
    /// @param character Source character/codepoint boundary.
    /// @param affinity Visual side to associate with the boundary.
    /// @return Zero-based line containing the boundary.
    [[nodiscard]] size_t lineForCharacter(uint32_t character,
                                          CaretAffinity affinity = CaretAffinity::Downstream) const noexcept;
    /// @param line Zero-based line index.
    /// @return Downstream-affinity caret at the beginning of the line.
    [[nodiscard]] CaretIndex lineBeginning(size_t line) const noexcept;
    /// @param line Zero-based line index.
    /// @return Upstream-affinity caret at the end of the line.
    [[nodiscard]] CaretIndex lineEnd(size_t line) const noexcept;
    /// @param index Caret to measure.
    /// @param width Width of the caret rectangle in pixels.
    /// @return Rectangle spanning the line's ascent, descent, and leading.
    [[nodiscard]] RectangleF caretRect(CaretIndex index, float width = 1.f) const noexcept;
    /// @param point Layout-coordinate point to inspect.
    /// @return Nearest caret, including affinity at bidirectional and wrapped boundaries.
    [[nodiscard]] CaretIndex hitTest(PointF point) const noexcept;
    /// @param line Zero-based line index.
    /// @param x Horizontal layout-coordinate position.
    /// @return Nearest caret on the requested line.
    [[nodiscard]] CaretIndex hitTestLine(size_t line, float x) const noexcept;
    /// @param current Current caret.
    /// @param preferredX Horizontal position to preserve while moving.
    /// @param direction Negative moves upward; positive moves downward; zero leaves the caret unchanged.
    /// @return Caret on the adjacent line nearest to `preferredX`.
    [[nodiscard]] CaretIndex moveCaretVertically(CaretIndex current, float preferredX,
                                                 int direction) const noexcept;
    /// Enumerates selection rectangles for a grapheme range.
    /// @param selection Half-open grapheme range.
    /// @param onRect Callback invoked once for each line segment.
    void selectionRects(Range<uint32_t> selection, function_ref<void(const TextSelectionRect&)> onRect) const;
    /// Enumerates selection rectangles for a source character/codepoint range.
    /// @param selection Half-open character/codepoint range.
    /// @param onRect Callback invoked once for each line segment.
    void selectionRectsByCharacter(Range<uint32_t> selection,
                                   function_ref<void(const TextSelectionRect&)> onRect) const;

private:
    friend struct PimplAccessor;
    explicit TextLayout(std::shared_ptr<const Impl> impl);

    std::shared_ptr<const Impl> m_impl;

    friend class ShapedText;
    friend class FontManager;
};

/**
 * @brief A collection of OpenType feature flags.
 */
using OpenTypeFeatureFlags = SmallVector<OpenTypeFeatureFlag, 1>;

namespace Internal {

inline std::string format_as(const OpenTypeFeatureFlags& features) {
    std::string result;
    for (const auto& f : features) {
        if (!result.empty()) {
            result += ',';
        }
        result += fmt::to_string(f.feature);
        if (!f.enabled) {
            result += "=0";
        }
    }
    return result;
}

} // namespace Internal

/**
 * @brief Represents font properties and settings for text rendering.
 */
struct Font {
    constexpr static char Default[]               = "@default";
    constexpr static char Monospace[]             = "@mono";
    constexpr static char Icons[]                 = "@icons";
    constexpr static char Emoji[]                 = "@emoji";
    constexpr static char DefaultPlusIcons[]      = "@default,@icons";
    constexpr static char DefaultPlusIconsEmoji[] = "@default,@icons,@emoji";

    std::string fontFamily                        = DefaultPlusIconsEmoji; ///< The font family.
    float fontSize                                = 12.f; ///< The size of the font in points.
    FontStyle style               = FontStyle::Normal;    ///< The style of the font (e.g., normal, italic).
    FontWeight weight             = FontWeight::Regular;  ///< The weight of the font (e.g., regular, bold).
    TextDecoration textDecoration = TextDecoration::None; ///< Text decoration (e.g., underline, none).
    float lineHeight              = 0.f;   ///< Line height as a multiplier, 0 means natural line height.
    float tabWidth                = 100.f; ///< Absolute tab interval.
    float letterSpacing           = 0.f;   ///< Additional space between letters.
    float wordSpacing             = 0.f;   ///< Additional space between words.
    float verticalAlign           = 0.f;   ///< Vertical alignment offset.
    OpenTypeFeatureFlags features{};       ///< OpenType features for advanced text styling.

    inline static const std::tuple reflection = {
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

template <>
constexpr inline bool isBitFlags<Internal::FontFormatFlags> = true;

struct TextWithOptions {
    std::u32string text;
    TextOptions options;
    TextDirection defaultDirection;
    Internal::RichText richText;

    constexpr static std::tuple reflection{
        ReflectionField{ "text", &TextWithOptions::text },
        ReflectionField{ "options", &TextWithOptions::options },
        ReflectionField{ "defaultDirection", &TextWithOptions::defaultDirection },
    };

    TextWithOptions(std::string_view text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u16string_view text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u32string_view text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);
    TextWithOptions(std::u32string text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR);

    template <std::convertible_to<std::string_view> T>
    TextWithOptions(T&& text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::string_view(text), options, defaultDirection) {}

    template <std::convertible_to<std::u16string_view> T>
    TextWithOptions(T&& text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::u16string_view(text), options, defaultDirection) {}

    template <std::convertible_to<std::u32string_view> T>
    TextWithOptions(T&& text, TextOptions options = TextOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : TextWithOptions(std::u32string_view(text), options, defaultDirection) {}

    bool operator==(const TextWithOptions& other) const noexcept = default;
};

struct OsFont {
    std::string family;
    FontStyle style;
    FontWeight weight;
    std::string styleName;
    fs::path path;
};

class FontManager final {
public:
    /**
     * @brief Constructs a FontManager instance.
     * @param mutex Pointer to a recursive mutex for thread safety, or nullptr if not needed.
     * @param hscale Horizontal scaling factor (default: 3).
     * @param cacheTimeMs Cache duration in milliseconds (default: 5000).
     */
    explicit FontManager(std::recursive_mutex* mutex, int hscale = 3);

    /**
     * @brief Destructor for FontManager.
     */
    ~FontManager();

    /**
     * @brief Adds an alias mapping from a new font family name to an existing one.
     * @param newFontFamily The new font family name to alias.
     * @param existingFontFamily The existing font family name to map to.
     */
    void addFontAlias(std::string_view newFontFamily, std::string_view existingFontFamily);

    /** Registers every face in a font file held in memory. */
    void addFont(BytesView data, std::string alias = {});

    /**
     * @brief Registers every face in a cached embedded resource.
     *
     * The cached resource has process lifetime, so its bytes are borrowed
     * without making another copy.
     *
     * @param resourceName Embedded resource name.
     * @param alias Optional family alias for all registered faces.
     * @param emptyOk Allow a missing resource to be ignored.
     * @return True if a non-empty resource was registered.
     */
    [[nodiscard]] bool addFontFromResource(std::string resourceName, std::string alias = {},
                                           bool emptyOk = false);

    /**
     * @brief Adds a font installed on the system.
     * @param fontFamily The font family name to register.
     * @param fontName The specific font name to add.
     * @return True if the font was successfully added, false otherwise.
     */
    [[nodiscard]] bool addFontByName(std::string_view fontName, std::string alias = {});

    /**
     * @brief Adds a system font to the manager.
     * @param fontFamily The font family name to register.
     * @return True if the system font was successfully added, false otherwise.
     */
    [[nodiscard]] bool addSystemFont(std::string alias = {});

    /**
     * @brief Adds a font from a file.
     * @param fontFamily The font family name.
     * @param style The font style.
     * @param weight The font weight.
     * @param path Filesystem path to the font file.
     * @return Status indicating success or an IoError on failure.
     */
    [[nodiscard]] status<IoError> addFontFromFile(const fs::path& path, std::string alias = {});

    /**
     * @brief Retrieves a list of installed system fonts.
     * @param rescan Whether to rescan the system for fonts (default: false).
     * @return Vector of OsFont objects representing installed fonts.
     */
    [[nodiscard]] std::vector<OsFont> installedFonts(bool rescan = false) const;

    /**
     * @brief Retrieves metrics for a given font.
     * @param font The font to measure.
     * @return FontMetrics containing size and spacing information.
     */
    [[nodiscard]] FontMetrics metrics(const Font& font) const;

    /** @brief Shapes text with the text-layout engine. */
    [[nodiscard]] ShapedText shapeText(const Font& font, const TextWithOptions& text) const;

    [[nodiscard]] ShapedText shapeText(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                       std::span<const uint32_t> offsets = {}) const;

    using FontKey = std::tuple<std::string, FontStyle, FontWeight>;

    // Internal use only
    int hscale() const noexcept {
        return m_hscale;
    }

    /// Sets the maximum memory retained by the new text-layout glyph cache.
    void setGlyphCacheMemoryBudget(size_t bytes);
    /// Removes all entries from the new text-layout glyph cache.
    void clearGlyphCache();

    void lock() const noexcept;
    bool try_lock() const noexcept;
    void unlock() const noexcept;

private:
    friend struct Font;
    mutable std::recursive_mutex* m_lock;
    void* m_ft_library{};
    std::shared_ptr<Internal::TextEngineState> m_textEngine;

    const int m_hscale;
    std::vector<std::string_view> fontList(std::string_view ff) const;
    mutable std::vector<OsFont> m_osFonts;
    void addFontImpl(BytesView data, std::string alias, bool makeCopy);
    FontMetrics getMetrics(const Font& font) const;
};

extern std::optional<FontManager> fonts;

} // namespace Brisk
