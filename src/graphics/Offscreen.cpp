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
#include <brisk/graphics/Offscreen.hpp>
#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Pixel.hpp>
#include <brisk/core/Exceptions.hpp>

namespace Brisk {

OffscreenRendering::OffscreenRendering(Size size, float pixelRatio) {
    auto renderDevice = getRenderDevice();
    if (!renderDevice.has_value()) {
        throwException(ERuntime("Render device is null"));
    }
    m_target            = (*renderDevice)->createImageTarget(size);
    m_encoder           = (*renderDevice)->createEncoder();
    Brisk::pixelRatio() = pixelRatio;
    m_context.reset(new RenderPipeline(m_encoder, m_target));
}

Rectangle OffscreenRendering::rect() const {
    Size size = m_target->size();
    return { 0, 0, size.width, size.height };
}

Canvas& OffscreenRendering::canvas() {
    if (!m_canvas)
        m_canvas.reset(new Canvas(*m_context));
    return *m_canvas;
}

Rc<Image> OffscreenRendering::render() {
    m_canvas.reset();  // Finish painting.
    m_context.reset(); // Finish rendering.
    m_encoder->wait(); // Wait until the image is fully rendered on the GPU.
    return m_target->image();
}

OffscreenRendering::~OffscreenRendering() {}
} // namespace Brisk
