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
#include <brisk/graphics/Pixel.hpp>
#include <brisk/graphics/Renderer.hpp>

namespace Brisk {

OffscreenCanvas::OffscreenCanvas(Size size, float pixelRatio, const VisualSettings& settings)
    : m_size(size), m_pixelRatio(pixelRatio), m_settings(settings) {}

OffscreenCanvas::State::State(Rc<RenderDevice> device, Size size, float pixelRatio,
                              const VisualSettings& settings) {
    target  = device->createImageTarget(size);
    encoder = device->createEncoder();
    encoder->setVisualSettings(settings);
    Brisk::pixelRatio() = pixelRatio;
    context.reset(new RenderPipeline(encoder, target));
    canvas.reset(new Canvas(*context));
}

Rc<Image> OffscreenCanvas::State::endPaint() && {
    BRISK_ASSERT(canvas);
    BRISK_ASSERT(context);
    BRISK_ASSERT(encoder);
    BRISK_ASSERT(target);
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
    BRISK_ASSERT_MSG("OffscreenCanvas::beginPaint() failed", m_state.has_value());
    return *m_state->canvas;
}

bool OffscreenCanvas::beginPaint() {
    if (m_state)
        return true;

    auto renderDevice = getRenderDevice();
    if (!renderDevice)
        return false;

    m_state.emplace(*renderDevice, m_size, m_pixelRatio, m_settings);
    return true;
}

Rc<Image> OffscreenCanvas::endPaint() {
    if (!m_state)
        return nullptr;
    Rc<Image> result = std::move(*m_state).endPaint();
    m_state.reset();
    return result;
}

OffscreenCanvas::~OffscreenCanvas() = default;
} // namespace Brisk
