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
#include <brisk/widgets/RadioButton.hpp>

namespace Brisk {

static void radioMark(Canvas& canvas, RectangleF markRect, ColorW color, float interpolatedValue) {
    float side = markRect.shortestSide();
    canvas.setStrokeColor(color.multiplyAlpha(0.35f));
    canvas.setStrokeWidth(1._dp);
    canvas.strokeEllipse(markRect.withPadding(1_dp));
    if (interpolatedValue > 0.f) {
        canvas.setFillColor(color.multiplyAlpha(0.75f));
        canvas.fillEllipse(markRect.alignedRect(interpolatedValue * side * 0.5f,
                                                interpolatedValue * side * 0.5f, 0.5f, 0.5f));
    }
}

void radioButtonPainter(Canvas& canvas, const Widget& widget_) {
    if (!dynamicCast<const RadioButton*>(&widget_)) {
        LOG_ERROR(widgets, "checkBoxPainter called for a non-CheckBox widget");
        return;
    }
    const RadioButton& widget = static_cast<const RadioButton&>(widget_);
    float interpolatedValue   = widget.interpolatedValue.get();

    Rectangle markRect        = widget.rect().alignedRect({ idp(14), idp(14) }, { 0.0f, 0.5f });
    boxPainter(canvas, widget, markRect);

    radioMark(canvas, markRect, widget.color.current(), interpolatedValue);
}

void RadioButton::paint(Canvas& canvas) const {
    radioButtonPainter(canvas, *this);
}

RC<Widget> RadioButton::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RadioButton::RadioButton(Construction construction, ArgumentsView<RadioButton> args)
    : Base(construction, nullptr) {
    args.apply(this);
}
} // namespace Brisk
