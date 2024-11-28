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
        bindings->connectBidir(Internal::asValue(this->text), std::move(text));
        endConstruction();
    }

    Range<int32_t> selection() const;

    int offsetToPosition(float x) const;

    void selectWordAtCursor();

    void selectAll();
    void deleteSelection();
    void pasteFromClipboard();
    void copyToClipboard();
    void cutToClipboard();

    int cursor         = 0;
    int selectedLength = 0;

protected:
    std::string m_text;
    char32_t m_passwordChar = 0;
    std::string m_placeholder;
    Trigger<> m_onEnter;
    int m_visibleOffset   = 0; // in pixels, positive means first letters hidden
    bool m_mouseSelection = false;
    void onEvent(Event& event) override;
    void paint(Canvas& canvas) const override;
    void onLayoutUpdated() override;
    void updateState();
    void setTextInternal(std::string text);

    std::u32string m_cachedText;

    PreparedText m_preparedText;
    Font m_cachedFont{};

    int moveCursor(int cursor, int graphemes) const;
    double m_blinkTime        = 0.0;
    int m_startCursorDragging = 0;
    void updateGraphemes();
    void makeCursorVisible(int textLen);

    void selectAll(const std::u32string& text);
    void deleteSelection(std::u32string& text);
    void pasteFromClipboard(std::u32string& text);
    void copyToClipboard(const std::u32string& text);
    void cutToClipboard(std::u32string& text);

    explicit TextEditor(Construction, ArgumentsView<TextEditor> args);

private:
    void normalizeCursor(int textLen);
    void normalizeVisibleOffset();
    void createContextMenu();

public:
    BRISK_PROPERTIES_BEGIN
    Property<TextEditor, std::string, &TextEditor::m_text, nullptr, nullptr, &TextEditor::updateState> text;
    Property<TextEditor, Trigger<>, &TextEditor::m_onEnter> onEnter;
    Property<TextEditor, std::string, &TextEditor::m_placeholder> placeholder;
    Property<TextEditor, char32_t, &TextEditor::m_passwordChar, nullptr, nullptr, &TextEditor::updateState>
        passwordChar;
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
        bindings->connectBidir(Internal::asValue(this->text), std::move(text));
        endConstruction();
    }

protected:
    explicit PasswordEditor(Construction construction, ArgumentsView<PasswordEditor> args);

    Ptr cloneThis() override;
};
} // namespace Brisk
