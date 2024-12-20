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
#include <brisk/widgets/CheckBox.hpp>

namespace Brisk {

static void checkMark(RawCanvas& canvas, RectangleF markRect, ColorF color, bool checked) {
    canvas.drawRectangle(markRect.withPadding(1_dp), 2_dp, 0.f, strokeColor = color.multiplyAlpha(0.35f),
                         fillColor = Palette::transparent, strokeWidth = 1._dp);
    if (checked) {
        PointF p1 = markRect.at(20 / 24.f, 6 / 24.f);
        PointF p2 = markRect.at(9 / 24.f, 17 / 24.f);
        PointF p3 = markRect.at(4 / 24.f, 12 / 24.f);
        canvas.drawLine(p1, p2, 1_dp, color.multiplyAlpha(0.75f), LineEnd::Round);
        canvas.drawLine(p2, p3, 1_dp, color.multiplyAlpha(0.75f), LineEnd::Round);
    }
}

void checkBoxPainter(Canvas& canvas, const Widget& widget) {
    Rectangle markRect = widget.rect().alignedRect({ idp(14), idp(14) }, { 0.0f, 0.5f });
    boxPainter(canvas, widget, markRect);
    checkMark(canvas.raw(), markRect, widget.color.current(), widget.state() && WidgetState::Selected);
}

void CheckBox::paint(Canvas& canvas) const {
    checkBoxPainter(canvas, *this);
}

Widget::Ptr CheckBox::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

CheckBox::CheckBox(Construction construction, ArgumentsView<CheckBox> args)
    : ToggleButton(construction, nullptr) {
    args.apply(this);
}

} // namespace Brisk
