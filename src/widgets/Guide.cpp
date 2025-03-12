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
#include <brisk/widgets/Guide.hpp>

namespace Brisk {

void Guide::paint(Canvas& canvas_) const {
    Widget::paint(canvas_);
    if (!m_tree)
        return;
    for (auto& f : m_focus) {
        RC<Widget> w = m_tree->root()->findById(f.id);
        PointF src   = m_rect.at(f.sourceAnchor);
        PointF tgt   = w->rect().at(f.targetAnchor);

        if (m_tree) {
            m_tree->requestLayer([this, src, tgt](Canvas& canvas_) {
                RawCanvas& canvas = canvas_.raw();
                canvas.drawLine(src, tgt, 3_dp, m_backgroundColor.current);
                canvas.drawEllipse(src.alignedRect(7_dp, 7_dp, 0.5f, 0.5f), strokeWidth = 0,
                                   fillColor = m_backgroundColor.current);
                canvas.drawEllipse(tgt.alignedRect(7_dp, 7_dp, 0.5f, 0.5f), strokeWidth = 0,
                                   fillColor = m_backgroundColor.current);
            });
        }
    }
}

} // namespace Brisk
