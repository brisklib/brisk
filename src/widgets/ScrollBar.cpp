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
#include <brisk/gui/Styles.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/widgets/ScrollBar.hpp>

namespace Brisk {

void scrollBarPainter(Canvas& canvas_, const Widget& widget_) {
    if (!dynamic_cast<const ScrollBar*>(&widget_)) {
        LOG_ERROR(widgets, "scrollBarPainter called for a non-ScrollBar widget");
        return;
    }
    boxPainter(canvas_, widget_);
    const ScrollBar& widget = static_cast<const ScrollBar&>(widget_);

    float borderRadius      = dp(4);
    ColorF color = widget.getStyleVar<ColorF>(selectedColor.id).value_or(Palette::Standard::indigo);

    canvas_.raw().drawRectangle(widget.handleRect(), borderRadius, 0.f, strokeWidth = 0.f,
                                fillColor = color.lighter(widget.isDragActive() ? -8
                                                          : widget.isHovered()  ? +8
                                                                                : 0));
}

void ScrollBar::paint(Canvas& canvas) const {
    scrollBarPainter(canvas, *this);
}

void ScrollBar::onEvent(Event& event) {
    Widget::onEvent(event);
    const int p                = m_rect.width() > m_rect.height() ? 0 : 1;
    const Rectangle handleRect = this->handleRect();
    Rectangle beforeHandleRect = m_rect;
    Rectangle afterHandleRect  = m_rect;
    beforeHandleRect[2 + p]    = handleRect[p];
    afterHandleRect[p]         = handleRect[2 + p];

    if (float y = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        pageUp(y * 0.1f);
        event.stopPropagation();
    } else if (event.released(beforeHandleRect)) {
        pageUp();
        event.stopPropagation();
    } else if (event.released(afterHandleRect)) {
        pageDown();
        event.stopPropagation();
    } else {
        switch (const auto [flag, offset, mod] = event.dragged(handleRect, m_dragActive); flag) {
        case DragEvent::Started: {
            m_savedPosition = value;
            event.stopPropagation();
            break;
        }
        case DragEvent::Dragging: {
            float range = m_pageStep / (m_pageStep + m_maximum);
            value       = m_savedPosition + offset[p] / range;
            event.stopPropagation();
            break;
        }
        case DragEvent::Dropped:
            event.stopPropagation();
            break;
        default:
            break;
        }
    }
}

Rectangle ScrollBar::handleRect() const {
    float range = m_pageStep / (m_pageStep + m_maximum);
    float value = this->normalizedValue;

    if (m_rect.width() > m_rect.height()) {
        return m_rect.alignedRect(std::max(std::clamp(range, 0.f, 1.f) * m_rect.width(), 8_dp),
                                  m_rect.height(), value, 0.5f);
    } else {
        return m_rect.alignedRect(m_rect.width(),
                                  std::max(std::clamp(range, 0.f, 1.f) * m_rect.height(), 8_dp), 0.5f, value);
    }
}

ScrollBar::ScrollBar(Construction construction, ArgumentsView<ScrollBar> args)
    : ValueWidget(construction, nullptr) {
    m_ignoreChildrenOffset = true;
    args.apply(this);
}

Widget::Ptr ScrollBar::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

bool ScrollBar::isDragActive() const noexcept {
    return m_dragActive;
}
} // namespace Brisk
