#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <brisk/core/BasicTypes.hpp>

namespace Brisk::TextLayout {

/**
 * @brief Abstract base class for Unicode text boundary iterators.
 *
 * Traverses boundary positions in UTF-32 text. Iteration protocol:
 * 1. The first call to nextBreak() returns index 0.
 * 2. Subsequent calls return intermediate boundary positions in ascending order.
 * 3. The final boundary returned is the text length (clamped to maxTextLength).
 * 4. Further calls return UINT32_MAX to indicate the end of iteration.
 *
 * @note The referenced text buffer must outlive the BreakIterator instance or until setText() is called
 * again.
 */
class BreakIterator {
public:
    /// Maximum number of UTF-32 codepoints processed by the iterator (256M).
    static constexpr uint32_t maxTextLength = 256 * 1024 * 1024; // 256M codepoints

    virtual ~BreakIterator()                = default;

    /**
     * @brief Sets the text to iterate over and resets the iterator state.
     * @param text The UTF-32 text view to process. Must outlive iteration.
     */
    void setText(std::u32string_view text);

    /**
     * @brief Resets the iterator state to the beginning of the currently set text.
     */
    void reset();

    /**
     * @brief Counts the total number of boundaries in the text and resets the iterator state.
     *
     * After counting, the iterator is reset back to the beginning of the text.
     * @return The total number of break positions in the current text.
     */
    virtual uint32_t countBreaks();

    /**
     * @brief Advances to and returns the next boundary offset.
     * @return Codepoint offset of the next boundary, or UINT32_MAX if iteration is complete.
     */
    uint32_t nextBreak();

    /**
     * @brief Returns the next text fragment bounded by consecutive break positions.
     *
     * Automatically advances the break position if not yet started.
     * @return A substring view between consecutive breaks, or an empty view if finished.
     */
    std::u32string_view nextFragment();

protected:
    /**
     * @brief Implementation hook for setting text and initializing subclass state.
     */
    virtual void doSetText()       = 0;

    /**
     * @brief Implementation hook for computing the next boundary position.
     * @return Next boundary offset, or UINT32_MAX when complete.
     */
    virtual uint32_t doNextBreak() = 0;

    std::u32string_view m_text{}; ///< Clamped UTF-32 text view being iterated over.

    static constexpr uint32_t end       = UINT32_MAX;
    static constexpr uint32_t unstarted = UINT32_MAX - 1;

    uint32_t m_lastBreakPos{ unstarted };
};

/**
 * @brief Line-break iterator implementing Unicode Standard Annex #14 (UAX #14).
 *
 * Breaks text into line segments based on language rules and break opportunity modes.
 */
class LineBreakIterator : public BreakIterator {
public:
    /**
     * @brief Break opportunity reporting mode.
     */
    enum class Mode {
        MandatoryOnly,      ///< Report mandatory (hard) line breaks only.
        MandatoryAndAllowed ///< Report both mandatory (hard) and allowed (soft) line breaks.
    };

    /**
     * @brief Constructs a line break iterator with the given language and mode.
     * @param lang BCP 47 language tag or ISO 639 code (e.g. "en", "zh") for tailoring, or empty for default.
     * @param mode Whether to report mandatory breaks only or both mandatory and allowed breaks.
     */
    explicit LineBreakIterator(std::string lang = {}, Mode mode = Mode::MandatoryAndAllowed);
    ~LineBreakIterator() override = default;

    uint32_t countBreaks() override;

protected:
    void doSetText() override;
    uint32_t doNextBreak() override;

private:
    std::string m_lang;
    Mode m_mode;
    std::vector<uint32_t> m_breaks;
    size_t m_currentIndex{ 0 };
};

/**
 * @brief Extended grapheme cluster boundary iterator implementing Unicode Standard Annex #29 (UAX #29).
 *
 * Iterates over user-perceived character boundaries, correctly handling combining marks,
 * emoji modifiers, and Zero Width Joiner (ZWJ) sequences.
 */
class GraphemeBreakIterator : public BreakIterator {
public:
    GraphemeBreakIterator()           = default;
    ~GraphemeBreakIterator() override = default;

protected:
    void doSetText() override;
    uint32_t doNextBreak() override;

private:
    int32_t m_state{ 0 };
};

/**
 * @brief Paragraph boundary iterator.
 *
 * Identifies paragraph boundaries delineated by LF (U+000A), CR (U+000D), CRLF pairs
 * (treated atomically), U+001C–U+001E (IS4–IS2), NEL (U+0085), or Paragraph Separator PS (U+2029).
 */
class ParagraphBreakIterator : public BreakIterator {
public:
    ParagraphBreakIterator()           = default;
    ~ParagraphBreakIterator() override = default;

protected:
    void doSetText() override;
    uint32_t doNextBreak() override;
};

/**
 * @brief Word boundary iterator using libunibreak's UAX #29 implementation.
 *
 * Segments text into words, attaching any trailing non-word characters (spaces, punctuation, symbols)
 * to the preceding word. Leading non-word characters form their own fragment.
 */
class WordBreakIterator : public BreakIterator {
public:
    WordBreakIterator()           = default;
    ~WordBreakIterator() override = default;

    uint32_t countBreaks() override;

protected:
    void doSetText() override;
    uint32_t doNextBreak() override;

private:
    std::vector<uint32_t> m_breaks;
    size_t m_currentIndex{ 0 };
};

/**
 * @brief Base paragraph direction policy for Unicode Bidirectional Algorithm.
 */
enum class BaseDirection : uint8_t {
    DefaultLTR,  ///< Auto-detect base direction from first strong character; defaults to LTR if none.
    DefaultRTL,  ///< Auto-detect base direction from first strong character; defaults to RTL if none.
    LeftToRight, ///< Force Left-to-Right base direction (level 0).
    RightToLeft  ///< Force Right-to-Left base direction (level 1).
};

using BiDiLevel = uint8_t;

/**
 * @brief Direction of a resolved BiDi run.
 */
enum class Direction : uint8_t {
    LeftToRight = 0, ///< Left-to-Right direction (even embedding level).
    RightToLeft = 1  ///< Right-to-Left direction (odd embedding level).
};

/**
 * @brief Maps a BiDi embedding level to its resolved Direction.
 */
[[nodiscard]] constexpr Direction directionFromLevel(BiDiLevel level) noexcept {
    return (level & 1) != 0 ? Direction::RightToLeft : Direction::LeftToRight;
}

/**
 * @brief Information about a resolved unidirectional text run from BiDi analysis.
 */
struct BiDiRun {
    Range<uint32_t> range{};   ///< Half-open codepoint range of the run within the text.
    uint32_t visualIndex{ 0 }; ///< 0-based visual index of the run (display order from left to right).
    uint8_t level{ 0 };        ///< Resolved Unicode BiDi embedding level.

    /// Returns true if this run has Right-to-Left direction (odd level).
    [[nodiscard]] constexpr bool isRTL() const noexcept {
        return (level & 1) != 0;
    }

    /// Returns true if this run has Left-to-Right direction (even level).
    [[nodiscard]] constexpr bool isLTR() const noexcept {
        return (level & 1) == 0;
    }

    /// Returns the Direction enum for this run.
    [[nodiscard]] constexpr Direction direction() const noexcept {
        return directionFromLevel(level);
    }
};

/**
 * @brief Unicode Bidirectional Algorithm (UAX #9) iterator.
 *
 * Follows the BreakIterator iteration protocol to traverse BiDi runs in logical order:
 * 1. The first call to nextBreak() returns index 0.
 * 2. Subsequent calls return boundary positions between runs of differing embedding levels,
 *    ending at the text length.
 * 3. Further calls return UINT32_MAX to indicate the end of iteration.
 *
 * In addition to break positions and text fragments, callers can query the resolved
 * BiDiRun descriptor or embedding level of the current fragment, or use nextRun() directly.
 *
 * @note The referenced text buffer must outlive the BiDiIterator instance or until setText()
 * is called again.
 */
class BiDiIterator : public BreakIterator {
public:
    /**
     * @brief Constructs a BiDi iterator.
     * @param baseDirection The default base direction policy to use.
     */
    explicit BiDiIterator(BaseDirection baseDirection = BaseDirection::DefaultLTR);
    ~BiDiIterator() override = default;

    /**
     * @brief Sets the text to process and the paragraph base direction policy.
     * @param text One UTF-32 paragraph to process. It must not contain paragraph separators
     * (LF, CR, CRLF, or PS); use ParagraphBreakIterator to segment multi-paragraph text first.
     * @param baseDirection The base direction policy for BiDi resolution.
     */
    void setText(std::u32string_view text, BaseDirection baseDirection = BaseDirection::DefaultLTR);

    /**
     * @brief Advances to and returns the next unidirectional BiDi run.
     * @return Const reference to the next BiDiRun, or a run with an empty range when iteration is complete.
     */
    const BiDiRun& nextRun();

    /**
     * @brief Returns the resolved BiDi run for the most recently retrieved fragment/break.
     * @return Const reference to the current BiDiRun, or a static empty run if unstarted or finished.
     */
    [[nodiscard]] const BiDiRun& currentRun() const noexcept;

    /**
     * @brief Returns the text fragment corresponding to the current BiDi run.
     * @return A substring view of the current run, or an empty view if unstarted or finished.
     */
    [[nodiscard]] std::u32string_view currentFragment() const noexcept;

    /**
     * @brief Returns the base direction policy configured for this iterator.
     */
    [[nodiscard]] BaseDirection baseDirection() const noexcept {
        return m_baseDirection;
    }

    /**
     * @brief Returns the resolved paragraph base embedding level (0 for LTR, 1 for RTL).
     */
    [[nodiscard]] uint8_t paragraphLevel() const noexcept {
        return m_paragraphLevel;
    }

protected:
    void doSetText() override;
    uint32_t doNextBreak() override;

private:
    BaseDirection m_baseDirection{ BaseDirection::DefaultLTR };
    uint8_t m_paragraphLevel{ 0 };
    std::vector<BiDiRun> m_runs;
    size_t m_currentRunIndex{ 0 };
};

} // namespace Brisk::TextLayout
