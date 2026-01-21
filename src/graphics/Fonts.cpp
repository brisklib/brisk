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
#include <brisk/graphics/Html.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <list>
#include <map>
#include <unordered_map>
#include <brisk/core/Log.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Resources.hpp>
#include <brisk/core/Text.hpp>
#include <text_layout/Layout.hpp>

#include "FontInternals.hpp"

#include <lunasvg.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include FT_SIZES_H
#include FT_TRUETYPE_TABLES_H
#include FT_OTSVG_H
#include FT_MODULE_H

namespace Brisk {

namespace {

TextLayout::Direction toTextLayoutDirection(TextDirection direction) {
    return direction == TextDirection::RTL ? TextLayout::Direction::RightToLeft
                                           : TextLayout::Direction::LeftToRight;
}

TextLayout::TextAlignment toTextLayoutAlignment(TextLayoutAlignment alignment) {
    switch (alignment) {
    case TextLayoutAlignment::Start:
        return TextLayout::TextAlignment::Start;
    case TextLayoutAlignment::End:
        return TextLayout::TextAlignment::End;
    case TextLayoutAlignment::Left:
        return TextLayout::TextAlignment::Left;
    case TextLayoutAlignment::Right:
        return TextLayout::TextAlignment::Right;
    case TextLayoutAlignment::Center:
        return TextLayout::TextAlignment::Center;
    }
    return TextLayout::TextAlignment::Start;
}

TextLayout::FontStyle toTextLayoutStyle(FontStyle style) {
    return style == FontStyle::Italic ? TextLayout::FontStyle::Italic : TextLayout::FontStyle::Normal;
}

TextLayout::FontWeight toTextLayoutWeight(FontWeight weight) {
    return static_cast<TextLayout::FontWeight>(static_cast<uint16_t>(weight));
}

struct ConvertedFont {
    std::vector<std::string_view> familyNames;
    std::vector<TextLayout::OpenTypeFeatureFlag> features;
    TextLayout::FontDef definition{};
};

ConvertedFont convertFont(const Font& source, const std::shared_ptr<TextLayout::FontDatabase>& database) {
    ConvertedFont converted;
    for (std::string_view family : split(source.fontFamily, ',')) {
        family = trim(family);
        if (!family.empty()) {
            converted.familyNames.push_back(family);
        }
    }
    converted.features.reserve(source.features.size());
    for (const OpenTypeFeatureFlag& feature : source.features) {
        converted.features.push_back(
            TextLayout::OpenTypeFeatureFlag{ static_cast<uint32_t>(feature.feature), feature.enabled });
    }

    converted.definition = TextLayout::FontDef{
        .familyNames   = converted.familyNames,
        .fontSize      = TextLayout::fromFloat(source.fontSize),
        .style         = toTextLayoutStyle(source.style),
        .weight        = toTextLayoutWeight(source.weight),
        .lineHeight    = TextLayout::kZero,
        .letterSpacing = TextLayout::fromFloat(source.letterSpacing),
        .wordSpacing   = TextLayout::fromFloat(source.wordSpacing),
        .verticalAlign = TextLayout::fromFloat(source.verticalAlign),
        .features      = converted.features,
        .variations    = {},
        .hinting       = TextLayout::Hinting::Auto,
    };

    if (source.lineHeight > 0.f) {
        if (TextLayout::FontHandle handle = database->resolveFont(converted.definition)) {
            const TextLayout::VerticalMetrics metrics =
                TextLayout::getVerticalMetrics(database.get(), handle);
            const float naturalHeight =
                TextLayout::toFloat(metrics.ascent + metrics.descent + metrics.lineGap);
            converted.definition.lineHeight = TextLayout::fromFloat(naturalHeight * source.lineHeight);
        }
    }
    return converted;
}

void convertFonts(std::span<const FontAndColor> source, std::span<ConvertedFont> result,
                  const std::shared_ptr<TextLayout::FontDatabase>& database) {
    BRISK_ASSERT(source.size() == result.size());

    for (size_t i = 0; i < source.size(); ++i) {
        result[i] = convertFont(source[i].font, database);
    }
}

} // namespace

namespace Internal {

namespace {

/// LRU glyph cache bounded by the total size of retained sprite data.
class BudgetedGlyphCache final : public GlyphCache {
    struct Entry {
        CachedGlyph glyph;
        size_t bytes{};
        std::list<::Brisk::GlyphCacheKey>::iterator lru;
    };

    struct KeyHash {
        size_t operator()(const ::Brisk::GlyphCacheKey& key) const noexcept {
            uint64_t hash = key.fontInstanceId;
            hash ^= static_cast<uint64_t>(key.glyphId) + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
            hash ^= static_cast<uint64_t>(key.horizontalScale) + 0x9E3779B97F4A7C15ull + (hash << 6) +
                    (hash >> 2);
            hash ^= static_cast<uint64_t>(key.renderMode) + 0x9E3779B97F4A7C15ull + (hash << 6) + (hash >> 2);
            return static_cast<size_t>(hash);
        }
    };

public:
    /// Creates a cache with the specified sprite-memory budget.
    explicit BudgetedGlyphCache(size_t budget = 64u * 1024u * 1024u) : m_budget(budget) {}

    std::optional<CachedGlyph> getOrCreate(const ::Brisk::GlyphCacheKey& key,
                                           function_ref<std::optional<CachedGlyph>()> factory) override {
        if (auto found = m_entries.find(key); found != m_entries.end()) {
            ++m_hits;
            m_lru.splice(m_lru.end(), m_lru, found->second.lru);
            return found->second.glyph;
        }

        ++m_misses;
        std::optional<CachedGlyph> created = factory();
        if (!created) {
            return std::nullopt;
        }

        const size_t bytes =
            created->sprite ? static_cast<size_t>(std::max(0, created->sprite->size.area())) : 0;
        if (m_budget == 0 || bytes > m_budget) {
            return created;
        }

        while (!m_lru.empty() && m_usedBytes + bytes > m_budget) {
            const ::Brisk::GlyphCacheKey& oldest = m_lru.front();
            auto found                           = m_entries.find(oldest);
            if (found != m_entries.end()) {
                m_usedBytes -= found->second.bytes;
                m_entries.erase(found);
            }
            m_lru.pop_front();
        }

        m_lru.push_back(key);
        auto lru = std::prev(m_lru.end());
        m_entries.emplace(key, Entry{ std::move(*created), bytes, lru });
        m_usedBytes += bytes;
        return m_entries.find(key)->second.glyph;
    }

    GlyphCacheStats getCacheStats() const noexcept override {
        return GlyphCacheStats{ m_hits, m_misses };
    }

    void setMemoryBudget(size_t bytes) override {
        m_budget = bytes;
        while (!m_lru.empty() && m_usedBytes > m_budget) {
            const ::Brisk::GlyphCacheKey& oldest = m_lru.front();
            auto found                           = m_entries.find(oldest);
            if (found != m_entries.end()) {
                m_usedBytes -= found->second.bytes;
                m_entries.erase(found);
            }
            m_lru.pop_front();
        }
    }

    void clear() override {
        m_entries.clear();
        m_lru.clear();
        m_usedBytes = 0;
    }

private:
    size_t m_budget{};
    size_t m_usedBytes{};
    uint64_t m_hits{};
    uint64_t m_misses{};
    std::list<::Brisk::GlyphCacheKey> m_lru;
    std::unordered_map<::Brisk::GlyphCacheKey, Entry, KeyHash> m_entries;
};

} // namespace

struct SharedLibraryOwner {
    void* library = nullptr;

    explicit SharedLibraryOwner(void* library) : library(library) {}

    ~SharedLibraryOwner() {
        if (library != nullptr) {
            FT_Done_FreeType(static_cast<FT_Library>(library));
        }
    }
};

struct TextLayoutState {
    std::vector<std::shared_ptr<const Bytes>> fontBlobs;
    std::shared_ptr<SharedLibraryOwner> libraryOwner;
    std::shared_ptr<TextLayout::FontDatabase> database;
    std::shared_ptr<GlyphCache> glyphCache;
    int hscale = 1;

    explicit TextLayoutState(std::shared_ptr<SharedLibraryOwner> owner, int hscale)
        : libraryOwner(std::move(owner)), hscale(hscale) {
        database   = TextLayout::createFontDatabase(libraryOwner, libraryOwner->library);
        glyphCache = std::make_shared<BudgetedGlyphCache>();
    }
};

struct PreparedRenderStyle {
    Color color{};
    uint8_t hasColor          = 0;
    TextDecoration decoration = TextDecoration::None;
};

} // namespace Internal

struct PreparedDocument::Impl {
    std::shared_ptr<const Internal::TextLayoutState> state;
    TextLayout::PreparedDocument prepared;
    std::vector<Internal::PreparedRenderStyle> renderStyles;
    TextOptions options                       = TextOptions::Default;
    TextLayout::LayoutUnit defaultTabInterval = TextLayout::kZero;
};

namespace {

PreparedParagraph convertParagraph(const TextLayout::PreparedDocument::Paragraph& paragraph) {
    return {
        .characterRange = paragraph.paragraphRange,
        .graphemeRange  = paragraph.graphemeRange,
        .direction      = paragraph.baseDirection == TextLayout::Direction::RightToLeft ? TextDirection::RTL
                                                                                        : TextDirection::LTR,
    };
}

DocumentLine convertLine(const TextLayout::LayoutLine& line) {
    DocumentLine result{
        .characterRange = line.codepointRange,
        .graphemeRange  = line.graphemeRange,
        .direction      = line.baseDirection == TextLayout::Direction::RightToLeft ? TextDirection::RTL
                                                                                   : TextDirection::LTR,
        .end            = static_cast<TextLayoutLineEnd>(line.endKind),
        .baseline       = TextLayout::toFloat(line.baselineY),
        .ascender       = TextLayout::toFloat(line.ascent),
        .descender      = TextLayout::toFloat(line.descent),
        .leading        = TextLayout::toFloat(line.leading),
        .width          = TextLayout::toFloat(line.width),
        .trimmedWidth   = TextLayout::toFloat(line.trimmedWidth),
    };
    return result;
}

RectangleF convertBounds(const TextLayout::DocumentLayout& layout) {
    return RectangleF{ TextLayout::toFloat(layout.horizontalTextBounds.min),
                       TextLayout::toFloat(layout.verticalTextBounds.min),
                       TextLayout::toFloat(layout.horizontalTextBounds.max),
                       TextLayout::toFloat(layout.verticalTextBounds.max) };
}

} // namespace

static std::shared_ptr<const PreparedDocument::Impl> createPreparedDocument(
    const std::shared_ptr<Internal::TextLayoutState>& state, const TextWithOptions& text,
    const std::vector<FontAndColor>& sourceFonts, std::span<const uint32_t> sourceOffsets) {
    if (sourceFonts.empty()) {
        throwException(EArgument("At least one font is required to prepare a document"));
    }
    if (sourceOffsets.size() + 1 != sourceFonts.size()) {
        throwException(EArgument("The number of font offsets must be one less than the number of fonts"));
    }

    // TODO: Also validate finite/representable numeric Font and layout values
    // before converting them to fixed-point engine units.
    for (uint32_t offset : sourceOffsets) {
        if (offset == 0 || offset >= text.text.size()) {
            throwException(EArgument("Font offsets must be strictly inside the document"));
        }
    }
    if (!std::is_sorted(sourceOffsets.begin(), sourceOffsets.end()) ||
        std::adjacent_find(sourceOffsets.begin(), sourceOffsets.end()) != sourceOffsets.end()) {
        throwException(EArgument("Font offsets must be sorted and unique"));
    }

    auto document                = std::make_shared<PreparedDocument::Impl>();
    document->state              = state;
    document->options            = text.options;
    document->defaultTabInterval = TextLayout::kZero;
    document->renderStyles.reserve(sourceFonts.size());
    for (const FontAndColor& sourceFont : sourceFonts) {
        document->renderStyles.push_back({ sourceFont.color.value_or(Color{}),
                                           static_cast<uint8_t>(sourceFont.color.has_value()),
                                           sourceFont.font.textDecoration });
    }

    std::vector<ConvertedFont> converted(sourceFonts.size());
    convertFonts(sourceFonts, converted, state->database);
    std::vector<const TextLayout::FontDef*> definitions;
    definitions.reserve(converted.size());
    for (const ConvertedFont& font : converted) {
        definitions.push_back(&font.definition);
    }

    std::vector<uint32_t> boundaries;
    boundaries.reserve(sourceOffsets.size() + 2);
    boundaries.push_back(0);
    for (uint32_t offset : sourceOffsets) {
        boundaries.push_back(offset);
    }
    boundaries.push_back(static_cast<uint32_t>(text.text.size()));

    const std::u32string sourceText = text.text;
    const TextLayout::DocumentSource source(sourceText);
    const TextLayout::BaseDirection baseDirection = text.defaultDirection == TextDirection::RTL
                                                        ? TextLayout::BaseDirection::DefaultRTL
                                                        : TextLayout::BaseDirection::DefaultLTR;
    document->prepared                            = TextLayout::prepareDocument(
        source, state->database, definitions, TextLayout::BoundaryTable(boundaries),
        std::span<const TextLayout::BaseDirection>(&baseDirection, 1));
    if (!sourceFonts.empty()) {
        if (TextLayout::FontHandle handle = state->database->resolveFont(converted.front().definition)) {
            const TextLayout::ExtendedMetrics metrics =
                TextLayout::getExtendedMetrics(state->database.get(), handle);
            document->defaultTabInterval = TextLayout::fromFloat(TextLayout::toFloat(metrics.spaceAdvanceX) *
                                                                 sourceFonts.front().font.tabWidth);
        }
    }
    return document;
}

struct DocumentLayout::Impl {
    RectangleF bounds{};
    RectangleF trimmedBounds{};
    std::shared_ptr<const PreparedDocument::Impl> document;
    TextLayout::DocumentLayout layout;
};

PreparedDocument::PreparedDocument() : m_impl(std::make_shared<Impl>()) {}

PreparedDocument::~PreparedDocument()                                      = default;

PreparedDocument::PreparedDocument(const PreparedDocument&)                = default;

PreparedDocument& PreparedDocument::operator=(const PreparedDocument&)     = default;

PreparedDocument::PreparedDocument(PreparedDocument&&) noexcept            = default;

PreparedDocument& PreparedDocument::operator=(PreparedDocument&&) noexcept = default;

PreparedDocument::PreparedDocument(std::shared_ptr<const Impl> impl) : m_impl(std::move(impl)) {}

bool PreparedDocument::empty() const noexcept {
    return !m_impl || m_impl->prepared.paragraphs.empty();
}

uint32_t PreparedDocument::characterCount() const noexcept {
    return m_impl ? m_impl->prepared.graphemeMap.codepointCount() : 0;
}

uint32_t PreparedDocument::graphemeCount() const noexcept {
    return m_impl ? m_impl->prepared.graphemeMap.graphemeCount() : 0;
}

std::span<const uint32_t> PreparedDocument::graphemeBoundaries() const noexcept {
    return m_impl ? m_impl->prepared.graphemeMap.graphemeBoundaries() : std::span<const uint32_t>{};
}

size_t PreparedDocument::paragraphCount() const noexcept {
    return m_impl ? m_impl->prepared.paragraphs.size() : 0;
}

PreparedParagraph PreparedDocument::paragraph(size_t index) const noexcept {
    if (!m_impl || index >= paragraphCount()) {
        return {};
    }
    return convertParagraph(m_impl->prepared.paragraphs[index]);
}

uint32_t PreparedDocument::characterToGrapheme(uint32_t character) const noexcept {
    if (!m_impl) {
        return 0;
    }
    character = std::min(character, characterCount());
    return m_impl->prepared.graphemeMap.toGrapheme(character);
}

uint32_t PreparedDocument::graphemeToCharacter(uint32_t grapheme) const noexcept {
    if (!m_impl) {
        return 0;
    }
    grapheme = std::min(grapheme, graphemeCount());
    return m_impl->prepared.graphemeMap.toCodepoint(grapheme);
}

Range<uint32_t> PreparedDocument::graphemeToCharacters(uint32_t grapheme) const noexcept {
    if (!m_impl || graphemeCount() == 0) {
        return { 0, 0 };
    }
    grapheme = std::min(grapheme, graphemeCount() - 1);
    return m_impl->prepared.graphemeMap.codepointRangeForGrapheme(grapheme);
}

bool PreparedDocument::isParagraphSeparator(uint32_t grapheme) const noexcept {
    return m_impl && grapheme < graphemeCount() &&
           m_impl->prepared.graphemes[grapheme].isParagraphSeparator;
}

CaretIndex PreparedDocument::caretFromCharacter(uint32_t character, CaretAffinity affinity) const noexcept {
    return { characterToGrapheme(character), affinity };
}

uint32_t PreparedDocument::characterFromCaret(CaretIndex caret) const noexcept {
    return graphemeToCharacter(caret.grapheme);
}

DocumentLayout PreparedDocument::layout(const TextLayoutOptions& options) const {
    if (!m_impl) {
        return DocumentLayout{};
    }

    const auto& document                      = *m_impl;
    const TextLayout::TextAlignment alignment = toTextLayoutAlignment(options.alignment);
    const TextLayout::LayoutUnit maxWidth =
        m_impl->options && TextOptions::SingleLine
            ? TextLayout::kInfinity
            : (std::isfinite(options.maxLineWidth) ? TextLayout::fromFloat(options.maxLineWidth)
                                                   : TextLayout::kInfinity);
    const TextLayout::LayoutUnit indent = TextLayout::fromFloat(options.firstLineIndent);
    const TextLayout::LayoutUnit tabInterval =
        options.tabWidth > 0.f ? TextLayout::fromFloat(options.tabWidth) : document.defaultTabInterval;
    const TextLayout::TabStops tabStops{ TextLayout::kZero, tabInterval };
    const TextLayout::DocumentLayout engineLayout = TextLayout::layoutPreparedDocument(
        document.prepared, std::span<const TextLayout::TextAlignment>(&alignment, 1), maxWidth, tabStops,
        std::span<const TextLayout::LayoutUnit>(&indent, 1),
        options.allowBreakAnywhere || (m_impl->options && TextOptions::WrapAnywhere));

    auto result           = std::make_shared<DocumentLayout::Impl>();
    result->document      = m_impl;
    result->layout        = engineLayout;
    result->bounds        = convertBounds(engineLayout);
    result->trimmedBounds = RectangleF{ TextLayout::toFloat(engineLayout.horizontalTrimmedTextBounds.min),
                                        TextLayout::toFloat(engineLayout.verticalTextBounds.min),
                                        TextLayout::toFloat(engineLayout.horizontalTrimmedTextBounds.max),
                                        TextLayout::toFloat(engineLayout.verticalTextBounds.max) };
    return DocumentLayout(std::move(result));
}

DocumentLayout::DocumentLayout() : m_impl(std::make_shared<Impl>()) {}

DocumentLayout::~DocumentLayout()                                    = default;

DocumentLayout::DocumentLayout(const DocumentLayout&)                = default;

DocumentLayout& DocumentLayout::operator=(const DocumentLayout&)     = default;

DocumentLayout::DocumentLayout(DocumentLayout&&) noexcept            = default;

DocumentLayout& DocumentLayout::operator=(DocumentLayout&&) noexcept = default;

DocumentLayout::DocumentLayout(std::shared_ptr<const Impl> impl) : m_impl(std::move(impl)) {}

bool DocumentLayout::empty() const noexcept {
    return !m_impl || m_impl->layout.lines.empty();
}

size_t DocumentLayout::lineCount() const noexcept {
    return m_impl ? m_impl->layout.lines.size() : 0;
}

DocumentLine DocumentLayout::line(size_t index) const noexcept {
    if (!m_impl || index >= m_impl->layout.lines.size()) {
        return {};
    }
    return convertLine(m_impl->layout.lines[index]);
}

RectangleF DocumentLayout::bounds() const noexcept {
    return m_impl ? m_impl->bounds : RectangleF{};
}

RectangleF DocumentLayout::trimmedBounds() const noexcept {
    return m_impl ? m_impl->trimmedBounds : RectangleF{};
}

CaretPosition DocumentLayout::caretPosition(CaretIndex index) const noexcept {
    if (!m_impl || !m_impl->document) {
        return {};
    }
    const TextLayout::CaretPosition result =
        TextLayout::caretPosition(m_impl->document->prepared, m_impl->layout,
                                  { index.grapheme, index.affinity == CaretAffinity::Upstream
                                                        ? TextLayout::CaretAffinity::Upstream
                                                        : TextLayout::CaretAffinity::Downstream });
    return { TextLayout::toFloat(result.x), result.line, result.level };
}

size_t DocumentLayout::lineForCaret(CaretIndex index) const noexcept {
    if (!m_impl || m_impl->layout.lines.empty() || !m_impl->document) {
        return 0;
    }
    index.grapheme = std::min(index.grapheme, m_impl->document->prepared.graphemeMap.graphemeCount());
    const TextLayout::CaretPosition position =
        TextLayout::caretPosition(m_impl->document->prepared, m_impl->layout,
                                  { index.grapheme, index.affinity == CaretAffinity::Upstream
                                                        ? TextLayout::CaretAffinity::Upstream
                                                        : TextLayout::CaretAffinity::Downstream });
    return position.line;
}

size_t DocumentLayout::lineForCharacter(uint32_t character, CaretAffinity affinity) const noexcept {
    if (!m_impl || !m_impl->document) {
        return 0;
    }
    character = std::min(character, m_impl->document->prepared.graphemeMap.codepointCount());
    return lineForCaret({ m_impl->document->prepared.graphemeMap.toGrapheme(character), affinity });
}

CaretIndex DocumentLayout::lineBeginning(size_t line) const noexcept {
    if (!m_impl || line >= m_impl->layout.lines.size()) {
        return {};
    }
    const auto& value = m_impl->layout.lines[line];
    return { value.graphemeRange.min, CaretAffinity::Downstream };
}

CaretIndex DocumentLayout::lineEnd(size_t line) const noexcept {
    if (!m_impl || line >= m_impl->layout.lines.size()) {
        return {};
    }
    const auto& value = m_impl->layout.lines[line];
    return { value.graphemeRange.max, CaretAffinity::Upstream };
}

RectangleF DocumentLayout::caretRect(CaretIndex index, float width) const noexcept {
    if (!m_impl || m_impl->layout.lines.empty()) {
        return {};
    }
    const CaretPosition position = caretPosition(index);
    if (position.line >= m_impl->layout.lines.size()) {
        return {};
    }
    const DocumentLine value = line(position.line);
    const float halfLeading = value.leading * 0.5f;
    return RectangleF{ position.x, value.baseline - value.ascender - halfLeading, position.x + width,
                       value.baseline + value.descender + halfLeading };
}

CaretIndex DocumentLayout::hitTest(PointF point) const noexcept {
    if (!m_impl || !m_impl->document) {
        return {};
    }
    const TextLayout::CaretIndex result =
        TextLayout::hitTestPoint(m_impl->document->prepared, m_impl->layout, TextLayout::fromFloat(point.x),
                                 TextLayout::fromFloat(point.y));
    return { result.grapheme, result.affinity == TextLayout::CaretAffinity::Upstream
                                  ? CaretAffinity::Upstream
                                  : CaretAffinity::Downstream };
}

CaretIndex DocumentLayout::hitTestLine(size_t line, float x) const noexcept {
    if (!m_impl || line >= m_impl->layout.lines.size()) {
        return {};
    }
    const auto& value                   = m_impl->layout.lines[line];
    const TextLayout::CaretIndex result = TextLayout::hitTestPoint(m_impl->document->prepared, m_impl->layout,
                                                                   TextLayout::fromFloat(x), value.baselineY);
    return { result.grapheme, result.affinity == TextLayout::CaretAffinity::Upstream
                                  ? CaretAffinity::Upstream
                                  : CaretAffinity::Downstream };
}

CaretIndex DocumentLayout::moveCaretVertically(CaretIndex current, float preferredX,
                                               int direction) const noexcept {
    if (!m_impl || m_impl->layout.lines.empty() || direction == 0) {
        return current;
    }
    const size_t currentLine = lineForCaret(current);
    const size_t targetLine  = direction < 0 ? (currentLine == 0 ? 0 : currentLine - 1)
                                             : std::min(currentLine + 1, m_impl->layout.lines.size() - 1);
    return hitTestLine(targetLine, preferredX);
}

void DocumentLayout::selectionRects(Range<uint32_t> selection,
                                    function_ref<void(const TextSelectionRect&)> onRect) const {
    if (!m_impl || !m_impl->document) {
        return;
    }
    TextLayout::selectionRects(m_impl->document->prepared, m_impl->layout,
                               TextLayout::GraphemeRange{ selection.min, selection.max },
                               [&](const TextLayout::SelectionRect& rect) {
                                   onRect(TextSelectionRect{ rect.line, TextLayout::toFloat(rect.x0),
                                                             TextLayout::toFloat(rect.x1) });
                               });
}

void DocumentLayout::selectionRectsByCharacter(Range<uint32_t> selection,
                                               function_ref<void(const TextSelectionRect&)> onRect) const {
    if (!m_impl || !m_impl->document) {
        return;
    }
    const uint32_t count = m_impl->document->prepared.graphemeMap.codepointCount();
    selection.min        = std::min(selection.min, count);
    selection.max        = std::min(selection.max, count);
    if (selection.min > selection.max) {
        std::swap(selection.min, selection.max);
    }
    selectionRects({ m_impl->document->prepared.graphemeMap.toGrapheme(selection.min),
                     m_impl->document->prepared.graphemeMap.toGrapheme(selection.max) },
                   onRect);
}

void uncompressICUData();

static std::string_view freeTypeError(FT_Error err) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)                                                                                 \
    case e:                                                                                                  \
        return s;
#define FT_ERROR_START_LIST switch (err) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
    return "(unknown)";
}

[[noreturn]] static void handleFTErr(FT_Error err) {
    throwException(EFreeType("FreeType Error: {}", freeTypeError(err)));
}

static void handleFTErrSoft(FT_Error err) {
    BRISK_LOG_ERROR("FreeType Error: {}", freeTypeError(err));
}

#define HANDLE_FT_ERROR(expression)                                                                          \
    do {                                                                                                     \
        FT_Error error_ = expression;                                                                        \
        if (error_) {                                                                                        \
            handleFTErr(error_);                                                                             \
        }                                                                                                    \
    } while (0)

#define HANDLE_FT_ERROR_SOFT(expression, ...)                                                                \
    do {                                                                                                     \
        FT_Error error_ = expression;                                                                        \
        if (error_) {                                                                                        \
            handleFTErrSoft(error_);                                                                         \
            __VA_ARGS__;                                                                                     \
        }                                                                                                    \
    } while (0)

using namespace Internal;

namespace {

struct SvgGlyphState {
    std::unique_ptr<lunasvg::Document> doc;
    lunasvg::Box bbox;
};

FT_Error svg_port_init(FT_Pointer* state) {
    *state = new SvgGlyphState{};
    return FT_Err_Ok;
}

void svg_port_free(FT_Pointer* state) {
    delete reinterpret_cast<SvgGlyphState*>(*state);
}

FT_Error svg_port_render(FT_GlyphSlot slot, FT_Pointer* state) {
    lunasvg::Bitmap bmp(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch);

    bmp.clear(0x00000000u);

    auto& svgState = *reinterpret_cast<SvgGlyphState*>(*state);
    lunasvg::Matrix mat;
    mat.translate(-svgState.bbox.x, -svgState.bbox.y);
    svgState.doc->render(bmp, mat);

    uint8_t* pixels = slot->bitmap.buffer;
    for (int i = 0; i < slot->bitmap.rows; ++i) {
        for (int j = 0; j < slot->bitmap.width; ++j) {
            std::swap(pixels[j * 4 + 0], pixels[j * 4 + 2]);
        }
        pixels += slot->bitmap.pitch;
    }

    // Rc<Image> img           = rcnew Image(slot->bitmap.buffer, Size(slot->bitmap.width, slot->bitmap.rows),
    //   slot->bitmap.pitch, ImageFormat::RGBA);

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    slot->bitmap.num_grays  = 256;
    slot->format            = FT_GLYPH_FORMAT_BITMAP;

    return FT_Err_Ok;
}

static RectangleF viewBoxToRect(const std::string& str) {
    std::array<float, 4> values;
    int num       = 0;
    const char* p = str.c_str();
    char* end     = nullptr;
    for (float f = std::strtof(p, &end); p != end; f = std::strtof(p, &end)) {
        values[num++] = f;
        if (num == 4)
            break;
        p = end;
        while (*p && std::isspace(*p))
            ++p;
        if (*p && *p == ',')
            ++p;
    }
    return RectangleF(values[0], values[1], values[0] + values[2], values[1] + values[3]);
}

FT_Error svg_port_preset_slot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* state) {
    FT_SVG_Document document = (FT_SVG_Document)slot->other;
    FT_Size_Metrics metrics  = document->metrics;

    FT_UShort units_per_EM   = document->units_per_EM;

    std::string svg((const char*)document->svg_document, document->svg_document_length);

    auto& svgState                         = *reinterpret_cast<SvgGlyphState*>(*state);

    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromData(svg);

    auto root                              = doc->rootElement();
    std::string attr_viewBox               = root.getAttribute("viewBox");
    std::string attr_width                 = root.getAttribute("width");
    std::string attr_height                = root.getAttribute("height");

    Size dimensions;
    PointF offset{ 0, 0 };

    if (!attr_viewBox.empty()) {
        RectangleF vbox = viewBoxToRect(attr_viewBox);
        dimensions      = vbox.size();
        offset          = vbox.p1;
    } else if (!attr_width.empty() && !attr_height.empty()) {
        dimensions.width  = atoi(attr_width.c_str());
        dimensions.height = atoi(attr_height.c_str());

        if (dimensions == Size{ 1, 1 }) {
            dimensions.width  = units_per_EM;
            dimensions.height = units_per_EM;
        }
    } else {
        dimensions.width  = units_per_EM;
        dimensions.height = units_per_EM;
    }

    lunasvg::Matrix mat;
    SizeF svgScale = SizeF(metrics.x_ppem, metrics.y_ppem) / SizeF(dimensions);
    mat.scale(svgScale.x, svgScale.y);
    mat.transform(+(double)document->transform.xx / (1 << 16),                         //
                  -(double)document->transform.xy / (1 << 16),                         //
                  -(double)document->transform.yx / (1 << 16),                         //
                  +(double)document->transform.yy / (1 << 16),                         //
                  +(double)document->delta.x / 64 * dimensions.width / metrics.x_ppem, //
                  -(double)document->delta.y / 64 * dimensions.height / metrics.y_ppem //
    );
    mat.translate(-offset.x, -offset.y);
    doc->setMatrix(mat);

    auto box                = doc->box();
    slot->bitmap_left       = std::floor(box.x);
    slot->bitmap_top        = -std::floor(box.y);
    slot->bitmap.rows       = std::ceil(box.y + box.h) - -slot->bitmap_top;
    slot->bitmap.width      = std::ceil(box.x + box.w) - slot->bitmap_left;
    slot->bitmap.pitch      = slot->bitmap.width * 4;
    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    if (cache) {
        svgState.doc  = std::move(doc);
        svgState.bbox = box;
    }

    return FT_Err_Ok;
}

SVG_RendererHooks svgHooks = {
    (SVG_Lib_Init_Func)svg_port_init,
    (SVG_Lib_Free_Func)svg_port_free,
    (SVG_Lib_Render_Func)svg_port_render,
    (SVG_Lib_Preset_Slot_Func)svg_port_preset_slot,
};
} // namespace

static void* createFreeTypeLibrary() {
    FT_Library library = nullptr;
    HANDLE_FT_ERROR(FT_Init_FreeType(&library));
    return library;
}

FontManager::FontManager(std::recursive_mutex* mutex, int hscale)
    : m_lock(mutex), m_ft_library(createFreeTypeLibrary()),
      m_textLayout(std::make_shared<Internal::TextLayoutState>(
          std::make_shared<Internal::SharedLibraryOwner>(m_ft_library), hscale)),
      m_hscale(hscale) {

    FT_Module mod = FT_Get_Module(reinterpret_cast<FT_Library&>(m_ft_library), "ot-svg");
    if (!mod) {
        BRISK_LOG_ERROR("ot-svg module is not found");
    }

    HANDLE_FT_ERROR(
        FT_Property_Set(reinterpret_cast<FT_Library&>(m_ft_library), "ot-svg", "svg-hooks", &svgHooks));
}

FontManager::~FontManager() {
    m_textLayout.reset();
}

static void loadTextLayoutGlyphRun(
    const PreparedDocument::Impl& pdocument, uint32_t preparedRunIndex, GlyphCache* glyphCache,
    function_ref<void(uint32_t, const Internal::TextLayoutGlyphBitmap&)> onGlyph) {
    if (preparedRunIndex >= pdocument.prepared.glyphRuns.size()) {
        return;
    }

    const TextLayout::GlyphRun& run = pdocument.prepared.glyphRuns[preparedRunIndex];
    if (!run.fontHandle || !pdocument.state || !pdocument.state->database) {
        return;
    }
    const TextLayout::RasterizationOptions options{
        .horizontalScale = pdocument.state->hscale,
        .enableColor     = true,
    };
    TextLayout::ActiveFont activeFont = pdocument.state->database->activate(run.fontHandle);
    if (activeFont.ftFace == nullptr) {
        return;
    }
    FT_Face face                     = static_cast<FT_Face>(activeFont.ftFace);
    const bool colorFont             = options.enableColor && (FT_HAS_SVG(face) || FT_HAS_COLOR(face));
    const GlyphRenderMode renderMode = colorFont ? GlyphRenderMode::Color : GlyphRenderMode::Mask;
    const int effectiveScale         = colorFont ? 1 : options.horizontalScale;
    for (uint32_t preparedGlyphIndex = run.glyphRange.min; preparedGlyphIndex < run.glyphRange.max;
         ++preparedGlyphIndex) {
        const uint32_t glyphId = pdocument.prepared.glyphs[preparedGlyphIndex].glyphId;

        auto createGlyph       = [&]() -> std::optional<CachedGlyph> {
            std::optional<CachedGlyph> result;
            std::ignore = TextLayout::rasterize(
                activeFont, run.fontHandle, glyphId, options,
                [&](const TextLayout::RasterizedGlyph& glyph, const uint8_t* pixels) {
                    const uint32_t components = glyph.bytesPerPixel;
                    const Size spriteSize{ static_cast<int32_t>(glyph.width * components),
                                           static_cast<int32_t>(glyph.height) };
                    Rc<SpriteResource> sprite = makeSprite(spriteSize);
                    const size_t rowBytes     = static_cast<size_t>(glyph.width) * components;
                    const int pitch           = glyph.pitch;
                    const uint8_t* firstRow   = pixels;
                    if (pitch < 0 && glyph.height > 0) {
                        firstRow += static_cast<size_t>(-pitch) * (glyph.height - 1);
                    }
                    for (uint32_t row = 0; row < glyph.height; ++row) {
                        const uint8_t* source = firstRow + static_cast<ptrdiff_t>(row) * pitch;
                        std::memcpy(sprite->data() + row * rowBytes, source, rowBytes);
                    }
                    result = CachedGlyph{
                        .size            = spriteSize,
                        .logicalSize     = { static_cast<int32_t>(glyph.width / glyph.horizontalScale),
                                             static_cast<int32_t>(glyph.height) },
                        .sprite          = std::move(sprite),
                        .offsetX         = static_cast<float>(glyph.left) / glyph.horizontalScale,
                        .offsetY         = glyph.top,
                        .renderMode      = glyph.format == TextLayout::RasterizedGlyph::Format::BGRA8
                                               ? GlyphRenderMode::Color
                                               : GlyphRenderMode::Mask,
                        .horizontalScale = glyph.horizontalScale,
                    };
                });
            return result;
        };

        const ::Brisk::GlyphCacheKey key{ activeFont.instanceId, glyphId,
                                          static_cast<uint16_t>(effectiveScale), renderMode };
        std::optional<CachedGlyph> cached =
            glyphCache ? glyphCache->getOrCreate(key, createGlyph) : createGlyph();
        if (cached) {
            onGlyph(preparedGlyphIndex, Internal::TextLayoutGlyphBitmap{
                                            .size            = cached->size,
                                            .logicalSize     = cached->logicalSize,
                                            .sprite          = cached->sprite,
                                            .offsetX         = cached->offsetX,
                                            .offsetY         = cached->offsetY,
                                            .color           = cached->renderMode == GlyphRenderMode::Color,
                                            .horizontalScale = cached->horizontalScale,
                                        });
        }
    }
}

void Internal::loadTextLayoutGlyphRun(
    const PreparedDocument& document, uint32_t preparedRunIndex, GlyphCache* glyphCache,
    function_ref<void(uint32_t, const Internal::TextLayoutGlyphBitmap&)> onGlyph) {
    const PreparedDocument::Impl* pdocument = PimplAccessor::getImpl(document);
    if (!pdocument) {
        return;
    }
    loadTextLayoutGlyphRun(*pdocument, preparedRunIndex, glyphCache, onGlyph);
}

void Internal::forEachTextLayoutGlyph(const DocumentLayout& layout, PointF origin,
                                      function_ref<void(const Internal::TextLayoutGlyph&)> onGlyph) {
    const DocumentLayout::Impl* playout = PimplAccessor::getImpl(layout);
    if (!playout || !playout->document || !playout->document->state) {
        return;
    }

    const PreparedDocument::Impl& document = *playout->document;
    for (const TextLayout::LayoutGlyphRun& layoutRun : playout->layout.glyphRuns) {
        if (layoutRun.glyphRange.empty() ||
            layoutRun.preparedRunIndex >= document.prepared.glyphRuns.size()) {
            continue;
        }
        const TextLayout::GlyphRun& preparedRun    = document.prepared.glyphRuns[layoutRun.preparedRunIndex];
        const Internal::PreparedRenderStyle* style = preparedRun.fontRunIndex < document.renderStyles.size()
                                                         ? &document.renderStyles[preparedRun.fontRunIndex]
                                                         : nullptr;
        loadTextLayoutGlyphRun(
            *(playout->document), layoutRun.preparedRunIndex, document.state->glyphCache.get(),
            [&](uint32_t glyphIndex, const Internal::TextLayoutGlyphBitmap& bitmap) {
                if (glyphIndex < layoutRun.glyphRange.min || glyphIndex >= layoutRun.glyphRange.max ||
                    glyphIndex >= document.prepared.glyphs.size()) {
                    return;
                }
                const TextLayout::Glyph& glyph = document.prepared.glyphs[glyphIndex];
                onGlyph(Internal::TextLayoutGlyph{
                    .position = origin + PointF{ TextLayout::toFloat(layoutRun.xOffset + glyph.xOffset),
                                                 TextLayout::toFloat(layoutRun.yOffset + glyph.yOffset) },
                    .bitmap   = bitmap,
                    .color = style && style->hasColor ? std::optional<Color>{ style->color } : std::nullopt,
                });
            });
    }
}

void Internal::forEachTextLayoutDecoration(
    const DocumentLayout& layout, PointF origin,
    function_ref<void(const Internal::TextLayoutDecoration&)> onDecoration) {
    const DocumentLayout::Impl* playout = PimplAccessor::getImpl(layout);
    if (!playout || !playout->document || !playout->document->state) {
        return;
    }

    const PreparedDocument::Impl& document = *playout->document;
    for (const TextLayout::LayoutGlyphRun& layoutRun : playout->layout.glyphRuns) {
        if (layoutRun.glyphRange.empty() ||
            layoutRun.preparedRunIndex >= document.prepared.glyphRuns.size()) {
            continue;
        }
        const TextLayout::GlyphRun& preparedRun = document.prepared.glyphRuns[layoutRun.preparedRunIndex];
        if (preparedRun.fontRunIndex >= document.renderStyles.size()) {
            continue;
        }
        const Internal::PreparedRenderStyle& style = document.renderStyles[preparedRun.fontRunIndex];
        if (style.decoration == TextDecoration::None) {
            continue;
        }

        float left  = std::numeric_limits<float>::infinity();
        float right = -std::numeric_limits<float>::infinity();
        for (uint32_t glyphIndex = layoutRun.glyphRange.min; glyphIndex < layoutRun.glyphRange.max;
             ++glyphIndex) {
            if (glyphIndex >= document.prepared.glyphs.size()) {
                break;
            }
            const TextLayout::Glyph& glyph = document.prepared.glyphs[glyphIndex];
            const float glyphLeft          = TextLayout::toFloat(layoutRun.xOffset + glyph.xOffset);
            const float glyphRight = TextLayout::toFloat(layoutRun.xOffset + glyph.xOffset + glyph.xAdvance);
            left                   = std::min(left, std::min(glyphLeft, glyphRight));
            right                  = std::max(right, std::max(glyphLeft, glyphRight));
        }
        if (!std::isfinite(left) || !std::isfinite(right) || right <= left) {
            continue;
        }

        const TextLayout::ExtendedMetrics metrics =
            TextLayout::getExtendedMetrics(document.state->database.get(), preparedRun.fontHandle);
        const float ascent            = TextLayout::toFloat(preparedRun.metrics.ascent);
        const float descent           = TextLayout::toFloat(preparedRun.metrics.descent);
        const float underlineOffset   = metrics.underlinePosition != TextLayout::kZero
                                            ? TextLayout::toFloat(metrics.underlinePosition)
                                            : descent * 0.5f;
        const float overlineOffset    = -ascent * 0.84375f;
        const float lineThroughOffset = (underlineOffset + overlineOffset) * 0.5f;
        const float thickness         = metrics.lineThickness != TextLayout::kZero
                                            ? TextLayout::toFloat(metrics.lineThickness)
                                            : std::max(1.f, descent * 0.125f);
        onDecoration(Internal::TextLayoutDecoration{
            .start             = origin + PointF{ left, TextLayout::toFloat(layoutRun.yOffset) },
            .end               = origin + PointF{ right, TextLayout::toFloat(layoutRun.yOffset) },
            .underlineOffset   = underlineOffset,
            .overlineOffset    = overlineOffset,
            .lineThroughOffset = lineThroughOffset,
            .thickness         = thickness,
            .decoration        = style.decoration,
            .color             = style.hasColor ? std::optional<Color>{ style.color } : std::nullopt,
        });
    }
}

void Internal::textLayoutSelectionRects(const DocumentLayout& layout, Range<uint32_t> characterSelection,
                                        function_ref<void(const ::Brisk::TextSelectionRect&)> onRect) {
    const DocumentLayout::Impl* playout = PimplAccessor::getImpl(layout);
    if (!playout || !playout->document) {
        return;
    }
    const PreparedDocument::Impl& document = *playout->document;
    const uint32_t characterCount          = document.prepared.graphemeMap.codepointCount();
    characterSelection.min                 = std::min(characterSelection.min, characterCount);
    characterSelection.max                 = std::min(characterSelection.max, characterCount);
    if (characterSelection.min >= characterSelection.max) {
        return;
    }
    const TextLayout::GraphemeRange selection{
        document.prepared.graphemeMap.toGrapheme(characterSelection.min),
        document.prepared.graphemeMap.toGrapheme(characterSelection.max),
    };
    TextLayout::selectionRects(document.prepared, playout->layout, selection,
                               [&](const TextLayout::SelectionRect& rect) {
                                   onRect(::Brisk::TextSelectionRect{ rect.line, TextLayout::toFloat(rect.x0),
                                                                      TextLayout::toFloat(rect.x1) });
                               });
}

template <typename Pixels>
static void renderGlyphs(Pixels& pixels, Rc<Image> image, Point origin, const PreparedDocument& document,
                         const DocumentLayout& layout, bool rgba) {
    const PreparedDocument::Impl* pdocument = PimplAccessor::getImpl(document);
    const DocumentLayout::Impl* playout     = PimplAccessor::getImpl(layout);
    using PixelType                         = std::remove_cvref_t<decltype(pixels(0, 0))>;
    for (const TextLayout::LayoutGlyphRun& layoutRun : playout->layout.glyphRuns) {
        if (layoutRun.glyphRange.empty() ||
            layoutRun.preparedRunIndex >= pdocument->prepared.glyphRuns.size()) {
            continue;
        }

        // Rasterize and blit the whole glyph run at once; the font is activated
        // a single time per run inside loadTextLayoutGlyphRun.
        Internal::loadTextLayoutGlyphRun(
            document, layoutRun.preparedRunIndex, pdocument->state->glyphCache.get(),
            [&](uint32_t glyphIndex, const Internal::TextLayoutGlyphBitmap& bitmap) {
                if (glyphIndex < layoutRun.glyphRange.min || glyphIndex >= layoutRun.glyphRange.max ||
                    !bitmap.sprite || bitmap.color != rgba) {
                    return;
                }
                const TextLayout::Glyph& glyph = pdocument->prepared.glyphs[glyphIndex];
                const PointF glyphOrigin{ TextLayout::toFloat(layoutRun.xOffset + glyph.xOffset) + origin.x,
                                          TextLayout::toFloat(layoutRun.yOffset + glyph.yOffset) + origin.y };
                const PointF topLeft      = glyphOrigin + PointF{ bitmap.offsetX, -float(bitmap.offsetY) };
                const int x0              = static_cast<int>(std::floor(topLeft.x));
                const int y0              = static_cast<int>(std::floor(topLeft.y));
                const auto source         = bitmap.sprite->bytes();
                const int sourceWidth     = bitmap.sprite->size.width / (bitmap.color ? 4 : 1);
                const size_t sourceStride = static_cast<size_t>(bitmap.sprite->size.width);
                for (int y = 0; y < bitmap.logicalSize.height; ++y) {
                    const int dstY = y0 + y;
                    if (dstY < 0 || dstY >= image->height()) {
                        continue;
                    }
                    const uint8_t* src = reinterpret_cast<const uint8_t*>(source.data()) + y * sourceStride;
                    for (int x = 0; x < sourceWidth; ++x) {
                        const int dstX = x0 + x;
                        if (dstX < 0 || dstX >= image->width()) {
                            continue;
                        }
                        if constexpr (std::is_same_v<PixelType, PixelGreyscale8>) {
                            const uint8_t srcValue  = src[x];
                            const uint32_t inverse  = 255u - srcValue;
                            pixels(dstX, dstY).grey = static_cast<uint8_t>(
                                srcValue + (pixels(dstX, dstY).grey * inverse + 127u) / 255u);
                        } else if constexpr (std::is_same_v<PixelType, PixelRGBA8>) {
                            const uint8_t alpha    = src[x * 4 + 3];
                            auto& destination      = pixels(dstX, dstY);
                            const uint32_t inverse = 255u - alpha;
                            destination.r = static_cast<uint8_t>(src[x * 4 + 0] +
                                                                 (destination.r * inverse + 127u) / 255u);
                            destination.g = static_cast<uint8_t>(src[x * 4 + 1] +
                                                                 (destination.g * inverse + 127u) / 255u);
                            destination.b = static_cast<uint8_t>(src[x * 4 + 2] +
                                                                 (destination.b * inverse + 127u) / 255u);
                            destination.a =
                                static_cast<uint8_t>(alpha + (destination.a * inverse + 127u) / 255u);
                        }
                    }
                }
            });
    }
}

void Internal::renderPreparedDocument(Rc<Image> image, Point origin, const PreparedDocument& document,
                                      const DocumentLayout& layout) {
    const PreparedDocument::Impl* pdocument = PimplAccessor::getImpl(document);
    const DocumentLayout::Impl* playout     = PimplAccessor::getImpl(layout);
    if (!image || !pdocument || !playout || playout->document.get() != pdocument) {
        return;
    }
    const bool rgba = image->format() == ImageFormat::RGBA_U8Gamma;
    if (rgba) {
        auto pixels = image->mapWrite<ImageFormat::RGBA_U8Gamma>();
        renderGlyphs(pixels, image, origin, document, layout, rgba);
    } else {
        auto pixels = image->mapWrite<ImageFormat::Greyscale_U8Gamma>();
        renderGlyphs(pixels, image, origin, document, layout, rgba);
    }
}

void Internal::registerTextLayoutFont(FontManager& manager, BytesView data, std::string_view alias) {
    lock_quard_cond lk(manager.m_lock);
    if (!manager.m_textLayout || data.empty()) {
        return;
    }

    auto bytes = std::make_shared<const Bytes>(data.begin(), data.end());
    manager.m_textLayout->fontBlobs.push_back(bytes);
    const std::vector<std::string> families = manager.m_textLayout->database->registerFont(*bytes);
    if (!alias.empty()) {
        for (const std::string& family : families) {
            if (alias != family) {
                std::ignore = manager.m_textLayout->database->addAlias(family, alias);
            }
        }
    }
}

PreparedDocument FontManager::prepareDocument(const Font& font, const TextWithOptions& text) const {
    const FontAndColor fontAndColor{ font, std::nullopt };
    if (!text.richText.empty()) {
        RichText richText = text.richText;
        richText.setBaseFont(font);
        return prepareDocument(text, richText.fonts, richText.offsets);
    }
    return prepareDocument(text, std::span<const FontAndColor>(&fontAndColor, 1), {});
}

PreparedDocument FontManager::prepareDocument(const TextWithOptions& text,
                                              std::span<const FontAndColor> sourceFonts,
                                              std::span<const uint32_t> offsets) const {
    if (sourceFonts.empty()) {
        throwException(EArgument("At least one font is required to prepare a document"));
    }

    lock_quard_cond lk(m_lock);
    std::vector<FontAndColor> fontsCopy(sourceFonts.begin(), sourceFonts.end());
    std::vector<uint32_t> offsetsCopy(offsets.begin(), offsets.end());
    auto result = createPreparedDocument(m_textLayout, text, fontsCopy, offsetsCopy);
    return PreparedDocument(std::move(result));
}

std::vector<std::string_view> FontManager::fontList(std::string_view ff) const {
    std::vector<std::string_view> list = split(ff, ',');
    for (std::string_view& sv : list) {
        sv = trim(sv);
    }
    return list;
}

void FontManager::addFont(BytesView data, std::string alias) {
    addFontImpl(data, std::move(alias), true);
}

bool FontManager::addFontFromResource(std::string resourceName, std::string alias, bool emptyOk) {
    const Bytes& data = Resources::loadCached(std::move(resourceName), emptyOk);
    if (data.empty()) {
        return false;
    }
    addFontImpl(data, std::move(alias), false);
    return true;
}

void FontManager::addFontImpl(BytesView data, std::string alias, bool makeCopy) {
    lock_quard_cond lk(m_lock);
    if (data.empty()) {
        return;
    }

    if (makeCopy) {
        m_textLayout->fontBlobs.push_back(std::make_shared<const Bytes>(data.begin(), data.end()));
        data = *m_textLayout->fontBlobs.back();
    }

    const std::vector<std::string> registeredFamilies = m_textLayout->database->registerFont(data);
    for (const std::string& family : registeredFamilies) {
        if (!alias.empty() && alias != family) {
            std::ignore = m_textLayout->database->addAlias(family, alias);
        }
    }
}

status<IoError> FontManager::addFontFromFile(const fs::path& path, std::string alias) {
    lock_quard_cond lk(m_lock);
    expected<Bytes, IoError> b = readBytes(path);
    if (b) {
        addFont(*b, std::move(alias));
        return {};
    }
    return unexpected(b.error());
}

static bool cmpi(std::string_view a, std::string_view b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

static bool isFontExt(std::string_view ext) {
    return cmpi(ext, ".ttf") || cmpi(ext, ".otf");
}

static std::optional<OsFont> fontQuickInfo(FT_Library library, const fs::path& path) {
    FT_Face face;
    FT_Error err = FT_New_Face(library, path.string().c_str(), 0, &face);
    if (err)
        return std::nullopt;
    SCOPE_EXIT {
        FT_Done_Face(face);
    };

    OsFont font;
    font.path   = path;
    font.weight = FontWeight::Regular;
    font.style  = FontStyle::Normal;
    if (face->family_name == nullptr)
        return std::nullopt;
    font.family = face->family_name;
    if (face->style_name != nullptr) {
        std::string_view styleName = face->style_name;
        std::vector<std::string_view> extraStyles;
        for (std::string_view s : split(styleName, ' ')) {
            if (cmpi(s, "Thin") || cmpi(s, "UltraLight")) {
                font.weight = FontWeight::Thin;
            } else if (cmpi(s, "ExtraLight")) {
                font.weight = FontWeight::ExtraLight;
            } else if (cmpi(s, "Light") || cmpi(s, "SemiLight")) {
                font.weight = FontWeight::Light;
            } else if (cmpi(s, "Regular") || cmpi(s, "Normal") || cmpi(s, "Book")) {
                font.weight = FontWeight::Regular;
            } else if (cmpi(s, "Medium") || cmpi(s, "Roman")) {
                font.weight = FontWeight::Medium;
            } else if (cmpi(s, "SemiBold") || cmpi(s, "DemiBold") || cmpi(s, "Demi")) {
                font.weight = FontWeight::SemiBold;
            } else if (cmpi(s, "Bold")) {
                font.weight = FontWeight::Bold;
            } else if (cmpi(s, "ExtraBold") || cmpi(s, "Heavy")) {
                font.weight = FontWeight::ExtraBold;
            } else if (cmpi(s, "Black")) {
                font.weight = FontWeight::Black;
            } else if (cmpi(s, "Italic") || cmpi(s, "Oblique")) {
                font.style = FontStyle::Italic;
            } else {
                if (!s.empty()) {
                    extraStyles.push_back(s);
                }
            }
        }
        font.styleName = join(extraStyles, ' ');
    }

    return font;
}

std::vector<OsFont> FontManager::installedFonts(bool rescan) const {
    lock_quard_cond lk(m_lock);
    if (m_osFonts.empty() || rescan) {
        m_osFonts.clear();
        for (fs::path path : fontFolders()) {
            for (auto f : fs::directory_iterator(path)) {
                if (f.is_regular_file() && isFontExt(f.path().extension().string())) {
                    if (std::optional<OsFont> fontInfo =
                            fontQuickInfo(static_cast<FT_Library>(m_ft_library), f.path())) {
                        m_osFonts.push_back(std::move(*fontInfo));
                    }
                }
            }
        }
    }
    return m_osFonts;
}

bool FontManager::addSystemFont(std::string alias) {
    lock_quard_cond lk(m_lock);
    fs::path path = fontFolders().front();
#ifdef BRISK_WINDOWS
    return addFontFromFile(path / "segoeui.ttf", alias) && addFontFromFile(path / "segoeuii.ttf", alias) &&
           addFontFromFile(path / "segoeuib.ttf", alias) && addFontFromFile(path / "segoeuiz.ttf", alias);
#elif defined BRISK_MACOS
    return addFontFromFile(path / "SFNS.ttf", alias) && addFontFromFile(path / "SFNSItalic.ttf", alias);
#else
    return false;
#endif
}

bool FontManager::addFontByName(std::string_view fontName, std::string alias) {
    lock_quard_cond lk(m_lock);
    std::ignore = installedFonts();
    int num     = 0;
    for (const auto& f : m_osFonts) {
        if (f.family == fontName && f.styleName.empty()) {
            if (!addFontFromFile(f.path, alias))
                return false;
            ++num;
        }
    }
    return num > 0;
}

FontMetrics FontManager::metrics(const Font& font) const {
    lock_quard_cond lk(m_lock);
    return getMetrics(font);
}

FontMetrics FontManager::getMetrics(const Font& font) const {
    ConvertedFont cvtFont = convertFont(font, m_textLayout->database);
    auto fontHandle       = m_textLayout->database->resolveFont(cvtFont.definition);
    TextLayout::VerticalMetrics metrics =
        TextLayout::getVerticalMetrics(m_textLayout->database.get(), fontHandle);
    TextLayout::ExtendedMetrics extendedMetrics =
        TextLayout::getExtendedMetrics(m_textLayout->database.get(), fontHandle);
    return FontMetrics{
        .size          = font.fontSize,
        .ascender      = TextLayout::toFloat(metrics.ascent),
        .descender     = -TextLayout::toFloat(metrics.descent),
        .height        = TextLayout::toFloat(metrics.height()),
        .spaceAdvanceX = TextLayout::toFloat(extendedMetrics.spaceAdvanceX),
        .lineThickness = TextLayout::toFloat(extendedMetrics.lineThickness),
        .xHeight       = TextLayout::toFloat(extendedMetrics.xHeight),
        .capitalHeight = TextLayout::toFloat(extendedMetrics.capitalHeight),
    };
}

float FontMetrics::linegap() const noexcept {
    return height - ascender + descender;
}

float FontMetrics::underlineOffset() const noexcept {
    return -descender * 0.5f;
}

float FontMetrics::overlineOffset() const noexcept {
    return -ascender * 0.84375f;
}

float FontMetrics::lineThroughOffset() const noexcept {
    return (underlineOffset() + overlineOffset()) * 0.5f;
}

float FontMetrics::vertBounds() const noexcept {
    return -descender + ascender;
}

TextWithOptions::TextWithOptions(std::string_view text, TextOptions options, TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(text);
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = utf8ToUtf32(text);
    }
}

TextWithOptions::TextWithOptions(std::u16string_view text, TextOptions options,
                                 TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf16ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = utf16ToUtf32(text);
    }
}

TextWithOptions::TextWithOptions(std::u32string_view text, TextOptions options,
                                 TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf32ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = text;
    }
}

TextWithOptions::TextWithOptions(std::u32string text, TextOptions options, TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf32ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = std::move(text);
    }
}

namespace Internal {

static Font overrideFont(const Font& base, Font&& font, FontFormatFlags flags) {
    font.letterSpacing = base.letterSpacing;
    font.wordSpacing   = base.wordSpacing;
    font.verticalAlign = base.verticalAlign;
    font.tabWidth      = base.tabWidth;
    font.lineHeight    = base.lineHeight;
    if (!(flags && FontFormatFlags::Family))
        font.fontFamily = base.fontFamily;
    if (!(flags && FontFormatFlags::Size))
        font.fontSize = base.fontSize;
    else if (flags && FontFormatFlags::SizeIsRelative) {
        font.fontSize = base.fontSize * font.fontSize;
    }
    if (!(flags && FontFormatFlags::Weight))
        font.weight = base.weight;
    if (!(flags && FontFormatFlags::Style))
        font.style = base.style;
    if (!(flags && FontFormatFlags::TextDecoration))
        font.textDecoration = base.textDecoration;
    return font;
}

void RichText::setBaseFont(const Font& font) {
    BRISK_ASSERT(fonts.size() == flags.size());
    for (size_t i = 0; i < fonts.size(); ++i) {
        fonts[i].font = overrideFont(font, std::move(fonts[i].font), flags[i]);
    }
    flags.clear();
}

struct FontFormatEx : FontAndColor {
    FontFormatFlags flags                      = FontFormatFlags::None;

    bool operator==(const FontFormatEx&) const = default;
};

struct Visitor final : public HtmlSax {
    std::u32string text;
    RichText richText;
    std::vector<FontFormatEx> fontStack{
        FontFormatEx{},
    };
    std::string_view tag;
    std::string_view attr;
    std::string attrValue;

    void closeDocument() {
        if (!richText.offsets.empty())
            richText.offsets.pop_back();
    }

    void openTag(std::string_view tagName) {
        tag = tagName;
        fontStack.push_back(fontStack.back());

        if (tagName == "b"sv || tagName == "strong"sv) {
            fontStack.back().font.weight = FontWeight::Bold;
            fontStack.back().flags |= FontFormatFlags::Weight;
        } else if (tagName == "i"sv || tagName == "em"sv) {
            fontStack.back().font.style = FontStyle::Italic;
            fontStack.back().flags |= FontFormatFlags::Style;
        } else if (tagName == "big"sv) {
            if (fontStack.back().flags && FontFormatFlags::Size) {
                fontStack.back().font.fontSize *= 2.f;
            } else {
                fontStack.back().font.fontSize = 2.f;
            }
            fontStack.back().flags |= FontFormatFlags::Size;
            fontStack.back().flags |= FontFormatFlags::SizeIsRelative;
        } else if (tagName == "small"sv) {
            if (fontStack.back().flags && FontFormatFlags::Size) {
                fontStack.back().font.fontSize *= 0.5f;
            } else {
                fontStack.back().font.fontSize = 0.5f;
            }
            fontStack.back().flags |= FontFormatFlags::Size;
            fontStack.back().flags |= FontFormatFlags::SizeIsRelative;
        } else if (tagName == "s"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::LineThrough;
            fontStack.back().flags |= FontFormatFlags::TextDecoration;
        } else if (tagName == "u"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::Underline;
            fontStack.back().flags |= FontFormatFlags::TextDecoration;
        } else if (tagName == "br"sv) {
            emitText(U"\n");
        } else if (tagName == "code"sv || tagName == "kbd"sv) {
            fontStack.back().font.fontFamily = Font::Monospace;
            fontStack.back().flags |= FontFormatFlags::Family;
        }
    }

    void closeTag() {
        fontStack.pop_back();
    }

    void attrName(std::string_view name) {
        attr = name;
    }

    void attrValueFragment(std::string_view value) {
        attrValue += value;
    }

    void attrFinished() {
        if (tag == "font" && attr == "color") {
            fontStack.back().color = parseHtmlColor(attrValue);
        }
        if (tag == "font" && attr == "face") {
            fontStack.back().font.fontFamily = attrValue;
            fontStack.back().flags |= FontFormatFlags::Family;
        }
        if (tag == "font" && attr == "size") {
            float val = strtof(attrValue.c_str(), nullptr);
            if (val != 0)
                fontStack.back().font.fontSize = val;
        }
        attrValue = {};
    }

    void emitText(std::u32string_view str) {
        FontFormatEx newFont = fontStack.back();
        text += str;
        // OPTIMIZE: Avoid comparison
        if (richText.fonts.empty() || newFont != richText.fonts.back()) {
            richText.flags.push_back(newFont.flags);
            richText.fonts.push_back(std::move(newFont));
            richText.offsets.push_back(text.size());
        } else if (!richText.offsets.empty()) {
            richText.offsets.back() = text.size();
        }
    }

    void textFragment(std::string_view text) {
        emitText(utf8ToUtf32(text));
    }
};

std::optional<std::pair<std::u32string, RichText>> RichText::fromHtml(std::string_view html) {
    Visitor visitor;
    if (parseHtml(html, &visitor)) {
        return std::pair{ std::move(visitor.text), std::move(visitor.richText) };
    } else {
        return std::nullopt;
    }
}

} // namespace Internal


Font Font::operator()(FontWeight weight) const {
    Font result   = *this;
    result.weight = weight;
    return result;
}

Font Font::operator()(FontStyle style) const {
    Font result  = *this;
    result.style = style;
    return result;
}

Font Font::operator()(float fontSize) const {
    Font result     = *this;
    result.fontSize = fontSize;
    return result;
}

Font Font::operator()(std::string fontFamily) const {
    Font result       = *this;
    result.fontFamily = std::move(fontFamily);
    return result;
}

std::recursive_mutex fontMutex;

std::optional<FontManager> fonts(std::in_place, &fontMutex, 3);

} // namespace Brisk
