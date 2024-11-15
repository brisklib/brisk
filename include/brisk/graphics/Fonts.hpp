#pragma once
#include <brisk/core/internal/InlineVector.hpp>
#include <brisk/core/Stream.hpp>
#include <brisk/core/Hash.hpp>
#include <mutex>
#include "internal/OpenType.hpp"
#include "Image.hpp"
#include <brisk/core/IO.hpp>
#include "internal/Sprites.hpp"

namespace Brisk {

class EUnicode : public ELogic {
public:
    using ELogic::ELogic;
};

class EFreeType : public ELogic {
public:
    using ELogic::ELogic;
};

enum class TextBreakMode {
    Grapheme,
    Word,
    Line,
};

constexpr auto operator+(TextBreakMode value) noexcept {
    return static_cast<std::underlying_type_t<decltype(value)>>(value);
}

std::vector<int32_t> textBreakPositions(std::u32string_view text, TextBreakMode mode);

using GlyphID = uint32_t;

enum class TextDirection : uint8_t {
    LTR,
    RTL,
};

struct BidiText {
    std::optional<TextDirection> direction; // nullopt means mixed
};

RC<BidiText> bidiText(std::u32string_view text, TextDirection defaultDirection);

template <>
inline constexpr std::initializer_list<NameValuePair<TextDirection>> defaultNames<TextDirection>{
    { "LTR", TextDirection::LTR },
    { "RTL", TextDirection::RTL },
};

enum class LayoutOptions : uint32_t {
    Default    = 0,
    SingleLine = 1,
};

BRISK_FLAGS(LayoutOptions)

struct TextSpan {
    uint32_t start;
    uint32_t stop;
    uint32_t format;
};

struct TextWithOptions {
    using Char = char32_t;
    u32string text;
    LayoutOptions options;
    TextDirection defaultDirection;

    constexpr static std::tuple Reflection{
        ReflectionField{ "text", &TextWithOptions::text },
        ReflectionField{ "options", &TextWithOptions::options },
        ReflectionField{ "defaultDirection", &TextWithOptions::defaultDirection },
    };

    TextWithOptions(string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(utf8ToUtf32(text)), options(options), defaultDirection(defaultDirection) {}

    template <std::convertible_to<std::string_view> T>
    TextWithOptions(T&& text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(utf8ToUtf32(text)), options(options), defaultDirection(defaultDirection) {}

    template <std::convertible_to<std::u32string_view> T>
    TextWithOptions(T&& text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(text), options(options), defaultDirection(defaultDirection) {}

    TextWithOptions(u16string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(utf16ToUtf32(text)), options(options), defaultDirection(defaultDirection) {}

    TextWithOptions(u32string_view text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(text), options(options), defaultDirection(defaultDirection) {}

    TextWithOptions(u32string text, LayoutOptions options = LayoutOptions::Default,
                    TextDirection defaultDirection = TextDirection::LTR)
        : text(std::move(text)), options(options), defaultDirection(defaultDirection) {}

    bool operator==(const TextWithOptions& other) const noexcept = default;
};

struct OpenTypeFeatureFlag {
    OpenTypeFeature feature;
    bool enabled;
    bool operator==(const OpenTypeFeatureFlag& b) const noexcept = default;
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

enum class FontFamily : uint32_t {
    Default,
};

constexpr std::underlying_type_t<FontFamily> operator+(FontFamily ff) {
    return static_cast<std::underlying_type_t<FontFamily>>(ff);
}

class FontManager;

struct FontMetrics {
    float size;
    float ascender;
    float descender;
    float height;
    float spaceAdvanceX;
    float lineThickness;
    float xHeight;
    float capitalHeight;
    float linegap() const noexcept;
    float vertBounds() const noexcept;
    float underlineOffset() const noexcept;
    float overlineOffset() const noexcept;
    float lineThroughOffset() const noexcept;

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

struct TextRun {
    TextDirection direction;
    int32_t begin;
    int32_t end;
    int32_t visualOrder;
    FontFace* face;
    bool operator==(const TextRun&) const noexcept = default;

    inline static const std::tuple Reflection      = {
        ReflectionField{ "direction", &TextRun::direction },
        ReflectionField{ "begin", &TextRun::begin },
        ReflectionField{ "end", &TextRun::end },
        ReflectionField{ "visualOrder", &TextRun::visualOrder },
    };
};

std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection,
                                   bool visualOrder);
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

struct Glyph {
    uint32_t glyph      = UINT32_MAX;
    char32_t codepoint  = UINT32_MAX;
    PointF pos          = { -1.f, -1.f };
    float left_caret    = -1.f;
    float right_caret   = -1.f;
    uint32_t begin_char = UINT32_MAX;
    uint32_t end_char   = UINT32_MAX;
    TextDirection dir   = TextDirection::LTR;
    GlyphFlags flags    = GlyphFlags::None;

    // internal methods, do not use directly
    float caretForDirection(bool inverse) const;
    optional<GlyphData> load(const GlyphRun& run) const;
};

struct GlyphData {
    Size size;
    RC<SpriteResource> sprite;
    float offset_x; // left bearing
    int offset_y;   // top bearing, upwards y coordinates being positive
    float advance_x;
};

using GlyphList = std::vector<Glyph>;

} // namespace Internal

enum class GlyphRunBounds {
    Text,
    Alignment,
    Printable,
};

struct GlyphRun {
    Internal::GlyphList glyphs;
    Internal::FontFace* face;
    float fontSize = 0;
    FontMetrics metrics;
    TextDecoration decoration = TextDecoration::None;
    TextDirection direction;
    mutable bool rangesValid = false;
    mutable Range<float> textHRange;      // considers all glyphs
    mutable Range<float> alignmentHRange; // considers all glyphs except whitespace at line breaks
    mutable Range<float> printableHRange; // considers all printable glyphs
    int32_t visualOrder;
    float verticalAlign;
    int line = 0;
    PointF position;
    RectangleF bounds(GlyphRunBounds boundsType) const;
    SizeF size(GlyphRunBounds boundsType) const;
    void invalidateRanges();
    void updateRanges() const;
    GlyphRun breakAt(float width, bool allowEmpty) &;
    Internal::GlyphFlags flags() const;
};

using GlyphRuns = std::vector<GlyphRun>;

struct Font;

struct ShapedRuns;
using PrerenderedText = ShapedRuns;

enum class ShapedRunsState {
    Logical,
    Visual,
};

struct ShapedRuns {
    GlyphRuns runs;
    ShapedRunsState state = ShapedRunsState::Logical;
    LayoutOptions options = LayoutOptions::Default;
    RectangleF bounds(GlyphRunBounds boundsType = GlyphRunBounds::Alignment) const;
    PrerenderedText prerender(const Font& font, float maxWidth) &&;
    PrerenderedText prerender(const Font& font, float maxWidth) const&;

    void applyOffset(PointF offset);
    void align(PointF pos, float alignment_x, float alignment_y);
    void align(RectangleF rect, float alignment_x, float alignment_y);
    void alignLines(PointF pos, float alignment_x, float alignment_y);
    void alignLines(RectangleF rect, float alignment_x, float alignment_y);

private:
    static size_t extractLine(GlyphRuns& output, GlyphRuns& input, float maxWidth);
    static void formatLine(std::span<GlyphRun> input, float y, int lineNum, float tabWidth);
    static RectangleF bounds(std::span<const GlyphRun> runs, GlyphRunBounds boundsType);
};

using OpenTypeFeatureFlags = inline_vector<OpenTypeFeatureFlag, 7>;

struct Font {
    FontFamily fontFamily         = FontFamily::Default;
    float fontSize                = 10.f;
    FontStyle style               = FontStyle::Normal;
    FontWeight weight             = FontWeight::Regular;
    TextDecoration textDecoration = TextDecoration::None;
    float lineHeight              = 1.2f;
    float tabWidth                = 8.f; // in widths of space
    float letterSpacing           = 0.f;
    float wordSpacing             = 0.f;
    float verticalAlign           = 0.f;
    OpenTypeFeatureFlags features{};

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

    Font operator()(FontFamily fontFamily) const {
        Font result       = *this;
        result.fontFamily = fontFamily;
        return result;
    }

    Font operator()(float fontSize) const {
        Font result     = *this;
        result.fontSize = fontSize;
        return result;
    }

    Font operator()(FontStyle style) const {
        Font result  = *this;
        result.style = style;
        return result;
    }

    Font operator()(FontWeight weight) const {
        Font result   = *this;
        result.weight = weight;
        return result;
    }

    bool operator==(const Font& b) const noexcept = default;
};

struct FontStyleAndWeight {
    FontStyle style                                             = FontStyle::Normal;
    FontWeight weight                                           = FontWeight::Regular;

    bool operator==(const FontStyleAndWeight& b) const noexcept = default;
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

    void addMergedFont(FontFamily fontFamily, std::initializer_list<FontFamily> families);
    void addFont(FontFamily fontFamily, FontStyle style, FontWeight weight, bytes_view data,
                 bool makeCopy = true, FontFlags flags = FontFlags::Default);
    [[nodiscard]] bool addFontByName(FontFamily fontFamily, std::string_view fontName);
    [[nodiscard]] bool addSystemFont(FontFamily fontFamily);
    [[nodiscard]] status<IOError> addFontFromFile(FontFamily family, FontStyle style, FontWeight weight,
                                                  const fs::path& path);
    [[nodiscard]] std::vector<OSFont> installedFonts(bool rescan = false) const;
    std::vector<FontStyleAndWeight> fontFamilyStyles(FontFamily fontFamily) const;

    FontMetrics metrics(const Font& font) const;
    bool hasCodepoint(const Font& font, char32_t codepoint) const;
    ShapedRuns shape(const Font& font, const TextWithOptions& text) const;
    PrerenderedText prerender(const Font& font, const TextWithOptions& text, float width = HUGE_VALF) const;
    RectangleF bounds(const Font& font, const TextWithOptions& text,
                      GlyphRunBounds boundsType = GlyphRunBounds::Alignment) const;

    using FontKey = std::tuple<FontFamily, FontStyle, FontWeight>;

    FontKey faceToKey(Internal::FontFace* face) const;
    void testRender(RC<Image> image, const PrerenderedText& run, Point origin,
                    TestRenderFlags flags = TestRenderFlags::None, std::initializer_list<int> xlines = {},
                    std::initializer_list<int> ylines = {}) const;

    int hscale() const {
        return m_hscale;
    }

    void garbageCollectCache();

private:
    friend struct Internal::FontFace;
    friend struct Font;
    std::map<FontKey, std::unique_ptr<Internal::FontFace>> m_fonts;
    std::map<FontFamily, inline_vector<FontFamily, maxFontsInMergedFonts>> m_mergedFonts;
    void* m_ft_library;
    mutable std::recursive_mutex* m_lock;

    struct ShapeCacheEntry {
        ShapedRuns runs;
        uint64_t counter;
    };

    mutable std::unordered_map<Internal::ShapingCacheKey, ShapeCacheEntry, FastHash> m_shapeCache;
    mutable uint64_t m_cacheCounter = 0;
    int m_hscale;
    uint32_t m_cacheTimeMs;
    inline_vector<FontFamily, maxFontsInMergedFonts> fontList(FontFamily ff) const;
    mutable std::vector<OSFont> m_osFonts;
    Internal::FontFace* lookup(const Font& font) const;
    std::pair<Internal::FontFace*, GlyphID> lookupCodepoint(const Font& font, char32_t codepoint,
                                                            bool fallbackToUndef) const;
    FontMetrics getMetrics(const Font& font) const;
    static RectangleF glyphBounds(const Internal::Glyph& g, const Internal::GlyphData& d);
    ShapedRuns shapeRuns(const Font& font, const TextWithOptions& text,
                         const std::vector<Internal::TextRun>& textRuns) const;
    std::vector<Internal::TextRun> assignFontsToTextRuns(
        const Font& font, std::u32string_view text, const std::vector<Internal::TextRun>& textRuns) const;
    std::vector<Internal::TextRun> splitControls(std::u32string_view text,
                                                 const std::vector<Internal::TextRun>& textRuns) const;
    PrerenderedText doPrerender(const Font& font, const TextWithOptions& text, float width = HUGE_VALF) const;
    ShapedRuns doShapeCached(const Font& font, const TextWithOptions& text) const;
    ShapedRuns doShape(const Font& font, const TextWithOptions& text) const;
};

extern std::optional<FontManager> fonts;

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
