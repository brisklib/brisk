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

static void radioMark(RawCanvas& canvas, RectangleF markRect, ColorF color, bool checked) {
    float side = markRect.shortestSide();
    canvas.drawEllipse(markRect.withPadding(1_dp), 0.f, strokeColor = color.multiplyAlpha(0.35f),
                       fillColor = Palette::transparent, strokeWidth = 1._dp);
    if (checked) {
        canvas.drawEllipse(markRect.alignedRect(side * 0.5f, side * 0.5f, 0.5f, 0.5f), 0.f, strokeWidth = 0.f,
                           fillColor = color.multiplyAlpha(0.75f));
    }
}

void radioButtonPainter(Canvas& canvas, const Widget& widget) {
    Rectangle markRect = widget.rect().alignedRect({ idp(14), idp(14) }, { 0.0f, 0.5f });
    boxPainter(canvas, widget, markRect);

    radioMark(canvas.raw(), markRect, widget.color.current(), widget.state() && WidgetState::Selected);
}

void RadioButton::paint(Canvas& canvas) const {
    radioButtonPainter(canvas, *this);
}

Widget::Ptr RadioButton::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

RadioButton::RadioButton(Construction construction, ArgumentsView<RadioButton> args)
    : Base(construction, nullptr) {
    args.apply(this);
}
} // namespace Brisk
