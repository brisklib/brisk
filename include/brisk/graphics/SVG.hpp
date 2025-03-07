#pragma once

#include "Image.hpp"
#include "Color.hpp"

namespace Brisk {

namespace Internal {
struct SVGImpl;
}

/**
 * @struct SVGImage
 * @brief A class to represent and render SVG images.
 *
 * This class provides functionality to load an SVG image from a string and
 * render it as a raster image with a specified size and background color.
 */
struct SVGImage {
public:
    /**
     * @brief Constructs an SVGImage from a given SVG string.
     *
     * @param svg A string view representing the SVG data.
     */
    SVGImage(std::string_view svg);
    SVGImage(BytesView svg);
    SVGImage(const SVGImage&) noexcept            = default;
    SVGImage(SVGImage&&) noexcept                 = default;
    SVGImage& operator=(const SVGImage&) noexcept = default;
    SVGImage& operator=(SVGImage&&) noexcept      = default;

    /**
     * @brief Destructor for the SVGImage class.
     */
    ~SVGImage();

    /**
     * @brief Renders the SVG image to an RGBA format.
     *
     * @param size The desired size of the output image.
     * @param background The background color to use (default is transparent).
     * @param format The image format for the output (default is 8-bit RGBA).
     *
     * @return A smart pointer to an Image object representing the rendered image.
     */
    RC<Image> render(Size size, Color background = Color(0, 0), ImageFormat format = ImageFormat::RGBA) const;

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
    void renderTo(const RC<Image>& destination) const;

    /**
     * @brief Gets the native image format used by the SVG renderer.
     *
     * The native format could be either ImageFormat::BGRA or ImageFormat::RGBA.
     *
     * @return The native image format used by the renderer.
     */
    static ImageFormat nativeFormat();

private:
    RC<Internal::SVGImpl> m_impl; ///< Pointer to the internal SVG implementation.
};

} // namespace Brisk
