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
#include <brisk/widgets/Line.hpp>

namespace Brisk {

void Line::paint(Canvas& canvas) const {
    RectangleF r    = m_clientRect;
    float thickness = m_computedBorderWidth.y1;
    canvas.setFillColor(m_color.current.multiplyAlpha(m_opacity));
    if (m_orientation == Orientation::Horizontal)
        canvas.fillRect(r.alignedRect(r.width(), thickness, 0.5f, 0.5f));
    else
        canvas.fillRect(r.alignedRect(thickness, r.height(), 0.5f, 0.5f));
}

Rc<Widget> VLine::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Rc<Widget> HLine::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Line::Line(Construction construction, Orientation orientation, ArgumentsView<Line> args)
    : Widget(construction,
             std::tuple{
                 Arg::borderWidth = EdgesL{ 1_px },
             }),
      m_orientation(orientation) {
    args.apply(this);
}

Rc<Widget> Line::cloneThis() const { BRISK_CLONE_IMPLEMENTATION }

Rc<Widget> MenuLine::cloneThis() const {
    BRISK_CLONE_IMPLEMENTATION
}

} // namespace Brisk
