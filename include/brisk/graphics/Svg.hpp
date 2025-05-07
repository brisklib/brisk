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
 */                                                                                                          \
#pragma once

#include "Image.hpp"
#include "Color.hpp"

namespace Brisk {

namespace Internal {
struct SvgImpl;
}

/**
 * @struct SvgImage
 * @brief A class to represent and render SVG images.
 *
 * This class provides functionality to load an SVG image from a string and
 * render it as a raster image with a specified size and background color.
 */
struct SvgImage {
public:
    /**
     * @brief Constructs an SvgImage from a given SVG string.
     *
     * @param svg A string view representing the SVG data.
     */
    SvgImage(std::string_view svg);
    SvgImage(BytesView svg);
    SvgImage(const SvgImage&) noexcept            = default;
    SvgImage(SvgImage&&) noexcept                 = default;
    SvgImage& operator=(const SvgImage&) noexcept = default;
    SvgImage& operator=(SvgImage&&) noexcept      = default;

    /**
     * @brief Destructor for the SvgImage class.
     */
    ~SvgImage();

    /**
     * @brief Renders the SVG image to an RGBA format.
     *
     * @param size The desired size of the output image.
     * @param background The background color to use (default is transparent).
     * @param format The image format for the output (default is 8-bit RGBA).
     *
     * @return A smart pointer to an Image object representing the rendered image.
     */
    Rc<Image> render(Size size, Color background = Color(0, 0), ImageFormat format = ImageFormat::RGBA) const;

    /**
     * @brief Renders SVG to a destination image, preserving existing content.
     *
     * This is a faster alternative to `render()` as it avoids memory allocation.
     * The format of the destination image must match the native format of the SVG renderer
     * as returned by `nativeFormat()`.
     *
     * @param destination The image to which the SVG will be rendered.
     *
     * @exception EImageError if the format does not match.
     */
    void renderTo(const Rc<Image>& destination) const;

    /**
     * @brief Gets the native image format used by the SVG renderer.
     *
     * The native format could be either ImageFormat::BGRA or ImageFormat::RGBA.
     *
     * @return The native image format used by the renderer.
     */
    static ImageFormat nativeFormat();

private:
    Rc<Internal::SvgImpl> m_impl; ///< Pointer to the internal SVG implementation.
};

} // namespace Brisk
