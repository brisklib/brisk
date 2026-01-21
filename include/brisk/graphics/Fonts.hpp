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

#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/Stream.hpp>
#include <brisk/core/Hash.hpp>
#include <mutex>
#include "Color.hpp"
#include <brisk/core/internal/SmallVector.hpp>
#include "internal/OpenType.hpp"
#include "Image.hpp"
#include <brisk/core/Io.hpp>
#include "internal/Sprites.hpp"
#include "I18n.hpp"
#include <memory>
#include <optional>
#include <span>
#include <brisk/core/internal/FunctionRef.hpp>

namespace Brisk {

/// Identifies the pixel format produced for a cached glyph.
enum class GlyphRenderMode : uint8_t {
    Mask,
    Color,
};

/// Identifies one renderer-ready glyph bitmap in a glyph cache.
struct GlyphCacheKey {
    uint64_t fontInstanceId{};
    uint32_t glyphId{};
    uint16_t horizontalScale{};
    GlyphRenderMode renderMode                           = GlyphRenderMode::Mask;

    bool operator==(const GlyphCacheKey&) const noexcept = default;
};

/// Renderer-ready glyph bitmap and its placement metadata.
struct CachedGlyph {
    Size size;
    Size logicalSize;
    Rc<SpriteResource> sprite;
    float offsetX{};
    int offsetY{};
    GlyphRenderMode renderMode = GlyphRenderMode::Mask;
    int horizontalScale        = 1;
};

/// Cumulative lookup counters reported by a glyph cache.
struct GlyphCacheStats {
    uint64_t hits{};
    uint64_t misses{};
};

/** @brief Abstract cache for renderer-ready text glyph bitmaps. */
class GlyphCache {
public:
    virtual ~GlyphCache() = default;

    /// Finds a glyph or creates and stores it using @p factory on a miss.
    virtual std::optional<CachedGlyph> getOrCreate(const GlyphCacheKey& key,
                                                   function_ref<std::optional<CachedGlyph>()> factory) = 0;
    /// Returns cumulative cache hit and miss counters.
    virtual GlyphCacheStats getCacheStats() const noexcept                                             = 0;
    /// Sets the maximum retained cache memory in bytes.
    virtual void setMemoryBudget(size_t bytes)                                                         = 0;
    /// Removes all retained glyphs without resetting statistics.
    virtual void clear()                                                                               = 0;
};

class EUnicode : public ELogic {
public:
    using ELogic::ELogic;
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
class PreparedDocument;
class DocumentLayout;
struct TextSelectionRect;

namespace TextLayout {
class FontDatabase;
}

namespace Internal {
struct TextLayoutState;
void registerTextLayoutFont(FontManager& manager, BytesView data, std::string_view alias = {});

/**
 * @brief Private bridge data for rasterizing a new-engine glyph into Brisk sprites.
 *
 * This type is intentionally confined to the Internal namespace. FreeType,
 * HarfBuzz, and text-engine glyph types do not cross the public API boundary.
 */
struct TextLayoutGlyphBitmap {
    Size size;
    Size logicalSize;
    Rc<SpriteResource> sprite;
    uint32_t glyphIndex = 0;
    float offsetX       = 0.f;
    int offsetY         = 0;
    bool color          = false;
    int horizontalScale = 1;
};

/** @brief Renderer-neutral glyph data emitted by the private document renderer. */
struct TextLayoutGlyph {
    PointF position;
    TextLayoutGlyphBitmap bitmap;
    std::optional<Color> color;
};

/** @brief Renderer-neutral text decoration data emitted by the private document renderer. */
struct TextLayoutDecoration {
    PointF start;
    PointF end;
    float underlineOffset     = 0.f;
    float overlineOffset      = 0.f;
    float lineThroughOffset   = 0.f;
    float thickness           = 0.f;
    TextDecoration decoration = TextDecoration::None;
    std::optional<Color> color;
};

void loadTextLayoutGlyphRun(const PreparedDocument& document, uint32_t preparedRunIndex,
                            GlyphCache* glyphCache,
                            function_ref<void(uint32_t, const Internal::TextLayoutGlyphBitmap&)> onGlyph);
void forEachTextLayoutGlyph(const DocumentLayout& layout, PointF origin,
                            function_ref<void(const Internal::TextLayoutGlyph&)> onGlyph);
void forEachTextLayoutDecoration(const DocumentLayout& layout, PointF origin,
                                 function_ref<void(const Internal::TextLayoutDecoration&)> onDecoration);
void textLayoutSelectionRects(const DocumentLayout& layout, Range<uint32_t> characterSelection,
                              function_ref<void(const ::Brisk::TextSelectionRect&)> onRect);
void renderPreparedDocument(Rc<Image> image, Point origin, const PreparedDocument& document,
                            const DocumentLayout& layout);
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

/**
 * @brief Logical alignment used by the document text layout interface.
 *
 * Unlike the legacy scalar alignment API, Start and End are resolved relative
 * to the paragraph direction. Left and Right are physical alignments.
 */
enum class TextLayoutAlignment : uint8_t {
    Start,
    End,
    Left,
    Right,
    Center,
};

/**
 * @brief Options for producing a width-dependent document layout.
 */
struct TextLayoutOptions {
    /// Maximum line width in pixels. Zero is a special no-wrap alignment container: lines are
    /// positioned around x = 0 (left-aligned starts there, centered text straddles it, and
    /// right-aligned text ends there).
    float maxLineWidth            = HUGE_VALF;
    float firstLineIndent         = 0.f;
    /// Absolute tab interval in pixels. Zero uses the interval derived from the prepared font.
    float tabWidth                = 0.f;
    TextLayoutAlignment alignment = TextLayoutAlignment::Start;
    bool allowBreakAnywhere       = false;
};

/**
 * @brief Identifies which side of a logical grapheme boundary owns a caret.
 *
 * Affinity is required at bidirectional run boundaries and soft-wrap
 * boundaries, where one grapheme boundary can have two visual positions.
 */
enum class CaretAffinity : uint8_t {
    Upstream,
    Downstream,
};

struct CaretIndex {
    uint32_t grapheme      = 0;
    CaretAffinity affinity = CaretAffinity::Downstream;
};

struct CaretPosition {
    float x           = 0.f;
    uint32_t line     = 0;
    uint8_t bidiLevel = 0;
};

struct TextSelectionRect {
    uint32_t line = 0;
    float x0      = 0.f;
    float x1      = 0.f;
};

/**
 * @brief Width-independent paragraph information exposed by PreparedDocument.
 *
 * This is semantic layout information; glyph runs, glyph IDs, clusters, and
 * font-face pointers deliberately remain private to the text engine.
 */
struct PreparedParagraph {
    Range<uint32_t> characterRange{ 0, 0 };
    Range<uint32_t> graphemeRange{ 0, 0 };
    TextDirection direction = TextDirection::LTR;
};

enum class TextLayoutLineEnd : uint8_t {
    SoftWrap,
    MandatoryBreak,
    ParagraphEnd,
};

/**
 * @brief Width-dependent line information exposed by DocumentLayout.
 */
struct DocumentLine {
    Range<uint32_t> characterRange{ 0, 0 };
    Range<uint32_t> graphemeRange{ 0, 0 };
    TextDirection direction = TextDirection::LTR;
    TextLayoutLineEnd end   = TextLayoutLineEnd::ParagraphEnd;
    float baseline          = 0.f;
    float ascender          = 0.f;
    float descender         = 0.f;
    float leading           = 0.f;
    float width             = 0.f;
    float trimmedWidth      = 0.f;
};

class DocumentLayout;

struct PimplAccessor;

/**
 * @brief Immutable, width-independent result of text analysis and shaping.
 *
 * The implementation is hidden behind a shared pimpl. This type is the public
 * Brisk boundary for prepared text; implementation-dependent glyph and font
 * records are not part of the API.
 */
class PreparedDocument {
public:
    /// Opaque implementation type owned by the prepared-document handle.
    struct Impl;

    PreparedDocument();
    ~PreparedDocument();

    PreparedDocument(const PreparedDocument&);
    PreparedDocument& operator=(const PreparedDocument&);
    PreparedDocument(PreparedDocument&&) noexcept;
    PreparedDocument& operator=(PreparedDocument&&) noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] uint32_t characterCount() const noexcept;
    [[nodiscard]] uint32_t graphemeCount() const noexcept;
    [[nodiscard]] std::span<const uint32_t> graphemeBoundaries() const noexcept;
    [[nodiscard]] size_t paragraphCount() const noexcept;
    [[nodiscard]] PreparedParagraph paragraph(size_t index) const noexcept;

    [[nodiscard]] uint32_t characterToGrapheme(uint32_t character) const noexcept;
    [[nodiscard]] uint32_t graphemeToCharacter(uint32_t grapheme) const noexcept;
    [[nodiscard]] Range<uint32_t> graphemeToCharacters(uint32_t grapheme) const noexcept;
    /// Returns whether a grapheme is a paragraph separator excluded from paragraph content.
    [[nodiscard]] bool isParagraphSeparator(uint32_t grapheme) const noexcept;
    /// Converts a character/codepoint boundary to an affinity-aware caret index.
    [[nodiscard]] CaretIndex caretFromCharacter(
        uint32_t character, CaretAffinity affinity = CaretAffinity::Downstream) const noexcept;
    /// Converts an affinity-aware caret index to its character/codepoint boundary.
    [[nodiscard]] uint32_t characterFromCaret(CaretIndex caret) const noexcept;

    [[nodiscard]] DocumentLayout layout(const TextLayoutOptions& options = {}) const;

private:
    explicit PreparedDocument(std::shared_ptr<const Impl> impl);
    friend struct PimplAccessor;

    std::shared_ptr<const Impl> m_impl;

    friend class FontManager;
};

/**
 * @brief Immutable width-dependent layout and text interaction result.
 *
 * Caret, hit-testing, selection, line metrics, and bounds are exposed here.
 * Glyph rasterization and renderer-specific caching remain private to the
 * graphics/text-engine integration.
 */
class DocumentLayout {
public:
    struct Impl;
    DocumentLayout();
    ~DocumentLayout();

    DocumentLayout(const DocumentLayout&);
    DocumentLayout& operator=(const DocumentLayout&);
    DocumentLayout(DocumentLayout&&) noexcept;
    DocumentLayout& operator=(DocumentLayout&&) noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] size_t lineCount() const noexcept;
    [[nodiscard]] DocumentLine line(size_t index) const noexcept;
    [[nodiscard]] RectangleF bounds() const noexcept;
    [[nodiscard]] RectangleF trimmedBounds() const noexcept;

    [[nodiscard]] CaretPosition caretPosition(CaretIndex index) const noexcept;
    /// Returns the line containing the caret, preserving affinity at shared boundaries.
    [[nodiscard]] size_t lineForCaret(CaretIndex index) const noexcept;
    /// Returns the line containing a character/codepoint boundary.
    [[nodiscard]] size_t lineForCharacter(uint32_t character,
                                          CaretAffinity affinity = CaretAffinity::Downstream) const noexcept;
    /// Returns the beginning caret of a line. Line beginnings use downstream affinity.
    [[nodiscard]] CaretIndex lineBeginning(size_t line) const noexcept;
    /// Returns the ending caret of a line. Line ends use upstream affinity.
    [[nodiscard]] CaretIndex lineEnd(size_t line) const noexcept;
    /// Returns a one-pixel-wide caret rectangle using the line's ascent and descent.
    [[nodiscard]] RectangleF caretRect(CaretIndex index, float width = 1.f) const noexcept;
    [[nodiscard]] CaretIndex hitTest(PointF point) const noexcept;
    /// Hit-tests a specific line while preserving the requested horizontal coordinate.
    [[nodiscard]] CaretIndex hitTestLine(size_t line, float x) const noexcept;
    /// Moves a caret to an adjacent line while preserving the caller's preferred X coordinate.
    [[nodiscard]] CaretIndex moveCaretVertically(CaretIndex current, float preferredX,
                                                 int direction) const noexcept;
    void selectionRects(Range<uint32_t> selection, function_ref<void(const TextSelectionRect&)> onRect) const;
    /// Enumerates selection rectangles for a character/codepoint range.
    void selectionRectsByCharacter(Range<uint32_t> selection,
                                   function_ref<void(const TextSelectionRect&)> onRect) const;

private:
    friend struct PimplAccessor;
    explicit DocumentLayout(std::shared_ptr<const Impl> impl);

    std::shared_ptr<const Impl> m_impl;

    friend class PreparedDocument;
    friend class FontManager;
};

struct Font;

/**
 * @brief A collection of OpenType feature flags.
 */
using OpenTypeFeatureFlags = inline_vector<OpenTypeFeatureFlag, 7>;

namespace Internal {

inline std::string format_as(const inline_vector<OpenTypeFeatureFlag, 7>& features) {
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
    float lineHeight              = 1.2f;                 ///< Line height as a multiplier.
    float tabWidth                = 8.f;                  ///< Tab width in space units.
    float letterSpacing           = 0.f;                  ///< Additional space between letters.
    float wordSpacing             = 0.f;                  ///< Additional space between words.
    float verticalAlign           = 0.f;                  ///< Vertical alignment offset.
    OpenTypeFeatureFlags features{};                      ///< OpenType features for advanced text styling.

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

class FontError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
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
     * @brief Gets available styles and weights for a font family.
     * @param fontFamily The font family to query.
     * @return Vector of FontStyleAndWeight pairs.
     */
    std::vector<FontStyleAndWeight> fontFamilyStyles(std::string_view fontFamily) const;

    /**
     * @brief Retrieves metrics for a given font.
     * @param font The font to measure.
     * @return FontMetrics containing size and spacing information.
     */
    [[nodiscard]] FontMetrics metrics(const Font& font) const;

    /**
     * @brief Checks if a font supports a specific Unicode codepoint.
     * @param font The font to check.
     * @param codepoint The Unicode codepoint to verify.
     * @return True if the codepoint is supported, false otherwise.
     */
    [[nodiscard]] bool hasCodepoint(const Font& font, char32_t codepoint) const;

    /** @brief Prepares text with the document text-layout engine. */
    [[nodiscard]] PreparedDocument prepareDocument(const Font& font, const TextWithOptions& text) const;

    [[nodiscard]] PreparedDocument prepareDocument(const TextWithOptions& text,
                                                   std::span<const FontAndColor> fonts,
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

private:
    friend void Internal::registerTextLayoutFont(FontManager& manager, BytesView data,
                                                 std::string_view alias);
    friend struct Font;
    mutable std::recursive_mutex* m_lock;
    void* m_ft_library{};
    std::shared_ptr<Internal::TextLayoutState> m_textLayout;

    int m_hscale;
    std::vector<std::string_view> fontList(std::string_view ff) const;
    mutable std::vector<OsFont> m_osFonts;
    void addFontImpl(BytesView data, std::string alias, bool makeCopy);
    FontMetrics getMetrics(const Font& font) const;
};

extern std::optional<FontManager> fonts;

inline std::vector<uint32_t> textBreakPositions(std::u32string_view text, TextBreakMode mode) {
    std::vector<uint32_t> result(1, 0);
    Rc<Internal::TextBreakIterator> iter = Internal::textBreakIterator(text, mode);
    while (auto p = iter->next()) {
        result.push_back(*p);
    }
    return result;
}

/**
 * @brief Indicates whether the ICU library is available for full Unicode support.
 *
 * When `icuAvailable` is `true`, the font functions will have full Unicode support
 * for Bidirectional (BiDi) text processing and grapheme/line
 * breaking functionality (textBreakPositions).
 *
 */
extern bool icuAvailable;

} // namespace Brisk
