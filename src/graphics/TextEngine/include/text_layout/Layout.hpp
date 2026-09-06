#pragma once

#include <cstdint>
#include <span>
#include <memory>
#include <string_view>
#include <vector>
#include <cassert>

#include "BreakIterator.hpp"
#include "Types.hpp"
#include "FontDatabase.hpp"
#include <brisk/core/internal/FunctionRef.hpp>

namespace Brisk::TextEngine {

// Non-owning view over a boundary table of the form:
//   [0, b_1, b_2, ..., b_{n-1}, total]
// describing n half-open entities: [boundaries[i], boundaries[i+1]).
//
// Backing storage (e.g. arena-allocated array) must outlive this view.
struct BoundaryTable {
    std::span<const uint32_t> boundaries; // size == count() + 1, size >= 1

    explicit BoundaryTable(std::span<const uint32_t> boundaries) : boundaries(boundaries) {}

    [[nodiscard]] bool correct() const noexcept {
        if (boundaries.empty() || boundaries.front() != 0) {
            return false;
        }
        // An empty document still has one font slot, represented by the single
        // zero-length range [0, 0). No offset lookup is performed for it because
        // there are no graphemes to resolve.
        if (boundaries.size() == 2 && boundaries[1] == 0) {
            return true;
        }
        for (size_t i = 1; i < boundaries.size(); ++i) {
            if (boundaries[i] <= boundaries[i - 1]) {
                return false;
            }
        }
        return true;
    }

    // Number of entities described by this table.
    [[nodiscard]] uint32_t count() const noexcept {
        return boundaries.empty() ? 0 : static_cast<uint32_t>(boundaries.size() - 1);
    }

    // O(1) lookup: entity -> half-open offset range.
    [[nodiscard]] Range<uint32_t> rangeOf(uint32_t index) const noexcept {
        assert(index < count());
        return Range<uint32_t>{ boundaries[index], boundaries[index + 1] };
    }

    [[nodiscard]] uint32_t total() const noexcept {
        return boundaries.empty() ? 0 : boundaries.back();
    }

    [[nodiscard]] bool empty() const noexcept {
        return count() == 0;
    }

    [[nodiscard]] uint32_t begin(uint32_t index) const noexcept {
        assert(index < count());
        return boundaries[index];
    }

    [[nodiscard]] uint32_t end(uint32_t index) const noexcept {
        assert(index < count());
        return boundaries[index + 1];
    }

    // O(log n) lookup: offset -> entity index such that
    // boundaries[i] <= offset < boundaries[i+1].
    // Precondition: offset < boundaries.back() (i.e. offset is within range).
    [[nodiscard]] uint32_t indexOf(uint32_t offset) const noexcept {
        assert(!boundaries.empty());
        assert(offset < total());

        const auto it = std::upper_bound(boundaries.begin(), boundaries.end(), offset);
        return static_cast<uint32_t>(it - boundaries.begin()) - 1;
    }
};

/**
 * @brief Bidirectional mapping between codepoint indices and grapheme cluster indices.
 *
 * Maintains two prefix tables over the input text:
 * - `codepointToGrapheme`: for each codepoint offset, the index of the grapheme
 *   cluster containing it (size == codepoint count + 1, last entry == grapheme count).
 * - `graphemeToCodepoint`: for each grapheme index, the starting codepoint offset
 *   of that cluster (size == grapheme count + 1, last entry == total codepoints).
 *
 * Example for text "áb" (U+0061 U+0301 U+0062):
 * - codepointToGrapheme = [0, 0, 1]
 * - graphemeToCodepoint = [0, 2, 3]
 */
struct GraphemeMap {

    /// @brief Constructs an empty map with a terminating entry in the grapheme table.
    GraphemeMap() {
        graphemeToCodepoint.push_back(0);
        codepointToGrapheme.push_back(0);
    }

    /**
     * @brief Constructs the map by segmenting @p text into extended grapheme clusters.
     *
     * @param text UTF-32 text to segment.
     */
    explicit GraphemeMap(std::u32string_view text) {
        GraphemeBreakIterator iterator;
        iterator.setText(text);
        uint32_t prev = iterator.nextBreak(); // first break is always 0
        assert(prev == 0);
        for (uint32_t boundary = iterator.nextBreak(); boundary != UINT32_MAX;
             boundary          = iterator.nextBreak()) {
            graphemeToCodepoint.push_back(prev);
            codepointToGrapheme.insert(codepointToGrapheme.end(), boundary - prev,
                                       static_cast<uint32_t>(graphemeToCodepoint.size() - 1));
            prev = boundary;
        }
        graphemeToCodepoint.push_back(prev); // terminating entry == total codepoints
        codepointToGrapheme.push_back(static_cast<uint32_t>(graphemeToCodepoint.size() - 1));
    }

    /**
     * @brief Constructs a map covering @p range of an existing document-wide map.
     *
     * The resulting map is re-indexed so that grapheme 0 corresponds to the first
     * grapheme cluster of @p range, without re-running the grapheme break iterator.
     *
     * @param document Document-wide grapheme map.
     * @param range Grapheme range within @p document to slice.
     */
    GraphemeMap(const GraphemeMap& document, GraphemeRange range) {
        assert(range.min <= range.max);
        assert(range.max <= document.graphemeCount());
        graphemeToCodepoint.push_back(0);
        for (GraphemeIndex g = range.min; g < range.max; ++g) {
            const CodepointRange codepointRange = document.codepointRangeForGrapheme(g);
            graphemeToCodepoint.push_back(graphemeToCodepoint.back() + codepointRange.distance());
            codepointToGrapheme.insert(codepointToGrapheme.end(), codepointRange.distance(),
                                       static_cast<uint32_t>(graphemeToCodepoint.size() - 2));
        }
        codepointToGrapheme.push_back(static_cast<uint32_t>(graphemeToCodepoint.size() - 1));
    }

    /// @brief Returns the number of extended grapheme clusters in the mapped text.
    uint32_t graphemeCount() const noexcept {
        return static_cast<uint32_t>(graphemeToCodepoint.size() - 1);
    }

    /// @brief Returns the number of codepoints in the mapped text.
    uint32_t codepointCount() const noexcept {
        return static_cast<uint32_t>(codepointToGrapheme.size() - 1);
    }

    /// Returns the document-relative codepoint offset of every grapheme boundary.
    /// The returned view remains valid while this map is alive and is not modified.
    [[nodiscard]] std::span<const uint32_t> graphemeBoundaries() const noexcept {
        return graphemeToCodepoint;
    }

    /**
     * @brief Returns the starting codepoint offset of grapheme cluster @p g.
     * @pre @p g <= graphemeCount().
     */
    CodepointIndex toCodepoint(GraphemeIndex g) const noexcept {
        assert(g < graphemeToCodepoint.size());
        return graphemeToCodepoint[g];
    }

    /**
     * @brief Returns the index of the grapheme cluster containing codepoint @p cp.
     * @pre @p cp <= codepointCount().
     */
    GraphemeIndex toGrapheme(CodepointIndex cp) const noexcept {
        assert(cp < codepointToGrapheme.size());
        return codepointToGrapheme[cp];
    }

    /**
     * @brief Returns the half-open codepoint range covered by grapheme cluster @p g.
     * @pre @p g < graphemeCount().
     */
    CodepointRange codepointRangeForGrapheme(GraphemeIndex g) const noexcept {
        assert(g < graphemeToCodepoint.size() - 1);
        return CodepointRange{ graphemeToCodepoint[g], graphemeToCodepoint[g + 1] };
    }

    /**
     * @brief Converts a document-relative GraphemeRange to the exact half-open CodepointRange it covers.
     * @pre @p range.min <= range.max && range.max <= graphemeCount().
     */
    [[nodiscard]] CodepointRange toCodepointRange(GraphemeRange range) const noexcept {
        assert(range.min <= range.max);
        assert(range.max <= graphemeCount());
        return CodepointRange{ toCodepoint(range.min), toCodepoint(range.max) };
    }

    /**
     * @brief Converts a document-relative CodepointRange to the tightest enclosing GraphemeRange.
     *
     * If range is empty (min == max), returns an empty GraphemeRange at toGrapheme(min).
     * Otherwise spans from the cluster containing range.min through the cluster containing range.max - 1.
     *
     * @pre @p range.min <= range.max && range.max <= codepointCount().
     */
    [[nodiscard]] GraphemeRange toGraphemeRange(CodepointRange range) const noexcept {
        assert(range.min <= range.max);
        assert(range.max <= codepointCount());
        if (range.empty()) {
            const GraphemeIndex g = toGrapheme(range.min);
            return GraphemeRange{ g, g };
        }
        return GraphemeRange{ toGrapheme(range.min), toGrapheme(range.max - 1) + 1 };
    }

    /**
     * @brief Iterates over extended grapheme cluster ranges covered by the specified @p range.
     */
    template <typename Callback>
        requires std::invocable<Callback&, CodepointRange>
    void iterate(GraphemeRange range, Callback&& onGrapheme) const {
        assert(range.min <= range.max);
        assert(range.max <= graphemeCount());
        for (GraphemeIndex g = range.min; g < range.max; ++g) {
            onGrapheme(codepointRangeForGrapheme(g));
        }
    }

    /**
     * @brief Iterates over all extended grapheme cluster ranges in the map.
     */
    template <typename Callback>
        requires std::invocable<Callback&, CodepointRange>
    void iterate(Callback&& onGrapheme) const {
        iterate(GraphemeRange{ 0, graphemeCount() }, std::forward<Callback>(onGrapheme));
    }

    /**
     * @brief Returns a view of this map as a boundary table over grapheme clusters.
     *
     * Entity @c i covers the half-open codepoint range of grapheme cluster @c i, i.e.
     * `[toCodepoint(i), toCodepoint(i + 1))`.
     * @pre The map must outlive the returned table.
     */
    [[nodiscard]] BoundaryTable boundaryTable() const noexcept {
        return BoundaryTable{ graphemeToCodepoint };
    }

private:
    std::vector<uint32_t>
        codepointToGrapheme; // .size() == {codepoints count} + 1, last entry == {grapheme count}
    std::vector<uint32_t>
        graphemeToCodepoint; // .size() == {grapheme count} + 1, last entry == {total codepoints}
    // Example for text "áb" (U+0061 U+0301 U+0062):
    // codepointToGrapheme = [0, 0, 1]
    // graphemeToCodepoint = [0, 2, 3]
};

/**
 * @brief Complete immutable document text and document-wide analysis tables.
 */
struct DocumentSource {
    std::u32string_view text;
    GraphemeMap graphemes;

    DocumentSource() = default;

    explicit DocumentSource(std::u32string_view text) : text(text), graphemes(text) {}

    [[nodiscard]] uint32_t codepointCount() const noexcept {
        return static_cast<uint32_t>(text.size());
    }

    [[nodiscard]] uint32_t graphemeCount() const noexcept {
        return graphemes.graphemeCount();
    }

    [[nodiscard]] std::u32string_view substr(CodepointRange range) const noexcept {
        assert(range.min <= range.max);
        assert(range.max <= text.size());
        return text.substr(range.min, range.distance());
    }

    [[nodiscard]] CodepointRange clamp(CodepointRange range) const noexcept {
        range.min = std::min(range.min, static_cast<CodepointIndex>(text.size()));
        range.max = std::clamp(range.max, range.min, static_cast<CodepointIndex>(text.size()));
        return range;
    }
};

VerticalMetrics getVerticalMetrics(const FontDatabase* fontDatabase, const FontHandle& fontHandle);

/**
 * @brief Returns additional scaled metrics for decoration and common glyph dimensions.
 *
 * Returns zeroed metrics when @p fontDatabase or @p fontHandle is null, or when the
 * font cannot be activated. Metrics unavailable in a font's OpenType tables remain zero.
 */
ExtendedMetrics getExtendedMetrics(const FontDatabase* fontDatabase, const FontHandle& fontHandle);

std::shared_ptr<const FontDatabase> getDefaultFontDatabase();

/** @brief Creates an independently owned, empty database. Use FontDatabase::scanDirectory()
 *         to register fonts from a directory. */
[[nodiscard]] std::shared_ptr<FontDatabase> createFontDatabase();

/**
 * @brief Creates a database using an existing shared FreeType library.
 *
 * `library` is the `FT_Library` value erased as `void*`; `owner` keeps the
 * library alive until the database and its font handles are destroyed.
 */
[[nodiscard]] std::shared_ptr<FontDatabase> createFontDatabase(std::shared_ptr<void> owner, void* library);

// ---------------------------------------------------------------------------
// Step 1: Paragraph segmentation
// ---------------------------------------------------------------------------

/**
 * @brief Identifies the codepoint sequence following and terminating a paragraph.
 */
enum class ParagraphSeparatorKind : uint8_t {
    None,                   ///< No following paragraph separator.
    LineFeed,               ///< Line feed (LF, U+000A).
    CarriageReturn,         ///< Lone carriage return (CR, U+000D).
    CarriageReturnLineFeed, ///< CRLF (U+000D U+000A), treated as one separator.
    InformationSeparator4,  ///< File Separator / Information Separator Four (FS / IS4, U+001C).
    InformationSeparator3,  ///< Group Separator / Information Separator Three (GS / IS3, U+001D).
    InformationSeparator2,  ///< Record Separator / Information Separator Two (RS / IS2, U+001E).
    NextLine,               ///< Next Line (NEL, U+0085).
    ParagraphSeparator,     ///< Unicode paragraph separator (PS, U+2029).
};

/**
 * @brief Returns the length in codepoints of the given paragraph separator kind.
 *
 * @param kind The paragraph separator kind.
 * @return 0 for `None`, 2 for `CarriageReturnLineFeed`, and 1 for all other separator kinds.
 */
uint32_t paragraphSeparatorKindCodepoints(ParagraphSeparatorKind kind);

/**
 * @brief Segments a range of input text into independent BiDi paragraphs in a streaming manner.
 *
 * Recognized separators are LF, lone CR, CRLF, IS4..IS2 (U+001C..U+001E), NEL (U+0085),
 * and PS (U+2029). CRLF is recognized atomically. Leading and consecutive separators
 * produce empty paragraphs. A trailing separator produces an additional final empty paragraph
 * with no separator, and empty input produces one such paragraph.
 *
 * For each detected paragraph, @p onParagraph is invoked with:
 * - A `CodepointRange` spanning the entire paragraph, including any trailing separator codepoints.
 * - The `ParagraphSeparatorKind` identifying the trailing separator (or `None` for the final paragraph
 *   if it has no separator, or for the trailing empty paragraph).
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param range Codepoint range within @p document to segment.
 * @param onParagraph Callback invoked in source order for each detected paragraph.
 */
void segmentParagraphs(const DocumentSource& document, CodepointRange range,
                       function_ref<void(CodepointRange, ParagraphSeparatorKind)> onParagraph);

// ---------------------------------------------------------------------------
// Step 2: BiDi resolution
// ---------------------------------------------------------------------------

/**
 * @brief Resolves bidirectional (BiDi) embedding levels for a single paragraph in a streaming manner.
 *
 * Runs are reported in source (logical) order with document-relative codepoint ranges.
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param paragraphRange Document-relative codepoint range of the single paragraph to resolve.
 * @param baseDirection Base direction policy for resolving the paragraph's embedding level.
 * @param onRun Callback invoked for each unidirectional BiDi run with `(CodepointRange range, BiDiLevel
 * level)`.
 * @return The resolved paragraph base direction (`Direction::LeftToRight` or `Direction::RightToLeft`).
 */
Direction resolveBidi(const DocumentSource& document, CodepointRange paragraphRange,
                      BaseDirection baseDirection, function_ref<void(CodepointRange, BiDiLevel)> onRun);

// ---------------------------------------------------------------------------
// Step 3: Script itemization
// ---------------------------------------------------------------------------

using ScriptTag = uint32_t;

/**
 * @brief Resolves the Unicode script for each extended grapheme cluster in a paragraph.
 *
 * For empty input, returns an empty vector.
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param paragraphRange Document-relative codepoint range of the paragraph.
 * @param out Output span receiving one ISO 15924 script tag per extended grapheme cluster in source order.
 */
void detectScripts(const DocumentSource& document, CodepointRange paragraphRange, std::span<ScriptTag> out);

/**
 * @brief Represents a single script-itemized run within a BiDi run.
 */
struct ScriptRun {
    BiDiLevel level;  ///< Resolved BiDi embedding level.
    ScriptTag script; ///< ISO 15924 four-character script tag.
};

/**
 * @brief Itemizes a single BiDi run into script runs in a streaming manner.
 *
 * For empty input, no callbacks are invoked.
 * Emitted ranges are document-relative subsets of @p bidiRange.
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param bidiRange Document-relative codepoint range of the single BiDi run to itemize.
 * @param level BiDi embedding level of this run.
 * @param script Script tag override (zero for automatic script detection).
 * @param runScripts Pre-detected scripts per grapheme cluster for this BiDi run.
 * @param onRun Callback invoked in source order for each itemized `ScriptRun`.
 */
void itemizeScripts(const DocumentSource& document, CodepointRange bidiRange, BiDiLevel level,
                    ScriptTag script, std::span<const ScriptTag> runScripts,
                    function_ref<void(CodepointRange range, const ScriptRun&)> onRun);

// ---------------------------------------------------------------------------
// Step 4: Font resolving
// ---------------------------------------------------------------------------

/**
 * @brief Represents a single resolved font run ready for shaping.
 */
struct ResolvedFont {
    FontHandle fontHandle;     ///< Resolved concrete font handle (empty if no font found).
    FontRunIndex fontRunIndex; ///< Source FontDef index in fonts list.
};

/**
 * @brief Resolves fonts for a script run in a streaming manner.
 *
 * Emits document-relative codepoint ranges for merged shapeable runs and control runs.
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param scriptRange Document-relative codepoint range of the script run.
 * @param run The script run metadata.
 * @param fontDatabase The font database used to open fonts (may be null).
 * @param fonts Span of font definitions.
 * @param fontBoundaries Boundary table mapping document codepoint offsets to font definition indices.
 * @param onShapeableRun Callback invoked for each merged shapeable font run.
 * @param onControlRun Callback invoked for each single control code run.
 */
void resolveFonts(
    const DocumentSource& document, CodepointRange scriptRange, const ScriptRun& run,
    const FontDatabase* fontDatabase, std::span<const FontDef* const> fonts, BoundaryTable fontBoundaries,
    function_ref<void(CodepointRange range, const ResolvedFont&)> onShapeableRun,
    function_ref<void(CodepointRange range, char32_t codepoint, const ResolvedFont&)> onControlRun);

// ---------------------------------------------------------------------------
// Step 5: Shaping runs
// ---------------------------------------------------------------------------

/**
 * @brief Represents a single shaped glyph produced by HarfBuzz.
 */
struct Glyph {
    uint32_t glyphId;            ///< Font-specific glyph ID.
    GraphemeIndex graphemeIndex; ///< Source grapheme that produced this glyph (relative to run graphemes).
    LayoutUnit xAdvance;         ///< The horizontal advance of the glyph.
    LayoutUnit xOffset;          ///< The horizontal offset of the glyph.
    LayoutUnit yOffset;          ///< The vertical offset of the glyph.
};

constexpr inline size_t maximumOpenTypeFeatures = 16;

/**
 * @brief Shapes a single font-resolved run with HarfBuzz in a streaming manner.
 *
 * @param document Complete UTF-32 document source and its document-wide grapheme map.
 * @param textContextRange Context range for determining BeginOfText/EndOfText flags.
 * @param runRange Document-relative codepoint range of the font-resolved run to shape.
 * @param fontDatabase Font database (may be null).
 * @param run The font-resolved run metadata.
 * @param level BiDi embedding level of the run.
 * @param script Resolved script code.
 * @param fontDef Font definition.
 * @param language Optional BCP-47 language tag.
 * @param onGlyph Callback invoked for each shaped glyph.
 */
void shapeRun(const DocumentSource& document, CodepointRange textContextRange, CodepointRange runRange,
              const FontDatabase* fontDatabase, const ResolvedFont& run, BiDiLevel level, ScriptTag script,
              const FontDef& fontDef, std::string_view language,
              function_ref<void(const Glyph&, bool unsafeToBreak)> onGlyph);

// ---------------------------------------------------------------------------
// Step 6: Line breaking
// ---------------------------------------------------------------------------

/**
 * @brief Describes how a laid-out line ended.
 */
enum class LineEndKind : uint8_t {
    SoftWrap,       ///< Line ended at an automatically selected wrap point.
    MandatoryBreak, ///< Line consumed an intra-paragraph mandatory break.
    ParagraphEnd,   ///< Line ended at a paragraph separator.
};

/**
 * @brief Classifies a codepoint's line break opportunity (UAX #14 style).
 *
 * Any value other than the enumerators below is treated as "no break opportunity".
 */
enum class LineBreakKind : uint8_t {
    MustBreak       = 0, ///< A mandatory break is required after this codepoint.
    AllowBreak      = 1, ///< A soft wrap is permitted after this codepoint.
    NoBreak         = 2, ///< No line break is permitted after this codepoint.
    InsideCharacter = 3, ///< The position is inside a legacy encoded character.
};

/**
 * @brief Computes line break opportunities for a codepoint range in text.
 *
 * Fills @p out with one `LineBreakKind` per codepoint in @p range.
 * `out.size()` must equal `range.distance()`.
 *
 * @param text Complete UTF-32 document text.
 * @param range Document-relative codepoint range to analyze.
 * @param out Output span receiving the break kind for each codepoint.
 * @param language Optional BCP-47 language tag.
 */
void breakOpportunities(std::u32string_view text, CodepointRange range, std::span<LineBreakKind> out,
                        const char* language);

/**
 * @brief Computes line break opportunities for the given standalone text.
 */
inline void breakOpportunities(std::u32string_view paragraphText, std::span<LineBreakKind> out,
                               const char* language) {
    breakOpportunities(paragraphText, CodepointRange{ 0, static_cast<CodepointIndex>(paragraphText.size()) },
                       out, language);
}

// ---------------------------------------------------------------------------
// Prepared paragraph -- cacheable output of analysis and shaping
// ---------------------------------------------------------------------------

/** @brief Per-grapheme flags consumed by concrete line layout. */
struct GraphemeFlags {
    /// Whether this grapheme cluster consists of trimmable whitespace (e.g. trailing space before wrap).
    bool trimmableWhitespace : 1 { false };
    /// Whether this grapheme cluster represents a tab character (`\t`) subject to tab-stop expansion.
    bool isTab : 1 { false };
    /// Whether this grapheme cluster is a separator between paragraphs.
    bool isParagraphSeparator : 1 { false };
    /// Whether HarfBuzz marked the boundary after this grapheme unsafe for line breaking.
    bool unsafeToBreakAfter : 1 { false };
};

static_assert(sizeof(GraphemeFlags) == sizeof(std::uint8_t));

/**
 * @brief One font-resolved shaped run in prepared text.
 *
 * In a `PreparedDocument`, ranges are document-relative. A control run may have an empty glyph range while
 * still contributing grapheme advance.
 */
struct GlyphRun {
    /// Codepoint range covered by this run, relative to the owning prepared object.
    CodepointRange codepointRange;
    /// Grapheme cluster range covered by this run, relative to the owning prepared object.
    GraphemeRange graphemeRange;
    /// Glyph range covered by this run in the owning prepared object's `glyphs` array.
    GlyphRange glyphRange;
    /// Concrete font handle resolved for shaping this run (empty for unresolved or control runs).
    FontHandle fontHandle{};
    /// Index into the input font definitions span (`fonts`).
    FontRunIndex fontRunIndex{ 0 };
    /// Resolved BiDi embedding level of this run.
    BiDiLevel level{ 0 };
    /// Resolved ISO 15924 four-character script tag of this run.
    ScriptTag script{ 0 };
    /// Vertical baseline offset for this run; positive values move the glyphs upward.
    LayoutUnit verticalAlign{ kZero };
    /// Scaled font vertical metrics (ascent, descent, lineGap/leading).
    VerticalMetrics metrics{};
};

constexpr inline CodepointRange entireText{ 0, UINT32_MAX };

// ---------------------------------------------------------------------------
// Step 7: Layout output
// ---------------------------------------------------------------------------

/**
 * @brief One directly renderable glyph-run slice placed on a concrete line.
 *
 * Indices refer to the owning prepared object's flat glyph/run arrays. They are paragraph-relative
 * for the legacy paragraph API and document-relative for the document API.
 */
struct LayoutGlyphRun {
    /// Index into the owning prepared object's `glyphRuns` array.
    RunIndex preparedRunIndex{ 0 };
    /// Glyph range in the owning prepared object's `glyphs` array.
    GlyphRange glyphRange;
    /// Horizontal origin in the associated layout's coordinates (added to glyph run-local positions).
    LayoutUnit xOffset{ kZero };
    /// Baseline vertical position in the associated layout's coordinates.
    LayoutUnit yOffset{ kZero };
};

/**
 * @brief Final geometry and source coverage of one visible line.
 *
 * Source ranges use the coordinate system of the associated prepared object. They are
 * document-relative in `DocumentLayout`. A paragraph separator is not prepared as paragraph
 * content; an intra-paragraph mandatory line break may
 * still be included in the line that consumes it.
 */
struct LayoutLine {
    /// Logical source coverage in grapheme clusters, relative to the owning prepared object.
    GraphemeRange graphemeRange;
    /// Logical source coverage in codepoints, relative to the owning prepared object.
    CodepointRange codepointRange;
    /// Base direction of the paragraph containing this line.
    Direction baseDirection{ Direction::LeftToRight };
    /// Range in the associated layout's `glyphRuns`, ordered visually from left to right.
    RunRange glyphRunRange;
    /// How the line ended (soft wrap, mandatory break, or paragraph end).
    LineEndKind endKind;
    /// Intrinsic inline origin in layout coordinates. The selected alignment edge is at x = 0;
    /// maxLineWidth is used only for wrapping.
    LayoutUnit originX;
    /// Baseline in layout coordinates.
    LayoutUnit baselineY;
    /// Distance above the baseline.
    LayoutUnit ascent;
    /// Signed distance to the descender below the baseline; negative values point downward.
    LayoutUnit descent;
    /// Additional inter-line spacing.
    LayoutUnit leading;
    /// Full laid-out width, including trailing spacing.
    LayoutUnit width;
    /// Width used for alignment, excluding trimmable trailing spacing.
    LayoutUnit trimmedWidth;
};

/**
 * @brief Cacheable output of analysis, font resolution, and shaping for a complete document.
 *
 * The preparation buffers are document-wide and use document-relative indices. Paragraph
 * metadata identifies the portions of those flat buffers belonging to each paragraph.
 */
struct PreparedDocument {
    struct Paragraph {
        /// Document-relative content range, excluding the trailing paragraph separator.
        CodepointRange paragraphRange{ 0, 0 };
        /// Resolved base embedding direction for this paragraph.
        Direction baseDirection{ Direction::LeftToRight };
        /// Separator following this paragraph, if any.
        ParagraphSeparatorKind paragraphSeparatorKind{ ParagraphSeparatorKind::None };
        /// Range in the document-wide grapheme and prepared-grapheme arrays. Separator graphemes
        /// remain globally indexed but are not part of paragraph content.
        GraphemeRange graphemeRange{ 0, 0 };
        /// Range in the document-wide glyph array.
        GlyphRange glyphRange{ 0, 0 };
        /// Range in the document-wide glyph-run array.
        RunRange glyphRunRange{ 0, 0 };
        /// Font metrics used to give an empty paragraph a visible line and caret.
        VerticalMetrics emptyLineMetrics{};
    };

    /// Owns the font database instance needed for later glyph rasterization.
    std::shared_ptr<const FontDatabase> fontDatabase;
    /// Unique identifier copied into the corresponding document layout.
    uint64_t cookie{ 0 };
    /// Document-wide grapheme-to-codepoint and codepoint-to-grapheme mapping table.
    GraphemeMap graphemeMap;
    /// One entry for every segmented paragraph, in source order.
    std::vector<Paragraph> paragraphs;
    /// Line break opportunity per document codepoint.
    std::vector<LineBreakKind> breakOpportunities;
    /// Per-grapheme layout data, indexed by document grapheme offset.
    std::vector<GraphemeFlags> graphemes;
    /// Document-wide prefix sum of grapheme advances; size is `graphemes.size() + 1`. Separator
    /// graphemes have default data and therefore contribute no advance.
    std::vector<LayoutUnit> graphemeAdvancePrefix;
    /// All shaped glyphs in document order.
    std::vector<Glyph> glyphs;
    /// All font-resolved and shaped runs in document order.
    std::vector<GlyphRun> glyphRuns;
};

/**
 * @brief Performs analysis and shaping for the complete document.
 *
 * The document is segmented into paragraphs. Each paragraph range excludes its trailing
 * separator, while `PreparedDocument::Paragraph::paragraphSeparatorKind` records the separator.
 * Separator graphemes remain in the global `graphemeMap` and corresponding flat-array positions;
 * their `GraphemeFlags` entries are default-constructed and are not layout content. All flat
 * arrays and `GlyphRun` ranges use document-relative indices.
 *
 * An empty @p baseDirections span uses `BaseDirection::DefaultLTR` for every paragraph. A
 * one-element span broadcasts its value to every paragraph. Otherwise it must contain one value
 * per prepared paragraph.
 *
 * @param document Complete UTF-32 document and document-wide grapheme map.
 * @param fontDatabase Font database retained for later rasterization.
 * @param fonts Complete span of font definitions.
 * @param fontBoundaries Document-relative font boundary table.
 * @param baseDirections Empty, one-element, or one-per-paragraph direction policies.
 * @param script Explicit script override, or zero for automatic detection.
 * @param language Optional BCP-47 language tag.
 * @pre `fontBoundaries.correct()` and `fontBoundaries.count() == fonts.size()`.
 * @pre `baseDirections` is empty, has one element, or has one element per prepared paragraph.
 */
[[nodiscard]] PreparedDocument prepareDocument(const DocumentSource& document,
                                               std::shared_ptr<const FontDatabase> fontDatabase,
                                               std::span<const FontDef* const> fonts,
                                               BoundaryTable fontBoundaries,
                                               std::span<const BaseDirection> baseDirections = {},
                                               ScriptTag script = 0, std::string_view language = {});

/**
 * @brief Repeating tab stops measured from each paragraph's base-direction margin.
 *
 * Stops are at `firstStop + N * interval` for non-negative integers @c N. An interval of zero
 * disables tab expansion, leaving tab characters with zero advance.
 */
struct TabStops {
    LayoutUnit firstStop{ kZero };
    LayoutUnit interval{ kZero };
};

/**
 * @brief Complete document layout produced by the text layout pipeline.
 *
 * Lines are ordered by paragraph and source order. Their source ranges are document-relative,
 * and their glyph-run ranges index the document-wide `glyphRuns` array. Vertical coordinates are
 * continuous across paragraphs. Every paragraph contributes at least one `ParagraphEnd` line,
 * including empty paragraphs.
 */
struct DocumentLayout {
    /// Copy of `PreparedDocument::cookie` from the document this layout was produced from.
    uint64_t cookie{ 0 };
    /// Lines in document/source order. Source ranges and indices are document-relative.
    std::vector<LayoutLine> lines;
    /// Directly renderable run slices in per-line visual order.
    std::vector<LayoutGlyphRun> glyphRuns;
    /// Union of vertical line bounds in document layout coordinates.
    LayoutRange verticalTextBounds{ kZero, kZero };
    /// Union of horizontal line bounds in document layout coordinates.
    LayoutRange horizontalTextBounds{ kZero, kZero };
    /// Union of horizontal trimmed line bounds in document layout coordinates.
    LayoutRange horizontalTrimmedTextBounds{ kZero, kZero };
};

/**
 * @brief Greedily lays out a complete prepared document.
 *
 * Paragraphs are processed sequentially without changing the document-relative indices in the
 * prepared buffers. A paragraph's text indent applies only to its first line. Empty @p alignments
 * use `TextAlignment::Start`, and empty @p textIndents use `kZero`. A one-element span broadcasts
 * its value to every paragraph. Otherwise each span must contain one value per paragraph.
 * `maxLineWidth` and @p tabStops apply to every paragraph. When `maxLineWidth` is `kInfinity`,
 * optional line-break opportunities are ignored: each paragraph is laid out on one line, except
 * that mandatory breaks still end the current line. Tab stops and first-line indents remain active.
 *
 * @param document Complete prepared document.
 * @param alignments Empty, one-element, or one-per-paragraph alignment values.
 * @param maxLineWidth Maximum width used for every paragraph; use `kInfinity` or `kZero` to
 * disable wrapping while continuing to honor mandatory line breaks. Horizontal coordinates are
 * intrinsic and independent of this value: left/start-LTR/end-RTL lines start at x = 0, centered
 * lines straddle x = 0, and right/end-LTR/start-RTL lines end at x = 0. If @p allowBreakAnywhere
 * is true, wrapping is also allowed between adjacent grapheme clusters where HarfBuzz does not
 * report an unsafe break.
 * @param tabStops Repeating tab-stop configuration used for every paragraph.
 * @param textIndents Empty, one-element, or one-per-paragraph first-line indents.
 * @param allowBreakAnywhere Permit wrapping at any HarfBuzz-safe grapheme boundary.
 * @pre `alignments` and `textIndents` are empty, contain one value, or contain one value per paragraph.
 */
[[nodiscard]] DocumentLayout layoutPreparedDocument(const PreparedDocument& document,
                                                    std::span<const TextAlignment> alignments,
                                                    LayoutUnit maxLineWidth, TabStops tabStops = {},
                                                    std::span<const LayoutUnit> textIndents = {},
                                                    bool allowBreakAnywhere                 = false);

// ---------------------------------------------------------------------------
// Caret & selection queries
// ---------------------------------------------------------------------------

/**
 * @brief Disambiguates a caret that sits at a boundary shared by two valid renderings.
 *
 * A `GraphemeIndex` alone is ambiguous exactly at the boundary between two BiDi runs of
 * different level, and at a soft-wrap boundary where the same offset ends one line and begins
 * the next. `Upstream` attaches the caret to the content immediately before the index (the run
 * or line it terminates); `Downstream` attaches it to the content immediately after (the run or
 * line it begins). Away from such boundaries, both values resolve to the same position.
 */
enum class CaretAffinity : uint8_t {
    Upstream,   ///< Attach to the content preceding the grapheme index.
    Downstream, ///< Attach to the content following the grapheme index.
};

/**
 * @brief A logical caret location: a grapheme boundary plus its boundary disambiguation.
 */
struct CaretIndex {
    GraphemeIndex grapheme{ 0 };
    CaretAffinity affinity{ CaretAffinity::Downstream };
};

/**
 * @brief Single-point rendering geometry for a caret.
 *
 * Vertical extent is intentionally not duplicated here: use the returned line index to look up
 * `baselineY`, `ascent`, and `descent` in the associated layout's `lines` array.
 */
struct CaretPosition {
    LayoutUnit x{ kZero }; ///< Horizontal position in layout coordinates.
    LineIndex line{ 0 };   ///< Index into the associated layout's `lines` array.
    BiDiLevel level{ 0 };  ///< Embedding level of the run the caret is attached to.
};

/**
 * @brief Computes a caret position using document-relative grapheme offsets.
 *
 * The returned line index refers to `DocumentLayout::lines`. At a paragraph boundary, upstream
 * affinity attaches to the preceding paragraph and downstream affinity attaches to the following
 * paragraph. Separator graphemes are not layout content; caret positions remain at adjacent
 * paragraph boundaries. Empty paragraphs have a valid zero-width caret line.
 *
 * @param document Complete prepared document.
 * @param layout Document layout produced from @p document.
 * @param index Document-relative grapheme boundary and affinity.
 * @pre `index.grapheme <= document.graphemeMap.graphemeCount()`.
 * @pre `layout.cookie == document.cookie`.
 */
[[nodiscard]] CaretPosition caretPosition(const PreparedDocument& document, const DocumentLayout& layout,
                                          CaretIndex index);

/**
 * @brief Hit-tests a document layout and returns a document-relative grapheme offset.
 *
 * The point is clamped to the nearest document line vertically and to the nearest caret boundary
 * horizontally. Pass the returned index to `caretPosition` to obtain its geometry.
 *
 * @param document Complete prepared document.
 * @param layout Document layout produced from @p document.
 * @param x Horizontal layout coordinate.
 * @param y Vertical layout coordinate.
 * @pre `layout.cookie == document.cookie`.
 * @return A document-relative grapheme boundary and its affinity. The hit-tested line is available
 *         by passing the result to `caretPosition`.
 */
[[nodiscard]] CaretIndex hitTestPoint(const PreparedDocument& document, const DocumentLayout& layout,
                                      LayoutUnit x, LayoutUnit y);

/**
 * @brief One highlight rectangle contributed by a single visual run to a selection.
 *
 * A logically contiguous selection can require more than one rectangle on a single line when it
 * crosses a BiDi run boundary, since selected text need not be visually contiguous. Affinity
 * plays no role here: unlike a caret, a highlighted range is unambiguous regardless of which
 * side of a boundary its endpoints are conceptually attached to.
 */
struct SelectionRect {
    LineIndex line{ 0 };    ///< Index into the associated layout's `lines` array.
    LayoutUnit x0{ kZero }; ///< Left edge in layout coordinates.
    LayoutUnit x1{ kZero }; ///< Right edge in layout coordinates.
};

/**
 * @brief Enumerates document-relative selection rectangles, including cross-paragraph selections.
 *
 * A selection may span paragraph separators. Rectangles are emitted only for rendered content, in
 * visual order within each line and document line order overall. Separator and empty-paragraph
 * positions do not produce rectangles.
 *
 * @param document Complete prepared document.
 * @param layout Document layout produced from @p document.
 * @param selection Document-relative grapheme range with an exclusive maximum.
 * @param onRect Callback invoked once for each emitted rectangle.
 * @pre `layout.cookie == document.cookie`.
 */
void selectionRects(const PreparedDocument& document, const DocumentLayout& layout, GraphemeRange selection,
                    function_ref<void(const SelectionRect&)> onRect);

/**
 * @brief Metrics and dimensions of a rasterized glyph bitmap.
 */
struct RasterizedGlyph {
    enum class Format : uint8_t {
        Mask8,
        BGRA8,
    };

    /// Horizontal offset of the bitmap's left edge from the glyph origin (pixels).
    int left{};
    /// Vertical offset of the bitmap's top edge from the glyph origin (pixels, up is positive).
    int top{};
    /// Width of the bitmap in pixels.
    uint32_t width{};
    /// Height of the bitmap in pixels.
    uint32_t height{};
    /// Number of bytes between rows in the FreeType bitmap (may be negative).
    int pitch{};
    /// Pixel format of the borrowed bitmap buffer.
    Format format          = Format::Mask8;
    /// Bytes between rows in the returned bitmap, normalized to positive stride.
    uint32_t bytesPerPixel = 1;
    /// Effective horizontal raster scale. Color/SVG glyphs may use native scale (1).
    int horizontalScale    = 1;
};

/** @brief Rendering policy applied while rasterizing a glyph. */
struct RasterizationOptions {
    /// Horizontal oversampling factor. The vertical size remains unchanged.
    int horizontalScale = 1;
    /// Permit color bitmap and SVG glyph output when available.
    bool enableColor    = false;
};

/// Rasterizes a glyph at the requested scale and invokes `callback` with its metrics
/// and the borrowed FreeType bitmap buffer. The buffer is valid only for the duration
/// of the callback and must not be retained or modified.
[[nodiscard]] bool rasterize(const ActiveFont& activeFont, const FontHandle& fontHandle, uint32_t glyphId,
                             const RasterizationOptions& options,
                             function_ref<void(const RasterizedGlyph&, const uint8_t*)> callback);

inline bool rasterize(const ActiveFont& activeFont, const FontHandle& fontHandle, uint32_t glyphId, int scale,
                      function_ref<void(const RasterizedGlyph&, const uint8_t*)> callback) {
    return rasterize(activeFont, fontHandle, glyphId, RasterizationOptions{ .horizontalScale = scale },
                     std::move(callback));
}

} // namespace Brisk::TextEngine
