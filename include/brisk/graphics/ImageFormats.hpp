#pragma once

#include <brisk/core/Utilities.hpp>
#include <brisk/core/IO.hpp>
#include <brisk/graphics/Image.hpp>
#include <brisk/core/Exceptions.hpp>

namespace Brisk {

/**
 * @brief Enum representing the various image codecs supported for encoding and decoding.
 */
enum class ImageCodec {
    PNG,  ///< Portable Network Graphics
    BMP,  ///< Bitmap Image File
    JPEG, ///< Joint Photographic Experts Group
    WEBP, ///< WebP Image Format
};

template <>
inline constexpr std::initializer_list<NameValuePair<ImageCodec>> defaultNames<ImageCodec>{
    { "PNG", ImageCodec::PNG },
    { "BMP", ImageCodec::BMP },
    { "JPEG", ImageCodec::JPEG },
    { "WEBP", ImageCodec::WEBP },
};

/**
 * @brief Enum representing potential image I/O errors.
 */
enum class ImageIOError {
    CodecError,    ///< Error related to codec processing
    InvalidFormat, ///< Error due to an invalid image format
};

/**
 * @brief Default image quality for encoding.
 *
 * This value is used as the default quality setting when encoding images,
 * with a typical range of 0 (lowest quality) to 100 (highest quality).
 */
inline int defaultImageQuality = 98;

/**
 * @brief Enum representing color subsampling methods for images.
 */
enum class ColorSubsampling {
    S444, ///< 4:4:4 color subsampling (no subsampling)
    S422, ///< 4:2:2 color subsampling (horizontal subsampling)
    S420, ///< 4:2:0 color subsampling (both horizontal and vertical subsampling)
};

/**
 * @brief Default color subsampling method.
 */
inline ColorSubsampling defaultColorSubsampling = ColorSubsampling::S420;

/**
 * @brief Guesses the image codec based on the provided byte data.
 *
 * @param bytes A view of the byte data to analyze for codec detection.
 * @return An optional ImageCodec if the codec can be guessed; otherwise, an empty optional.
 */
[[nodiscard]] optional<ImageCodec> guessImageCodec(BytesView bytes);

/**
 * @brief Encodes an image to PNG format.
 *
 * @param image A reference-counted pointer to the image to be encoded.
 * @return A byte vector containing the encoded PNG image data.
 */
[[nodiscard]] Bytes pngEncode(RC<Image> image);

/**
 * @brief Encodes an image to BMP format.
 *
 * @param image A reference-counted pointer to the image to be encoded.
 * @return A byte vector containing the encoded BMP image data.
 */
[[nodiscard]] Bytes bmpEncode(RC<Image> image);

/**
 * @brief Encodes an image to JPEG format.
 *
 * @param image A reference-counted pointer to the image to be encoded.
 * @param quality Optional quality parameter for encoding (default is nullopt, which uses default quality).
 * @param ss Optional color subsampling parameter (default is nullopt).
 * @return A byte vector containing the encoded JPEG image data.
 */
[[nodiscard]] Bytes jpegEncode(RC<Image> image, optional<int> quality = nullopt,
                               optional<ColorSubsampling> ss = nullopt);

/**
 * @brief Encodes an image to WEBP format.
 *
 * @param image A reference-counted pointer to the image to be encoded.
 * @param quality Optional quality parameter for encoding (default is nullopt).
 * @param lossless Flag indicating whether to use lossless encoding (default is false).
 * @return A byte vector containing the encoded WEBP image data.
 */
[[nodiscard]] Bytes webpEncode(RC<Image> image, optional<float> quality = nullopt, bool lossless = false);

/**
 * @brief Encodes an image to the specified format using the provided codec.
 *
 * @param codec The image codec to use for encoding.
 * @param image A reference-counted pointer to the image to be encoded.
 * @param quality Optional quality parameter for encoding (default is nullopt).
 * @param ss Optional color subsampling parameter (default is nullopt).
 * @return A byte vector containing the encoded image data.
 */
[[nodiscard]] Bytes imageEncode(ImageCodec codec, RC<Image> image, optional<int> quality = nullopt,
                                optional<ColorSubsampling> ss = nullopt);

/**
 * @brief Decodes a PNG image from the provided byte data.
 *
 * @param bytes A view of the byte data representing a PNG image.
 * @param format Optional image format to use for decoding (returns original format if not specified).
 * @return An expected result containing a reference-counted pointer to the decoded image or an ImageIOError.
 */
[[nodiscard]] expected<RC<Image>, ImageIOError> pngDecode(BytesView bytes,
                                                          ImageFormat format = ImageFormat::Unknown);

/**
 * @brief Decodes a BMP image from the provided byte data.
 *
 * @param bytes A view of the byte data representing a BMP image.
 * @param format Optional image format to use for decoding (returns original format if not specified).
 * @return An expected result containing a reference-counted pointer to the decoded image or an ImageIOError.
 */
[[nodiscard]] expected<RC<Image>, ImageIOError> bmpDecode(BytesView bytes,
                                                          ImageFormat format = ImageFormat::Unknown);

/**
 * @brief Decodes a JPEG image from the provided byte data.
 *
 * @param bytes A view of the byte data representing a JPEG image.
 * @param format Optional image format to use for decoding (returns original format if not specified).
 * @return An expected result containing a reference-counted pointer to the decoded image or an ImageIOError.
 */
[[nodiscard]] expected<RC<Image>, ImageIOError> jpegDecode(BytesView bytes,
                                                           ImageFormat format = ImageFormat::Unknown);

/**
 * @brief Decodes a WEBP image from the provided byte data.
 *
 * @param bytes A view of the byte data representing a WEBP image.
 * @param format Optional image format to use for decoding (returns original format if not specified).
 * @return An expected result containing a reference-counted pointer to the decoded image or an ImageIOError.
 */
[[nodiscard]] expected<RC<Image>, ImageIOError> webpDecode(BytesView bytes,
                                                           ImageFormat format = ImageFormat::Unknown);

/**
 * @brief Decodes an image from the provided byte data using the specified codec.
 *
 * @param codec The image codec to use for decoding.
 * @param bytes A view of the byte data representing the image.
 * @param format Optional image format to use for decoding (returns original format if not specified).
 * @return An expected result containing a reference-counted pointer to the decoded image or an ImageIOError.
 */
[[nodiscard]] expected<RC<Image>, ImageIOError> imageDecode(ImageCodec codec, BytesView bytes,
                                                            ImageFormat format = ImageFormat::Unknown);

[[nodiscard]] expected<RC<Image>, ImageIOError> imageDecode(BytesView bytes,
                                                            ImageFormat format = ImageFormat::Unknown);

} // namespace Brisk
