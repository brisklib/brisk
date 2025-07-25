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

#include <brisk/gui/Gui.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Localization.hpp>

namespace Brisk {

class WIDGET Text : public Widget {
    BRISK_DYNAMIC_CLASS(Text, Widget)
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "text";

    template <WidgetArgument... Args>
    explicit Text(std::string_view text, const Args&... args)
        : Text{ Construction{ widgetType }, std::string(text), std::tuple{ args... } } {
        endConstruction();
    }

    template <size_t N, WidgetArgument... Args>
    explicit Text(const char (&text)[N], const Args&... args)
        : Text{ Construction{ widgetType }, std::string(text), std::tuple{ args... } } {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit Text(std::string text, const Args&... args)
        : Text{ Construction{ widgetType }, std::move(text), std::tuple{ args... } } {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit Text(const Args&... args)
        : Text{ Construction{ widgetType }, std::string{}, std::tuple{ args... } } {
        endConstruction();
    }

    using Widget::apply;

protected:
    std::string m_text;
    TextAutoSize m_textAutoSize               = TextAutoSize::None;
    InclusiveRange<float> m_textAutoSizeRange = { 6.f, 96.f };
    Rotation m_rotation                       = Rotation::NoRotation;
    bool m_wordWrap                           = false;
    TextOptions m_textOptions                 = TextOptions::Default;

    struct CacheKey {
        Font font;
        std::string text;
        bool operator==(const CacheKey&) const noexcept = default;
    };

    struct Cached {
        PreparedText shaped;
    };

    struct CacheKey2 {
        int width;
        bool operator==(const CacheKey2&) const noexcept = default;
    };

    struct Cached2 {
        SizeF textSize;
        PreparedText prepared;
    };

    Cached updateCache(const CacheKey&);
    Cached2 updateCache2(const CacheKey2&);
    CacheWithInvalidation<Cached, CacheKey, Text, &Text::updateCache> m_cache{ this };
    CacheWithInvalidation<Cached2, CacheKey2, Text, &Text::updateCache2> m_cache2{ this };

    void paint(Canvas& canvas) const override;
    std::optional<std::string> textContent() const override;
    void onFontChanged() override;
    void onChanged();
    void onLayoutUpdated() override;
    SizeF measure(AvailableSize size) const override;
    Ptr cloneThis() const override;
    explicit Text(Construction construction, std::string text, ArgumentsView<Text> args);

private:
    float calcFontSizeFor(const Font& font, const std::string& m_text) const;

public:
    static const auto& properties() noexcept {
        static constexpr tuplet::tuple props{
            /*0*/ Internal::PropFieldNotify{ &Text::m_text, &Text::onChanged, "text" },
            /*1*/ Internal::PropFieldNotify{ &Text::m_wordWrap, &Text::onChanged, "wordWrap" },
            /*2*/ Internal::PropFieldNotify{ &Text::m_rotation, &Text::onChanged, "rotation" },
            /*3*/ Internal::PropFieldNotify{ &Text::m_textAutoSize, &Text::onChanged, "textAutoSize" },
            /*4*/
            Internal::PropFieldNotify{ &Text::m_textAutoSizeRange, &Text::onChanged, "textAutoSizeRange" },
            /*5*/ Internal::PropFieldNotify{ &Text::m_textOptions, &Text::onChanged, "textOptions" },
        };
        return props;
    }

public:
    BRISK_PROPERTIES_BEGIN
    Property<Text, std::string, 0> text;
    Property<Text, bool, 1> wordWrap;
    Property<Text, Rotation, 2> rotation;
    Property<Text, TextAutoSize, 3> textAutoSize;
    Property<Text, Range<float>, 4> textAutoSizeRange;
    Property<Text, TextOptions, 5> textOptions;
    BRISK_PROPERTIES_END
};

template <typename T>
void applier(Text* target, ArgVal<Tag::Named<"text">, T> value) {
    target->text = value.value;
}

inline Rc<Text> operator""_Text(const char* text, size_t size) {
    return rcnew Text{ locale->translate(std::string_view(text, size)) };
}

inline namespace Arg {
#ifndef BRISK__TEXT_ARG_DEFINED
#define BRISK__TEXT_ARG_DEFINED
constexpr inline Argument<Tag::Named<"text">> text{};
#endif
constexpr inline PropArgument<decltype(Text::rotation)> rotation{};
constexpr inline PropArgument<decltype(Text::textAutoSize)> textAutoSize{};
constexpr inline PropArgument<decltype(Text::textAutoSizeRange)> textAutoSizeRange{};
constexpr inline PropArgument<decltype(Text::wordWrap)> wordWrap{};
constexpr inline PropArgument<decltype(Text::textOptions)> textOptions{};
} // namespace Arg

class WIDGET BackStrikedText final : public Text {
    BRISK_DYNAMIC_CLASS(BackStrikedText, Text)
public:
    using Base                                   = Text;
    constexpr static std::string_view widgetType = "backstrikedtext";

    template <WidgetArgument... Args>
    explicit BackStrikedText(std::string text, const Args&... args)
        : Text{ Construction{ widgetType }, std::move(text), std::tuple{ args... } } {
        endConstruction();
    }

    template <WidgetArgument... Args>
    explicit BackStrikedText(Value<std::string> text, const Args&... args)
        : Text{ Construction{ widgetType }, std::move(text), std::tuple{ args... } } {
        endConstruction();
    }

protected:
    void paint(Canvas& canvas) const override;
    Ptr cloneThis() const override;
};

struct TextBuilder : IndexedBuilder {
    template <WidgetArgument... Args>
    explicit TextBuilder(std::vector<std::string> texts, const Args&... args)
        : IndexedBuilder(
              [saved_texts = std::move(texts), args...](size_t index) BRISK_INLINE_LAMBDA -> Rc<Widget> {
                  return index < saved_texts.size() ? rcnew Text{ saved_texts[index], args... } : nullptr;
              }) {}
};

class WIDGET HoveredDescription final : public Text {
    BRISK_DYNAMIC_CLASS(HoveredDescription, Text)
public:
    using Base = Text;
    using Text::Text;
    using Text::widgetType;
    constexpr static double hoverDelay = 0.15;

protected:
    Ptr cloneThis() const override;
    void paint(Canvas& canvas) const override;

private:
    mutable std::optional<std::string> m_cachedText;
    mutable std::optional<double> m_lastChange;
};

class WIDGET ShortcutHint final : public Text {
    BRISK_DYNAMIC_CLASS(ShortcutHint, Text)
public:
    using Base                                   = Text;
    constexpr static std::string_view widgetType = "shortcuthint";

    template <WidgetArgument... Args>
    explicit ShortcutHint(Shortcut shortcut, const Args&... args)
        : Text{ Construction{ widgetType }, fmt::to_string(shortcut), std::tuple{ args... } } {
        endConstruction();
    }

protected:
    Ptr cloneThis() const override;
};

} // namespace Brisk
