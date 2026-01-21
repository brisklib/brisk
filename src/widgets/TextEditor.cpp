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
#include <brisk/widgets/TextEditor.hpp>
#include <brisk/widgets/Item.hpp>
#include <brisk/widgets/Menu.hpp>
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

// Determines the caret affinity a cursor position should have, based purely on the
// (already edited) text buffer. This mirrors the rule used for horizontal caret
// movement in moveCursor(): a caret that sits immediately before a paragraph
// separator stays attached to the end of the line that precedes it (Upstream),
// while a caret that sits immediately after one belongs to the beginning of the
// following line (Downstream). Every code path that moves the cursor by directly
// editing the text (typing, backspace, delete, paste, deleting a selection) must
// recompute affinity this way instead of assuming Downstream, otherwise the caret
// can end up rendered on the wrong visual line at a hard paragraph break.
static CaretAffinity caretAffinityForPosition(const std::u32string& text, uint32_t cursor) {
    const auto isParagraphSeparator = [](char32_t character) {
        switch (character) {
        case U'\n':
        case U'\r':
        case U'\u001C':
        case U'\u001D':
        case U'\u001E':
        case U'\u0085':
        case U'\u2029':
            return true;
        default:
            return false;
        }
    };

    if (cursor > 0 && isParagraphSeparator(text[cursor - 1]))
        return CaretAffinity::Downstream;
    if (cursor < text.size() && isParagraphSeparator(text[cursor]))
        return CaretAffinity::Upstream;
    return CaretAffinity::Downstream;
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
    apply(rcnew Menu{
        Arg::role       = "menu",
        Arg::classes    = { "withicons" },
        Arg::fontFamily = Font::DefaultPlusIconsEmoji,
        Arg::fontSize   = FontSize::Normal,
        rcnew Item{
            Arg::icon = ICON_scissors,
            rcnew Text{ "Cut||Menu"_tr },
            rcnew Spacer{},
            rcnew ShortcutHint{
                { KeyModifiers::ControlOrCommand, KeyCode::X },
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
            rcnew ShortcutHint{
                { KeyModifiers::ControlOrCommand, KeyCode::C },
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
            rcnew ShortcutHint{
                { KeyModifiers::ControlOrCommand, KeyCode::V },
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
            rcnew ShortcutHint{
                { KeyModifiers::None, KeyCode::Del },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               deleteSelection();
                           },
        },
        rcnew Item{
            rcnew Text{ "Select All||Menu"_tr },
            rcnew Spacer{},
            rcnew ShortcutHint{
                { KeyModifiers::ControlOrCommand, KeyCode::A },
            },
            Arg::onClick = lifetime() |
                           [this] {
                               selectAll();
                           },
        },
    });
}

Range<uint32_t> TextEditor::selection() const {
    const int32_t cursorPosition = static_cast<int32_t>(cursor);
    const int32_t selectionEnd   = cursorPosition + selectedLength;
    return { static_cast<uint32_t>(std::min(cursorPosition, selectionEnd)),
             static_cast<uint32_t>(std::max(cursorPosition, selectionEnd)) };
}

CaretIndex TextEditor::caretIndex() const {
    return m_preparedDocument.caretFromCharacter(std::min(cursor, m_preparedDocument.characterCount()),
                                                 m_caretAffinity);
}

void TextEditor::setCaretIndex(CaretIndex caret) {
    m_caretAffinity = caret.affinity;
    cursor          = m_preparedDocument.characterFromCaret(caret);
}

CaretIndex TextEditor::caretAtPoint(PointF pt) const {
    if (m_documentLayout.lineCount() == 0) {
        return {};
    }
    return m_documentLayout.hitTest(PointF(pt) - textOrigin());
}

PointF TextEditor::textOrigin() const {
    PointF alignment{ toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };
    return PointF(m_clientRect.at(alignment)) - PointF(m_visibleOffset);
}

void TextEditor::moveCursor(MoveCursor move, bool select) {
    uint32_t oldCursor = cursor;
    using enum MoveCursor;
    switch (move) {
    case Up:
    case Down: {
        CaretIndex current     = caretIndex();
        CaretPosition position = m_documentLayout.caretPosition(current);
        const int direction    = move == Up ? -1 : 1;
        const size_t line      = m_documentLayout.lineForCaret(current);
        if (m_documentLayout.lineCount() == 0 || (direction < 0 && line == 0) ||
            (direction > 0 && line + 1 >= m_documentLayout.lineCount()))
            break;
        if (!m_preferredCaretX)
            m_preferredCaretX = position.x;
        setCaretIndex(m_documentLayout.moveCaretVertically(current, *m_preferredCaretX, direction));
        break;
    }

    case Right:
    case Left: {
        if (!select && selectedLength != 0) {
            const Range<uint32_t> currentSelection = selection();
            cursor          = move == Left ? currentSelection.min : currentSelection.max;
            selectedLength  = 0;
            m_caretAffinity = caretAffinityForPosition(m_cachedText, cursor);
            m_preferredCaretX.reset();
            break;
        }
        uint32_t grapheme = m_preparedDocument.characterToGrapheme(cursor);
        if ((grapheme == 0 && move == Left) ||
            (grapheme == m_preparedDocument.graphemeCount() && move == Right))
            break;
        grapheme += move == Right ? +1 : -1;
        CaretAffinity affinity = move == Left ? CaretAffinity::Upstream : CaretAffinity::Downstream;
        if (m_preparedDocument.isParagraphSeparator(grapheme))
            affinity = CaretAffinity::Upstream;
        else if (grapheme > 0 && m_preparedDocument.isParagraphSeparator(grapheme - 1))
            affinity = CaretAffinity::Downstream;
        setCaretIndex({ grapheme, affinity });
        m_preferredCaretX.reset();
        break;
    }

    case LineBeginning:
    case LineEnd: {
        const size_t line = m_documentLayout.lineForCaret(caretIndex());
        if (line >= m_documentLayout.lineCount())
            break;
        setCaretIndex(move == LineBeginning ? m_documentLayout.lineBeginning(line)
                                            : m_documentLayout.lineEnd(line));
        m_preferredCaretX.reset();
        break;
    }

    case TextBeginning:
        cursor          = 0;
        m_caretAffinity = CaretAffinity::Downstream;
        m_preferredCaretX.reset();
        break;
    case TextEnd:
        cursor          = m_cachedText.size();
        m_caretAffinity = CaretAffinity::Upstream;
        m_preferredCaretX.reset();
        break;
    }

    if (select) {
        selectedLength += static_cast<int32_t>(oldCursor) - static_cast<int32_t>(cursor);
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

    ColorW textColor = m_color.current;
    if (isPlaceholder)
        textColor = textColor.multiplyAlpha(0.5f);

    canvas.setFillColor(ColorW(Palette::Standard::indigo).multiplyAlpha(isFocused() ? 0.85f : 0.5f));
    Point pos = Point(textOrigin());
    canvas.fillTextSelection(pos, alignment, m_documentLayout, selection);
    canvas.setFillColor(textColor);
    canvas.fillText(pos, alignment, m_documentLayout);

    if (isFocused() && m_blinkState && !isDisabled()) {
        const RectangleF caret = m_documentLayout.caretRect(caretIndex(), 1_dp);
        if (!caret.empty()) {
            const RectangleF bounds = m_documentLayout.bounds();
            const PointF offset     = PointF(pos) - PointF(bounds.size()) * alignment;
            const RectangleF translated{ caret.x1 + offset.x, caret.y1 + offset.y, caret.x2 + offset.x,
                                         caret.y2 + offset.y };
            canvas.setFillColor(textColor);
            canvas.fillRect(translated, 0.f);
        }
    }
}

void TextEditor::normalizeCursor(uint32_t textLen) {
    const int32_t length            = static_cast<int32_t>(textLen);
    const int32_t cursorPosition    = static_cast<int32_t>(cursor);
    const int32_t selectionEnd      = cursorPosition + selectedLength;
    const int32_t newCursor         = std::clamp(cursorPosition, 0, length);
    const int32_t newSelectionEnd   = std::clamp(selectionEnd, 0, length);
    const int32_t newSelectedLength = newSelectionEnd - newCursor;
    if (newCursor != static_cast<int32_t>(cursor) || newSelectedLength != selectedLength) {
        cursor         = static_cast<uint32_t>(newCursor);
        selectedLength = newSelectedLength;
        selectionChanged();
    }
}

void TextEditor::makeCursorVisible() {
    if (m_documentLayout.lineCount() == 0) {
        m_visibleOffset = { 0, 0 };
        invalidate();
        return;
    }
    SizeF bounds = m_documentLayout.bounds().size();
    if (std::ceil(bounds.width) < m_clientRect.width() && std::ceil(bounds.height) < m_clientRect.height()) {
        m_visibleOffset = { 0, 0 };
        invalidate();
        return;
    }

    const CaretPosition caret = m_documentLayout.caretPosition(caretIndex());
    const uint32_t lineIndex  = caret.line;
    if (lineIndex >= m_documentLayout.lineCount())
        return;
    const Rectangle caretRect = m_documentLayout.caretRect(caretIndex(), 1_dp).roundOutward();
    const PointF alignment{ toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };
    const PointF layoutOffset =
        PointF(m_clientRect.at(alignment)) - PointF(m_visibleOffset) - PointF(bounds) * alignment;
    const RectangleF visibleCaret{ caretRect.x1 + layoutOffset.x, caretRect.y1 + layoutOffset.y,
                                   caretRect.x2 + layoutOffset.x, caretRect.y2 + layoutOffset.y };

    const auto keepVisible = [](float caretStart, float caretEnd, float clientStart, float clientEnd,
                                int32_t& offset) {
        if (caretStart < clientStart && caretEnd > clientEnd) {
            // An oversized caret cannot fit; show its center in the client rectangle.
            offset += static_cast<int32_t>(
                std::lround((caretStart + caretEnd) * 0.5f - (clientStart + clientEnd) * 0.5f));
        } else if (caretStart < clientStart) {
            offset += static_cast<int32_t>(std::lround(caretStart - clientStart));
        } else if (caretEnd > clientEnd) {
            offset += static_cast<int32_t>(std::lround(caretEnd - clientEnd));
        }
        offset = std::max(0, offset);
    };

    keepVisible(visibleCaret.x1, visibleCaret.x2, m_clientRect.x1, m_clientRect.x2, m_visibleOffset.x);
    keepVisible(visibleCaret.y1, visibleCaret.y2, m_clientRect.y1, m_clientRect.y2, m_visibleOffset.y);

    invalidate();
}

uint32_t TextEditor::caretToOffset(PointF pt) const {
    if (m_documentLayout.lineCount() == 0) {
        return 0;
    }
    return m_preparedDocument.characterFromCaret(caretAtPoint(pt));
}

static bool char_is_alphanum(char32_t ch) {
    utf8proc_category_t cat = utf8proc_category(ch);
    return cat >= UTF8PROC_CATEGORY_LU && cat <= UTF8PROC_CATEGORY_NO;
}

void TextEditor::selectWordAtCursor() {
    std::u32string text = utf8ToUtf32(m_text);
    normalizeCursor(text.size());
    const int32_t textLength = static_cast<int32_t>(text.size());
    const int32_t cursorPos  = static_cast<int32_t>(cursor);
    for (int32_t i = cursorPos - 1;; i--) {
        if (i < 0 || !char_is_alphanum(text[static_cast<size_t>(i)])) {
            cursor = static_cast<uint32_t>(i + 1);
            break;
        }
    }
    for (int32_t i = cursorPos;; i++) {
        if (i >= textLength || !char_is_alphanum(text[static_cast<size_t>(i)])) {
            selectedLength = i - static_cast<int32_t>(cursor);
            break;
        }
    }
    selectedLength  = -selectedLength;
    cursor          = static_cast<uint32_t>(static_cast<int32_t>(cursor) - selectedLength);
    m_caretAffinity = caretAffinityForPosition(text, cursor);
    selectionChanged();
    normalizeCursor(text.size());
}

void TextEditor::onEvent(Event& event) {
    Base::onEvent(event);

    if (isDisabled())
        return;
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
            resetBlinking();
            selectAll();
        }
    }
    switch (const auto [flag, offset, mods] = event.dragged(m_mouseSelection); flag) {
    case DragEvent::Started: {
        text = utf8ToUtf32(m_text);
        resetBlinking();
        focus();
        m_preferredCaretX.reset();
        setCaretIndex(caretAtPoint(Point(*event.as<EventMouse>()->downPoint)));
        cursor         = std::min(cursor, static_cast<uint32_t>(text.size()));
        selectedLength = 0;
        selectionChanged();
        normalizeCursor(text.size());
        m_startCursorDragging = static_cast<int32_t>(cursor);
        event.stopPropagation();
    } break;
    case DragEvent::Dragging: {
        text = utf8ToUtf32(m_text);
        resetBlinking();
        const CaretIndex endCaret = caretAtPoint(Point(event.as<EventMouse>()->point));
        const int endCursor       = static_cast<int>(m_preparedDocument.characterFromCaret(endCaret));
        m_caretAffinity           = endCaret.affinity;
        selectedLength            = m_startCursorDragging - endCursor;
        cursor                    = endCursor;
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
        text = utf8ToUtf32(m_text);

        resetBlinking();
        normalizeCursor(text.size());
        if (auto ch = event.as<EventCharacterTyped>()) {
            typeCharacter(text, ch->character);
            event.stopPropagation();
        } else {
            switch (auto e = event.as<EventKeyPressed>(); e->key) {
            case KeyCode::A:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand) {
                    selectAll(text);
                    makeCursorVisible();
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
                    makeCursorVisible();
                    event.stopPropagation();
                }
                break;
            case KeyCode::Down:
                if (m_multiline) {
                    moveCursor(MoveCursor::Down, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                    makeCursorVisible();
                    event.stopPropagation();
                }
                break;
            case KeyCode::Left:
                moveCursor(MoveCursor::Left, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible();
                event.stopPropagation();
                break;
            case KeyCode::Right:
                moveCursor(MoveCursor::Right, (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible();
                event.stopPropagation();
                break;
            case KeyCode::Home:
                moveCursor((e->mods & KeyModifiers::Regular) == KeyModifiers::Control
                               ? MoveCursor::TextBeginning
                               : MoveCursor::LineBeginning,
                           (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible();
                event.stopPropagation();
                break;
            case KeyCode::End:
                moveCursor((e->mods & KeyModifiers::Regular) == KeyModifiers::Control ? MoveCursor::TextEnd
                                                                                      : MoveCursor::LineEnd,
                           (e->mods & KeyModifiers::Regular) == KeyModifiers::Shift);
                makeCursorVisible();
                event.stopPropagation();
                break;
            case KeyCode::Backspace:
                if (selectedLength) {
                    deleteSelection(text);
                } else {
                    // Delete one codepoint, except that CRLF is one logical paragraph
                    // separator and must be deleted atomically.
                    if (cursor > 0) {
                        const uint32_t eraseBegin =
                            cursor >= 2 && text[cursor - 2] == U'\r' && text[cursor - 1] == U'\n'
                                ? cursor - 2
                                : cursor - 1;
                        text.erase(eraseBegin, cursor - eraseBegin);
                        cursor          = eraseBegin;
                        m_caretAffinity = caretAffinityForPosition(text, cursor);
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
                        const auto graphemeBoundaries = m_preparedDocument.graphemeBoundaries();
                        auto endOfGrapheme =
                            std::upper_bound(graphemeBoundaries.begin(), graphemeBoundaries.end(), cursor);
                        if (endOfGrapheme != graphemeBoundaries.end()) {
                            text.erase(cursor, *endOfGrapheme - cursor);
                            m_caretAffinity = caretAffinityForPosition(text, cursor);
                        }
                        selectionChanged();
                    }
                }
                event.stopPropagation();
                setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                break;
            case KeyCode::Enter:
            case KeyCode::NumEnter:
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
    std::u32string result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == U'\r' && i + 1 < text.size() && text[i + 1] == U'\n')
            ++i;
        if (text[i] == U'\r' || text[i] == U'\n')
            result += nl;
        else
            result += text[i];
    }
    return result;
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
    cursor         = static_cast<uint32_t>(text.size());
    selectedLength = -static_cast<int32_t>(text.size());
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
        cursor          = selection.min;
        selectedLength  = 0;
        m_caretAffinity = caretAffinityForPosition(text, cursor);
        selectionChanged();
    }
}

void TextEditor::pasteFromClipboard(std::u32string& text) {
    if (auto t = Clipboard::getText()) {
        deleteSelection(text);
        std::u32string t32 = utf8ToUtf32(*t);
        t32                = newLinesToInternal(std::move(t32));
        text.insert(cursor, t32);
        cursor          = cursor + t32.size();
        selectedLength  = 0;
        m_caretAffinity = caretAffinityForPosition(text, cursor);
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
    const TextOptions options = m_multiline ? TextOptions::Default : TextOptions::SingleLine;
    m_preparedDocument        = fonts->prepareDocument(
        m_cachedFont, TextWithOptions{ m_text.empty() ? utf8ToUtf32(m_placeholder) : m_cachedText, options });
    TextLayoutOptions layoutOptions;
    layoutOptions.maxLineWidth = std::max(0.f, static_cast<float>(m_clientRect.width()));
    layoutOptions.alignment    = TextLayoutAlignment::Start;
    m_documentLayout           = m_preparedDocument.layout(layoutOptions);
    m_cachedLayoutWidth        = layoutOptions.maxLineWidth;
}

void TextEditor::updateState() {
    invalidate();
    std::u32string text32     = utf8ToUtf32(m_text);
    const uint32_t textLength = static_cast<uint32_t>(text32.size());

    if (m_passwordChar) {
        std::fill(text32.begin(), text32.end(), m_passwordChar);
    }
    if (text32 != m_cachedText || m_cachedFont != font()) {
        m_cachedText = std::move(text32);
        m_cachedFont = font();
        updateGraphemes();
    } else if (m_documentLayout.lineCount() == 0 ||
               m_cachedLayoutWidth != static_cast<float>(m_clientRect.width())) {
        updateGraphemes();
    }
    normalizeCursor(textLength);
    makeCursorVisible();
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
    cursor          = cursor + 1; // no need to align
    m_caretAffinity = caretAffinityForPosition(text, cursor);
    selectionChanged();
    setTextInternal(utf32ToUtf8(normalizeCompose(text)));
}

PasswordEditor::PasswordEditor(Construction construction, ArgumentsView<PasswordEditor> args)
    : TextEditor{ construction, nullptr } {
    m_passwordChar = defaultPasswordChar;
    args.apply(this);
}

Rc<Widget> PasswordEditor::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void TextEditor::onRefresh() {
    Base::onRefresh();
    if (isFocused()) {
        if (frameStartTime - m_blinkTime > 0.5) {
            m_blinkState = !m_blinkState;
            m_blinkTime  = frameStartTime;
            invalidate();
        }
    }
}

void TextEditor::resetBlinking() {
    m_blinkState = true;
    m_blinkTime  = frameStartTime;
}
} // namespace Brisk
