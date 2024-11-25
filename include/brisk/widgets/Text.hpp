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

#include <brisk/gui/GUI.hpp>
#include <brisk/core/Utilities.hpp>

namespace Brisk {

class WIDGET Text : public Widget {
public:
    using Base                                   = Widget;
    constexpr static std::string_view widgetType = "text";

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
    TextAutoSize m_textAutoSize      = TextAutoSize::None;
    Range<float> m_textAutoSizeRange = { 6.f, 96.f };
    Rotation m_rotation              = Rotation::NoRotation;
    bool m_wordWrap                  = false;

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
    optional<std::string> textContent() const override;
    void onFontChanged() override;
    void onChanged();
    void onLayoutUpdated() override;
    SizeF measure(AvailableSize size) const override;
    Ptr cloneThis() override;
    explicit Text(Construction construction, std::string text, ArgumentsView<Text> args);

private:
    float calcFontSizeFor(const Font& font, const std::string& m_text) const;

public:
    BRISK_PROPERTIES_BEGIN
    Property<Text, std::string, &Text::m_text, nullptr, nullptr, &Text::onChanged> //
        text;
    Property<Text, bool, &Text::m_wordWrap, nullptr, nullptr, &Text::onChanged> //
        wordWrap;
    Property<Text, Rotation, &Text::m_rotation, nullptr, nullptr, &Text::onChanged> //
        rotation;
    Property<Text, TextAutoSize, &Text::m_textAutoSize, nullptr, nullptr, &Text::onChanged> //
        textAutoSize;
    Property<Text, Range<float>, &Text::m_textAutoSizeRange, nullptr, nullptr, &Text::onChanged> //
        textAutoSizeRange;
    BRISK_PROPERTIES_END
};

inline namespace Arg {
constexpr inline Argument<Tag::PropArg<decltype(Text::text)>> text{};
constexpr inline Argument<Tag::PropArg<decltype(Text::rotation)>> rotation{};
constexpr inline Argument<Tag::PropArg<decltype(Text::textAutoSize)>> textAutoSize{};
constexpr inline Argument<Tag::PropArg<decltype(Text::textAutoSizeRange)>> textAutoSizeRange{};
constexpr inline Argument<Tag::PropArg<decltype(Text::wordWrap)>> wordWrap{};
} // namespace Arg

class WIDGET BackStrikedText final : public Text {
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
    Ptr cloneThis() override;
};

struct TextBuilder : IndexedBuilder {
    template <WidgetArgument... Args>
    explicit TextBuilder(std::vector<std::string> texts, const Args&... args)
        : IndexedBuilder(
              [saved_texts = std::move(texts), args...](size_t index) BRISK_INLINE_LAMBDA -> Widget* {
                  return index < saved_texts.size() ? new Text{ saved_texts[index], args... } : nullptr;
              }) {}
};

class WIDGET HoveredDescription final : public Text {
public:
    using Base = Text;
    using Text::Text;
    using Text::widgetType;
    constexpr static double hoverDelay = 0.15;

protected:
    Ptr cloneThis() override;
    void paint(Canvas& canvas) const override;

private:
    mutable optional<std::string> m_cachedText;
    mutable optional<double> m_lastChange;
};

} // namespace Brisk
