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
#pragma once

#include "Widgets.hpp"
#include <brisk/core/Binding.hpp>

namespace Brisk {

class TextEditor : public Widget {
private:
    using Base = Widget;

public:
    constexpr static std::string_view widgetType = "texteditor";

    template <WidgetArgument... Args>
    explicit TextEditor(const Args&... args) : TextEditor(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit TextEditor(Value<std::string> text, const Args&... args)
        : TextEditor(Construction{ widgetType }, std::tuple{ args... }) {
        bindings->connectBidir(Value{ &this->text }, std::move(text));
        endConstruction();
    }

    Range<uint32_t> selection() const;

    uint32_t caretToOffset(PointF pt) const;

    void selectWordAtCursor();

    void selectAll();
    void deleteSelection();
    void pasteFromClipboard();
    void copyToClipboard();
    void cutToClipboard();

    uint32_t cursor        = 0;
    int32_t selectedLength = 0; // May be negative

    enum class MoveCursor {
        Up,
        Down,
        Right,
        Left,
        LineBeginning,
        LineEnd,
        TextBeginning,
        TextEnd,
    };

    void moveCursor(MoveCursor move, bool select = false);

protected:
    std::string m_text;
    char32_t m_passwordChar = 0;
    std::string m_placeholder;
    Trigger<> m_onEnter;
    Point m_visibleOffset{ 0, 0 };
    mutable Point m_alignmentOffset{ 0, 0 };
    bool m_mouseSelection = false;
    void onEvent(Event& event) override;
    void paint(Canvas& canvas) const override;
    void onLayoutUpdated() override;
    void updateState();
    void setTextInternal(std::string text);

    void typeCharacter(std::u32string& text, char32_t character);

    std::u32string m_cachedText;

    mutable PreparedText m_preparedText;
    Font m_cachedFont{};

    double m_blinkTime            = 0.0;
    int32_t m_startCursorDragging = 0;
    bool m_multiline              = false;
    void updateGraphemes();
    void makeCursorVisible(uint32_t textLen);

    void selectAll(const std::u32string& text);
    void deleteSelection(std::u32string& text);
    void pasteFromClipboard(std::u32string& text);
    void copyToClipboard(const std::u32string& text);
    void cutToClipboard(std::u32string& text);
    virtual void onSelectionChanged();
    void onRefresh() override;

    explicit TextEditor(Construction, ArgumentsView<TextEditor> args);

private:
    void normalizeCursor(uint32_t textLen);
    void createContextMenu();
    void selectionChanged();

public:
    BRISK_PROPERTIES_BEGIN
    Property<TextEditor, std::string, &TextEditor::m_text, nullptr, nullptr, &TextEditor::updateState> text;
    Property<TextEditor, Trigger<>, &TextEditor::m_onEnter> onEnter;
    Property<TextEditor, std::string, &TextEditor::m_placeholder> placeholder;
    Property<TextEditor, char32_t, &TextEditor::m_passwordChar, nullptr, nullptr, &TextEditor::updateState>
        passwordChar;
    Property<TextEditor, bool, &TextEditor::m_multiline, nullptr, nullptr, &TextEditor::updateState>
        multiline;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(TextEditor* target, ArgVal<Tag::Named<"text">, T> value) {
    target->text = value.value;
}

inline namespace Arg {
#ifndef BRISK__TEXT_ARG_DEFINED
#define BRISK__TEXT_ARG_DEFINED
constexpr inline Argument<Tag::Named<"text">> text{};
#endif
constexpr inline Argument<Tag::PropArg<decltype(TextEditor::onEnter)>> onEnter{};
constexpr inline Argument<Tag::PropArg<decltype(TextEditor::multiline)>> multiline{};
constexpr inline Argument<Tag::PropArg<decltype(TextEditor::placeholder)>> placeholder{};
constexpr inline Argument<Tag::PropArg<decltype(TextEditor::passwordChar)>> passwordChar{};
} // namespace Arg

inline constexpr char32_t defaultPasswordChar = U'\U00002022';

class WIDGET PasswordEditor : public TextEditor {
public:
    using Base = TextEditor;

    template <WidgetArgument... Args>
    explicit PasswordEditor(const Args&... args)
        : PasswordEditor(Construction{ widgetType }, std::tuple{ args... }) {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit PasswordEditor(Value<std::string> text, const Args&... args)
        : PasswordEditor(Construction{ widgetType }, std::tuple{ args... }) {
        bindings->connectBidir(Value{ &this->text }, std::move(text));
        endConstruction();
    }

protected:
    explicit PasswordEditor(Construction construction, ArgumentsView<PasswordEditor> args);

    Ptr cloneThis() const override;
};
} // namespace Brisk
