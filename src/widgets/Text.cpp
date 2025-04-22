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
    registerBuiltinFonts();
    args.apply(this);
    onChanged();
    enableCustomMeasure();
}

RC<Widget> Text::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void Text::onLayoutUpdated() {
    if (m_textAutoSize != TextAutoSize::None || m_wordWrap) {
        onChanged();
    }
}

void Text::onChanged() {
    invalidate();
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
        uint32_t width        = size.x.valueOr(16777216.f);
        PreparedText prepared = m_cache->shaped.wrap(width);
        return alignInflate(prepared.bounds()).size();
    }
}

void Text::paint(Canvas& canvas) const {
    Widget::paint(canvas);
    if (m_opacity > 0.f) {
        RectangleF inner = m_clientRect;
        ColorW color     = m_color.current.multiplyAlpha(m_opacity);
        auto prepared    = m_cache2->prepared;

        canvas.setFillColor(color);
        if (m_rotation != Rotation::NoRotation) {
            RectangleF rotated = RectangleF{ 0, 0, inner.width(), inner.height() }.flippedIf(
                toOrientation(m_rotation) == Orientation::Vertical);
            Matrix m = Matrix()
                           .translate(-rotated.center().x, -rotated.center().y)
                           .rotate90(static_cast<int>(m_rotation))
                           .translate(inner.center().x, inner.center().y);
            PointF offset = prepared.alignLines(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign));
            auto&& state  = canvas.saveState();
            state->transform = m;
            canvas.fillText(rotated.at(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign)) + offset,
                            prepared);
        } else {
            PointF offset = prepared.alignLines(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign));
            prepared.updateCaretData();
            offset += inner.at(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign));
            canvas.fillText(offset, prepared);
        }
    }
}

std::optional<std::string> Text::textContent() const {
    return m_text;
}

void Text::onFontChanged() {
    onChanged();
}

Text::Cached Text::updateCache(const CacheKey& key) {
    PreparedText shaped = fonts->prepare(key.font, TextWithOptions(key.text, m_textOptions));
    return { std::move(shaped) };
}

Text::Cached2 Text::updateCache2(const CacheKey2& key) {
    m_cache.update();
    auto prepared  = m_cache->shaped.wrap(m_wordWrap ? key.width : 16777216.f);
    SizeF textSize = prepared.bounds().size();
    textSize       = max(textSize, SizeF{ 0, fonts->metrics(m_cache.key().font).vertBounds() });
    return { textSize, std::move(prepared) };
}

void BackStrikedText::paint(Canvas& canvas) const {
    ColorW color = m_color.current.multiplyAlpha(m_opacity);
    canvas.setFillColor(color);
    canvas.setFont(font());
    canvas.fillText(m_text, m_clientRect,
                    PointF(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign)));
    const int p         = 10_idp;
    const float x_align = toFloatAlign(m_textAlign);
    const int tw        = m_cache2->textSize.x;
    const Point c       = m_rect.withPadding(tw / 2, 0).at(x_align, 0.5f);
    Rectangle r1{ m_rect.x1 + p, c.y, c.x - tw / 2 - p, c.y + 1_idp };
    Rectangle r2{ c.x + tw / 2 + p, c.y, m_rect.x2 - p, c.y + 1_idp };
    if (r1.width() > 0)
        canvas.fillRect(r1);
    if (r2.width() > 0)
        canvas.fillRect(r2);
    Widget::paint(canvas);
}

RC<Widget> BackStrikedText::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void HoveredDescription::paint(Canvas& canvas) const {
    Widget::paintBackground(canvas, m_rect);
    std::string newText = inputQueue() ? inputQueue()->getHintAtMouse().value_or(m_text) : m_text;
    if (newText != m_cachedText) {
        m_cachedText = std::move(newText);
        m_lastChange = frameStartTime;
    }
    if (m_lastChange && frameStartTime - *m_lastChange > hoverDelay) {
        canvas.setFont(font());
        canvas.setFillColor(m_color.current);
        canvas.fillText(*m_cachedText, m_clientRect,
                        PointF(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign)));
    }
    paintHint(canvas);
}

RC<Widget> HoveredDescription::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
