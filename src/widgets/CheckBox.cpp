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
#include <brisk/widgets/CheckBox.hpp>

namespace Brisk {

static void checkMark(Canvas& canvas, RectangleF markRect, ColorW color, float interpolatedValue,
                      bool disabled) {
    canvas.setStrokeColor(color.multiplyAlpha(0.35f));
    canvas.setStrokeWidth(1._dp);
    if (disabled) {
        canvas.setFillColor(0x80808080_rgba);
        canvas.fillRect(markRect.withPadding(1_dp), 2_dp);
    } else {
        canvas.strokeRect(markRect.withPadding(1_dp), 2_dp);
    }

    if (interpolatedValue == 0)
        return;

    PointF p1 = markRect.at(4 / 24.f, 12 / 24.f);
    PointF p2 = markRect.at(9 / 24.f, 17 / 24.f);
    PointF p3 = markRect.at(20 / 24.f, 6 / 24.f);
    Path path;
    path.moveTo(p1);
    path.lineTo(PointF(mix(std::min(interpolatedValue * (16.f / 5.f), 1.f), p1.v, p2.v)));
    if (interpolatedValue * 16.f > 5.f) {
        path.lineTo(PointF(mix((interpolatedValue - (5.f / 16.f)) * (16.f / 11.f), p2.v, p3.v)));
    }
    canvas.setStrokeColor(color.multiplyAlpha(0.75f));
    canvas.setCapStyle(CapStyle::Round);
    canvas.strokePath(std::move(path));
}

void checkBoxPainter(Canvas& canvas, const Widget& widget_) {
    if (!dynamicCast<const CheckBox*>(&widget_)) {
        LOG_ERROR(widgets, "checkBoxPainter called for a non-CheckBox widget");
        return;
    }
    const CheckBox& widget  = static_cast<const CheckBox&>(widget_);
    float interpolatedValue = widget.interpolatedValue.get();

    Rectangle markRect      = widget.rect().alignedRect({ idp(14), idp(14) }, { 0.0f, 0.5f });
    boxPainter(canvas, widget, markRect);
    checkMark(canvas, markRect, widget.color.current(), interpolatedValue, widget_.isDisabled());
}

void CheckBox::paint(Canvas& canvas) const {
    checkBoxPainter(canvas, *this);
}

Rc<Widget> CheckBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

CheckBox::CheckBox(Construction construction, ArgumentsView<CheckBox> args)
    : ToggleButton(construction, nullptr) {
    args.apply(this);
}

} // namespace Brisk
