
#pragma once


#include "ImageData.h"
#include "Linearizer.h"
#include "Threads.h"


#include "Rasterizer_p.h"


namespace Blaze {

/**
 * Rasterize image.
 *
 * @param geometries Pointer to geometries to rasterize. Must not be nullptr.
 *
 * @param geometryCount A number of geometries in geometry array. Must be at
 * least 1.
 *
 * @param matrix Transformation matrix. All geometries will be pre-transformed
 * by this matrix when rasterizing.
 *
 * @param threads Threads to use.
 *
 * @param image Destination image.
 */
template <typename T>
static inline void Rasterize(const Geometry *geometries,
    const int geometryCount, const Matrix &matrix, Threads &threads,
    const ImageData &image) {
    Rasterizer<T>::Rasterize(geometries, geometryCount, matrix, threads, image);
}

} // namespace Blaze