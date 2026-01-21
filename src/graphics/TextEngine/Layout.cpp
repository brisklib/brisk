#include <text_layout/Layout.hpp>

#include <brisk/core/internal/InlineVector.hpp>

#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H

#include <utf8proc.h>

#include <linebreak.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <vector>

namespace Brisk::TextLayout {

namespace {
// Monotonic 64-bit cookie identifying each PreparedDocument, copied into DocumentLayout.
uint64_t nextCookie() {
    static std::atomic<uint64_t> counter{ 0 };
    const uint64_t sequence    = counter.fetch_add(1, std::memory_order_relaxed);
    // Mix in a per-process base from the high-resolution clock so cookies are not trivially
    // predictable across processes.
    static const uint64_t base = [] {
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<uint64_t>(now.count());
    }();
    return base ^ (sequence * 0x9E3779B97F4A7C15ull);
}
} // namespace

bool shouldSplitRunAt(char32_t cp) {
    // Defensive: invalid/surrogate codepoints never get shaped.
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return true;
    }

    utf8proc_category_t cat = utf8proc_category(static_cast<utf8proc_int32_t>(cp));

    switch (cat) {
    case UTF8PROC_CATEGORY_CC: // control chars: TAB, VT, FF, etc.
    case UTF8PROC_CATEGORY_ZL: // U+2028 LINE SEPARATOR
    case UTF8PROC_CATEGORY_ZP: // U+2029 (shouldn't appear post-paragraph-split, but be safe)
        return true;
    case UTF8PROC_CATEGORY_CF: // format controls: ZWJ, ZWNJ, LRM,
                               // RLM, embedding controls, SHY, etc.
    default:
        return false; // keep in the HarfBuzz buffer
    }
}

namespace {

ParagraphSeparatorKind paragraphSeparatorKind(std::u32string_view text, CodepointIndex begin,
                                              CodepointIndex end) {
    if (end == begin) {
        return ParagraphSeparatorKind::None;
    }

    switch (text[end - 1]) {
    case U'\n':
        if (end - begin >= 2 && text[end - 2] == U'\r') {
            return ParagraphSeparatorKind::CarriageReturnLineFeed;
        } else {
            return ParagraphSeparatorKind::LineFeed;
        }
    case U'\r':
        return ParagraphSeparatorKind::CarriageReturn;
    case U'\u001C':
        return ParagraphSeparatorKind::InformationSeparator4;
    case U'\u001D':
        return ParagraphSeparatorKind::InformationSeparator3;
    case U'\u001E':
        return ParagraphSeparatorKind::InformationSeparator2;
    case U'\u0085':
        return ParagraphSeparatorKind::NextLine;
    case U'\u2029':
        return ParagraphSeparatorKind::ParagraphSeparator;
    default:
        return ParagraphSeparatorKind::None;
    }
}

constexpr std::array<char32_t, 34> pairedCharacters{
    U'(',      U')',      U'<',      U'>',      U'[',      U']',      U'{',      U'}',      U'\u00AB',
    U'\u00BB', U'\u2018', U'\u2019', U'\u201C', U'\u201D', U'\u2039', U'\u203A', U'\u3008', U'\u3009',
    U'\u300A', U'\u300B', U'\u300C', U'\u300D', U'\u300E', U'\u300F', U'\u3010', U'\u3011', U'\u3014',
    U'\u3015', U'\u3016', U'\u3017', U'\u3018', U'\u3019', U'\u301A', U'\u301B',
};

int pairedCharacterIndex(char32_t codepoint) {
    const auto it = std::ranges::lower_bound(pairedCharacters, codepoint);
    return it != pairedCharacters.end() && *it == codepoint ? static_cast<int>(it - pairedCharacters.begin())
                                                            : -1;
}

bool isExplicitScript(hb_script_t script) {
    return script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED && script != HB_SCRIPT_UNKNOWN &&
           script != HB_SCRIPT_INVALID;
}

hb_script_t graphemeScript(std::u32string_view text, CodepointRange range,
                           hb_unicode_funcs_t* unicodeFunctions) {
    hb_script_t result = HB_SCRIPT_INVALID;
    for (CodepointIndex i = range.min; i < range.max; ++i) {
        const hb_script_t script = hb_unicode_script(unicodeFunctions, text[i]);
        if (isExplicitScript(script)) {
            return script;
        }
        if (result == HB_SCRIPT_INVALID || result == HB_SCRIPT_INHERITED) {
            result = script;
        }
    }
    return result == HB_SCRIPT_INVALID ? HB_SCRIPT_COMMON : result;
}

bool isAutomaticScript(ScriptTag script) {
    return script == 0;
}

bool isCursiveScript(ScriptTag script) {
    switch (static_cast<hb_script_t>(script)) {
    case HB_SCRIPT_ARABIC:
    case HB_SCRIPT_SYRIAC:
    case HB_SCRIPT_THAANA:
    case HB_SCRIPT_NKO:
    case HB_SCRIPT_SAMARITAN:
    case HB_SCRIPT_MANDAIC:
    case HB_SCRIPT_MONGOLIAN:
    case HB_SCRIPT_PHAGS_PA:
    case HB_SCRIPT_ADLAM:
        return true;
    default:
        return false;
    }
}

} // namespace

uint32_t paragraphSeparatorKindCodepoints(ParagraphSeparatorKind kind) {
    switch (kind) {
    case ParagraphSeparatorKind::None:
    default:
        return 0;
    case ParagraphSeparatorKind::LineFeed:
        [[fallthrough]];
    case ParagraphSeparatorKind::CarriageReturn:
        [[fallthrough]];
    case ParagraphSeparatorKind::InformationSeparator4:
        [[fallthrough]];
    case ParagraphSeparatorKind::InformationSeparator3:
        [[fallthrough]];
    case ParagraphSeparatorKind::InformationSeparator2:
        [[fallthrough]];
    case ParagraphSeparatorKind::NextLine:
        [[fallthrough]];
    case ParagraphSeparatorKind::ParagraphSeparator:
        return 1;
    case ParagraphSeparatorKind::CarriageReturnLineFeed:
        return 2;
    }
}

void segmentParagraphs(const DocumentSource& document, CodepointRange range,
                       function_ref<void(CodepointRange, ParagraphSeparatorKind)> onParagraph) {
    const std::u32string_view text = document.text;
    range                          = document.clamp(range);
    if (range.empty()) {
        onParagraph(range, ParagraphSeparatorKind::None);
        return;
    }

    ParagraphBreakIterator iterator;
    iterator.setText(text.substr(range.min, range.distance()));

    size_t count                    = 0;
    ParagraphSeparatorKind lastKind = ParagraphSeparatorKind::None;

    CodepointIndex localBegin       = iterator.nextBreak();
    for (CodepointIndex localEnd = iterator.nextBreak(); localEnd != UINT32_MAX;
         localEnd                = iterator.nextBreak()) {
        const CodepointIndex begin = range.min + localBegin;
        const CodepointIndex end   = range.min + localEnd;
        lastKind                   = paragraphSeparatorKind(text, begin, end);
        onParagraph(CodepointRange{ begin, end }, lastKind);
        count++;
        localBegin = localEnd;
    }

    if (count == 0 || lastKind != ParagraphSeparatorKind::None) {
        onParagraph(CodepointRange{ range.max, range.max }, ParagraphSeparatorKind::None);
    }
}

Direction resolveBidi(const DocumentSource& document, CodepointRange paragraphRange,
                      BaseDirection baseDirection, function_ref<void(CodepointRange, BiDiLevel)> onRun) {
    const std::u32string_view text = document.text;
    paragraphRange                 = document.clamp(paragraphRange);
    BiDiIterator iterator;
    iterator.setText(text.substr(paragraphRange.min, paragraphRange.distance()), baseDirection);

    for (;;) {
        const auto& run = iterator.nextRun();
        if (run.range.empty()) {
            break;
        }
        onRun(CodepointRange{ paragraphRange.min + run.range.min, paragraphRange.min + run.range.max },
              run.level);
    }

    return static_cast<Direction>(iterator.paragraphLevel());
}

void detectScripts(const DocumentSource& document, CodepointRange paragraphRange, std::span<ScriptTag> out) {
    const std::u32string_view text = document.text;
    assert(paragraphRange.max <= text.size());
    assert(document.graphemes.codepointCount() == text.size());

    const GraphemeRange paragraphGraphemeRange = document.graphemes.toGraphemeRange(paragraphRange);
    const size_t graphemeCount                 = paragraphGraphemeRange.distance();

    assert(out.size() == graphemeCount);
    if (graphemeCount == 0) {
        return;
    }

    hb_unicode_funcs_t* unicodeFunctions = hb_unicode_funcs_get_default();

    size_t i                             = 0;
    document.graphemes.iterate(paragraphGraphemeRange, [&](CodepointRange range) {
        out[i++] = static_cast<ScriptTag>(graphemeScript(text, range, unicodeFunctions));
    });

    struct PairedEntry {
        int pairIndex;
        hb_script_t script;
    };

    hb_script_t lastScript                          = HB_SCRIPT_INVALID;
    size_t firstUnresolved                          = 0;

    // Real-world text rarely nests paired punctuation more than a few levels deep. Keep a
    // bounded stack here so pathological input cannot turn script detection into an allocation
    // hotspot. If the bound is reached, discard the incomplete pairing context and continue
    // using the surrounding script rather than allowing inline_vector::push_back to throw.
    constexpr size_t kMaximumPairedCharacterNesting = 128;
    inline_vector<PairedEntry, kMaximumPairedCharacterNesting> pairedStack;

    for (size_t i = 0; i < graphemeCount; ++i) {
        hb_script_t gScript                 = static_cast<hb_script_t>(out[i]);
        const CodepointRange codepointRange = document.graphemes.codepointRangeForGrapheme(
            paragraphGraphemeRange.min + static_cast<GraphemeIndex>(i));
        const int pairIndex =
            codepointRange.distance() == 1 ? pairedCharacterIndex(text[codepointRange.min]) : -1;

        if (gScript == HB_SCRIPT_COMMON && lastScript != HB_SCRIPT_INVALID) {
            if (pairIndex >= 0 && (pairIndex & 1) == 0) {
                gScript = lastScript;
                if (pairedStack.size() < pairedStack.capacity()) {
                    pairedStack.push_back(PairedEntry{ pairIndex, gScript });
                } else {
                    // There is no reliable way to match a close after the stack overflows.
                    // Dropping the context is preferable to throwing or misapplying a stale
                    // nested pairing to the rest of the paragraph.
                    pairedStack = {};
                }
            } else if (pairIndex >= 0) {
                while (!pairedStack.empty() && pairedStack.back().pairIndex != (pairIndex & ~1)) {
                    pairedStack.pop_back();
                }
                gScript    = pairedStack.empty() ? lastScript : pairedStack.back().script;
                lastScript = gScript;
                if (!pairedStack.empty()) {
                    pairedStack.pop_back();
                }
            } else {
                gScript = lastScript;
            }
        } else if (gScript == HB_SCRIPT_INHERITED && lastScript != HB_SCRIPT_INVALID) {
            gScript = lastScript;
        } else if (isExplicitScript(gScript)) {
            for (size_t j = firstUnresolved; j < i; ++j) {
                if (out[j] == static_cast<ScriptTag>(HB_SCRIPT_COMMON) ||
                    out[j] == static_cast<ScriptTag>(HB_SCRIPT_INHERITED)) {
                    out[j] = static_cast<ScriptTag>(gScript);
                }
            }
            lastScript      = gScript;
            firstUnresolved = i + 1;
        }
        out[i] = static_cast<ScriptTag>(gScript);
    }

    for (size_t i = graphemeCount; i-- > 0;) {
        if (out[i] != static_cast<ScriptTag>(HB_SCRIPT_COMMON) &&
            out[i] != static_cast<ScriptTag>(HB_SCRIPT_INHERITED)) {
            break;
        }
        if (i + 1 < graphemeCount) {
            out[i] = out[i + 1];
        }
    }
}

void itemizeScripts(const DocumentSource& document, CodepointRange bidiRange, BiDiLevel level,
                    ScriptTag script, std::span<const ScriptTag> runScripts,
                    function_ref<void(CodepointRange range, const ScriptRun&)> onRun) {
    const std::u32string_view text = document.text;
    bidiRange                      = document.clamp(bidiRange);
    if (bidiRange.empty()) {
        return;
    }

    if (!isAutomaticScript(script)) {
        onRun(bidiRange, ScriptRun{
                             .level  = level,
                             .script = script,
                         });
        return;
    }

    const GraphemeRange bidiGraphemeRange = document.graphemes.toGraphemeRange(bidiRange);
    const size_t graphemeCount            = bidiGraphemeRange.distance();
    if (graphemeCount == 0) {
        return;
    }

    assert(runScripts.size() >= graphemeCount);
    if (runScripts.empty()) {
        return;
    }

    CodepointIndex runStart = document.graphemes.codepointRangeForGrapheme(bidiGraphemeRange.min).min;
    uint32_t currentScript  = runScripts[0];

    for (size_t i = 1; i < graphemeCount; ++i) {
        const uint32_t gScript = runScripts[i];
        if (gScript != currentScript) {
            const CodepointIndex gMin =
                document.graphemes
                    .codepointRangeForGrapheme(bidiGraphemeRange.min + static_cast<GraphemeIndex>(i))
                    .min;
            onRun({ runStart, gMin }, ScriptRun{
                                          .level  = level,
                                          .script = currentScript,
                                      });
            runStart      = gMin;
            currentScript = gScript;
        }
    }

    onRun({ runStart, document.graphemes.codepointRangeForGrapheme(bidiGraphemeRange.max - 1).max },
          {
              .level  = level,
              .script = currentScript,
          });
}

namespace {

bool isDefaultIgnorableCodepoint(char32_t codepoint) {
    return codepoint == 0x00AD || // SOFT HYPHEN
           codepoint == 0x034F || // COMBINING GRAPHEME JOINER
           codepoint == 0x061C || // ARABIC LETTER MARK
           (codepoint >= 0x115F && codepoint <= 0x1160) || (codepoint >= 0x17B4 && codepoint <= 0x17B5) ||
           (codepoint >= 0x180B && codepoint <= 0x180F) || (codepoint >= 0x200B && codepoint <= 0x200F) ||
           (codepoint >= 0x202A && codepoint <= 0x202E) || (codepoint >= 0x2060 && codepoint <= 0x206F) ||
           (codepoint >= 0xFE00 && codepoint <= 0xFE0F) || codepoint == 0xFEFF ||
           (codepoint >= 0xFFF0 && codepoint <= 0xFFF8) || (codepoint >= 0xE0000 && codepoint <= 0xE0FFF);
}

bool fontSupportsGrapheme(const FontDatabase* fontDatabase, const FontHandle& fontHandle,
                          std::u32string_view text, CodepointRange range) {
    if (fontDatabase == nullptr || !fontHandle) {
        return false;
    }
    const ActiveFont font = fontDatabase->activate(fontHandle);
    const FT_Face face    = static_cast<FT_Face>(font.ftFace);
    if (face == nullptr) {
        return false;
    }
    for (CodepointIndex i = range.min; i < range.max; ++i) {
        const char32_t codepoint = text[i];
        // The registration-time ASCII probe already verified this complete
        // range. Avoid repeating cmap lookups for the overwhelmingly common
        // ASCII case; non-ASCII and partial/unknown ranges still use the
        // exact per-codepoint lookup below.
        const bool knownAscii    = codepoint >= U' ' && codepoint <= U'~' && font.asciiRangePresent;
        if (!isDefaultIgnorableCodepoint(codepoint) && !knownAscii &&
            FT_Get_Char_Index(face, static_cast<FT_ULong>(codepoint)) == 0) {
            return false;
        }
    }
    return true;
}

FontHandle chooseFontForGrapheme(std::u32string_view text, CodepointRange gRange,
                                 std::span<const FontHandle> fontHandles, const FontDatabase* fontDatabase) {
    if (fontDatabase == nullptr) {
        return {};
    }

    for (const FontHandle& handle : fontHandles) {
        if (handle && fontSupportsGrapheme(fontDatabase, handle, text, gRange)) {
            return handle;
        }
    }
    return {};
}

} // namespace

VerticalMetrics getVerticalMetrics(const FontDatabase* fontDatabase, const FontHandle& fontHandle) {
    if (fontDatabase == nullptr || !fontHandle) {
        return VerticalMetrics{
            .ascent  = kZero,
            .descent = kZero,
            .lineGap = kZero,
        };
    }

    const ActiveFont font = fontDatabase->activate(fontHandle);
    const FT_Face face    = static_cast<FT_Face>(font.ftFace);
    if (face == nullptr || face->size == nullptr) {
        return {};
    }

    const FT_Size_Metrics& metrics = face->size->metrics;
    return VerticalMetrics{
        .ascent  = from26Dot6(metrics.ascender),
        .descent = from26Dot6(-metrics.descender),
        .lineGap = from26Dot6(metrics.height - (metrics.ascender - metrics.descender)),
    };
}

ExtendedMetrics getExtendedMetrics(const FontDatabase* fontDatabase, const FontHandle& fontHandle) {
    if (fontDatabase == nullptr || !fontHandle) {
        return {};
    }

    const ActiveFont font = fontDatabase->activate(fontHandle);
    const FT_Face face    = static_cast<FT_Face>(font.ftFace);
    if (face == nullptr || face->size == nullptr) {
        return {};
    }

    const FT_Fixed yScale     = face->size->metrics.y_scale;
    const auto scaleFontUnits = [yScale](FT_Pos value) {
        return from26Dot6(FT_MulFix(value, yScale));
    };

    ExtendedMetrics result{
        .underlinePosition = scaleFontUnits(-face->underline_position),
        .lineThickness     = scaleFontUnits(face->underline_thickness),
    };

    const FT_UInt spaceGlyph = FT_Get_Char_Index(face, static_cast<FT_ULong>(' '));
    FT_Fixed spaceAdvance    = 0;
    if (spaceGlyph != 0 &&
        FT_Get_Advance(face, spaceGlyph, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP, &spaceAdvance) == 0) {
        result.spaceAdvanceX = fromFloat(static_cast<float>(spaceAdvance) * (1.0f / 65536.0f));
    }

    if (const auto* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, ft_sfnt_os2)); os2 != nullptr) {
        result.xHeight       = scaleFontUnits(os2->sxHeight);
        result.capitalHeight = scaleFontUnits(os2->sCapHeight);
    }

    return result;
}

void resolveFonts(
    const DocumentSource& document, CodepointRange scriptRange, const ScriptRun& run,
    const FontDatabase* fontDatabase, std::span<const FontDef* const> fonts, BoundaryTable fontBoundaries,
    function_ref<void(CodepointRange range, const ResolvedFont&)> onShapeableRun,
    function_ref<void(CodepointRange range, char32_t codepoint, const ResolvedFont&)> onControlRun) {
    const std::u32string_view text = document.text;
    scriptRange                    = document.clamp(scriptRange);
    if (scriptRange.empty()) {
        return;
    }

    assert(fontBoundaries.correct());
    assert(fontBoundaries.count() == fonts.size());

    std::vector<std::vector<FontHandle>> fontHandlesPerRun(fonts.size());

    auto getOrResolveFontHandle = [&](FontRunIndex fontRunIdx, std::u32string_view gText,
                                      CodepointRange gRange) -> FontHandle {
        if (fontDatabase == nullptr || fontRunIdx >= fonts.size()) {
            return {};
        }
        const FontDef& fontDef               = *fonts[fontRunIdx];
        std::vector<FontHandle>& fontHandles = fontHandlesPerRun[fontRunIdx];

        if (fontHandles.empty() && !fontDef.familyNames.empty()) {
            FontHandle handle =
                fontDatabase->resolveFont(fontDef.familyNames[0], fontDef.style, fontDef.weight,
                                          fontDef.fontSize, fontDef.variations, fontDef.hinting);
            fontHandles.push_back(std::move(handle));
        }

        size_t triedCount = 0;
        while (triedCount < fontHandles.size()) {
            std::span<const FontHandle> handlesToTry{ fontHandles.data() + triedCount,
                                                      fontHandles.size() - triedCount };
            FontHandle chosen = chooseFontForGrapheme(gText, gRange, handlesToTry, fontDatabase);
            if (chosen) {
                return chosen;
            }
            triedCount = fontHandles.size();

            if (fontHandles.size() < fontDef.familyNames.size()) {
                const size_t nextFamilyIdx = fontHandles.size();
                FontHandle handle = fontDatabase->resolveFont(fontDef.familyNames[nextFamilyIdx],
                                                              fontDef.style, fontDef.weight, fontDef.fontSize,
                                                              fontDef.variations, fontDef.hinting);
                fontHandles.push_back(std::move(handle));
            }
        }

        // Return the first valid font handle as fallback if none supported the grapheme
        for (const FontHandle& handle : fontHandles) {
            if (handle) {
                return handle;
            }
        }
        return {};
    };

    CodepointRange pendingRange{ scriptRange.min, scriptRange.min };
    FontHandle pendingFontHandle{};
    FontRunIndex pendingFontRunIdx = 0;
    bool hasPendingRun             = false;

    auto flushPending              = [&]() {
        if (hasPendingRun) {
            onShapeableRun(pendingRange, ResolvedFont{
                                             .fontHandle   = pendingFontHandle,
                                             .fontRunIndex = pendingFontRunIdx,
                                         });
            hasPendingRun = false;
        }
    };

    const GraphemeRange scriptGraphemeRange = document.graphemes.toGraphemeRange(scriptRange);
    document.graphemes.iterate(scriptGraphemeRange, [&](CodepointRange gRange) {
        // Check if this grapheme cluster is a single control codepoint
        if (gRange.distance() == 1 && shouldSplitRunAt(text[gRange.min])) {
            flushPending();
            const FontRunIndex fontRunIdx = fontBoundaries.indexOf(gRange.min);
            const FontHandle fontHandle   = getOrResolveFontHandle(fontRunIdx, text, gRange);
            onControlRun(gRange, text[gRange.min],
                         ResolvedFont{
                             .fontHandle   = fontHandle,
                             .fontRunIndex = fontRunIdx,
                         });
            return;
        }

        const FontRunIndex fontRunIdx = fontBoundaries.indexOf(gRange.min);
        const FontHandle chosenFont   = getOrResolveFontHandle(fontRunIdx, text, gRange);

        if (hasPendingRun) {
            if (chosenFont == pendingFontHandle && fontRunIdx == pendingFontRunIdx) {
                // Extend the current shapeable run
                pendingRange.max = gRange.max;
            } else {
                flushPending();
                pendingRange      = gRange;
                pendingFontHandle = chosenFont;
                pendingFontRunIdx = fontRunIdx;
                hasPendingRun     = true;
            }
        } else {
            pendingRange      = gRange;
            pendingFontHandle = chosenFont;
            pendingFontRunIdx = fontRunIdx;
            hasPendingRun     = true;
        }
    });

    flushPending();
}

namespace {

hb_language_t toHbLanguage(std::string_view language) {
    if (language.empty()) {
        return hb_language_get_default();
    }
    return hb_language_from_string(language.data(), static_cast<int>(language.size()));
}

hb_direction_t levelToHbDirection(BiDiLevel level) {
    return directionFromLevel(level) == Direction::RightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
}

void transformFreetypeVector(int32_t* x, int32_t* y, const FT_Matrix& matrix) {
    FT_Vector vector{ .x = *x, .y = *y };
    FT_Vector_Transform(&vector, &matrix);
    *x = static_cast<int32_t>(vector.x);
    *y = static_cast<int32_t>(vector.y);
}

struct hb_buffer_deleter {
    void operator()(hb_buffer_t* buffer) const noexcept {
        hb_buffer_destroy(buffer);
    }
};

} // namespace

void shapeRun(const DocumentSource& document, CodepointRange textContextRange, CodepointRange runRange,
              const FontDatabase* fontDatabase, const ResolvedFont& run, BiDiLevel level, ScriptTag script,
              const FontDef& fontDef, std::string_view language,
              function_ref<void(const Glyph&, bool unsafeToBreak)> onGlyph) {
    const std::u32string_view text = document.text;
    assert(runRange.min >= textContextRange.min && runRange.max <= textContextRange.max);
    assert(textContextRange.max <= text.size());
    if (runRange.empty()) {
        // No glyphs to shape
        return;
    }

    const GraphemeRange runGraphemeRange = document.graphemes.toGraphemeRange(runRange);
    const auto graphemeCount             = static_cast<GraphemeIndex>(runGraphemeRange.distance());

    if (!run.fontHandle) {
        // Unrenderable run: produce a missing/replacement glyph (glyphId 0) per grapheme
        for (GraphemeIndex gIdx = 0; gIdx < graphemeCount; ++gIdx) {
            onGlyph(
                Glyph{
                    .glyphId       = 0,
                    .graphemeIndex = gIdx,
                    .xAdvance      = kZero,
                    .xOffset       = kZero,
                    .yOffset       = kZero,
                },
                false);
        }

        return;
    }

    const ActiveFont font = fontDatabase == nullptr ? ActiveFont{} : fontDatabase->activate(run.fontHandle);
    if (font.ftFace == nullptr || font.hbFont == nullptr) {
        return;
    }
    const FT_Face ftFace = static_cast<FT_Face>(font.ftFace);
    hb_font_t* hbFont    = static_cast<hb_font_t*>(font.hbFont);

    // TODO: Create a HarfBuzz buffer pool
    std::unique_ptr<hb_buffer_t, hb_buffer_deleter> buffer(hb_buffer_create());
    assert(buffer.get() != nullptr);

    hb_buffer_add_utf32(buffer.get(), reinterpret_cast<const uint32_t*>(text.data()),
                        static_cast<int>(text.size()), runRange.min, runRange.distance());
    hb_buffer_set_script(buffer.get(), hb_script_from_iso15924_tag(script));
    // TODO: Cache the HarfBuzz language object per language string
    hb_buffer_set_language(buffer.get(), toHbLanguage(language));
    hb_buffer_set_direction(buffer.get(), levelToHbDirection(level));
    hb_buffer_flags_t flags = HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES;
    if (runRange.min == textContextRange.min) {
        flags = static_cast<hb_buffer_flags_t>(flags | HB_BUFFER_FLAG_BOT);
    }
    if (runRange.max == textContextRange.max) {
        flags = static_cast<hb_buffer_flags_t>(flags | HB_BUFFER_FLAG_EOT);
    }
    hb_buffer_set_flags(buffer.get(), flags);

    inline_vector<hb_feature_t, maximumOpenTypeFeatures> hbFeatures;
    auto features =
        fontDef.features.subspan(0, std::min<size_t>(fontDef.features.size(), hbFeatures.capacity()));

    for (const auto& feat : features) {
        hbFeatures.push_back(hb_feature_t{
            .tag   = feat.feature,
            .value = feat.enabled ? 1u : 0u,
            .start = HB_FEATURE_GLOBAL_START,
            .end   = HB_FEATURE_GLOBAL_END,
        });
    }

    const bool shaped = hb_shape_full(hbFont, buffer.get(), hbFeatures.empty() ? nullptr : hbFeatures.data(),
                                      static_cast<unsigned int>(hbFeatures.size()), nullptr);
    assert(shaped);

    unsigned int glyphCount              = 0;
    const hb_glyph_info_t* infos         = hb_buffer_get_glyph_infos(buffer.get(), &glyphCount);
    // hb_glyph_flags_t hb_glyph_info_get_glyph_flags (const hb_glyph_info_t *info);

    const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer.get(), nullptr);

    FT_Matrix matrix;
    FT_Get_Transform(ftFace, &matrix, nullptr);

    for (unsigned int i = 0; i < glyphCount; ++i) {
        int32_t xAdvance = positions[i].x_advance;
        assert(positions[i].y_advance == 0);
        int32_t xOffset = positions[i].x_offset;
        int32_t yOffset = positions[i].y_offset;

        transformFreetypeVector(&xOffset, &yOffset, matrix);

        const GraphemeIndex graphemeIndex =
            document.graphemes.toGrapheme(infos[i].cluster) - runGraphemeRange.min;
        const LayoutUnit xAdv  = from26Dot6(xAdvance);

        hb_glyph_flags_t flags = hb_glyph_info_get_glyph_flags(&infos[i]);
        bool unsafeToBreak     = static_cast<bool>(flags & HB_GLYPH_FLAG_UNSAFE_TO_BREAK);

        onGlyph(
            Glyph{
                .glyphId       = infos[i].codepoint,
                .graphemeIndex = graphemeIndex,
                .xAdvance      = xAdv,
                .xOffset       = from26Dot6(xOffset),
                .yOffset       = from26Dot6(yOffset),
            },
            unsafeToBreak);
    }
}

void breakOpportunities(std::u32string_view text, CodepointRange range, std::span<LineBreakKind> out,
                        const char* language) {
    assert(range.max <= text.size());
    assert(out.size() == range.distance());
    if (range.empty()) {
        return;
    }
    set_linebreaks_utf32(reinterpret_cast<const utf32_t*>(text.data() + range.min),
                         static_cast<size_t>(range.distance()), language,
                         reinterpret_cast<char*>(out.data()));
}

namespace {

bool isTrimmableWhitespace(std::u32string_view text, CodepointRange range) {
    if (range.empty()) {
        return false;
    }
    for (CodepointIndex i = range.min; i < range.max; ++i) {
        const char32_t cp                  = text[i];
        const utf8proc_category_t category = utf8proc_category(static_cast<utf8proc_int32_t>(cp));
        if (category != UTF8PROC_CATEGORY_ZS && cp != U'\t') {
            return false;
        }
    }
    return true;
}

} // namespace

void appendPreparedParagraph(const DocumentSource& document, CodepointRange paragraphRange,
                             std::shared_ptr<const FontDatabase> fontDatabase,
                             std::span<const FontDef* const> fonts, BoundaryTable fontBoundaries,
                             BaseDirection baseDirection, ScriptTag script, std::string_view language,
                             PreparedDocument& documentResult, PreparedDocument::Paragraph& paragraphResult) {
    paragraphRange                    = document.clamp(paragraphRange);
    const GraphemeRange graphemeRange = document.graphemes.toGraphemeRange(paragraphRange);
    const GlyphIndex glyphBase        = static_cast<GlyphIndex>(documentResult.glyphs.size());
    const RunIndex runBase            = static_cast<RunIndex>(documentResult.glyphRuns.size());
    std::vector<ScriptTag> paragraphScripts(document.graphemes.toGraphemeRange(paragraphRange).distance());
    detectScripts(document, paragraphRange, paragraphScripts);

    for (GraphemeIndex localG = 0; localG < graphemeRange.distance(); ++localG) {
        const GraphemeIndex globalG         = graphemeRange.min + localG;
        const CodepointRange codepointRange = document.graphemes.codepointRangeForGrapheme(globalG);
        documentResult.graphemes[globalG].trimmableWhitespace =
            isTrimmableWhitespace(document.text, codepointRange);
    }

    const Direction resolvedBaseDirection = resolveBidi(
        document, paragraphRange, baseDirection, [&](CodepointRange bidiRange, BiDiLevel bidiLevel) {
            const CodepointIndex localBidiMin    = bidiRange.min - paragraphRange.min;
            const CodepointIndex localBidiMax    = bidiRange.max - paragraphRange.min;
            const GraphemeIndex localGraphemeMin = document.graphemes.toGrapheme(bidiRange.min);
            const GraphemeIndex localGraphemeMax = document.graphemes.toGrapheme(bidiRange.max - 1) + 1;
            const std::span<const ScriptTag> bidiScripts{ paragraphScripts.data() +
                                                              (localGraphemeMin - graphemeRange.min),
                                                          localGraphemeMax - localGraphemeMin };

            itemizeScripts(
                document, bidiRange, bidiLevel, script, bidiScripts,
                [&](CodepointRange scriptRange, const ScriptRun& scriptRun) {
                    resolveFonts(
                        document, scriptRange, scriptRun, fontDatabase.get(), fonts, fontBoundaries,
                        [&](CodepointRange fontRange, const ResolvedFont& resolvedFont) {
                            const GraphemeIndex graphemeMin = document.graphemes.toGrapheme(fontRange.min);
                            const GraphemeIndex graphemeMax =
                                document.graphemes.toGrapheme(fontRange.max - 1) + 1;
                            const GlyphIndex glyphMin = static_cast<GlyphIndex>(documentResult.glyphs.size());
                            const FontDef& fontDef    = *fonts[resolvedFont.fontRunIndex];
                            const bool applyLetterSpacing = !isCursiveScript(scriptRun.script);
                            const bool hasNextGrapheme    = fontRange.max < paragraphRange.max;
                            LayoutUnit pen                = kZero;
                            std::optional<std::pair<Glyph, bool>> pendingGlyph;

                            auto emitPending = [&](bool endOfGrapheme, bool hasFollowingGrapheme) {
                                if (!pendingGlyph) {
                                    return;
                                }
                                Glyph glyph              = pendingGlyph->first;
                                const bool unsafeToBreak = pendingGlyph->second;
                                glyph.xOffset += pen;
                                const GraphemeIndex documentGrapheme = graphemeMin + glyph.graphemeIndex;
                                if (endOfGrapheme &&
                                    documentResult.graphemes[documentGrapheme].trimmableWhitespace &&
                                    !documentResult.graphemes[documentGrapheme].isTab) {
                                    glyph.xAdvance +=
                                        bidiLevel % 2 == 0 ? fontDef.wordSpacing : -fontDef.wordSpacing;
                                }
                                if (endOfGrapheme && applyLetterSpacing &&
                                    (hasFollowingGrapheme || hasNextGrapheme)) {
                                    glyph.xAdvance +=
                                        bidiLevel % 2 == 0 ? fontDef.letterSpacing : -fontDef.letterSpacing;
                                }
                                pen += glyph.xAdvance;
                                documentResult.graphemeAdvancePrefix[documentGrapheme + 1] +=
                                    glyph.xAdvance >= kZero ? glyph.xAdvance : -glyph.xAdvance;
                                if (unsafeToBreak && documentGrapheme > 0) {
                                    documentResult.graphemes[documentGrapheme - 1].unsafeToBreakAfter = true;
                                }
                                documentResult.glyphs.push_back(glyph);
                                pendingGlyph.reset();
                            };

                            shapeRun(document, paragraphRange, fontRange, fontDatabase.get(), resolvedFont,
                                     bidiLevel, scriptRun.script, fontDef, language,
                                     [&](const Glyph& sourceGlyph, bool unsafeToBreak) {
                                         if (pendingGlyph &&
                                             pendingGlyph->first.graphemeIndex != sourceGlyph.graphemeIndex) {
                                             emitPending(true, true);
                                         }
                                         if (pendingGlyph) {
                                             emitPending(false, true);
                                         }
                                         pendingGlyph = std::pair{ sourceGlyph, unsafeToBreak };
                                     });
                            emitPending(true, false);
                            if (pen < kZero) {
                                for (GlyphIndex i = glyphMin; i < documentResult.glyphs.size(); ++i) {
                                    documentResult.glyphs[i].xOffset -= pen;
                                }
                            }
                            documentResult.glyphRuns.push_back(GlyphRun{
                                .codepointRange = fontRange,
                                .graphemeRange  = { graphemeMin, graphemeMax },
                                .glyphRange     = { glyphMin,
                                                    static_cast<GlyphIndex>(documentResult.glyphs.size()) },
                                .fontHandle     = resolvedFont.fontHandle,
                                .fontRunIndex   = resolvedFont.fontRunIndex,
                                .level          = bidiLevel,
                                .script         = scriptRun.script,
                                .verticalAlign  = fonts[resolvedFont.fontRunIndex]->verticalAlign,
                                .metrics = getVerticalMetrics(fontDatabase.get(), resolvedFont.fontHandle),
                            });
                        },
                        [&](CodepointRange controlRange, char32_t codepoint,
                            const ResolvedFont& resolvedFont) {
                            const GraphemeIndex graphemeMin = document.graphemes.toGrapheme(controlRange.min);
                            documentResult.graphemes[graphemeMin].isTab = codepoint == U'\t';
                            documentResult.glyphRuns.push_back(GlyphRun{
                                .codepointRange = controlRange,
                                .graphemeRange  = { graphemeMin, graphemeMin + 1 },
                                .glyphRange     = { static_cast<GlyphIndex>(documentResult.glyphs.size()),
                                                    static_cast<GlyphIndex>(documentResult.glyphs.size()) },
                                .fontHandle     = resolvedFont.fontHandle,
                                .fontRunIndex   = resolvedFont.fontRunIndex,
                                .level          = bidiLevel,
                                .script         = scriptRun.script,
                                .metrics = getVerticalMetrics(fontDatabase.get(), resolvedFont.fontHandle),
                            });
                        });
                });
        });

    for (RunIndex runIndex = runBase; runIndex < documentResult.glyphRuns.size(); ++runIndex) {
        GlyphRun& run          = documentResult.glyphRuns[runIndex];
        const FontDef& fontDef = *fonts[run.fontRunIndex];
        if (fontDef.lineHeight > kZero) {
            const LayoutUnit contentHeight = run.metrics.ascent + run.metrics.descent;
            run.metrics.lineGap =
                fontDef.lineHeight > contentHeight ? fontDef.lineHeight - contentHeight : kZero;
        }
    }

    paragraphResult.paragraphRange = paragraphRange;
    paragraphResult.baseDirection  = resolvedBaseDirection;
    paragraphResult.graphemeRange  = graphemeRange;
    paragraphResult.glyphRange     = { glyphBase, static_cast<GlyphIndex>(documentResult.glyphs.size()) };
    paragraphResult.glyphRunRange  = { runBase, static_cast<RunIndex>(documentResult.glyphRuns.size()) };
}

PreparedDocument prepareDocument(const DocumentSource& document,
                                 std::shared_ptr<const FontDatabase> fontDatabase,
                                 std::span<const FontDef* const> fonts, BoundaryTable fontBoundaries,
                                 std::span<const BaseDirection> baseDirections, ScriptTag script,
                                 std::string_view language) {
    struct SegmentedParagraph {
        CodepointRange range;
        ParagraphSeparatorKind separator;
    };

    std::vector<SegmentedParagraph> segmented;
    segmentParagraphs(document, entireText, [&](CodepointRange range, ParagraphSeparatorKind separator) {
        const uint32_t separatorLength = paragraphSeparatorKindCodepoints(separator);
        assert(range.distance() >= separatorLength);
        segmented.push_back(SegmentedParagraph{
            .range     = { range.min, range.max - separatorLength },
            .separator = separator,
        });
    });

    assert(baseDirections.empty() || baseDirections.size() == 1 || baseDirections.size() == segmented.size());
    assert(fontBoundaries.correct());
    assert(fontBoundaries.count() == fonts.size());

    PreparedDocument result;
    result.fontDatabase = std::move(fontDatabase);
    result.cookie       = nextCookie();
    result.graphemeMap  = document.graphemes;
    result.breakOpportunities.resize(document.codepointCount());
    result.graphemes.resize(document.graphemeCount());
    result.graphemeAdvancePrefix.resize(document.graphemeCount() + 1);

    result.paragraphs.reserve(segmented.size());
    for (size_t paragraphIndex = 0; paragraphIndex < segmented.size(); ++paragraphIndex) {
        const SegmentedParagraph& segment = segmented[paragraphIndex];
        const BaseDirection direction = baseDirections.empty()
                                            ? BaseDirection::DefaultLTR
                                            : baseDirections[baseDirections.size() == 1 ? 0 : paragraphIndex];
        const GraphemeRange documentGraphemeRange = document.graphemes.toGraphemeRange(segment.range);
        VerticalMetrics emptyLineMetrics{};
        if (segment.range.empty()) {
            const uint32_t fontIndex =
                fontBoundaries.total() == 0
                    ? 0
                    : fontBoundaries.indexOf(std::min(segment.range.min, fontBoundaries.total() - 1));
            emptyLineMetrics = getVerticalMetrics(result.fontDatabase.get(),
                                                  result.fontDatabase->resolveFont(*fonts[fontIndex]));
            if (fonts[fontIndex]->lineHeight > kZero) {
                const LayoutUnit contentHeight = emptyLineMetrics.ascent + emptyLineMetrics.descent;
                emptyLineMetrics.lineGap       = fonts[fontIndex]->lineHeight > contentHeight
                                                     ? fonts[fontIndex]->lineHeight - contentHeight
                                                     : kZero;
            }
        }
        result.paragraphs.push_back(PreparedDocument::Paragraph{
            .paragraphRange         = segment.range,
            .paragraphSeparatorKind = segment.separator,
            .graphemeRange          = documentGraphemeRange,
            .emptyLineMetrics       = emptyLineMetrics,
        });
        if (segment.separator != ParagraphSeparatorKind::None) {
            const CodepointRange separatorRange{
                segment.range.max, segment.range.max + paragraphSeparatorKindCodepoints(segment.separator)
            };
            const GraphemeRange separatorGraphemes = document.graphemes.toGraphemeRange(separatorRange);
            for (GraphemeIndex grapheme = separatorGraphemes.min; grapheme < separatorGraphemes.max;
                 ++grapheme) {
                result.graphemes[grapheme].isParagraphSeparator = true;
            }
        }
        if (!segment.range.empty()) {
            breakOpportunities(document.text, segment.range,
                               std::span<LineBreakKind>{ result.breakOpportunities.data() + segment.range.min,
                                                         segment.range.distance() },
                               language.empty() ? nullptr : std::string(language).c_str());
        }
        appendPreparedParagraph(document, segment.range, result.fontDatabase, fonts, fontBoundaries,
                                direction, script, language, result, result.paragraphs.back());
    }

    LayoutUnit currentSum = kZero;
    for (GraphemeIndex g = 0; g < result.graphemes.size(); ++g) {
        currentSum += result.graphemeAdvancePrefix[g + 1];
        result.graphemeAdvancePrefix[g + 1] = currentSum;
    }
    return result;
}

namespace {

// Width of a line excluding whatever trailing-whitespace mass has accumulated at its end;
// clamped defensively so float rounding can never push it below zero.
LayoutUnit trimmedWidth(LayoutUnit width, LayoutUnit trailingWhitespaceMass) {
    return width > trailingWhitespaceMass ? width - trailingWhitespaceMass : kZero;
}

class DocumentGreedyLineBreaker {
public:
    DocumentGreedyLineBreaker(const PreparedDocument& document, std::span<const TextAlignment> alignments,
                              LayoutUnit maxLineWidth, TabStops tabStops,
                              std::span<const LayoutUnit> textIndents, bool allowBreakAnywhere);

    [[nodiscard]] DocumentLayout layoutPreparedDocument();

private:
    struct LineRunInfo {
        RunIndex preparedRunIndex;
        BiDiLevel level;
        VerticalMetrics metrics;
        FontHandle fontHandle{};
        GlyphRange glyphRange{};
        GraphemeRange graphemeRange{};
    };

    struct PendingSlice {
        RunIndex runIndex;
        GraphemeIndex runBase;
        GraphemeRange rangeInParagraph;
    };

    struct LineRunSlice {
        RunIndex runIndex;
        GraphemeRange graphemeRange;
    };

    [[nodiscard]] LayoutUnit currentMaxLineWidth() const;
    void addRun(const GlyphRun& run, RunIndex preparedRunIndex, bool finishParagraph);
    void appendGrapheme(RunIndex runIndex, GraphemeIndex runBase, GraphemeIndex g,
                        const GraphemeFlags& grapheme);
    [[nodiscard]] LayoutUnit resolveAdvance(GraphemeIndex g, const GraphemeFlags& grapheme) const;
    void rebuildOpenLineFrom(GraphemeIndex begin);
    [[nodiscard]] LayoutUnit sliceWidth(GraphemeRange range) const;
    void commitLine(GraphemeIndex cutGrapheme, LineEndKind kind, LayoutUnit contentAdvance,
                    LayoutUnit trimmedAdvance);

    const PreparedDocument& m_preparedDocument;
    std::vector<LineRunInfo> m_runInfos;
    std::span<const TextAlignment> m_alignments;
    LayoutUnit m_maxLineWidth;
    std::span<const LayoutUnit> m_textIndents;
    TabStops m_tabStops;
    bool m_allowBreakAnywhere;
    VerticalMetrics m_emptyLineMetrics{};

    DocumentLayout m_output;
    LayoutUnit m_cursor{ kZero };

    GraphemeIndex m_lineStart{ 0 };
    GraphemeIndex m_nextGrapheme{ 0 };
    GraphemeIndex m_paragraphGraphemeBegin{ 0 };
    GraphemeIndex m_paragraphGraphemeEnd{ 0 };
    CodepointIndex m_paragraphCodepointBegin{ 0 };
    CodepointIndex m_paragraphCodepointEnd{ 0 };
    Direction m_baseDirection{ Direction::LeftToRight };
    TextAlignment m_alignment{ TextAlignment::Start };
    LayoutUnit m_textIndent{ kZero };
    bool m_paragraphFirstLine{ true };
    LayoutUnit m_lineWidth{ kZero };
    LayoutUnit m_trailingWhitespaceMass{ kZero };
    LayoutUnit m_postCandidateTrailingWhitespaceMass{ kZero };

    bool m_hasCandidate{ false };
    GraphemeIndex m_candidateGrapheme{ 0 };
    LayoutUnit m_candidateWidth{ kZero };
    LayoutUnit m_candidateTrimmedWidth{ kZero };

    std::vector<LayoutUnit> m_resolvedAdvances;

    std::vector<PendingSlice> m_lineSlices;
    std::vector<LineRunSlice> m_outputSlices;
    std::vector<LineRunSlice> m_visualSlices;
};

} // namespace

DocumentGreedyLineBreaker::DocumentGreedyLineBreaker(const PreparedDocument& document,
                                                     std::span<const TextAlignment> alignments,
                                                     LayoutUnit maxLineWidth, TabStops tabStops,
                                                     std::span<const LayoutUnit> textIndents,
                                                     bool allowBreakAnywhere)
    : m_preparedDocument(document), m_alignments(alignments), m_maxLineWidth(maxLineWidth),
      m_textIndents(textIndents), m_tabStops(tabStops), m_allowBreakAnywhere(allowBreakAnywhere),
      m_resolvedAdvances(document.graphemes.size(), kZero) {
    assert(document.breakOpportunities.size() == document.graphemeMap.codepointCount());
    assert(document.graphemes.size() == document.graphemeMap.graphemeCount());
    assert(document.graphemeAdvancePrefix.size() == document.graphemes.size() + 1);
    assert(tabStops.firstStop >= kZero);
    assert(tabStops.interval >= kZero);
}

LayoutUnit DocumentGreedyLineBreaker::currentMaxLineWidth() const {
    if (m_paragraphFirstLine) {
        return m_maxLineWidth - m_textIndent;
    }
    return m_maxLineWidth;
}

DocumentLayout DocumentGreedyLineBreaker::layoutPreparedDocument() {
    assert(m_runInfos.empty());
    m_output.cookie = m_preparedDocument.cookie;

    assert(m_alignments.empty() || m_alignments.size() == 1 ||
           m_alignments.size() == m_preparedDocument.paragraphs.size());
    assert(m_textIndents.empty() || m_textIndents.size() == 1 ||
           m_textIndents.size() == m_preparedDocument.paragraphs.size());

    for (size_t paragraphIndex = 0; paragraphIndex < m_preparedDocument.paragraphs.size(); ++paragraphIndex) {
        const PreparedDocument::Paragraph& paragraph = m_preparedDocument.paragraphs[paragraphIndex];
        m_alignment = m_alignments.empty() ? TextAlignment::Start
                                           : m_alignments[m_alignments.size() == 1 ? 0 : paragraphIndex];
        m_textIndent =
            m_textIndents.empty() ? kZero : m_textIndents[m_textIndents.size() == 1 ? 0 : paragraphIndex];
        m_baseDirection                       = paragraph.baseDirection;
        m_paragraphGraphemeBegin              = paragraph.graphemeRange.min;
        m_paragraphGraphemeEnd                = paragraph.graphemeRange.max;
        m_paragraphCodepointBegin             = paragraph.paragraphRange.min;
        m_paragraphCodepointEnd               = paragraph.paragraphRange.max;
        m_emptyLineMetrics                    = paragraph.emptyLineMetrics;
        m_lineStart                           = m_paragraphGraphemeBegin;
        m_nextGrapheme                        = m_paragraphGraphemeBegin;
        m_lineWidth                           = kZero;
        m_trailingWhitespaceMass              = kZero;
        m_postCandidateTrailingWhitespaceMass = kZero;
        m_hasCandidate                        = false;
        m_lineSlices.clear();
        m_runInfos.clear();
        m_paragraphFirstLine = true;

        for (RunIndex runIndex = paragraph.glyphRunRange.min; runIndex < paragraph.glyphRunRange.max;
             ++runIndex) {
            addRun(m_preparedDocument.glyphRuns[runIndex], runIndex,
                   runIndex + 1 == paragraph.glyphRunRange.max);
        }
        if (paragraph.glyphRunRange.empty()) {
            commitLine(m_paragraphGraphemeBegin, LineEndKind::ParagraphEnd, kZero, kZero);
        }
    }

    return m_output;
}

void DocumentGreedyLineBreaker::addRun(const GlyphRun& run, RunIndex preparedRunIndex, bool finishParagraph) {
    const CodepointRange codepointRange = run.codepointRange;
    assert(codepointRange.min <= codepointRange.max);
    assert(codepointRange.max <= m_preparedDocument.graphemeMap.codepointCount());

    const GraphemeRange graphemeRange = run.graphemeRange;
    assert(graphemeRange.min == m_nextGrapheme);
    assert(graphemeRange.max <= m_preparedDocument.graphemes.size());

    const RunIndex runIndex = static_cast<RunIndex>(m_runInfos.size());
    LineRunInfo runInfo{
        .preparedRunIndex = preparedRunIndex,
        .level            = run.level,
        .metrics          = run.metrics,
        .fontHandle       = run.fontHandle,
        .glyphRange       = run.glyphRange,
        .graphemeRange    = run.graphemeRange,
    };
    m_runInfos.push_back(std::move(runInfo));

    for (GraphemeIndex g = graphemeRange.min; g < graphemeRange.max; ++g) {
        appendGrapheme(runIndex, graphemeRange.min, g, m_preparedDocument.graphemes[g]);
    }

    if (finishParagraph) {
        if (m_nextGrapheme > m_lineStart) {
            const GraphemeIndex lastGrapheme = m_nextGrapheme - 1;
            const uint32_t lastCodepoint =
                m_preparedDocument.graphemeMap.codepointRangeForGrapheme(lastGrapheme).max - 1;
            if (m_preparedDocument.breakOpportunities[lastCodepoint] == LineBreakKind::MustBreak) {
                commitLine(m_nextGrapheme, LineEndKind::MandatoryBreak, m_lineWidth,
                           trimmedWidth(m_lineWidth, m_trailingWhitespaceMass));
                m_lineStart                           = m_nextGrapheme;
                m_lineWidth                           = kZero;
                m_trailingWhitespaceMass              = kZero;
                m_postCandidateTrailingWhitespaceMass = kZero;
                m_hasCandidate                        = false;
            }
        }
        commitLine(m_nextGrapheme, LineEndKind::ParagraphEnd, m_lineWidth,
                   trimmedWidth(m_lineWidth, m_trailingWhitespaceMass));
        m_lineStart          = m_nextGrapheme;
        m_paragraphFirstLine = false;
    }
}

void DocumentGreedyLineBreaker::appendGrapheme(RunIndex runIndex, GraphemeIndex runBase, GraphemeIndex g,
                                               const GraphemeFlags& grapheme) {
    assert(g == m_nextGrapheme);

    // A zero width is an explicit alignment-only container. It must not cause wrapping:
    // lines are laid out as if unconstrained, then positioned around x = 0 according to
    // the requested alignment. This is useful for point-anchored text.
    const bool noWrap = m_maxLineWidth == kInfinity || m_maxLineWidth == kZero;

    // Step 1: is the boundary immediately before this grapheme cluster a break opportunity?
    // (There is no boundary to consider before the very first grapheme of the paragraph.)
    if (g > m_paragraphGraphemeBegin) {
        const uint32_t boundaryCodepoint = m_preparedDocument.graphemeMap.toCodepoint(g) - 1;
        const LineBreakKind kind         = m_preparedDocument.breakOpportunities[boundaryCodepoint];

        if (kind == LineBreakKind::MustBreak) {
            // Mandatory breaks are unconditional: end the line here (consuming everything up to
            // and including the previous grapheme) regardless of width or shaping safety.
            commitLine(g, LineEndKind::MandatoryBreak, m_lineWidth,
                       trimmedWidth(m_lineWidth, m_trailingWhitespaceMass));
            m_lineStart                           = g;
            m_lineWidth                           = kZero;
            m_trailingWhitespaceMass              = kZero;
            m_postCandidateTrailingWhitespaceMass = kZero;
            m_hasCandidate                        = false;
        } else if (!noWrap &&
                   (kind == LineBreakKind::AllowBreak ||
                    (m_allowBreakAnywhere && !m_preparedDocument.graphemes[g - 1].unsafeToBreakAfter))) {
            // Record (overwriting any earlier one) as the latest safe soft-wrap candidate for
            // the still-open line -- greedy wrapping always prefers the rightmost candidate.
            m_hasCandidate                        = true;
            m_candidateGrapheme                   = g;
            m_candidateWidth                      = m_lineWidth;
            m_candidateTrimmedWidth               = trimmedWidth(m_lineWidth, m_trailingWhitespaceMass);
            m_postCandidateTrailingWhitespaceMass = kZero;
        }
    }

    // Step 2: append this grapheme cluster to the (possibly just-reset) open line.
    if (m_lineSlices.empty() || m_lineSlices.back().runIndex != runIndex) {
        m_lineSlices.push_back(PendingSlice{ runIndex, runBase, GraphemeRange{ g, g } });
    }
    m_lineSlices.back().rangeInParagraph.max = g + 1;

    const LayoutUnit resolvedAdvance         = resolveAdvance(g, grapheme);
    m_resolvedAdvances[g]                    = resolvedAdvance;
    m_lineWidth += resolvedAdvance;
    m_trailingWhitespaceMass =
        grapheme.trimmableWhitespace ? (m_trailingWhitespaceMass + resolvedAdvance) : kZero;
    if (!noWrap && m_hasCandidate) {
        m_postCandidateTrailingWhitespaceMass =
            grapheme.trimmableWhitespace ? (m_postCandidateTrailingWhitespaceMass + resolvedAdvance) : kZero;
    }
    ++m_nextGrapheme;

    // Step 3: greedily wrap once the line overflows and a safe break point is available. If no
    // candidate exists yet, keep accumulating (allowing the line to overflow) until one appears.
    if (!noWrap && m_hasCandidate && m_lineWidth > currentMaxLineWidth()) {
        commitLine(m_candidateGrapheme, LineEndKind::SoftWrap, m_candidateWidth, m_candidateTrimmedWidth);
        m_lineStart = m_candidateGrapheme;
        rebuildOpenLineFrom(m_candidateGrapheme);
    }
}

LayoutUnit DocumentGreedyLineBreaker::resolveAdvance(GraphemeIndex g, const GraphemeFlags& grapheme) const {
    const LayoutUnit advance =
        m_preparedDocument.graphemeAdvancePrefix[g + 1] - m_preparedDocument.graphemeAdvancePrefix[g];
    if (!grapheme.isTab || m_tabStops.interval == kZero) {
        return advance;
    }

    if (m_lineWidth < m_tabStops.firstStop) {
        return m_tabStops.firstStop - m_lineWidth;
    }

    const LayoutUnit remainder = (m_lineWidth - m_tabStops.firstStop) % m_tabStops.interval;
    return remainder == kZero ? m_tabStops.interval : m_tabStops.interval - remainder;
}

void DocumentGreedyLineBreaker::rebuildOpenLineFrom(GraphemeIndex begin) {
    m_lineWidth                           = kZero;
    m_trailingWhitespaceMass              = kZero;
    m_postCandidateTrailingWhitespaceMass = kZero;
    m_hasCandidate                        = false;

    for (GraphemeIndex g = begin; g < m_nextGrapheme; ++g) {
        const GraphemeFlags& grapheme    = m_preparedDocument.graphemes[g];
        const LayoutUnit resolvedAdvance = resolveAdvance(g, grapheme);
        m_resolvedAdvances[g]            = resolvedAdvance;
        m_lineWidth += resolvedAdvance;
        m_trailingWhitespaceMass =
            grapheme.trimmableWhitespace ? m_trailingWhitespaceMass + resolvedAdvance : kZero;
    }
}

LayoutUnit DocumentGreedyLineBreaker::sliceWidth(GraphemeRange range) const {
    LayoutUnit width = kZero;
    for (GraphemeIndex g = range.min; g < range.max; ++g) {
        width += m_resolvedAdvances[g];
    }
    return width;
}

void DocumentGreedyLineBreaker::commitLine(GraphemeIndex cutGrapheme, LineEndKind kind,
                                           LayoutUnit contentAdvance, LayoutUnit trimmedAdvance) {
    // Slices are contiguous and sorted by construction, so at most one of them straddles the
    // cut point; everything before it is included whole, everything from it onward -- possibly
    // truncated at its start -- is left in place to seed the next line.
    m_outputSlices.clear();

    size_t i = 0;
    for (; i < m_lineSlices.size(); ++i) {
        const PendingSlice& slice = m_lineSlices[i];
        if (slice.rangeInParagraph.min >= cutGrapheme) {
            break; // This slice (and everything after it) belongs entirely to the next line.
        }
        const GraphemeIndex sliceEnd = std::min(slice.rangeInParagraph.max, cutGrapheme);
        m_outputSlices.push_back(LineRunSlice{
            .runIndex = slice.runIndex,
            .graphemeRange =
                GraphemeRange{ slice.rangeInParagraph.min - slice.runBase, sliceEnd - slice.runBase },
        });
        if (slice.rangeInParagraph.max > cutGrapheme) {
            break; // Straddles the cut; its remainder (from cutGrapheme onward) stays pending.
        }
    }

    // 1. Collect run slices for this line.
    m_visualSlices = m_outputSlices;

    // 2. Sort to visual order using UAX #9 L2 rule.
    if (!m_visualSlices.empty() && !m_runInfos.empty()) {
        BiDiLevel maxLevel    = 0;
        BiDiLevel minOddLevel = 255;
        for (const LineRunSlice& slice : m_visualSlices) {
            const RunIndex runIdx = slice.runIndex;
            if (runIdx < m_runInfos.size()) {
                const BiDiLevel level = m_runInfos[runIdx].level;
                if (level > maxLevel) {
                    maxLevel = level;
                }
                if ((level % 2 != 0) && level < minOddLevel) {
                    minOddLevel = level;
                }
            }
        }

        if (minOddLevel != 255) {
            for (int l = static_cast<int>(maxLevel); l >= static_cast<int>(minOddLevel); --l) {
                const auto targetLevel = static_cast<BiDiLevel>(l);
                size_t start           = 0;
                while (start < m_visualSlices.size()) {
                    if (m_visualSlices[start].runIndex < m_runInfos.size() &&
                        m_runInfos[m_visualSlices[start].runIndex].level >= targetLevel) {
                        size_t end = start + 1;
                        while (end < m_visualSlices.size() &&
                               m_visualSlices[end].runIndex < m_runInfos.size() &&
                               m_runInfos[m_visualSlices[end].runIndex].level >= targetLevel) {
                            ++end;
                        }
                        std::reverse(m_visualSlices.begin() + static_cast<std::ptrdiff_t>(start),
                                     m_visualSlices.begin() + static_cast<std::ptrdiff_t>(end));
                        start = end;
                    } else {
                        ++start;
                    }
                }
            }
        }
    }

    // 3. Apply horizontal alignment and textIndent.
    Direction concreteAlignDir = Direction::LeftToRight;
    switch (m_alignment) {
    case TextAlignment::Left:
        concreteAlignDir = Direction::LeftToRight;
        break;
    case TextAlignment::Right:
        concreteAlignDir = Direction::RightToLeft;
        break;
    case TextAlignment::Start:
    case TextAlignment::Justify:
        concreteAlignDir = m_baseDirection;
        break;
    case TextAlignment::End:
        concreteAlignDir =
            m_baseDirection == Direction::LeftToRight ? Direction::RightToLeft : Direction::LeftToRight;
        break;
    case TextAlignment::Center:
        break;
    }

    const bool isFirstLine      = m_paragraphFirstLine;
    const LayoutUnit lineIndent = isFirstLine ? m_textIndent : kZero;

    LayoutUnit originX          = kZero;
    const LayoutUnit remainingSpace =
        m_maxLineWidth > trimmedAdvance ? (m_maxLineWidth - trimmedAdvance) : kZero;

    if (m_maxLineWidth == kZero) {
        // With no alignment container, place the selected edge/center at x = 0.
        // Use trimmedAdvance consistently with finite-width alignment, so trailing
        // whitespace does not move the visible text anchor.
        if (m_alignment == TextAlignment::Center) {
            originX = -half(trimmedAdvance);
        } else if (concreteAlignDir == Direction::RightToLeft) {
            originX = -trimmedAdvance;
        }
    } else if (m_alignment == TextAlignment::Center) {
        // Center alignment: shift by remainingSpace / 2 + direction-aware indent
        if (m_baseDirection == Direction::RightToLeft) {
            originX = half(remainingSpace) - lineIndent;
        } else {
            originX = half(remainingSpace) + lineIndent;
        }
    } else if (concreteAlignDir == Direction::RightToLeft) {
        originX = remainingSpace - lineIndent;
    } else {
        originX = lineIndent;
    }

    // 4. Materialize directly renderable glyph-run slices in visual order.
    const auto glyphRunStart = static_cast<RunIndex>(m_output.glyphRuns.size());
    LayoutUnit visualCursor  = originX;
    for (const LineRunSlice& slice : m_visualSlices) {
        const LineRunInfo& info = m_runInfos[slice.runIndex];
        GlyphRange glyphRange{ info.glyphRange.min, info.glyphRange.min };
        bool foundGlyph = false;

        for (GlyphIndex glyphIndex = info.glyphRange.min; glyphIndex < info.glyphRange.max; ++glyphIndex) {
            const GraphemeIndex g = m_preparedDocument.glyphs[glyphIndex].graphemeIndex;
            if (g >= slice.graphemeRange.min && g < slice.graphemeRange.max) {
                if (!foundGlyph) {
                    glyphRange.min = glyphIndex;
                    foundGlyph     = true;
                }
                glyphRange.max = glyphIndex + 1;
            }
        }

        const GraphemeIndex documentSliceMin = info.graphemeRange.min + slice.graphemeRange.min;
        const GraphemeIndex documentSliceMax = info.graphemeRange.min + slice.graphemeRange.max;
        const LayoutUnit sliceWidth          = this->sliceWidth({ documentSliceMin, documentSliceMax });
        LayoutUnit glyphOrigin               = kZero;
        if (foundGlyph) {
            glyphOrigin = m_preparedDocument.glyphs[glyphRange.min].xOffset;
            for (GlyphIndex glyphIndex = glyphRange.min + 1; glyphIndex < glyphRange.max; ++glyphIndex) {
                glyphOrigin = std::min(glyphOrigin, m_preparedDocument.glyphs[glyphIndex].xOffset);
            }
        }
        m_output.glyphRuns.push_back(LayoutGlyphRun{
            .preparedRunIndex = info.preparedRunIndex,
            .glyphRange       = glyphRange,
            .xOffset          = visualCursor - glyphOrigin,
            .yOffset          = kZero,
        });
        visualCursor += sliceWidth;
    }
    const auto glyphRunEnd = static_cast<RunIndex>(m_output.glyphRuns.size());
    const RunRange glyphRunRange{ glyphRunStart, glyphRunEnd };

    // 5. Compute line vertical metrics.
    LayoutUnit maxAscent  = kZero;
    LayoutUnit maxDescent = kZero;
    LayoutUnit maxLeading = kZero;

    for (const auto& slice : m_outputSlices) {
        if (slice.runIndex < m_runInfos.size()) {
            const VerticalMetrics& vm   = m_runInfos[slice.runIndex].metrics;
            LayoutUnit runAscent        = vm.ascent;
            LayoutUnit runDescent       = vm.descent;
            const LayoutUnit runLeading = vm.lineGap;

            if (runAscent > maxAscent) {
                maxAscent = runAscent;
            }
            if (runDescent > maxDescent) {
                maxDescent = runDescent;
            }
            if (runLeading > maxLeading) {
                maxLeading = runLeading;
            }
        }
    }
    if (m_outputSlices.empty()) {
        const VerticalMetrics& metrics = m_runInfos.empty() ? m_emptyLineMetrics : m_runInfos.back().metrics;
        maxAscent                      = metrics.ascent;
        maxDescent                     = metrics.descent;
        maxLeading                     = metrics.lineGap;
    }

    // 6. Distribute baselines.
    const LayoutUnit baselineY = m_cursor + maxAscent;
    m_cursor += maxAscent + maxDescent + maxLeading;

    for (RunIndex run = glyphRunRange.min; run < glyphRunRange.max; ++run) {
        const RunIndex preparedRunIndex = m_output.glyphRuns[run].preparedRunIndex;
        m_output.glyphRuns[run].yOffset =
            baselineY - m_preparedDocument.glyphRuns[preparedRunIndex].verticalAlign;
    }

    // 7. Source coverage and emit LayoutLine.
    const GraphemeRange lineGraphemeRange{ m_lineStart, cutGrapheme };
    CodepointRange lineCodepointRange{ 0, 0 };

    if (lineGraphemeRange.min < m_preparedDocument.graphemeMap.graphemeCount() &&
        lineGraphemeRange.max <= m_preparedDocument.graphemeMap.graphemeCount() &&
        lineGraphemeRange.min < lineGraphemeRange.max) {
        lineCodepointRange = CodepointRange{
            m_preparedDocument.graphemeMap.toCodepoint(lineGraphemeRange.min),
            m_preparedDocument.graphemeMap.codepointRangeForGrapheme(lineGraphemeRange.max - 1).max,
        };
    } else if (lineGraphemeRange.min < m_preparedDocument.graphemeMap.graphemeCount()) {
        const CodepointIndex cp = m_preparedDocument.graphemeMap.toCodepoint(lineGraphemeRange.min);
        lineCodepointRange      = CodepointRange{ cp, cp };
    } else {
        const CodepointIndex totalCp = m_preparedDocument.graphemeMap.codepointCount();
        lineCodepointRange           = CodepointRange{ totalCp, totalCp };
    }

    const LayoutLine layoutLine{
        .graphemeRange  = lineGraphemeRange,
        .codepointRange = lineCodepointRange,
        .baseDirection  = m_baseDirection,
        .glyphRunRange  = glyphRunRange,
        .endKind        = kind,
        .originX        = originX,
        .baselineY      = baselineY,
        .ascent         = maxAscent,
        .descent        = maxDescent,
        .leading        = maxLeading,
        .width          = contentAdvance,
        .trimmedWidth   = trimmedAdvance,
    };

    const LayoutRange lineVerticalBounds{ baselineY - maxAscent, baselineY + maxDescent };
    const LayoutRange lineHorizontalBounds{ originX, originX + contentAdvance };
    const LayoutRange lineHorizontalTrimmedBounds{ originX, originX + trimmedAdvance };

    if (m_output.lines.empty()) {
        m_output.verticalTextBounds          = lineVerticalBounds;
        m_output.horizontalTextBounds        = lineHorizontalBounds;
        m_output.horizontalTrimmedTextBounds = lineHorizontalTrimmedBounds;
    } else {
        m_output.verticalTextBounds   = m_output.verticalTextBounds.union_(lineVerticalBounds);
        m_output.horizontalTextBounds = m_output.horizontalTextBounds.union_(lineHorizontalBounds);
        m_output.horizontalTrimmedTextBounds =
            m_output.horizontalTrimmedTextBounds.union_(lineHorizontalTrimmedBounds);
    }

    m_output.lines.push_back(layoutLine);
    m_paragraphFirstLine = false;

    if (i < m_lineSlices.size() && m_lineSlices[i].rangeInParagraph.min < cutGrapheme) {
        m_lineSlices[i].rangeInParagraph.min = cutGrapheme;
    }
    if (i > 0) {
        m_lineSlices.erase(m_lineSlices.begin(), m_lineSlices.begin() + static_cast<std::ptrdiff_t>(i));
    }
}

DocumentLayout layoutPreparedDocument(const PreparedDocument& document,
                                      std::span<const TextAlignment> alignments, LayoutUnit maxLineWidth,
                                      TabStops tabStops, std::span<const LayoutUnit> textIndents,
                                      bool allowBreakAnywhere) {
    return DocumentGreedyLineBreaker(document, alignments, maxLineWidth, tabStops, textIndents,
                                     allowBreakAnywhere)
        .layoutPreparedDocument();
}

namespace {

struct VisualRun {
    GraphemeRange graphemeRange;
    GraphemeIndex preparedGraphemeMin;
    GlyphRange glyphRange;
    BiDiLevel level;
    LayoutUnit left;
    LayoutUnit right;
};

LayoutUnit visualRunLeft(const PreparedDocument& document, const LayoutGlyphRun& layoutRun) {
    LayoutUnit glyphOrigin = kZero;
    if (!layoutRun.glyphRange.empty()) {
        assert(layoutRun.glyphRange.max <= document.glyphs.size());
        glyphOrigin = document.glyphs[layoutRun.glyphRange.min].xOffset;
        for (GlyphIndex glyph = layoutRun.glyphRange.min + 1; glyph < layoutRun.glyphRange.max; ++glyph) {
            glyphOrigin = std::min(glyphOrigin, document.glyphs[glyph].xOffset);
        }
    }
    return layoutRun.xOffset + glyphOrigin;
}

template <typename Callback>
void forEachVisualRun(const PreparedDocument& document, const DocumentLayout& layout, const LayoutLine& line,
                      Callback&& callback) {
    if (line.glyphRunRange.empty()) {
        return;
    }

    assert(line.glyphRunRange.max <= layout.glyphRuns.size());
    RunIndex layoutRunIndex = line.glyphRunRange.min;
    LayoutUnit left         = visualRunLeft(document, layout.glyphRuns[layoutRunIndex]);
    while (layoutRunIndex < line.glyphRunRange.max) {
        const LayoutGlyphRun& layoutRun = layout.glyphRuns[layoutRunIndex];
        assert(layoutRun.preparedRunIndex < document.glyphRuns.size());
        const GlyphRun& preparedRun       = document.glyphRuns[layoutRun.preparedRunIndex];
        const GraphemeRange graphemeRange = preparedRun.graphemeRange.intersection(line.graphemeRange);
        assert(!graphemeRange.empty());

        const RunIndex nextIndex = layoutRunIndex + 1;
        const LayoutUnit right   = nextIndex < line.glyphRunRange.max
                                       ? visualRunLeft(document, layout.glyphRuns[nextIndex])
                                       : line.originX + line.width;
        callback(VisualRun{ graphemeRange, preparedRun.graphemeRange.min, layoutRun.glyphRange,
                            preparedRun.level, left, right });
        left           = right;
        layoutRunIndex = nextIndex;
    }
}

LayoutUnit proportionalAdvanceDocument(LayoutUnit advance, uint32_t numerator, uint32_t denominator) {
#ifdef LAYOUT_FLOAT
    return advance * static_cast<float>(numerator) / static_cast<float>(denominator);
#else
    return static_cast<LayoutUnit>(static_cast<int64_t>(advance) * numerator / denominator);
#endif
}

LayoutUnit boundaryPosition(const PreparedDocument& document, const VisualRun& run, GraphemeIndex boundary) {
    assert(boundary >= run.graphemeRange.min && boundary <= run.graphemeRange.max);
    LayoutUnit advance = kZero;
    if (run.graphemeRange.distance() == 1 && document.graphemes[run.graphemeRange.min].isTab) {
        advance = boundary == run.graphemeRange.min ? kZero : run.right - run.left;
    } else {
        assert(document.graphemeAdvancePrefix.size() == document.graphemes.size() + 1);
        advance =
            document.graphemeAdvancePrefix[boundary] - document.graphemeAdvancePrefix[run.graphemeRange.min];
        if (boundary == run.graphemeRange.max) {
            return run.level % 2 == 0 ? run.left + advance : run.right - advance;
        }
        const LayoutUnit boundaryAdvance =
            document.graphemeAdvancePrefix[boundary + 1] - document.graphemeAdvancePrefix[boundary];
        if (boundaryAdvance != kZero) {
            return run.level % 2 == 0 ? run.left + advance : run.right - advance;
        }

        GraphemeIndex clusterStart = run.graphemeRange.min;
        GraphemeIndex nextCluster  = run.graphemeRange.max;
        const Glyph* clusterGlyph  = nullptr;
        uint32_t clusterGlyphCount = 0;
        for (GlyphIndex glyphIndex = run.glyphRange.min; glyphIndex < run.glyphRange.max; ++glyphIndex) {
            const Glyph& glyph               = document.glyphs[glyphIndex];
            const GraphemeIndex glyphCluster = run.preparedGraphemeMin + glyph.graphemeIndex;
            if (glyphCluster < boundary && glyphCluster >= clusterStart) {
                if (glyphCluster > clusterStart) {
                    clusterStart      = glyphCluster;
                    clusterGlyph      = &glyph;
                    clusterGlyphCount = 1;
                } else {
                    clusterGlyph = clusterGlyph == nullptr ? &glyph : clusterGlyph;
                    ++clusterGlyphCount;
                }
            }
        }
        for (GlyphIndex glyphIndex = run.glyphRange.min; glyphIndex < run.glyphRange.max; ++glyphIndex) {
            const GraphemeIndex glyphCluster =
                run.preparedGraphemeMin + document.glyphs[glyphIndex].graphemeIndex;
            if (glyphCluster > clusterStart) {
                nextCluster = std::min(nextCluster, glyphCluster);
            }
        }

        const GraphemeIndex componentCount = nextCluster - clusterStart;
        if (clusterGlyph != nullptr && clusterGlyphCount == 1 && componentCount > 1 &&
            boundary < nextCluster) {
            const LayoutUnit clusterAdvance =
                clusterGlyph->xAdvance >= kZero ? clusterGlyph->xAdvance : -clusterGlyph->xAdvance;
            advance = document.graphemeAdvancePrefix[clusterStart] -
                      document.graphemeAdvancePrefix[run.graphemeRange.min] +
                      proportionalAdvanceDocument(clusterAdvance, boundary - clusterStart, componentCount);
        }
    }
    return run.level % 2 == 0 ? run.left + advance : run.right - advance;
}

LineIndex lineForPoint(const DocumentLayout& layout, LayoutUnit y) {
    assert(!layout.lines.empty());
    LineIndex bestLine  = 0;
    double bestDistance = std::numeric_limits<double>::infinity();
    for (LineIndex index = 0; index < layout.lines.size(); ++index) {
        const LayoutLine& line  = layout.lines[index];
        const LayoutUnit top    = line.baselineY - line.ascent;
        const LayoutUnit bottom = line.baselineY + line.descent;
        const double distance   = y < top      ? static_cast<double>(top - y)
                                  : y > bottom ? static_cast<double>(y - bottom)
                                               : 0.0;
        if (distance < bestDistance) {
            bestDistance = distance;
            bestLine     = index;
        }
    }
    return bestLine;
}

size_t paragraphForCaret(const PreparedDocument& document, GraphemeIndex grapheme, CaretAffinity affinity) {
    assert(!document.paragraphs.empty());
    if (affinity == CaretAffinity::Upstream) {
        for (size_t index = 0; index < document.paragraphs.size(); ++index) {
            const auto& paragraph = document.paragraphs[index];
            if (grapheme <= paragraph.graphemeRange.min) {
                return index == 0 ? 0 : index - 1;
            }
            if (grapheme <= paragraph.graphemeRange.max) {
                return index;
            }
        }
        return document.paragraphs.size() - 1;
    }

    for (size_t index = 0; index < document.paragraphs.size(); ++index) {
        const auto& paragraph = document.paragraphs[index];
        if (grapheme < paragraph.graphemeRange.min) {
            // Downstream affinity in a separator gap attaches to the following paragraph.
            return index;
        }
        if (grapheme <= paragraph.graphemeRange.min || grapheme < paragraph.graphemeRange.max) {
            return index;
        }
    }
    return document.paragraphs.size() - 1;
}

LineIndex lineForCaret(const DocumentLayout& layout, size_t paragraphIndex, GraphemeIndex grapheme,
                       CaretAffinity affinity) {
    assert(!layout.lines.empty());
    size_t currentParagraph = 0;
    auto begin              = layout.lines.begin();
    while (currentParagraph < paragraphIndex) {
        while (begin != layout.lines.end() && begin->endKind != LineEndKind::ParagraphEnd) {
            ++begin;
        }
        if (begin != layout.lines.end()) {
            ++begin;
        }
        ++currentParagraph;
    }
    auto end = begin;
    while (end != layout.lines.end() && end->endKind != LineEndKind::ParagraphEnd) {
        ++end;
    }
    if (end != layout.lines.end()) {
        ++end;
    }
    if (begin == end) {
        return static_cast<LineIndex>(layout.lines.size() - 1);
    }
    auto line =
        affinity == CaretAffinity::Upstream
            ? std::lower_bound(begin, end, grapheme,
                               [](const LayoutLine& candidate, GraphemeIndex value) {
                                   return candidate.graphemeRange.max < value;
                               })
            : std::lower_bound(begin, end, grapheme, [](const LayoutLine& candidate, GraphemeIndex value) {
                  return candidate.graphemeRange.max <= value;
              });
    if (line == end) {
        line = end - 1;
    }
    return static_cast<LineIndex>(line - layout.lines.begin());
}

BiDiLevel baseLevel(Direction direction) {
    return direction == Direction::RightToLeft ? BiDiLevel{ 1 } : BiDiLevel{ 0 };
}

} // namespace

CaretPosition caretPosition(const PreparedDocument& document, const DocumentLayout& layout,
                            CaretIndex index) {
    assert(layout.cookie == document.cookie);
    assert(index.grapheme <= document.graphemeMap.graphemeCount());
    assert(!layout.lines.empty());

    const size_t paragraphIndex = paragraphForCaret(document, index.grapheme, index.affinity);
    const LineIndex lineIndex   = lineForCaret(layout, paragraphIndex, index.grapheme, index.affinity);
    const LayoutLine& line      = layout.lines[lineIndex];
    CaretPosition result{ line.originX, lineIndex, baseLevel(line.baseDirection) };
    const GraphemeIndex attachedGrapheme =
        index.affinity == CaretAffinity::Upstream && index.grapheme > 0 ? index.grapheme - 1 : index.grapheme;
    bool found             = false;
    CaretPosition fallback = result;
    bool hasFallback       = false;
    forEachVisualRun(document, layout, line, [&](const VisualRun& run) {
        if (run.graphemeRange.contains(attachedGrapheme)) {
            result.x     = boundaryPosition(document, run, index.grapheme);
            result.level = run.level;
            found        = true;
        } else if (run.graphemeRange.min == index.grapheme || run.graphemeRange.max == index.grapheme) {
            fallback.x     = boundaryPosition(document, run, index.grapheme);
            fallback.level = run.level;
            hasFallback    = true;
        }
    });
    return !found && hasFallback ? fallback : result;
}

CaretIndex hitTestPoint(const PreparedDocument& document, const DocumentLayout& layout, LayoutUnit x,
                        LayoutUnit y) {
    assert(layout.cookie == document.cookie);
    assert(!layout.lines.empty());
    const LineIndex lineIndex = lineForPoint(layout, y);
    const LayoutLine& line    = layout.lines[lineIndex];
    CaretIndex best{ line.graphemeRange.min, CaretAffinity::Downstream };
    double bestDistance = std::numeric_limits<double>::infinity();
    forEachVisualRun(document, layout, line, [&](const VisualRun& run) {
        for (GraphemeIndex boundary = run.graphemeRange.min; boundary <= run.graphemeRange.max; ++boundary) {
            const LayoutUnit candidateX = boundaryPosition(document, run, boundary);
            const double distance       = std::abs(static_cast<double>(x) - static_cast<double>(candidateX));
            if (distance < bestDistance) {
                bestDistance = distance;
                best         = { boundary, boundary == run.graphemeRange.max ? CaretAffinity::Upstream
                                                                             : CaretAffinity::Downstream };
            }
        }
    });
    return best;
}

void selectionRects(const PreparedDocument& document, const DocumentLayout& layout, GraphemeRange selection,
                    function_ref<void(const SelectionRect&)> onRect) {
    assert(layout.cookie == document.cookie);
    if (selection.min > selection.max || selection.max > document.graphemeMap.graphemeCount() ||
        selection.empty()) {
        return;
    }
    for (LineIndex lineIndex = 0; lineIndex < layout.lines.size(); ++lineIndex) {
        const LayoutLine& line           = layout.lines[lineIndex];
        const GraphemeRange selectedLine = line.graphemeRange.intersection(selection);
        if (selectedLine.empty()) {
            continue;
        }
        forEachVisualRun(document, layout, line, [&](const VisualRun& run) {
            const GraphemeRange selectedRun = run.graphemeRange.intersection(selection);
            if (selectedRun.empty()) {
                return;
            }
            const LayoutUnit a = boundaryPosition(document, run, selectedRun.min);
            const LayoutUnit b = boundaryPosition(document, run, selectedRun.max);
            onRect(SelectionRect{ lineIndex, std::min(a, b), std::max(a, b) });
        });
    }
}

bool rasterize(const ActiveFont& activeFont, const FontHandle& handle, uint32_t glyphId,
               const RasterizationOptions& options,
               function_ref<void(const RasterizedGlyph&, const uint8_t*)> callback) {
    if (options.horizontalScale <= 0 || !handle || !activeFont.hbFont) {
        return false;
    }
    FT_Face ftFace = static_cast<FT_Face>(activeFont.ftFace);
    FT_Set_Transform(ftFace, nullptr, nullptr);

    if (FT_Load_Glyph(ftFace, glyphId, activeFont.loadFlags | FT_LOAD_COLOR) != 0)
        return false;
    if (ftFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
        FT_Matrix matrix{ static_cast<FT_Fixed>(0x10000 * options.horizontalScale), 0, 0, 0x10000 };
        FT_Outline_Transform(&ftFace->glyph->outline, &matrix);
        FT_Vector_Transform(&ftFace->glyph->advance, &matrix);
    } else if (ftFace->glyph->format == FT_GLYPH_FORMAT_SVG) {
        //
    } else {
        return false;
    }
    if (FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL) != 0)
        return false;
    const FT_Bitmap& bitmap = ftFace->glyph->bitmap;
    const auto format       = bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? RasterizedGlyph::Format::BGRA8
                                                                      : RasterizedGlyph::Format::Mask8;
    if (!((bitmap.pixel_mode == FT_PIXEL_MODE_GRAY || bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) &&
          (bitmap.buffer || bitmap.width == 0 || bitmap.rows == 0)))
        return false;
    callback(
        RasterizedGlyph{
            ftFace->glyph->bitmap_left,
            ftFace->glyph->bitmap_top,
            bitmap.width,
            bitmap.rows,
            bitmap.pitch,
            format,
            format == RasterizedGlyph::Format::BGRA8 ? 4u : 1u,
            bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 1 : options.horizontalScale,
        },
        bitmap.buffer);
    return true;
}

} // namespace Brisk::TextLayout
