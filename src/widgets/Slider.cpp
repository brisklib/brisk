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

void Slider::onLayoutUpdated() {
    Base::onLayoutUpdated();
    updateSliderGeometry();
}

void Slider::onChanged() {
    Base::onChanged();
    updateSliderGeometry();
}

void Slider::updateSliderGeometry() {
    const bool horizontal = orientation() == Orientation::Horizontal;
    m_trackRect           = horizontal ? m_rect.alignedRect(m_rect.width(), idp(trackThickness), 0.5f, 0.5f)
                                       : m_rect.alignedRect(idp(trackThickness), m_rect.height(), 0.5f, 0.5f);

    m_thumbRect = horizontal ? RectangleF(m_rect).alignedRect(thumbRadius * 3_dp, thumbRadius * 3_dp,
                                                              normalizedValue, 0.5f)
                             : RectangleF(m_rect).alignedRect(thumbRadius * 3_dp, thumbRadius * 3_dp, 0.5f,
                                                              1.f - normalizedValue);
}

void sliderPainter(Canvas& canvas_, const Widget& widget_) {
    if (!dynamic_cast<const Slider*>(&widget_)) {
        LOG_ERROR(widgets, "sliderPainter called for a non-Slider widget");
        return;
    }
    const Slider& widget = static_cast<const Slider&>(widget_);
    RawCanvas& canvas    = canvas_.raw();
    const bool hover     = widget.isHovered();
    const bool pressed   = widget.isPressed();

    ColorF backColor     = widget.backgroundColor.current();

    PointF pt      = widget.orientation() == Orientation::Horizontal ? PointF{ +1, 0 } : PointF{ 0, -1 };

    auto trackRect = widget.trackRect();
    auto thumbRect = widget.thumbRect();

    canvas.drawRectangle(trackRect, 0.f, 0.f, fillColors = { backColor, backColor.multiplyAlpha(0.33f) },
                         linearGradient = { thumbRect.center() - pt, thumbRect.center() + pt },
                         strokeWidth    = 0);

    canvas.drawEllipse(thumbRect, 0.f,
                       fillColor   = widget.borderColor.current().lighter(pressed ? -8
                                                                          : hover ? +8
                                                                                  : 0),
                       strokeWidth = 0);
}

void Slider::paint(Canvas& canvas_) const {
    sliderPainter(canvas_, *this);
}

void Slider::onEvent(Event& event) {
    Base::onEvent(event);

    if (float delta = event.wheelScrolled(m_rect, m_wheelModifiers)) {
        float val       = std::clamp(static_cast<float>(normalizedValue) + delta / 24.f, 0.f, 1.f);
        normalizedValue = val;
        event.stopPropagation();
    } else {
        const bool horizontal = orientation() == Orientation::Horizontal;

        m_distance =
            horizontal ? m_trackRect.width() - thumbRadius * 2_dp : m_trackRect.height() - thumbRadius * 2_dp;

        switch (const auto [flag, offset, mods] = event.dragged(m_thumbRect, m_drag); flag) {
        case DragEvent::Started:
            savedValue = normalizedValue;
            startModifying();
            event.stopPropagation();
            break;
        case DragEvent::Dragging:
            float newValue;
            if (horizontal)
                newValue = (offset.x) / m_distance + savedValue;
            else
                newValue = (-offset.y) / m_distance + savedValue;
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

Widget::Ptr Slider::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RectangleF Slider::thumbRect() const noexcept {
    return m_thumbRect;
}

Rectangle Slider::trackRect() const noexcept {
    return m_trackRect;
}

Orientation Slider::orientation() const noexcept {
    return m_rect.width() > m_rect.height() ? Orientation::Horizontal : Orientation::Vertical;
}
} // namespace Brisk
