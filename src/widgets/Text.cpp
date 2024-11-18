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
#include <brisk/widgets/Text.hpp>

namespace Brisk {

Text::Text(Construction construction, std::string text, ArgumentsView<Text> args)
    : Widget{ construction, nullptr }, m_text(std::move(text)) {
    args.apply(this);
    onChanged();
    enableCustomMeasure();
}

Widget::Ptr Text::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

void Text::onLayoutUpdated() {
    if (m_textAutoSize != TextAutoSize::None || m_wordWrap) {
        onChanged();
    }
}

void Text::onChanged() {
    Font font = this->font();
    if (!m_wordWrap && m_textAutoSize != TextAutoSize::None && !m_text.empty()) {
        font.fontSize = font.fontSize = calcFontSizeFor(font, m_text);
    }
    if (m_cache.invalidate(CacheKey{ font, m_text })) {
        if (m_wordWrap || m_textAutoSize == TextAutoSize::None) {
            requestUpdateLayout();
        }
        m_cache2.invalidate({ m_clientRect.width() }, true);
    } else if (m_wordWrap) {
        m_cache2.invalidate({ m_clientRect.width() });
    }
}

float Text::calcFontSizeFor(const Font& font, const std::string& m_text) const {
    float fontSize          = m_fontSize.resolved;
    const float refFontSize = 32.f;
    Font refFont            = font;
    refFont.fontSize        = refFontSize;
    SizeF sz                = fonts->bounds(refFont, utf8ToUtf32(m_text))
                   .size()
                   .flippedIf(toOrientation(m_rotation) == Orientation::Vertical);
    if (sz.width != 0 && sz.height != 0) {
        switch (m_textAutoSize) {
        case TextAutoSize::FitWidth:
            fontSize = refFontSize * m_clientRect.width() / sz.width;
            break;
        case TextAutoSize::FitHeight:
            fontSize = refFontSize * m_clientRect.height() / sz.height;
            break;
        case TextAutoSize::FitSize:
            fontSize = std::min(refFontSize * m_clientRect.width() / sz.width,
                                refFontSize * m_clientRect.height() / sz.height);
            break;
        case TextAutoSize::None:
            break;
        default:
            break;
        }
        fontSize = std::clamp(fontSize, dp(m_textAutoSizeRange.min), dp(m_textAutoSizeRange.max));
    }
    return fontSize;
}

static RectangleF alignInflate(RectangleF rect) {
    rect.x1 = std::floor(rect.x1);
    rect.y1 = std::floor(rect.y1);
    rect.x2 = std::ceil(rect.x2);
    rect.y2 = std::ceil(rect.y2);
    return rect;
}

SizeF Text::measure(AvailableSize size) const {
    if (!m_wordWrap && m_textAutoSize != TextAutoSize::None) {
        return SizeF{ 1.f, 1.f };
    }
    if (!m_wordWrap) {
        SizeF result = m_cache2->textSize;
        if (toOrientation(m_rotation) == Orientation::Vertical) {
            result = result.flipped();
        }
        return result;
    } else {
        uint32_t width              = size.x.valueOr(16777216.f);
        PrerenderedText prerendered = m_cache->shaped.prerender(m_cache.key().font, width);
        return alignInflate(prerendered.bounds()).size();
    }
}

void Text::paint(Canvas& canvas) const {
    Widget::paint(canvas);
    if (m_opacity > 0.f) {
        auto&& state = canvas.raw().save();
        if (m_clip == WidgetClip::None)
            state->scissors = noScissors;
        else
            state.intersectScissors(m_rect);
        RectangleF inner = m_clientRect;
        ColorF color     = m_color.current.multiplyAlpha(m_opacity);
        auto prerendered = m_cache2->prerendered;

        if (m_rotation != Rotation::NoRotation) {
            RectangleF rotated = RectangleF{ 0, 0, inner.width(), inner.height() }.flippedIf(
                toOrientation(m_rotation) == Orientation::Vertical);
            Matrix2D m = Matrix2D()
                             .translate(-rotated.center().x, -rotated.center().y)
                             .rotate90(static_cast<int>(m_rotation))
                             .translate(inner.center().x, inner.center().y);
            Matrix2D invm = Matrix2D()
                                .translate(-inner.center().x, -inner.center().y)
                                .rotate90(-static_cast<int>(m_rotation))
                                .translate(rotated.center().x, rotated.center().y);
            prerendered.alignLines(rotated, toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign));
            state->scissors = invm.transform(state->scissors);
            canvas.raw().drawText(prerendered, fillColor = color, coordMatrix = m);
        } else {
            prerendered.alignLines(inner, toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign));
            canvas.raw().drawText(prerendered, fillColor = color);
        }
    }
}

optional<std::string> Text::textContent() const {
    return m_text;
}

void Text::onFontChanged() {
    onChanged();
}

Text::Cached Text::updateCache(const CacheKey& key) {
    ShapedRuns shaped = fonts->shape(key.font, key.text);
    return { std::move(shaped) };
}

Text::Cached2 Text::updateCache2(const CacheKey2& key) {
    m_cache.update();
    auto prerendered = m_cache->shaped.prerender(m_cache.key().font, m_wordWrap ? key.width : 16777216.f);
    SizeF textSize   = prerendered.bounds().size();
    textSize         = max(textSize, SizeF{ 0, fonts->metrics(m_cache.key().font).vertBounds() });
    return { textSize, std::move(prerendered) };
}

void BackStrikedText::paint(Canvas& canvas) const {
    canvas.raw().drawText(m_clientRect, toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign), m_text,
                          font(), m_color.current);
    const int p         = 10_idp;
    const float x_align = toFloatAlign(m_textAlign);
    const int tw        = m_cache2->textSize.x;
    const Point c       = m_rect.withPadding(tw / 2, 0).at(x_align, 0.5f);
    Rectangle r1{ m_rect.x1 + p, c.y, c.x - tw / 2 - p, c.y + 1_idp };
    Rectangle r2{ c.x + tw / 2 + p, c.y, m_rect.x2 - p, c.y + 1_idp };
    if (r1.width() > 0)
        canvas.raw().drawRectangle(r1, 0.f, 0.f, fillColor = m_color.current, strokeWidth = 0.f);
    if (r2.width() > 0)
        canvas.raw().drawRectangle(r2, 0.f, 0.f, fillColor = m_color.current, strokeWidth = 0.f);
    Widget::paint(canvas);
}

Widget::Ptr BackStrikedText::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

void HoveredDescription::paint(Canvas& canvas) const {
    Widget::paintBackground(canvas, m_rect);
    std::string newText = inputQueue->getDescriptionAtMouse().value_or(m_text);
    if (newText != m_cachedText) {
        m_cachedText = std::move(newText);
        m_lastChange = frameStartTime;
    }
    if (m_lastChange && frameStartTime - *m_lastChange > hoverDelay) {
        canvas.raw().drawText(m_clientRect, toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign),
                              *m_cachedText, font(), m_color.current);
    }
    paintHint(canvas);
}

Widget::Ptr HoveredDescription::cloneThis() {
    BRISK_CLONE_IMPLEMENTATION;
}

} // namespace Brisk
