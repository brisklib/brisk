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
#include <brisk/widgets/Switch.hpp>

namespace Brisk {

void Switch::onEvent(Event& event) {
    Widget::onEvent(event);
    if (event.pressed()) {
        focus();
        event.stopPropagation();
    } else if (event.released()) {
        if (auto m = event.as<EventMouse>()) {
            if (m_rect.contains(m->point) || m->point.x > m_rect.center().x != m_value) {
                doClick();
            }
        }
        event.stopPropagation();
    } else if (event.keyPressed(KeyCode::Enter) || event.keyPressed(KeyCode::Space)) {
        toggleState(WidgetState::Pressed, true);
        doClick();
        event.stopPropagation();
    } else if (event.keyReleased(KeyCode::Enter) || event.keyReleased(KeyCode::Space)) {
        toggleState(WidgetState::Pressed, false);
        event.stopPropagation();
    }
}

void switchPainter(Canvas& canvas, const Widget& widget_) {
    if (!dynamicCast<const Switch*>(&widget_)) {
        LOG_ERROR(widgets, "switchPainter called for a non-Switch widget");
        return;
    }
    const Switch& widget    = static_cast<const Switch&>(widget_);
    float interpolatedValue = widget.interpolatedValue.get();

    RectangleF outerRect = widget.rect().alignedRect({ idp(24), idp(14) }, { 0.0f, 0.5f }).withPadding(dp(1));
    RectangleF outerRectWithPadding = outerRect.withPadding(dp(2));
    RectangleF innerRect            = outerRectWithPadding.alignedRect(
        outerRectWithPadding.height(), outerRectWithPadding.height(), interpolatedValue, 0.5f);
    canvas.setFillColor(mix(interpolatedValue, ColorW(0.f, 0.f), widget.backgroundColor.current()));
    canvas.setStrokeWidth(1._dp);
    canvas.setStrokeColor(widget.color.current().multiplyAlpha(0.35f));
    canvas.drawRect(outerRect, outerRect.shortestSide() * 0.5f);
    canvas.setFillColor(widget.color.current().multiplyAlpha(0.75f));
    canvas.fillRect(innerRect, innerRect.shortestSide() * 0.5f);
}

void Switch::paint(Canvas& canvas) const {
    switchPainter(canvas, *this);
}

Rc<Widget> Switch::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Switch::Switch(Construction construction, ArgumentsView<Switch> args)
    : Base(construction, nullptr) {
    args.apply(this);
}

} // namespace Brisk
