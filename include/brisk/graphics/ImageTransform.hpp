#pragma once

#include <brisk/graphics/Image.hpp>

namespace Brisk {

/**
 * @enum ResizingFilter
 * @brief Enumeration of the various resizing filters available for image resizing.
 */
enum class ResizingFilter {
    Default      = 0, ///< Default resizing filter.
    Box          = 1, ///< Box filter for resizing.
    Triangle     = 2, ///< Triangle filter for resizing.
    CubicBSpline = 3, ///< Cubic B-Spline filter for resizing.
    CatmullRom   = 4, ///< Catmull-Rom filter for resizing.
    Mitchell     = 5, ///< Mitchell-Netravali filter for resizing.
};

/**
 * @brief Resizes an image and stores the result in the destination image.
 *
 * @param destination The image to store the resized result. Must be a valid image.
 * @param source The image to be resized. Must be a valid image.
 * @param filter The resizing filter to use. Defaults to ResizingFilter::Default.
 */
void imageResizeTo(RC<Image> destination, RC<Image> source, ResizingFilter filter = ResizingFilter::Default);

/**
 * @brief Resizes an image to the specified new size and returns a new image.
 *
 * @param image The image to be resized. Must be a valid image.
 * @param newSize The new size for the resized image.
 * @param filter The resizing filter to use. Defaults to ResizingFilter::Default.
 * @return A smart pointer to the newly resized image.
 */
[[nodiscard]] inline RC<Image> imageResize(RC<Image> image, Size newSize,
                                           ResizingFilter filter = ResizingFilter::Default) {
    RC<Image> result = image->copy(false);
    imageResizeTo(result, std::move(image), filter);
    return result;
}

} // namespace Brisk
