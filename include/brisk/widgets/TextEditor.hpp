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
#pragma once

#include "Widgets.hpp"
#include <brisk/core/Binding.hpp>

namespace Brisk {

class WIDGET TextEditor : public Widget {
    BRISK_DYNAMIC_CLASS(TextEditor, Widget)
private:
    using Base = Widget;

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
    bool m_blinkState             = true;
    int32_t m_startCursorDragging = 0;
    bool m_multiline              = false;
    void resetBlinking();
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
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &TextEditor::m_text, &TextEditor::updateState, "text" },
            /*1*/ Internal::PropField{ &TextEditor::m_onEnter, "onEnter" },
            /*2*/ Internal::PropField{ &TextEditor::m_placeholder, "placeholder" },
            /*3*/
            Internal::PropFieldNotify{ &TextEditor::m_passwordChar, &TextEditor::updateState,
                                       "passwordChar" },
            /*4*/
            Internal::PropFieldNotify{ &TextEditor::m_multiline, &TextEditor::updateState, "multiline" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<TextEditor, std::string, 0> text;
    Property<TextEditor, Trigger<>, 1> onEnter;
    Property<TextEditor, std::string, 2> placeholder;
    Property<TextEditor, char32_t, 3> passwordChar;
    Property<TextEditor, bool, 4> multiline;
    BRISK_PROPERTIES_END
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
constexpr inline PropArgument<decltype(TextEditor::onEnter)> onEnter{};
constexpr inline PropArgument<decltype(TextEditor::multiline)> multiline{};
constexpr inline PropArgument<decltype(TextEditor::placeholder)> placeholder{};
constexpr inline PropArgument<decltype(TextEditor::passwordChar)> passwordChar{};
} // namespace Arg

inline constexpr char32_t defaultPasswordChar = U'\U00002022';

class WIDGET PasswordEditor : public TextEditor {
    BRISK_DYNAMIC_CLASS(PasswordEditor, TextEditor)
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
