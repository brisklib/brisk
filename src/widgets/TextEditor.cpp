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

TextEditor::TextEditor(Construction construction, Value<std::string> text, ArgumentsView<TextEditor> args)
    : Base(construction, nullptr) {
    m_tabStop       = true;
    m_processClicks = false;
    m_boxSizing     = BoxSizingPerAxis::ContentBoxY;
    args.apply(this);
    createContextMenu();

    bindings->connectBidir(this->text(), std::move(text));
}

void TextEditor::createContextMenu() {
    apply(new ContextPopup{
        Arg::role       = "context",
        Arg::fontFamily = FontFamily::Default,
        Arg::fontSize   = FontSize::Normal,
        new Item{ Arg::icon = ICON_scissors, new Text{ "Cut||Menu"_tr }, new Spacer{},
                  new Text{
                      hotKeyToString(KeyCode::X, KeyModifiers::ControlOrCommand),
                      Arg::classes = { "hotkeyhint" },
                  },
                  Arg::onClick = listener(
                      [this] {
                          cutToClipboard();
                      },
                      this) },
        new Item{ Arg::icon = ICON_copy, new Text{ "Copy||Menu"_tr }, new Spacer{},
                  new Text{
                      hotKeyToString(KeyCode::C, KeyModifiers::ControlOrCommand),
                      Arg::classes = { "hotkeyhint" },
                  },
                  Arg::onClick = listener(
                      [this] {
                          copyToClipboard();
                      },
                      this) },
        new Item{ Arg::icon = ICON_clipboard, new Text{ "Paste||Menu"_tr }, new Spacer{},
                  new Text{
                      hotKeyToString(KeyCode::V, KeyModifiers::ControlOrCommand),
                      Arg::classes = { "hotkeyhint" },
                  },
                  Arg::onClick = listener(
                      [this] {
                          pasteFromClipboard();
                      },
                      this) },
        new Item{ Arg::icon = ICON_x, new Text{ "Delete||Menu"_tr }, new Spacer{},
                  new Text{
                      hotKeyToString(KeyCode::Del, KeyModifiers::None),
                      Arg::classes = { "hotkeyhint" },
                  },
                  Arg::onClick = listener(
                      [this] {
                          deleteSelection();
                      },
                      this) },
        new Item{ new Text{ "Select All||Menu"_tr }, new Spacer{},
                  new Text{
                      hotKeyToString(KeyCode::A, KeyModifiers::ControlOrCommand),
                      Arg::classes = { "hotkeyhint" },
                  },
                  Arg::onClick = listener(
                      [this] {
                          selectAll();
                      },
                      this) },
    });
}

std::pair<int, int> TextEditor::selection() const {
    return { std::min(cursor, cursor + selectedLength), std::max(cursor, cursor + selectedLength) };
}

int TextEditor::moveCursor(int cursor, int graphemes) const {
    return m_preparedText.graphemeToCharacter(m_preparedText.characterToGrapheme(cursor) + graphemes);
}

void TextEditor::paint(Canvas& canvas) const {
    paintBackground(canvas, m_rect);
    Font font                  = this->font();
    FontMetrics metrics        = fonts->metrics(font);

    std::u32string placeholder = utf8ToUtf32(this->m_placeholder);
    bool isPlaceholder         = m_text.empty();

    {
        const Rectangle textRect = m_clientRect;
        auto&& state             = canvas.raw().save();
        state.intersectScissors(m_rect.withPadding(1_idp));

        std::pair<int, int> selection = this->selection();
        selection.first               = std::clamp(selection.first, 0, (int)m_cachedText.size());
        selection.second              = std::clamp(selection.second, 0, (int)m_cachedText.size());

        if (selection.first != selection.second) {
            std::vector<bool> bits(m_preparedText.graphemeBoundaries.size() - 1, false);
            for (int i = selection.first; i < selection.second; ++i) {
                int gr = m_preparedText.characterToGrapheme(i);
                if (bits[gr])
                    continue;
                Range<float> range = m_preparedText.ranges[gr];
                canvas.raw().drawRectangle(
                    Rectangle{ int(textRect.x1 + range.min - visibleOffset), textRect.y1,
                               int(textRect.x1 + range.max - visibleOffset), textRect.y2 },
                    0.f, 0.f,
                    fillColor   = ColorF(Palette::Standard::indigo).multiplyAlpha(isFocused() ? 0.85f : 0.5f),
                    strokeWidth = 0);
                bits[gr] = true;
            }
        }

        ColorF textColor = m_color.current;
        int yoffset      = metrics.descender - (textRect.height() - metrics.height) * 0.5f;
        if (isPlaceholder) {
            canvas.raw().drawText(textRect.at(0, 1) + Point{ 0, yoffset },
                                  TextWithOptions{ placeholder, LayoutOptions::SingleLine }, font,
                                  textColor.multiplyAlpha(0.5f));
        } else {
            canvas.raw().drawText(textRect.at(0, 1) + Point{ -visibleOffset, yoffset }, m_preparedText,
                                  fillColor = textColor);
        }

        if (isFocused() && std::fmod(frameStartTime - m_blinkTime, 1.0) < 0.5) {
            canvas.raw().drawRectangle(
                Rectangle{ Point{ int(textRect.x1 +
                                      m_preparedText
                                          .carets[m_preparedText.characterToGrapheme(
                                              std::max(0, std::min(cursor, int(m_cachedText.size()))))]
                                          .x -
                                      visibleOffset),
                                  textRect.y1 },
                           Size{ 2_idp, textRect.height() } },
                0.f, 0.f, fillColor = textColor, strokeWidth = 0);
        }
    }
}

void TextEditor::normalizeCursor(int textLen) {
    cursor         = std::max(0, std::min(cursor, textLen));
    selectedLength = std::max(0, std::min(cursor + selectedLength, textLen)) - cursor;
}

void TextEditor::normalizeVisibleOffset() {
    const int availWidth = m_clientRect.width();
    if (m_preparedText.carets.empty() || m_preparedText.carets.back().x < availWidth)
        visibleOffset = 0;
    else
        visibleOffset = std::max(
            0, std::min(visibleOffset, static_cast<int>(m_preparedText.carets.back().x - availWidth)));
}

void TextEditor::makeCursorVisible(int textLen) {
    const int availWidth = m_clientRect.width();
    const int cursor     = std::max(0, std::min(this->cursor, textLen));
    const int cursorPos  = m_preparedText.carets[m_preparedText.characterToGrapheme(cursor)].x;
    if (cursorPos < visibleOffset)
        visibleOffset = cursorPos - 2_idp;
    else if (cursorPos > visibleOffset + availWidth)
        visibleOffset = cursorPos - availWidth + 2_idp;
    normalizeVisibleOffset();
}

int TextEditor::offsetToPosition(float x) const {
    if (m_preparedText.carets.size() <= 1) {
        return 0;
    }
    int nearest    = 0;
    float distance = std::abs(m_preparedText.carets.front().x - x);
    for (size_t i = 1; i < m_preparedText.carets.size(); ++i) {
        float new_distance = std::abs(m_preparedText.carets[i].x - x);
        if (new_distance < distance) {
            distance = new_distance;
            nearest  = i;
        }
    }
    return m_preparedText.graphemeToCharacter(nearest);
}

static bool char_is_alphanum(char32_t ch) {
    utf8proc_category_t cat = utf8proc_category(ch);
    return cat >= UTF8PROC_CATEGORY_LU && cat <= UTF8PROC_CATEGORY_NO;
}

void TextEditor::selectWordAtCursor() {
    u32string text = utf8ToUtf32(m_text);
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
    switch (const auto [flag, offset, mods] = event.dragged(mouseSelection); flag) {
    case DragEvent::Started: {
        text        = utf8ToUtf32(m_text);
        m_blinkTime = frameStartTime;
        focus();
        cursor         = offsetToPosition(event.as<EventMouse>()->downPoint->x - textRect.x1 + visibleOffset);
        selectedLength = 0;
        normalizeCursor(text.size());
        startCursorDragging =
            offsetToPosition(event.as<EventMouse>()->downPoint->x - textRect.x1 + visibleOffset);
        event.stopPropagation();
    } break;
    case DragEvent::Dragging: {
        text                = utf8ToUtf32(m_text);
        m_blinkTime         = frameStartTime;
        const int endCursor = offsetToPosition(event.as<EventMouse>()->point.x - textRect.x1 + visibleOffset);
        selectedLength      = startCursorDragging - endCursor;
        cursor              = endCursor;
        normalizeCursor(text.size());
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
            deleteSelection(text);
            text.insert(text.begin() + cursor, ch->character);
            cursor = cursor + 1; // no need to align
            event.stopPropagation();
            setTextInternal(utf32ToUtf8(normalizeCompose(text)));
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
            case KeyCode::Left:
                if (cursor > 0) {
                    if ((e->mods & KeyModifiers::Regular) == KeyModifiers::Shift) {
                        int oldCursor = cursor;
                        cursor        = moveCursor(cursor, -1);
                        selectedLength += oldCursor - cursor;
                    } else if ((e->mods & KeyModifiers::Regular) == KeyModifiers::None) {
                        if (selectedLength) {
                            cursor         = selection().first;
                            selectedLength = 0;
                        } else {
                            cursor = moveCursor(cursor, -1);
                        }
                    }
                }
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::Right:
                if (cursor < text.size()) {
                    if ((e->mods & KeyModifiers::Regular) == KeyModifiers::Shift) {
                        int oldCursor = cursor;
                        cursor        = moveCursor(cursor, +1);
                        selectedLength += oldCursor - cursor;
                    } else if ((e->mods & KeyModifiers::Regular) == KeyModifiers::None) {
                        if (selectedLength) {
                            cursor         = selection().second;
                            selectedLength = 0;
                        } else {
                            cursor = moveCursor(cursor, +1);
                        }
                    }
                }
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::Home:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::Shift) {
                    selectedLength = cursor;
                } else if ((e->mods & KeyModifiers::Regular) == KeyModifiers::None) {
                    selectedLength = 0;
                }
                cursor = 0;
                makeCursorVisible(m_cachedText.size());
                event.stopPropagation();
                break;
            case KeyCode::End:
                if ((e->mods & KeyModifiers::Regular) == KeyModifiers::Shift) {
                    selectedLength = cursor - text.size();
                } else if ((e->mods & KeyModifiers::Regular) == KeyModifiers::None) {
                    selectedLength = 0;
                }
                cursor = text.size();
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
                        int newCursor = moveCursor(cursor, +1);
                        text.erase(cursor, newCursor - cursor);
                    }
                }
                event.stopPropagation();
                setTextInternal(utf32ToUtf8(normalizeCompose(text)));
                break;
            case KeyCode::Enter:
                m_onEnter.trigger();
                event.stopPropagation();
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
}

void TextEditor::deleteSelection(std::u32string& text) {
    if (selectedLength) {
        const std::pair<int, int> selection = this->selection();
        text.erase(selection.first, selection.second - selection.first);
        cursor         = selection.first;
        selectedLength = 0;
    }
}

void TextEditor::pasteFromClipboard(std::u32string& text) {
    if (auto t = getTextFromClipboard()) {
        deleteSelection(text);
        std::u32string t32 = utf8ToUtf32(*t);
        t32                = newLinesToInternal(std::move(t32));
        text.insert(cursor, t32);
        cursor         = cursor + t32.size();
        selectedLength = 0;
    }
}

void TextEditor::copyToClipboard(const std::u32string& text) {
    if (selectedLength) {
        const std::pair<int, int> selection = this->selection();
        if (m_passwordChar == 0)
            copyTextToClipboard(utf32ToUtf8(newLinesToNative(
                normalizeCompose(text.substr(selection.first, selection.second - selection.first)))));
    }
}

void TextEditor::cutToClipboard(std::u32string& text) {
    if (selectedLength) {
        const std::pair<int, int> selection = this->selection();
        if (m_passwordChar == 0)
            copyTextToClipboard(utf32ToUtf8(newLinesToNative(
                normalizeCompose(text.substr(selection.first, selection.second - selection.first)))));
        deleteSelection(text);
    }
}

void TextEditor::updateGraphemes() {
    m_preparedText = fonts->prepare(m_cachedFont, TextWithOptions{ m_cachedText, LayoutOptions::SingleLine });
    m_preparedText.updateCaretData();
}

Value<std::string> TextEditor::text() {
    return Value<std::string>{
        &m_text,
        this,
        &TextEditor::updateState,
    };
}

void TextEditor::updateState() {
    u32string text32 = utf8ToUtf32(m_text);

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

PasswordEditor::PasswordEditor(Construction construction, Value<std::string> text,
                               ArgumentsView<PasswordEditor> args)
    : TextEditor{ construction, std::move(text), nullptr } {
    m_passwordChar = defaultPasswordChar;
    args.apply(this);
}

Widget::Ptr PasswordEditor::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}
} // namespace Brisk
