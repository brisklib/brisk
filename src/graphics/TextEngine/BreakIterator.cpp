#include "text_layout/BreakIterator.hpp"

#include <linebreak.h>
#include <wordbreak.h>
extern "C" {
#include <SheenBidi/SheenBidi.h>
}
#include <utf8proc.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace Brisk::TextEngine {

void BreakIterator::setText(std::u32string_view text) {
    const size_t clampedSize = std::min<size_t>(text.size(), maxTextLength);
    m_text                   = text.substr(0, clampedSize);
    m_lastBreakPos           = unstarted;
    doSetText();
}

void BreakIterator::reset() {
    m_lastBreakPos = unstarted;
    doSetText();
}

uint32_t BreakIterator::countBreaks() {
    reset();
    uint32_t count = 0;
    while (nextBreak() != end) {
        ++count;
    }
    reset();
    return count;
}

uint32_t BreakIterator::nextBreak() {
    if (m_lastBreakPos == unstarted) {
        m_lastBreakPos = 0;
        return 0;
    }
    uint32_t b     = doNextBreak();
    m_lastBreakPos = b;
    return b;
}

std::u32string_view BreakIterator::nextFragment() {
    if (m_lastBreakPos == unstarted) {
        uint32_t first = nextBreak();
        if (first == end) {
            return {};
        }
    }

    if (m_lastBreakPos == end) {
        return {};
    }

    uint32_t start  = m_lastBreakPos;
    uint32_t endPos = nextBreak();
    if (endPos == end) {
        return {};
    }

    if (start >= m_text.size() || endPos < start) {
        return {};
    }

    return m_text.substr(start, endPos - start);
}

LineBreakIterator::LineBreakIterator(std::string lang, Mode mode) : m_lang(std::move(lang)), m_mode(mode) {}

void LineBreakIterator::doSetText() {
    m_breaks.clear();
    m_currentIndex     = 0;

    const uint32_t len = static_cast<uint32_t>(m_text.size());

    if (len == 0) {
        return;
    }

    std::vector<char> brks(len);
    const char* langPtr = m_lang.empty() ? nullptr : m_lang.c_str();

    set_linebreaks_utf32(reinterpret_cast<const utf32_t*>(m_text.data()), len, langPtr, brks.data());

    for (uint32_t i = 0; i < len; ++i) {
        const bool isBreak = (brks[i] == LINEBREAK_MUSTBREAK) ||
                             (m_mode == Mode::MandatoryAndAllowed && brks[i] == LINEBREAK_ALLOWBREAK);
        if (isBreak) {
            uint32_t breakPos = i + 1;
            if (breakPos < len) {
                m_breaks.push_back(breakPos);
            }
        }
    }

    m_breaks.push_back(len);
}

uint32_t LineBreakIterator::countBreaks() {
    reset();
    return static_cast<uint32_t>(1 + m_breaks.size());
}

uint32_t LineBreakIterator::doNextBreak() {
    if (m_currentIndex < m_breaks.size()) {
        return m_breaks[m_currentIndex++];
    }
    return UINT32_MAX;
}

void GraphemeBreakIterator::doSetText() {
    m_state = 0;
}

uint32_t GraphemeBreakIterator::doNextBreak() {
    const uint32_t len = static_cast<uint32_t>(m_text.size());

    if (m_lastBreakPos >= len) {
        return UINT32_MAX;
    }

    uint32_t pos = m_lastBreakPos;
    while (pos < len - 1) {
        utf8proc_int32_t cp1 = static_cast<utf8proc_int32_t>(m_text[pos]);
        utf8proc_int32_t cp2 = static_cast<utf8proc_int32_t>(m_text[pos + 1]);
        ++pos;
        if (utf8proc_grapheme_break_stateful(cp1, cp2, &m_state)) {
            return pos;
        }
    }

    return len;
}

void ParagraphBreakIterator::doSetText() {}

uint32_t ParagraphBreakIterator::doNextBreak() {
    const uint32_t len = static_cast<uint32_t>(m_text.size());

    if (m_lastBreakPos >= len) {
        return UINT32_MAX;
    }

    uint32_t pos = m_lastBreakPos;
    while (pos < len) {
        char32_t cp = m_text[pos];
        if (cp == 0x000D) { // CR
            // Lookahead for CRLF
            if (pos + 1 < len && m_text[pos + 1] == 0x000A) {
                pos += 2;
            } else {
                pos += 1;
            }
            return pos;
        }
        if (cp == 0x000A || (cp >= 0x001C && cp <= 0x001E) || cp == 0x0085 || cp == 0x2029) {
            pos += 1;
            return pos;
        }
        ++pos;
    }

    return len;
}

namespace {

constexpr BiDiRun emptyRun{};

static bool isWordCharacter(char32_t cp) {
    utf8proc_category_t cat = utf8proc_category(static_cast<utf8proc_int32_t>(cp));
    switch (cat) {
    case UTF8PROC_CATEGORY_LU: // Letter, uppercase
    case UTF8PROC_CATEGORY_LL: // Letter, lowercase
    case UTF8PROC_CATEGORY_LT: // Letter, titlecase
    case UTF8PROC_CATEGORY_LM: // Letter, modifier
    case UTF8PROC_CATEGORY_LO: // Letter, other
    case UTF8PROC_CATEGORY_ND: // Number, decimal digit
    case UTF8PROC_CATEGORY_NL: // Number, letter
    case UTF8PROC_CATEGORY_NO: // Number, other
        return true;
    default:
        return false;
    }
}

} // namespace

void WordBreakIterator::doSetText() {
    m_breaks.clear();
    m_currentIndex     = 0;

    const uint32_t len = static_cast<uint32_t>(m_text.size());
    if (len == 0) {
        return;
    }

    std::vector<char> breaks(len);
    set_wordbreaks_utf32(reinterpret_cast<const utf32_t*>(m_text.data()), len, nullptr, breaks.data());

    for (uint32_t i = 0; i + 1 < len; ++i) {
        if (breaks[i] == WORDBREAK_BREAK && isWordCharacter(m_text[i + 1])) {
            m_breaks.push_back(i + 1);
        }
    }
    m_breaks.push_back(len);
}

uint32_t WordBreakIterator::countBreaks() {
    reset();
    return static_cast<uint32_t>(1 + m_breaks.size());
}

uint32_t WordBreakIterator::doNextBreak() {
    if (m_currentIndex < m_breaks.size()) {
        return m_breaks[m_currentIndex++];
    }
    return UINT32_MAX;
}

BiDiIterator::BiDiIterator(BaseDirection baseDirection) : m_baseDirection(baseDirection) {}

void BiDiIterator::setText(std::u32string_view text, BaseDirection baseDirection) {
    m_baseDirection = baseDirection;
    BreakIterator::setText(text);
}

const BiDiRun& BiDiIterator::nextRun() {
    if (m_lastBreakPos == unstarted) {
        nextBreak();
    }
    if (m_currentRunIndex < m_runs.size()) {
        const BiDiRun& run = m_runs[m_currentRunIndex];
        nextBreak();
        return run;
    }
    return emptyRun;
}

const BiDiRun& BiDiIterator::currentRun() const noexcept {
    if (m_lastBreakPos != end && m_currentRunIndex > 0 && m_currentRunIndex <= m_runs.size()) {
        return m_runs[m_currentRunIndex - 1];
    }
    return emptyRun;
}

std::u32string_view BiDiIterator::currentFragment() const noexcept {
    const BiDiRun& run = currentRun();
    if (run.range.empty() || run.range.min >= m_text.size()) {
        return {};
    }
    return m_text.substr(run.range.min, run.range.distance());
}

void BiDiIterator::doSetText() {
    m_runs.clear();
    m_currentRunIndex  = 0;
    m_paragraphLevel   = 0;

    const uint32_t len = static_cast<uint32_t>(m_text.size());
    if (len == 0) {
        if (m_baseDirection == BaseDirection::RightToLeft || m_baseDirection == BaseDirection::DefaultRTL) {
            m_paragraphLevel = 1;
        } else {
            m_paragraphLevel = 0;
        }
        return;
    }

    SBCodepointSequence codepoints;
    codepoints.stringEncoding = SBStringEncodingUTF32;
    codepoints.stringBuffer   = const_cast<void*>(static_cast<const void*>(m_text.data()));
    codepoints.stringLength   = len;

    SBAlgorithmRef algorithm  = SBAlgorithmCreate(&codepoints);
    if (!algorithm) {
        return;
    }

    SBLevel sbBaseLevel = SBLevelDefaultLTR;
    switch (m_baseDirection) {
    case BaseDirection::DefaultLTR:
        sbBaseLevel = SBLevelDefaultLTR;
        break;
    case BaseDirection::DefaultRTL:
        sbBaseLevel = SBLevelDefaultRTL;
        break;
    case BaseDirection::LeftToRight:
        sbBaseLevel = 0;
        break;
    case BaseDirection::RightToLeft:
        sbBaseLevel = 1;
        break;
    }

    SBParagraphRef paragraph = SBAlgorithmCreateParagraph(algorithm, 0, len, sbBaseLevel);
    if (paragraph) {
        m_paragraphLevel = static_cast<uint8_t>(SBParagraphGetBaseLevel(paragraph));

        SBLineRef line   = SBParagraphCreateLine(paragraph, 0, len);
        if (line) {
            SBUInteger runCount = SBLineGetRunCount(line);
            const SBRun* sbRuns = SBLineGetRunsPtr(line);

            m_runs.reserve(runCount);
            for (SBUInteger i = 0; i < runCount; ++i) {
                BiDiRun run;
                const auto offset = static_cast<uint32_t>(sbRuns[i].offset);
                run.range       = Range<uint32_t>{ offset, offset + static_cast<uint32_t>(sbRuns[i].length) };
                run.visualIndex = static_cast<uint32_t>(i);
                run.level       = static_cast<uint8_t>(sbRuns[i].level);
                m_runs.push_back(run);
            }

            // Ensure runs are in logical order sorted by offset
            std::sort(m_runs.begin(), m_runs.end(), [](const BiDiRun& a, const BiDiRun& b) {
                return a.range.min < b.range.min;
            });

            SBLineRelease(line);
        }

        SBParagraphRelease(paragraph);
    }

    SBAlgorithmRelease(algorithm);
}

uint32_t BiDiIterator::doNextBreak() {
    if (m_currentRunIndex < m_runs.size()) {
        const auto& run = m_runs[m_currentRunIndex++];
        return run.range.max;
    }
    return UINT32_MAX;
}

} // namespace Brisk::TextEngine
