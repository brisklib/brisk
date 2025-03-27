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
 */                                                                                                          \
#pragma once

#include <brisk/graphics/Renderer.hpp>
#include <brisk/graphics/Canvas.hpp>
#include <brisk/graphics/RenderState.hpp>

namespace Brisk {

/**
 * @class OffscreenRendering
 * @brief Class for handling offscreen rendering.
 */
class OffscreenRendering {
public:
    /**
     * @brief Constructs an OffscreenRendering object with the specified size and pixel ratio.
     * @param size The dimensions of the rendering target.
     * @param pixelRatio The ratio of pixels to physical pixels (default is 1.0).
     */
    OffscreenRendering(Size size, float pixelRatio = 1.f);

    /**
     * @brief Destructor for the OffscreenRendering object.
     */
    ~OffscreenRendering();

    /**
     * @brief Renders the offscreen image and returns the resulting image.
     * @return A reference-counted pointer to the rendered image in RGBA format.
     */
    [[nodiscard]] RC<Image> render();

    /**
     * @brief Gets the rectangle representing the size of the rendering target.
     * @return The rectangle of the rendering target.
     */
    Rectangle rect() const;

    /**
     * @brief Provides access to the canvas used for rendering.
     * @return A reference to the canvas.
     */
    Canvas& canvas();

private:
    RC<ImageRenderTarget> m_target;            ///< The render target for offscreen rendering.
    RC<RenderEncoder> m_encoder;               ///< The render encoder used during rendering.
    std::unique_ptr<RenderPipeline> m_context; ///< The context for the rendering pipeline.
    std::unique_ptr<Canvas> m_canvas;          ///< The canvas used for drawing.
};

} // namespace Brisk
