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
#include <brisk/widgets/Text.hpp>
#include <brisk/core/Text.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/window/Clipboard.hpp>
#include "utf8proc.h"

namespace Brisk {

static TextLayoutAlignment toTextLayoutAlignment(TextAlign align) {
    switch (align) {
    case TextAlign::Start:
        return TextLayoutAlignment::Start;
    case TextAlign::Center:
        return TextLayoutAlignment::Center;
    case TextAlign::End:
        return TextLayoutAlignment::End;
    }
    return TextLayoutAlignment::Start;
}

Text::Text(Construction construction, std::string text, ArgumentsView<Text> args)
    : Widget{ construction, nullptr }, m_text(std::move(text)) {
    args.apply(this);
    onChanged();
    enableCustomMeasure();
}

Rc<Widget> Text::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void Text::onLayoutUpdated() {
    if (m_textAutoSize != TextAutoSize::None || m_wordWrap) {
        onChanged();
    }
}

void Text::onChanged() {
    normalizeSelection();
    invalidate();
    Font font = this->font();
    if (!m_wordWrap && m_textAutoSize != TextAutoSize::None && !m_text.empty()) {
        font.fontSize = calcFontSizeFor(font, m_text);
    }
    if (m_cache.invalidate(CacheKey{ font, m_text })) {
        if (m_wordWrap || m_textAutoSize == TextAutoSize::None) {
            requestUpdateLayout();
        }
        m_cache2.invalidate({ m_clientRect.width(), toTextLayoutAlignment(m_textAlign) }, true);
    } else if (m_wordWrap) {
        m_cache2.invalidate({ m_clientRect.width(), toTextLayoutAlignment(m_textAlign) });
    }
}

void Text::onSelectableChanged() {
    m_tabStop       = m_selectable;
    m_processClicks = !m_selectable;
    if (!m_selectable) {
        m_mouseSelection      = false;
        m_startCursorDragging = 0;
        m_cursor              = 0;
        m_selectedLength      = 0;
    }
    invalidate();
}

float Text::calcFontSizeFor(const Font& font, const std::string& m_text) const {
    float fontSize            = m_fontSize.current;
    const float refFontSize   = 32.f;
    Font refFont              = font;
    refFont.fontSize          = refFontSize;
    const ShapedText document = fonts->shapeText(refFont, TextWithOptions{ m_text, m_textOptions });
    SizeF sz =
        document.layout().bounds().size().flippedIf(toOrientation(m_rotation) == Orientation::Vertical);
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

static RectangleF textContainer(RectangleF inner, Rotation rotation) {
    return RectangleF{ 0, 0, inner.width(), inner.height() }.flippedIf(
        toOrientation(rotation) == Orientation::Vertical);
}

static Matrix textTransform(RectangleF inner, Rotation rotation) {
    const RectangleF rotated = textContainer(inner, rotation);
    return Matrix()
        .translate(-rotated.center().x, -rotated.center().y)
        .rotate90(static_cast<int>(rotation))
        .translate(inner.center().x, inner.center().y);
}

static PointF textOrigin(RectangleF container, const TextLayout& layout, PointF alignment) {
    const RectangleF bounds = layout.bounds();
    const PointF anchor      = container.at(alignment.x, alignment.y);
    return anchor - PointF{ bounds.x1, bounds.y1 } - PointF(bounds.size()) * alignment;
}

static bool isWordCharacter(char32_t character) {
    const utf8proc_category_t category = utf8proc_category(character);
    return category >= UTF8PROC_CATEGORY_LU && category <= UTF8PROC_CATEGORY_NO;
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
        const int width = std::max(0, static_cast<int>(size.x.valueOr(16777216.f)));
        TextLayoutOptions options;
        options.maxLineWidth = static_cast<float>(width);
        options.alignment    = toTextLayoutAlignment(m_textAlign);
        return alignInflate(m_cache->shapedText.layout(options).bounds()).size();
    }
}

void Text::paint(Canvas& canvas) const {
    Widget::paint(canvas);
    if (m_opacity.current > 0.f) {
        m_cache2.invalidate({ m_clientRect.width(), toTextLayoutAlignment(m_textAlign) });
        const RectangleF inner   = m_clientRect;
        const ColorW color       = m_color.current.multiplyAlpha(m_opacity.current);
        const TextLayout layout  = m_cache2->layout;
        const PointF alignment    = { toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };
        const RectangleF container = m_rotation == Rotation::NoRotation ? inner : textContainer(inner, m_rotation);
        const PointF origin         = textOrigin(container, layout, alignment);

        if (m_selectable && m_selectedLength != 0) {
            canvas.setFillColor(ColorW(Palette::Standard::indigo).multiplyAlpha(isFocused() ? 0.85f : 0.5f));
            if (m_rotation != Rotation::NoRotation) {
                auto&& state     = canvas.saveState();
                state->transform = textTransform(inner, m_rotation);
                canvas.fillTextSelection(origin, layout, selection());
            } else {
                canvas.fillTextSelection(origin, layout, selection());
            }
        }
        canvas.setFillColor(color);
        if (m_rotation != Rotation::NoRotation) {
            auto&& state     = canvas.saveState();
            state->transform = textTransform(inner, m_rotation);
            canvas.fillText(origin, layout);
        } else {
            canvas.fillText(origin, layout);
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
    ShapedText shapedText = fonts->shapeText(key.font, TextWithOptions(key.text, m_textOptions));
    return { std::move(shapedText) };
}

Text::Cached2 Text::updateCache2(const CacheKey2& key) {
    m_cache.update();
    TextLayoutOptions options;
    options.maxLineWidth    = m_wordWrap ? static_cast<float>(key.width) : HUGE_VALF;
    options.alignment       = key.alignment;
    const TextLayout layout = m_cache->shapedText.layout(options);
    SizeF textSize          = layout.bounds().size();
    const TextLine line     = layout.line(0);
    textSize                = max(textSize, SizeF{ 0, line.ascender - line.descender });
    return { textSize, layout };
}

Range<uint32_t> Text::selection() const {
    const int32_t cursorPosition = static_cast<int32_t>(m_cursor);
    const int32_t selectionEnd   = cursorPosition + m_selectedLength;
    return { static_cast<uint32_t>(std::min(cursorPosition, selectionEnd)),
             static_cast<uint32_t>(std::max(cursorPosition, selectionEnd)) };
}

void Text::normalizeSelection() {
    const int32_t textLength      = static_cast<int32_t>(utf8ToUtf32(m_text).size());
    const int32_t cursorPosition  = static_cast<int32_t>(m_cursor);
    const int32_t selectionEnd    = cursorPosition + m_selectedLength;
    const int32_t newCursor       = std::clamp(cursorPosition, 0, textLength);
    const int32_t newSelectionEnd = std::clamp(selectionEnd, 0, textLength);
    m_cursor                      = static_cast<uint32_t>(newCursor);
    m_selectedLength              = newSelectionEnd - newCursor;
}

uint32_t Text::caretToOffset(PointF point) const {
    m_cache2.invalidate({ m_clientRect.width(), toTextLayoutAlignment(m_textAlign) });
    const TextLayout& layout = m_cache2->layout;
    if (layout.lineCount() == 0)
        return 0;

    const RectangleF inner   = m_clientRect;
    const RectangleF container = m_rotation == Rotation::NoRotation ? inner : textContainer(inner, m_rotation);
    if (m_rotation != Rotation::NoRotation) {
        if (auto inverse = textTransform(inner, m_rotation).invert())
            point = inverse->transform(point);
    }
    const PointF alignment = { toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign) };
    const CaretIndex caret = layout.hitTest(point - textOrigin(container, layout, alignment));
    return m_cache->shapedText.characterFromCaret(caret);
}

void Text::copyToClipboard() const {
    const Range<uint32_t> selected = selection();
    if (selected.min == selected.max)
        return;

    const std::u32string text = utf8ToUtf32(m_text);
    if (selected.max > text.size())
        return;
    Clipboard::setText(utf32ToUtf8(text.substr(selected.min, selected.distance())));
}

void Text::selectWordAtOffset(uint32_t offset) {
    const std::u32string text = utf8ToUtf32(m_text);
    const int32_t textLength   = static_cast<int32_t>(text.size());
    const int32_t cursor       = std::clamp(static_cast<int32_t>(offset), 0, textLength);
    int32_t begin = cursor;
    while (begin > 0 && isWordCharacter(text[static_cast<size_t>(begin - 1)]))
        --begin;

    int32_t end = cursor;
    while (end < textLength && isWordCharacter(text[static_cast<size_t>(end)]))
        ++end;

    m_cursor         = static_cast<uint32_t>(end);
    m_selectedLength = begin - end;
    invalidate();
}

void Text::onEvent(Event& event) {
    Base::onEvent(event);

    if (!m_selectable || isDisabled())
        return;

    if (event.doubleClicked()) {
        if (const auto mouse = event.as<EventMouse>())
            selectWordAtOffset(caretToOffset(mouse->point));
        event.stopPropagation();
        return;
    }

    switch (const auto [flag, offset, mods] = event.dragged(m_mouseSelection); flag) {
    case DragEvent::Started: {
        focus();
        const auto mouse         = event.as<EventMouse>();
        const PointF downPoint  = mouse && mouse->downPoint ? *mouse->downPoint : PointF{};
        m_cursor                = caretToOffset(downPoint);
        m_startCursorDragging  = static_cast<int32_t>(m_cursor);
        m_selectedLength       = 0;
        normalizeSelection();
        invalidate();
        event.stopPropagation();
    } break;
    case DragEvent::Dragging: {
        if (const auto mouse = event.as<EventMouse>()) {
            const uint32_t endCursor = caretToOffset(mouse->point);
            m_cursor                 = endCursor;
            m_selectedLength         = m_startCursorDragging - static_cast<int32_t>(endCursor);
            normalizeSelection();
            invalidate();
            event.stopPropagation();
        }
    } break;
    case DragEvent::Dropped:
        event.stopPropagation();
        break;
    default:
        break;
    }

    if (auto key = event.as<EventKeyPressed>(); key && key->key == KeyCode::C &&
        (key->mods & KeyModifiers::Regular) == KeyModifiers::ControlOrCommand &&
        m_selectedLength != 0) {
        copyToClipboard();
        event.stopPropagation();
    }
}

void BackStrikedText::paint(Canvas& canvas) const {
    Widget::paint(canvas);
    ColorW color = m_color.current.multiplyAlpha(m_opacity.current);
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
}

Rc<Widget> BackStrikedText::cloneThis() const {
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
        canvas.setFillColor(m_color.current.multiplyAlpha(m_opacity.current));
        canvas.fillText(*m_cachedText, m_clientRect,
                        PointF(toFloatAlign(m_textAlign), toFloatAlign(m_textVerticalAlign)));
    }
    paintHint(canvas);
}

Rc<Widget> HoveredDescription::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Rc<Widget> ShortcutHint::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
