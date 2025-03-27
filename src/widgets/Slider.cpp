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
#include <brisk/widgets/Slider.hpp>

namespace Brisk {

Rectangle Slider::trackRect() const noexcept {
    return orientation() == Orientation::Horizontal
               ? m_rect.alignedRect(m_rect.width(), idp(trackThickness), 0.5f, 0.5f)
               : m_rect.alignedRect(idp(trackThickness), m_rect.height(), 0.5f, 0.5f);
}

RectangleF Slider::thumbRect() const noexcept {
    return orientation() == Orientation::Horizontal
               ? RectangleF(m_rect).alignedRect(thumbRadius * 3_dp, thumbRadius * 3_dp, normalizedValue, 0.5f)
               : RectangleF(m_rect).alignedRect(thumbRadius * 3_dp, thumbRadius * 3_dp, 0.5f,
                                                1.f - normalizedValue);
}

void sliderPainter(Canvas& canvas, const Widget& widget_) {
    if (!dynamicCast<const Slider*>(&widget_)) {
        LOG_ERROR(widgets, "sliderPainter called for a non-Slider widget");
        return;
    }
    const Slider& widget = static_cast<const Slider&>(widget_);

    ColorW backColor     = widget.backgroundColor.current();

    PointF pt      = widget.orientation() == Orientation::Horizontal ? PointF{ +1, 0 } : PointF{ 0, -1 };

    auto trackRect = widget.trackRect();
    auto thumbRect = widget.thumbRect();

    Gradient grad(GradientType::Linear, thumbRect.center() - pt, thumbRect.center() + pt, backColor,
                  backColor.multiplyAlpha(0.33f));
    canvas.setFillPaint(std::move(grad));
    canvas.fillRect(trackRect);
    canvas.setFillColor(widget.borderColor.current());
    canvas.fillEllipse(thumbRect);
}

void Slider::paint(Canvas& canvas) const {
    sliderPainter(canvas, *this);
}

void Slider::onEvent(Event& event) {
    Base::onEvent(event);

    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        float val       = std::clamp(static_cast<float>(normalizedValue) + delta / 24.f, 0.f, 1.f);
        normalizedValue = val;
        event.stopPropagation();
    } else {
        const bool horizontal = orientation() == Orientation::Horizontal;

        auto trackRect        = this->trackRect();
        auto thumbRect        = this->thumbRect();

        m_distance =
            horizontal ? trackRect.width() - thumbRadius * 2_dp : trackRect.height() - thumbRadius * 2_dp;

        switch (const auto [flag, offset, mods] = event.dragged(thumbRect, m_drag); flag) {
        case DragEvent::Started:
            m_savedValue = normalizedValue;
            startModifying();
            event.stopPropagation();
            break;
        case DragEvent::Dragging:
            float newValue;
            if (horizontal)
                newValue = (offset.x) / m_distance + m_savedValue;
            else
                newValue = (-offset.y) / m_distance + m_savedValue;
            normalizedValue = std::clamp(newValue, 0.f, 1.f);
            startModifying();
            if (m_hintFormatter)
                m_hint = m_hintFormatter(m_value);
            event.stopPropagation();
            break;
        case DragEvent::Dropped:
            stopModifying();
            break;
        default:
            break;
        }
    }
}

Slider::Slider(Construction construction, ArgumentsView<Slider> args) : Base(construction, nullptr) {
    m_tabStop         = true;
    m_isHintExclusive = true;
    args.apply(this);
}

RC<Widget> Slider::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Orientation Slider::orientation() const noexcept {
    return m_rect.width() > m_rect.height() ? Orientation::Horizontal : Orientation::Vertical;
}
} // namespace Brisk
