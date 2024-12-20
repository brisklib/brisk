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
#include <brisk/widgets/ScrollBox.hpp>

namespace Brisk {

ScrollBox::ScrollBox(Construction construction, Orientation orientation, ArgumentsView<ScrollBox> args)
    : Widget{ construction,
              std::tuple{ Arg::overflow =
                              orientation == Orientation::Horizontal ? Overflow::ScrollX : Overflow::ScrollY,
                          Arg::alignItems = AlignItems::FlexStart } },
      m_orientation(orientation) {
    args.apply(this);
    createScrollBar();
}

Widget::Ptr ScrollBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Widget::Ptr HScrollBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Widget::Ptr VScrollBox::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

void ScrollBox::createScrollBar() {
    bool h                  = m_orientation == Orientation::Horizontal;
    RC<ScrollBar> scrollBar = rcnew ScrollBar{
        Arg::value            = Value{ &scrollPosition },
        Arg::placement        = Placement::Absolute,
        Arg::absolutePosition = PointL{ 100_perc, 0 }.flippedIf(h),
        Arg::anchor           = PointL{ 100_perc, 0 }.flippedIf(h),
        Arg::dimensions       = SizeL{ undef, 100_perc }.flippedIf(h),
    };
    scrollBar->minimum = 0;
    apply(std::move(scrollBar));
}

std::shared_ptr<ScrollBar> ScrollBox::scrollBar() const {
    return find<ScrollBar>(MatchAny{});
}

#ifdef BRISK_MACOS
constexpr float scrollPixels = 20.f;
#else
constexpr float scrollPixels = 140.f;
#endif

bool ScrollBox::setScrollOffset(float value) {
    if (bindings->assign(m_scrollPosition, value)) {
        updateOffsets();
        return true;
    }
    return false;
}

void ScrollBox::updateOffsets() {
    PointF p{ 0.f, 0.f };

    if (scrollable())
        p[+m_orientation] = -m_scrollPosition;
    else
        p[+m_orientation] = 0;
    setChildrenOffset(p);
}

void ScrollBox::onLayoutUpdated() {
    scrollBar()->step       = dp(30);
    const int contentSize   = m_contentSize[+m_orientation];
    const int availableSize = m_rect.size()[+m_orientation];
    if (contentSize > availableSize && contentSize > 0) {
        m_scrollSize          = contentSize - availableSize;
        scrollBar()->maximum  = m_scrollSize;
        scrollBar()->pageStep = availableSize;
        scrollBar()->visible  = true;
    } else {
        scrollBar()->visible  = false;
        scrollBar()->pageStep = 0;
        scrollBar()->maximum  = 0;
        m_scrollSize          = 0;
        bindings->assign(m_scrollPosition, 0);
    }
    updateOffsets();
}

void ScrollBox::onEvent(Event& event) {
    Widget::onEvent(event);
    if (float d = event.wheelScrolled(m_orientation == Orientation::Vertical ? WheelOrientation::Y
                                                                             : WheelOrientation::X)) {
        if (scrollable()) {
            if (setScrollOffset(std::clamp(m_scrollPosition - d * dp(scrollPixels), 0.f,
                                           static_cast<float>(m_scrollSize)))) {
                event.stopPropagation();
            }
        }
    }
}

bool ScrollBox::scrollable() const {
    return m_scrollSize > 0;
}

void ScrollBox::revealChild(Widget* child) {
    if (scrollable()) {
        Rectangle containerRect = m_rect;
        Rectangle childRect     = child->rect();
        int32_t offset          = childRect.p1[+m_orientation] - containerRect.p1[+m_orientation];
        if (offset < 0) {
            setScrollOffset(std::clamp(m_scrollPosition + offset, 0.f, static_cast<float>(m_scrollSize)));
        } else {
            offset = childRect.p2[+m_orientation] - containerRect.p2[+m_orientation];
            if (offset > 0) {
                setScrollOffset(std::clamp(m_scrollPosition + offset, 0.f, static_cast<float>(m_scrollSize)));
            }
        }
    }

    Widget::revealChild(child);
}
} // namespace Brisk
