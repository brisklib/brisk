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
#include <brisk/widgets/Knob.hpp>
#include <brisk/gui/Styles.hpp>
#include <brisk/graphics/Palette.hpp>

namespace Brisk {

void Knob::onEvent(Event& event) {
    Widget::onEvent(event);

    if (float delta = event.wheelScrolled(m_wheelModifiers)) {
        m_savedValue    = std::clamp(static_cast<float>(normalizedValue) + delta / 24.f, 0.f, 1.f);
        normalizedValue = m_savedValue;
        event.stopPropagation();
        return;
    }

    switch (const auto [flag, offset, mods] = event.dragged(m_dragActive); flag) {
    case DragEvent::Started:
        focus();
        m_savedValue = normalizedValue;
        startModifying();
        event.stopPropagation();
        return;
    case DragEvent::Dragging: {
        const float unit_distance = mods && KeyModifiers::Shift ? 1500_dp : 150.0_dp;
        normalizedValue = std::clamp((offset.x - offset.y) / unit_distance + m_savedValue, 0.f, 1.f);
        startModifying();
        event.stopPropagation();
        return;
    } break;
    case DragEvent::Dropped:
        stopModifying();
        event.stopPropagation();
        return;
    default:
        break;
    }

    if (auto e = event.as<EventKeyPressed>()) {
        switch (e->key) {
        case KeyCode::Up:
            value = std::min(static_cast<float>(normalizedValue) + 0.01f, 1.f);
            event.stopPropagation();
            break;
        case KeyCode::Down:
            value = std::min(static_cast<float>(normalizedValue) - 0.01f, 1.f);
            event.stopPropagation();
            break;
        case KeyCode::PageUp:
            value = std::min(static_cast<float>(normalizedValue) + 0.1f, 1.f);
            event.stopPropagation();
            break;
        case KeyCode::PageDown:
            value = std::max(static_cast<float>(normalizedValue) - 0.1f, 0.f);
            event.stopPropagation();
            break;
        case KeyCode::Home:
            value = 0.f;
            event.stopPropagation();
            break;
        case KeyCode::End:
            value = 1.f;
            event.stopPropagation();
            break;
        default:
            break;
        }
    }
}

void knobPainter(Canvas& canvas, const Widget& widget_) {
    if (!dynamicCast<const Knob*>(&widget_)) {
        LOG_ERROR(widgets, "knobPainter called for a non-Knob widget");
        return;
    }
    const Knob& widget               = static_cast<const Knob&>(widget_);

    RectangleF rect                  = widget.rect();

    ColorW selectColor               = widget.borderColor.current();
    ColorW backColor                 = selectColor.multiplyAlpha(0.33f);
    constexpr float spread           = 0.75f * 180;
    const PointF center              = rect.center().round();
    const float radius               = rect.shortestSide() * 0.5f;
    constexpr float innerRadiusRatio = 0.6f;
    Path path;
    path.addCircle(center.x, center.y, radius);
    path.addCircle(center.x, center.y, radius * innerRadiusRatio, Path::Direction::CCW);
    canvas.setFillColor(backColor);
    canvas.fillPath(path);
    path.reset();
    float startAngle  = -spread;
    float sweepLength = 2 * widget.normalizedValue * spread;
    path.arcTo(center.alignedRect(Size(radius * 2), PointF(0.5f, 0.5f)), startAngle, -sweepLength, true);
    path.arcTo(center.alignedRect(Size(radius * 2 * innerRadiusRatio), PointF(0.5f, 0.5f)),
               startAngle - sweepLength, sweepLength, false);
    path.close();
    canvas.setFillColor(selectColor);
    canvas.fillPath(path);
}

void Knob::paint(Canvas& canvas) const {
    knobPainter(canvas, *this);
}

Knob::Knob(Construction construction, ArgumentsView<Knob> args) : Base(construction, nullptr) {
    m_tabStop         = true;
    m_isHintExclusive = true;
    args.apply(this);
}

Rc<Widget> Knob::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}
} // namespace Brisk
