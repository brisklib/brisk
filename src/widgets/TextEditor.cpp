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
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/Item.hpp>
#include <brisk/widgets/ContextPopup.hpp>
#include <brisk/widgets/Spacer.hpp>
#include <brisk/core/Text.hpp>
#include "utf8proc.h"
#include <brisk/graphics/Palette.hpp>
#include <brisk/window/Clipboard.hpp>
#include <brisk/gui/Icons.hpp>
#include <brisk/core/Localization.hpp>

namespace Brisk {

static std::u32string normalizeCompose(std::u32string str) {
    return str;
}

TextEditor::TextEditor(Construction construction, ArgumentsView<TextEditor> args)
    : Base(construction, nullptr) {
    m_tabStop       = true;
    m_processClicks = false;
    m_boxSizing     = BoxSizingPerAxis::ContentBoxY;
    args.apply(this);
    createContextMenu();
}

void TextEditor::createContextMenu() {
    apply(rcnew ContextPopup{
        Arg::role       = "context",
        Arg::fontFamily = Font::DefaultPlusIconsEmoji,
        Arg::fontSize   = FontSize::Normal,
        rcnew Item{
            Arg::icon = ICON_scissors,
            rcnew Text{ "Cut||Menu"_tr },
            rcnew Spacer{},
            rcnew Text{
                hotKeyToString(KeyCode::X, KeyModifiers::ControlOrCommand),
                Arg::classes = { "hotkeyhint" },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               cutToClipboard();
                           },
        },
        rcnew Item{
            Arg::icon = ICON_copy,
            rcnew Text{ "Copy||Menu"_tr },
            rcnew Spacer{},
            rcnew Text{
                hotKeyToString(KeyCode::C, KeyModifiers::ControlOrCommand),
                Arg::classes = { "hotkeyhint" },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               copyToClipboard();
                           },
        },
        rcnew Item{
            Arg::icon = ICON_clipboard,
            rcnew Text{ "Paste||Menu"_tr },
            rcnew Spacer{},
            rcnew Text{
                hotKeyToString(KeyCode::V, KeyModifiers::ControlOrCommand),
                Arg::classes = { "hotkeyhint" },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               pasteFromClipboard();
                           },
        },
        rcnew Item{
            Arg::icon = ICON_x,
            rcnew Text{ "Delete||Menu"_tr },
            rcnew Spacer{},
            rcnew Text{
                hotKeyToString(KeyCode::Del, KeyModifiers::None),
                Arg::classes = { "hotkeyhint" },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               deleteSelection();
                           },
        },
        rcnew Item{
            rcnew Text{ "Select All||Menu"_tr },
            rcnew Spacer{},
            rcnew Text{
                hotKeyToString(KeyCode::A, KeyModifiers::ControlOrCommand),
                Arg::classes = { "hotkeyhint" },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               selectAll();
                           },
        },
    });
}

Range<uint32_t> TextEditor::selection() const {
    return { std::min(cursor, cursor + selectedLength), std::max(cursor, cursor + selectedLength) };
}

void TextEditor::moveCursor(MoveCursor move, bool select) {
    uint32_t oldCursor = cursor;
    using enum MoveCursor;
    switch (move) {
    case Up:
    case Down: {
        uint32_t grapheme = m_preparedText.characterToGrapheme(cursor);
        uint32_t line     = m_preparedText.graphemeToLine(grapheme);
        float offsetx     = m_preparedText.caretPositions[grapheme];
        if (line == UINT32_MAX || (line == 0 && move == Up) ||
            (line == m_preparedText.lines.size() - 1 && move == Down))
            break;
        line += move == Down ? +1 : -1;
        grapheme = m_preparedText.caretToGrapheme(line, offsetx);
        cursor   = m_preparedText.graphemeToCharacter(grapheme);
        break;
    }

    case Right:
    case Left: {
        uint32_t grapheme = m_preparedText.characterToGrapheme(cursor);
        if ((grapheme == 0 && move == Left) ||
            (grapheme == m_preparedText.graphemeBoundaries.size() - 1 && move == Right))
            break;
        grapheme += move == Right ? +1 : -1;
        cursor = m_preparedText.graphemeToCharacter(grapheme);
        break;
    }

    case LineBeginning:
    case LineEnd: {
        uint32_t grapheme = m_preparedText.characterToGrapheme(cursor);
        uint32_t line     = m_preparedText.graphemeToLine(grapheme);
        if (line == UINT32_MAX)
            break;
        cursor = m_preparedText.graphemeToCharacter(move == LineBeginning
                                                        ? m_preparedText.lines[line].graphemeRange.min
                                                        : m_preparedText.lines[line].graphemeRange.max - 1);
        break;
    }

    case TextBeginning:
        cursor = 0;
        break;
    case TextEnd:
        cursor = m_cachedText.size();
        break;
    }

    if (select) {
        selectedLength += oldCursor - cursor;
    } else {
        selectedLength = 0;
    }
    selectionChanged();
}

void TextEditor::paint(Canvas& canvas) const {
    paintBackground(canvas, m_rect);
    Font font                  = this->font();

    std::u32string placeholder = utf8ToUtf32(this->m_placeholder);
    bool isPlaceholder         = m_text.empty();

    Range<uint32_t> selection  = this->selection();
    selection.min              = std::clamp(selection.min, 0u, (uint32_t)m_cachedText.size());
    selection.max              = std::clamp(selection.max, 0u, (uint32_t)m_cachedText.size());

    PointF alignment{ toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };

    ColorW textColor  = m_color.current;
    m_alignmentOffset = m_preparedText.alignLines(alignment);
    if (isPlaceholder)
        textColor = textColor.multiplyAlpha(0.5f);

    canvas.setFillColor(ColorW(Palette::Standard::indigo).multiplyAlpha(isFocused() ? 0.85f : 0.5f));
    Point pos = m_clientRect.at(alignment) + Point(m_alignmentOffset - m_visibleOffset);
    canvas.fillTextSelection(pos, m_preparedText, selection);
    canvas.setFillColor(textColor);
    canvas.fillText(pos, m_preparedText);

    if (isFocused() && std::fmod(frameStartTime - m_blinkTime, 1.0) < 0.5) {
        uint32_t caretGrapheme =
            m_preparedText.characterToGrapheme(std::clamp(cursor, 0u, uint32_t(m_cachedText.size())));

        uint32_t lineIndex = m_preparedText.graphemeToLine(caretGrapheme);
        if (lineIndex != UINT32_MAX) {
            const auto& line = m_preparedText.lines[lineIndex];
            Rectangle caretRect(m_clientRect.at(alignment) + Point(m_alignmentOffset - m_visibleOffset) +
                                    Point(m_preparedText.caretPositions[caretGrapheme], line.baseline) +
                                    Point(0, -line.ascDesc.ascender),
                                Size(1_idp, line.ascDesc.height()));
            canvas.setFillColor(textColor);
            canvas.fillRect(caretRect, 0.f);
        }
    }
}

void TextEditor::normalizeCursor(uint32_t textLen) {
    uint32_t newCursor         = std::clamp(cursor, 0u, textLen);
    uint32_t newSelectedLength = std::clamp(cursor + selectedLength, 0u, textLen) - cursor;
    if (newCursor != cursor || newSelectedLength != selectedLength) {
        cursor         = newCursor;
        selectedLength = newSelectedLength;
        selectionChanged();
    }
}

void TextEditor::makeCursorVisible(uint32_t textLen) {
    if (!m_preparedText.hasCaretData() || m_preparedText.caretPositions.size() <= 1) {
        m_visibleOffset = { 0, 0 };
        invalidate();
        return;
    }
    SizeF bounds = m_preparedText.bounds().size();
    if (std::ceil(bounds.width) < m_clientRect.width() && std::ceil(bounds.height) < m_clientRect.height()) {
        m_visibleOffset = { 0, 0 };
        invalidate();
        return;
    }

    uint32_t grapheme  = m_preparedText.characterToGrapheme(cursor);
    uint32_t lineIndex = m_preparedText.graphemeToLine(grapheme);
    if (lineIndex == UINT32_MAX)
        return;
    const auto& line = m_preparedText.lines[lineIndex];
    float caretx     = m_preparedText.caretPositions[grapheme];
    float carety     = line.baseline;

    m_visibleOffset -= m_alignmentOffset;

    if (caretx < m_visibleOffset.x)
        m_visibleOffset.x = std::floor(caretx) - 2_idp;
    else if (caretx > m_visibleOffset.x + m_clientRect.width())
        m_visibleOffset.x = std::ceil(caretx) - m_clientRect.width() + 2_idp;
    if (carety - line.ascDesc.ascender < m_visibleOffset.y)
        m_visibleOffset.y = std::floor(carety - line.ascDesc.ascender) - 2_idp;
    else if (carety + line.ascDesc.descender > m_visibleOffset.y + m_clientRect.height())
        m_visibleOffset.y = std::ceil(carety + line.ascDesc.descender) - m_clientRect.height() + 2_idp;

    m_visibleOffset += m_alignmentOffset;
    m_visibleOffset.x = std::max(0, m_visibleOffset.x);
    m_visibleOffset.y = std::max(0, m_visibleOffset.y);
    invalidate();
}

uint32_t TextEditor::caretToOffset(PointF pt) const {
    if (!m_preparedText.hasCaretData() || m_preparedText.caretPositions.size() == 1) {
        return 0;
    }
    PointF alignment{ toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };
    return m_preparedText.graphemeToCharacter(m_preparedText.caretToGrapheme(
        Point(pt) - (m_clientRect.at(alignment) + Point(m_alignmentOffset - m_visibleOffset))));
}

static bool char_is_alphanum(char32_t ch) {
    utf8proc_category_t cat = utf8proc_category(ch);
    return cat >= UTF8PROC_CATEGORY_LU && cat <= UTF8PROC_CATEGORY_NO;
}

void TextEditor::selectWordAtCursor() {
    std::u32string text = utf8ToUtf32(m_text);
    normalizeCursor(text.size());
    const int cursorPos = cursor;
    for (int i = cursorPos;; i--) {
        if (i < 0 || !char_is_alphanum(text[i])) {
            cursor = i + 1;
            break;
        }
    }
    for (int i = cursorPos;; i++) {
        if (i >= text.size() || !char_is_alphanum(text[i])) {
            selectedLength = i - cursor;
            break;
        }
    }
    selectedLength = -selectedLength;
    cursor         = cursor - selectedLength;
    selectionChanged();
    normalizeCursor(text.size());
}

void TextEditor::onEvent(Event& event) {
    Base::onEvent(event);
    const Rectangle textRect = m_clientRect;
    std::u32string text;
    if (event.doubleClicked()) {
        selectWordAtCursor();
        event.stopPropagation();
    } else if (event.tripleClicked()) {
        selectAll(utf8ToUtf32(m_text));
        event.stopPropagation();
    } else if (auto e = event.as<EventFocused>()) {
        if (e->keyboard) {
            selectAll();
        }
    }
    switch (const auto [flag, offset, mods] = event.dragged(m_mouseSelection); flag) {
    case DragEvent::Started: {
        text        = utf8ToUtf32(m_text);
        m_blinkTime = frameStartTime;
        focus();
        cursor         = caretToOffset(Point(*event.as<EventMouse>()->downPoint));
        selectedLength = 0;
        selectionChanged();
        normalizeCursor(text.size());
        m_startCursorDragging = caretToOffset(Point(*event.as<EventMouse>()->downPoint));
        event.stopPropagation();
    } break;
    case DragEvent::Dragging: {
        text                = utf8ToUtf32(m_text);
        m_blinkTime         = frameStartTime;
        const int endCursor = caretToOffset(Point(event.as<EventMouse>()->point));
        selectedLength      = m_startCursorDragging - endCursor;
        cursor              = endCursor;
        selectionChanged();
        normalizeCursor(text.size());
        invalidate();
        event.stopPropagation();
    } break;
    case DragEvent::Dropped:
        event.stopPropagation();
        break;
    default:
        break;
    }

    if (event.type() == EventType::KeyPressed || event.type() == EventType::CharacterTyped) {
        text        = utf8ToUtf32(m_text);

        m_blinkTime = frameStartTime;
        normalizeCursor(text.size());
        if (auto ch = event.as<EventCharacterTyped>()) {
            typeCharacter(text, ch->character);
            event.stopPropagation();
        } else {
            switch (auto e = event.as<EventKeyPressed>(); e->key) {
            case KeyCode::A:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand) {
                    selectAll(text);
                    makeCursorVisible(m_cachedText.size());
                    event.stopPropagation();
                }
                break;
            case KeyCode::V:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand) {
                    pasteFromClipboard(text);
                    event.stopPropagation();
                    setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                }
                break;
            case KeyCode::X:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand) {
                    cutToClipboard(text);
                    event.stopPropagation();
                    setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                }
                break;
            case KeyCode::C:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand) {
                    copyToClipboard(text);
                    event.stopPropagation();
                }
                break;
            case KeyCode::Up:
                if (m_multiline) {
                    moveCursor(MoveCursor::Up, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                    makeCursorVisible(m_cachedText.size());
                    event.stopPropagation();
                }
                break;
            case KeyCode::Down:
                if (m_multiline) {
                    moveCursor(MoveCursor::Down, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                    makeCursorVisible(m_cachedText.size());
                    event.stopPropagation();
                }
                break;
            case KeyCode::Left:
                moveCursor(MoveCursor::Left, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::Right:
                moveCursor(MoveCursor::Right, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::Home:
                moveCursor((e->mods & KeyModifiers::Regular) == KeyModifiers::Control
                               ? MoveCursor::TextBeginning
                               : MoveCursor::LineBeginning,
                           (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::End:
                moveCursor((e->mods & KeyModifiers::Regular) == KeyModifiers::Control ? MoveCursor::TextEnd
                                                                                      : MoveCursor::LineEnd,
                           (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::Backspace:
                if (selectedLength) {
                    deleteSelection(text);
                } else {
                    // delete one codepoint
                    if (cursor > 0) {
                        text.erase(cursor - 1, 1);
                        cursor = cursor - 1; // no need to align
                        selectionChanged();
                    }
                }
                event.stopPropagation();
                setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                break;
            case KeyCode::Del:
                if (selectedLength) {
                    deleteSelection(text);
                } else {
                    // delete whole grapheme
                    if (cursor < text.size()) {
                        auto endOfGrapheme =
                            std::upper_bound(m_preparedText.graphemeBoundaries.begin(),
                                             m_preparedText.graphemeBoundaries.end(), cursor);
                        if (endOfGrapheme != m_preparedText.graphemeBoundaries.end())
                            text.erase(cursor, *endOfGrapheme - cursor);
                        selectionChanged();
                    }
                }
                event.stopPropagation();
                setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                break;
            case KeyCode::Enter:
            case KeyCode::KPEnter:
                if (m_multiline) {
                    typeCharacter(text, '\n');
                    event.stopPropagation();
                } else {
                    if (m_onEnter.trigger())
                        event.stopPropagation();
                }
                break;
            default:
                break;
            }
        }
        normalizeCursor(text.size());
    }
}

void TextEditor::selectAll() {
    std::u32string t = utf8ToUtf32(m_text);
    selectAll(t);
}

void TextEditor::deleteSelection() {
    std::u32string t = utf8ToUtf32(m_text);
    deleteSelection(t);
    setTextInternal(utf32ToUtf8(t));
}

constexpr std::u32string_view internalNewLine = U"\n";
#ifdef BRISK_WINDOWS
constexpr std::u32string_view newLine = U"\r\n";
#else
constexpr std::u32string_view newLine = U"\n";
#endif

static std::u32string newLinesConvert(std::u32string text, std::u32string_view nl) {
    return replaceAll(replaceAll(std::move(text), U"\r\n", nl), U"\n", nl);
}

static std::u32string newLinesToNative(std::u32string text) {
    return newLinesConvert(std::move(text), newLine);
}

static std::u32string newLinesToInternal(std::u32string text) {
    return newLinesConvert(std::move(text), internalNewLine);
}

void TextEditor::pasteFromClipboard() {
    std::u32string t = utf8ToUtf32(m_text);
    pasteFromClipboard(t);
    setTextInternal(utf32ToUtf8(t));
}

void TextEditor::copyToClipboard() {
    std::u32string t = utf8ToUtf32(m_text);
    copyToClipboard(t);
}

void TextEditor::cutToClipboard() {
    std::u32string t = utf8ToUtf32(m_text);
    cutToClipboard(t);
    setTextInternal(utf32ToUtf8(t));
}

void TextEditor::selectAll(const std::u32string& text) {
    cursor         = text.size();
    selectedLength = -text.size();
    selectionChanged();
}

void TextEditor::selectionChanged() {
    onSelectionChanged();
    invalidate();
}

void TextEditor::onSelectionChanged() {}

void TextEditor::deleteSelection(std::u32string& text) {
    if (selectedLength) {
        const Range<int32_t> selection = this->selection();
        text.erase(selection.min, selection.distance());
        cursor         = selection.min;
        selectedLength = 0;
        selectionChanged();
    }
}

void TextEditor::pasteFromClipboard(std::u32string& text) {
    if (auto t = Clipboard::getText()) {
        deleteSelection(text);
        std::u32string t32 = utf8ToUtf32(*t);
        t32                = newLinesToInternal(std::move(t32));
        text.insert(cursor, t32);
        cursor         = cursor + t32.size();
        selectedLength = 0;
        selectionChanged();
    }
}

void TextEditor::copyToClipboard(const std::u32string& text) {
    if (selectedLength) {
        const Range<int32_t> selection = this->selection();
        if (m_passwordChar == 0)
            Clipboard::setText(utf32ToUtf8(
                newLinesToNative(normalizeCompose(text.substr(selection.min, selection.distance())))));
    }
}

void TextEditor::cutToClipboard(std::u32string& text) {
    if (selectedLength) {
        const Range<int32_t> selection = this->selection();
        if (m_passwordChar == 0)
            Clipboard::setText(utf32ToUtf8(
                newLinesToNative(normalizeCompose(text.substr(selection.min, selection.distance())))));
        deleteSelection(text);
    }
}

void TextEditor::updateGraphemes() {
    m_preparedText = fonts->prepare(
        m_cachedFont, TextWithOptions{ m_text.empty() ? utf8ToUtf32(m_placeholder) : m_cachedText,
                                       m_multiline ? TextOptions::Default : TextOptions::SingleLine });

    m_preparedText.updateCaretData();
}

void TextEditor::updateState() {
    invalidate();
    std::u32string text32 = utf8ToUtf32(m_text);

    if (m_passwordChar) {
        std::fill(text32.begin(), text32.end(), m_passwordChar);
    }
    if (text32 != m_cachedText || m_cachedFont != font()) {
        m_cachedText = std::move(text32);
        m_cachedFont = font();
        updateGraphemes();
    }
    makeCursorVisible(m_cachedText.size());
}

void TextEditor::setTextInternal(std::string text) {
    if (text != m_text) {
        m_text = std::move(text);
        bindings->notify(&m_text);
        updateState();
    }
}

void TextEditor::onLayoutUpdated() {
    updateState();
}

void TextEditor::typeCharacter(std::u32string& text, char32_t character) {
    deleteSelection(text);
    text.insert(text.begin() + cursor, character);
    cursor = cursor + 1; // no need to align
    selectionChanged();
    setTextInternal(utf32ToUtf8(normalizeCompose(text)));
}

PasswordEditor::PasswordEditor(Construction construction, ArgumentsView<PasswordEditor> args)
    : TextEditor{ construction, nullptr } {
    m_passwordChar = defaultPasswordChar;
    args.apply(this);
}

RC<Widget> PasswordEditor::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void TextEditor::onRefresh() {
    Base::onRefresh();
    if (frameStartTime - m_blinkTime > 1.0) {
        m_blinkTime = fmod(m_blinkTime, 1.0);
        invalidate();
    } else if (frameStartTime - m_blinkTime > 0.5) {
        invalidate();
    }
}
} // namespace Brisk
