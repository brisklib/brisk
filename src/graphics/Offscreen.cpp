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
#include <brisk/graphics/Offscreen.hpp>
#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Pixel.hpp>
#include <brisk/core/Exceptions.hpp>

namespace Brisk {

OffscreenCanvas::OffscreenCanvas(Size size, float pixelRatio)
    : m_size(size), m_pixelRatio(pixelRatio) {}

OffscreenCanvas::State::State(Rc<RenderDevice> device, Size size, float pixelRatio) {
    target              = device->createImageTarget(size);
    encoder             = device->createEncoder();
    Brisk::pixelRatio() = pixelRatio;
    context.reset(new RenderPipeline(encoder, target));
    canvas.reset(new Canvas(*context));
}

Rc<Image> OffscreenCanvas::State::render() && {
    canvas.reset();
    context.reset();
    encoder->wait();
    encoder.reset();
    Rc<Image> result = target->image();
    target.reset();
    return result;
}

Rectangle OffscreenCanvas::rect() const {
    if (!m_state)
        return {};
    return { Point{}, m_state->target->size() };
}

Canvas& OffscreenCanvas::canvas() {
    if (!m_state) {
        auto renderDevice = getRenderDevice();
        if (!renderDevice.has_value()) {
            throwException(ERuntime("Render device is null"));
        }
        m_state.emplace(renderDevice.value(), m_size, m_pixelRatio);
    }
    return *m_state->canvas;
}

Rc<Image> OffscreenCanvas::render() {
    if (!m_state)
        return nullptr;
    Rc<Image> result = std::move(*m_state).render();
    m_state.reset();
    return result;
}

OffscreenCanvas::~OffscreenCanvas() = default;
} // namespace Brisk
