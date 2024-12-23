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
#include <brisk/widgets/Switch.hpp>

namespace Brisk {

void Switch::onEvent(Event& event) {
    Widget::onEvent(event);
    if (event.pressed()) {
        if (!isDisabled()) {
            focus();
        }
        event.stopPropagation();
    } else if (event.released()) {
        if (!isDisabled()) {
            if (auto m = event.as<EventMouse>()) {
                if (m_rect.contains(m->point) || m->point.x > m_rect.center().x != m_value) {
                    doClick();
                }
            }
        }
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        if (!isDisabled()) {
            toggleState(WidgetState::Pressed, true);
            doClick();
        }
        event.stopPropagation();
    } else if (event.keyReleased(KeyCode::Enter) || event.keyReleased(KeyCode::Space)) {
        toggleState(WidgetState::Pressed, false);
        event.stopPropagation();
    }
}

void switchPainter(Canvas& canvas_, const Widget& widget_) {
    if (!dynamic_cast<const Switch*>(&widget_)) {
        LOG_ERROR(widgets, "switchPainter called for a non-Switch widget");
        return;
    }
    const Switch& widget = static_cast<const Switch&>(widget_);
    auto& animatedValue  = widget.animatedValue();

    RawCanvas& canvas    = canvas_.raw();
    if (!animatedValue)
        animatedValue = widget.value.get() ? 1.f : 0.f;
    else {
        if (widget.value.get())
            animatedValue = std::min(*animatedValue + 0.2f, 1.f);
        else
            animatedValue = std::max(*animatedValue - 0.2f, 0.f);
    }
    RectangleF outerRect = widget.rect().alignedRect({ idp(24), idp(14) }, { 0.0f, 0.5f }).withPadding(dp(1));
    RectangleF outerRectWithPadding = outerRect.withPadding(dp(2));
    RectangleF innerRect            = outerRectWithPadding.alignedRect(
        outerRectWithPadding.height(), outerRectWithPadding.height(), *animatedValue, 0.5f);
    canvas.drawRectangle(outerRect, outerRect.shortestSide() * 0.5f, 0.f,
                         fillColor = mix(*animatedValue, ColorF(0.f, 0.f), widget.backgroundColor.current()),
                         strokeWidth = 1._dp, strokeColor = widget.color.current().multiplyAlpha(0.35f));
    canvas.drawRectangle(innerRect, innerRect.shortestSide() * 0.5f, 0.f,
                         fillColor = widget.color.current().multiplyAlpha(0.75f), strokeWidth = 0.f);
}

void Switch::paint(Canvas& canvas_) const {
    switchPainter(canvas_, *this);
}

RC<Widget> Switch::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Switch::Switch(Construction construction, ArgumentsView<Switch> args)
    : Base(construction, nullptr) {
    args.apply(this);
}

optional<float>& Switch::animatedValue() const {
    return m_animatedValue;
}
} // namespace Brisk
