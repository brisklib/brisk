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
#include <brisk/widgets/AutoScrollable.hpp>
#include <brisk/graphics/Palette.hpp>
#include <brisk/gui/Icons.hpp>

namespace Brisk {

void AutoScrollable::onEvent(Event& event) {
    Widget::onEvent(event);
    updateState(m_chevron1, event, chevronRect(LogicalDirection::UpOrLeft));
    updateState(m_chevron2, event, chevronRect(LogicalDirection::DownOrRight));
    if ((m_chevron1 && WidgetState::Hover) || (m_chevron2 && WidgetState::Hover)) {
        requestAnimationFrame();
    }
}

Rectangle AutoScrollable::chevronRect(LogicalDirection direction) const {
    Edge edge = direction == LogicalDirection::UpOrLeft
                    ? (m_orientation == Orientation::Vertical ? Edge::Top : Edge::Left)
                    : (m_orientation == Orientation::Vertical ? Edge::Bottom : Edge::Right);
    Size sz{ 22_idp, 22_idp };
    sz[-m_orientation] = m_rect.size()[-m_orientation];
    return m_rect.alignedRect(sz, { edge == Edge::Left ? 0.f : 1.f, edge == Edge::Top ? 0.f : 1.f });
}

void AutoScrollable::onLayoutUpdated() {
    const int innerSize = m_contentSize[+m_orientation];
    const int availSize = m_rect.size()[+m_orientation];
    if (innerSize > availSize && innerSize > 0) {
        m_scrollSize = innerSize - availSize;
    } else {
        m_scrollSize = 0;
    }
}

void AutoScrollable::postPaint(Canvas& canvas_) const {
    Widget::postPaint(canvas_);
    RawCanvas& canvas = canvas_.raw();
    ColorF selection  = getStyleVar<ColorF>(selectedColor.id).value_or(Palette::Standard::blue);
    if (m_enableAutoScroll) {
        if (m_scrollSize > 0) {
            if (m_offset > 0) {
                Rectangle chevron = chevronRect(LogicalDirection::UpOrLeft);
                canvas.drawRectangle(chevron, 0.f, 0.f, strokeWidth = 0.f,
                                     fillColor = selection.multiplyAlpha(0.9f));
                canvas.drawText(chevron, 0.5f, 0.5f,
                                m_orientation == Orientation::Vertical ? ICON_chevron_up : ICON_chevron_left,
                                font()(12_dp), Palette::white);
            }

            if (m_scrollSize - m_offset > 0) {
                Rectangle chevron = chevronRect(LogicalDirection::DownOrRight);
                canvas.drawRectangle(chevron, 0.f, 0.f, strokeWidth = 0.f,
                                     fillColor = selection.multiplyAlpha(0.9f));
                canvas.drawText(chevron, 0.5f, 0.5f,
                                m_orientation == Orientation::Vertical ? ICON_chevron_down
                                                                       : ICON_chevron_right,
                                font()(12_dp), Palette::white);
            }
        }
    }
}

void AutoScrollable::onAnimationFrame() {
    int newOffset = m_offset;
    if (m_enableAutoScroll) {
        if (m_visible) {
            if (m_chevron1 && WidgetState::Hover || m_chevron2 && WidgetState::Hover) {
                newOffset = std::max(0, std::min(newOffset, m_scrollSize));
                if (m_scrollSize > 0) {
                    if (newOffset > 0) {
                        if (m_chevron1 && WidgetState::Hover) {
                            newOffset = std::max(0, newOffset - 2_idp);
                        }
                    }
                    if (m_scrollSize - newOffset > 0) {
                        if (m_chevron2 && WidgetState::Hover) {
                            newOffset = std::min(newOffset + 2_idp, m_scrollSize);
                        }
                    }
                }
            }
        } else {
            newOffset = 0;
        }
    } else {
        newOffset = 0;
    }
    if (newOffset != m_offset) {
        m_offset = newOffset;
        setChildrenOffset(Point{ -m_offset, 0 }.flippedIf(m_orientation == Orientation::Vertical));
        requestAnimationFrame();
    }
}

AutoScrollable::AutoScrollable(Construction construction, Orientation orientation,
                               ArgumentsView<AutoScrollable> args)
    : Widget{ construction, nullptr }, m_orientation(orientation) {
    args.apply(this);
}

Widget::Ptr AutoScrollable::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}
} // namespace Brisk
