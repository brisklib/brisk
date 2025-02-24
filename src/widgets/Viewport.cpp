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
#include <brisk/widgets/Viewport.hpp>

namespace Brisk {

void Viewport::paint(Canvas& canvas) const {
    if (m_renderer)
        m_renderer(canvas, m_rect);
}

void Viewport::onEvent(Event& event) {
    Widget::onEvent(event);
    if (m_controller)
        m_controller(event, m_rect);
}

Viewport::Viewport(Construction construction, Renderer renderer, Controller controller,
                   ArgumentsView<Viewport> args)
    : Widget{ construction, nullptr }, m_renderer(std::move(renderer)), m_controller(std::move(controller)) {
    args.apply(this);
    requestAnimationFrame();
}

void Viewport::onAnimationFrame() {
    invalidate();
    requestAnimationFrame();
}
} // namespace Brisk
