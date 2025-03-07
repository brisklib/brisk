#pragma once

#include "Pixel.hpp"
#include "Geometry.hpp"
#include "Color.hpp"
#include <memory>
#include <brisk/core/RC.hpp>
#include <brisk/core/internal/Optional.hpp>
#include <fmt/format.h>
#include <brisk/core/Exceptions.hpp>
#include <brisk/core/Memory.hpp>

namespace Brisk {

/**
 * @brief Custom exception class for image-related errors.
 *
 * This class derives from the standard runtime_error to provide specific error handling for image processing.
 */
class EImageError : public Exception<std::runtime_error> {
public:
    using Exception<std::runtime_error>::Exception; ///< Inherit constructors from the base exception class
};

enum class ImageFormat : uint16_t;

constexpr uint16_t operator+(ImageFormat fmt) noexcept {
    return static_cast<uint16_t>(fmt);
}

/**
 * @brief Combines PixelType and PixelFormat enumerations into an ImageFormat type.
 *
 * @param type The pixel type (e.g., U8, U16, F32).
 * @param format The pixel format (e.g., RGB, RGBA, Greyscale).
 * @return The combined ImageFormat value.
 */
constexpr ImageFormat imageFormat(PixelType type, PixelFormat format) {
    return static_cast<ImageFormat>((+type << 8) | +format);
}

/**
 * @brief Enumeration representing various image formats,
 *        combining pixel type and pixel format.
 */
enum class ImageFormat : uint16_t {
    Unknown                = 0xFFFF,

    Unknown_U8Gamma        = +imageFormat(PixelType::U8Gamma, PixelFormat::Unknown),
    RGB_U8Gamma            = +imageFormat(PixelType::U8Gamma, PixelFormat::RGB),
    RGBA_U8Gamma           = +imageFormat(PixelType::U8Gamma, PixelFormat::RGBA),
    ARGB_U8Gamma           = +imageFormat(PixelType::U8Gamma, PixelFormat::ARGB),
    BGR_U8Gamma            = +imageFormat(PixelType::U8Gamma, PixelFormat::BGR),
    BGRA_U8Gamma           = +imageFormat(PixelType::U8Gamma, PixelFormat::BGRA),
    ABGR_U8Gamma           = +imageFormat(PixelType::U8Gamma, PixelFormat::ABGR),
    GreyscaleAlpha_U8Gamma = +imageFormat(PixelType::U8Gamma, PixelFormat::GreyscaleAlpha),
    Greyscale_U8Gamma      = +imageFormat(PixelType::U8Gamma, PixelFormat::Greyscale),
    Alpha_U8Gamma          = +imageFormat(PixelType::U8Gamma, PixelFormat::Alpha),

    Unknown_U8             = +imageFormat(PixelType::U8, PixelFormat::Unknown),
    RGB_U8                 = +imageFormat(PixelType::U8, PixelFormat::RGB),
    RGBA_U8                = +imageFormat(PixelType::U8, PixelFormat::RGBA),
    ARGB_U8                = +imageFormat(PixelType::U8, PixelFormat::ARGB),
    BGR_U8                 = +imageFormat(PixelType::U8, PixelFormat::BGR),
    BGRA_U8                = +imageFormat(PixelType::U8, PixelFormat::BGRA),
    ABGR_U8                = +imageFormat(PixelType::U8, PixelFormat::ABGR),
    GreyscaleAlpha_U8      = +imageFormat(PixelType::U8, PixelFormat::GreyscaleAlpha),
    Greyscale_U8           = +imageFormat(PixelType::U8, PixelFormat::Greyscale),
    Alpha_U8               = +imageFormat(PixelType::U8, PixelFormat::Alpha),

    Unknown_U16            = +imageFormat(PixelType::U16, PixelFormat::Unknown),
    RGB_U16                = +imageFormat(PixelType::U16, PixelFormat::RGB),
    RGBA_U16               = +imageFormat(PixelType::U16, PixelFormat::RGBA),
    ARGB_U16               = +imageFormat(PixelType::U16, PixelFormat::ARGB),
    BGR_U16                = +imageFormat(PixelType::U16, PixelFormat::BGR),
    BGRA_U16               = +imageFormat(PixelType::U16, PixelFormat::BGRA),
    ABGR_U16               = +imageFormat(PixelType::U16, PixelFormat::ABGR),
    GreyscaleAlpha_U16     = +imageFormat(PixelType::U16, PixelFormat::GreyscaleAlpha),
    Greyscale_U16          = +imageFormat(PixelType::U16, PixelFormat::Greyscale),
    Alpha_U16              = +imageFormat(PixelType::U16, PixelFormat::Alpha),

    Unknown_F32            = +imageFormat(PixelType::F32, PixelFormat::Unknown),
    RGB_F32                = +imageFormat(PixelType::F32, PixelFormat::RGB),
    RGBA_F32               = +imageFormat(PixelType::F32, PixelFormat::RGBA),
    ARGB_F32               = +imageFormat(PixelType::F32, PixelFormat::ARGB),
    BGR_F32                = +imageFormat(PixelType::F32, PixelFormat::BGR),
    BGRA_F32               = +imageFormat(PixelType::F32, PixelFormat::BGRA),
    ABGR_F32               = +imageFormat(PixelType::F32, PixelFormat::ABGR),
    GreyscaleAlpha_F32     = +imageFormat(PixelType::F32, PixelFormat::GreyscaleAlpha),
    Greyscale_F32          = +imageFormat(PixelType::F32, PixelFormat::Greyscale),
    Alpha_F32              = +imageFormat(PixelType::F32, PixelFormat::Alpha),

    RGB_Unknown            = +imageFormat(PixelType::Unknown, PixelFormat::RGB),
    RGBA_Unknown           = +imageFormat(PixelType::Unknown, PixelFormat::RGBA),
    ARGB_Unknown           = +imageFormat(PixelType::Unknown, PixelFormat::ARGB),
    BGR_Unknown            = +imageFormat(PixelType::Unknown, PixelFormat::BGR),
    BGRA_Unknown           = +imageFormat(PixelType::Unknown, PixelFormat::BGRA),
    ABGR_Unknown           = +imageFormat(PixelType::Unknown, PixelFormat::ABGR),
    GreyscaleAlpha_Unknown = +imageFormat(PixelType::Unknown, PixelFormat::GreyscaleAlpha),
    Greyscale_Unknown      = +imageFormat(PixelType::Unknown, PixelFormat::Greyscale),
    Alpha_Unknown          = +imageFormat(PixelType::Unknown, PixelFormat::Alpha),

    RGB                    = RGB_U8Gamma,
    RGBA                   = RGBA_U8Gamma,
    ARGB                   = ARGB_U8Gamma,
    BGR                    = BGR_U8Gamma,
    BGRA                   = BGRA_U8Gamma,
    ABGR                   = ABGR_U8Gamma,
    GreyscaleAlpha         = GreyscaleAlpha_U8Gamma,
    Greyscale              = Greyscale_U8Gamma,
    Alpha                  = Alpha_U8Gamma,
};

/**
 * @brief Extracts the PixelType from an ImageFormat.
 * @param format The ImageFormat value.
 * @return The corresponding PixelType.
 */
constexpr PixelType toPixelType(ImageFormat format) noexcept {
    return static_cast<PixelType>((+format >> 8) & 0xFF);
}

/**
 * @brief Extracts the PixelFormat from an ImageFormat.
 * @param format The ImageFormat value.
 * @return The corresponding PixelFormat.
 */
constexpr PixelFormat toPixelFormat(ImageFormat format) noexcept {
    return static_cast<PixelFormat>(+format & 0xFF);
}

/**
 * @brief Checks if a requested PixelFormat is compatible with an actual PixelFormat.
 * @param requestedFormat The requested pixel format.
 * @param actualFormat The actual pixel format.
 * @return True if compatible, false otherwise.
 */
constexpr bool pixelFormatCompatible(PixelFormat requestedFormat, PixelFormat actualFormat) noexcept {
    return requestedFormat == actualFormat || requestedFormat == PixelFormat::Unknown;
}

/**
 * @brief Checks if a requested PixelType is compatible with an actual PixelType.
 * @param requestedType The requested pixel type.
 * @param actualType The actual pixel type.
 * @return True if compatible, false otherwise.
 */
constexpr bool pixelTypeCompatible(PixelType requestedType, PixelType actualType) noexcept {
    return requestedType == actualType || requestedType == PixelType::Unknown;
}

/**
 * @brief Checks if a requested ImageFormat is compatible with an actual ImageFormat.
 * @param requestedFormat The requested image format.
 * @param actualFormat The actual image format.
 * @return True if compatible, false otherwise.
 */
constexpr bool imageFormatCompatible(ImageFormat requestedFormat, ImageFormat actualFormat) noexcept {
    return pixelFormatCompatible(toPixelFormat(requestedFormat), toPixelFormat(actualFormat)) &&
           pixelTypeCompatible(toPixelType(requestedFormat), toPixelType(actualFormat));
}

} // namespace Brisk

template <>
struct fmt::formatter<Brisk::ImageFormat> : fmt::formatter<std::string> {
    template <typename FormatContext>
    auto format(const Brisk::ImageFormat& val, FormatContext& ctx) const {
        std::string str;
        if (val == Brisk::ImageFormat::Unknown)
            str = "Unknown";
        else
            str = fmt::to_string(Brisk::toPixelFormat(val)) + "_" + fmt::to_string(Brisk::toPixelType(val));
        return fmt::formatter<std::string>::format(str, ctx);
    }
};

namespace Brisk {

/**
 * @brief Represents a strided data structure for accessing image data.
 *
 * @tparam T The data type of the pixel data (can be const or non-const).
 */
template <typename T>
struct StridedData {
    T* data;            ///< Pointer to the pixel data.
    int32_t byteStride; ///< Stride in bytes between consecutive rows. Can be negative.

    /**
     * @brief Unsigned 8-bit type with the const qualifier if T is const-qualified.
     */
    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    /**
     * @brief Retrieves a pointer to the start of a given row.
     *
     * @param y The row index.
     * @return Pointer to the first pixel in the specified row.
     */
    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }
};

/**
 * @brief Converts pixel data between different formats.
 *
 * @param dstFmt Destination pixel format.
 * @param dst Destination image data with stride.
 * @param srcFmt Source pixel format.
 * @param src Source image data with stride.
 * @param size The dimensions of the image.
 */
void convertPixels(PixelFormat dstFmt, StridedData<uint8_t> dst, PixelFormat srcFmt,
                   StridedData<const uint8_t> src, Size size);

/**
 * @brief Defines access modes for image data.
 */
enum class AccessMode {
    R,  ///< Read access.
    W,  ///< Write access.
    RW, ///< Read and write access.
};

template <typename U, AccessMode Mode>
using ConstIfR = std::conditional_t<Mode == AccessMode::R, const U, U>;

template <AccessMode Mode>
using CAccessMode = constant<AccessMode, Mode>;

using CAccessR    = CAccessMode<AccessMode::R>;
using CAccessW    = CAccessMode<AccessMode::W>;
using CAccessRW   = CAccessMode<AccessMode::RW>;

enum class ImageMapFlags : uint32_t {
    Default = 0,
};

template <>
constexpr inline bool isBitFlags<ImageMapFlags> = true;

/**
 * @brief Represents image data with size, stride, and component count.
 *
 * @tparam T The data type of the image pixels (can be const or non-const).
 */
template <typename T>
struct ImageData {
    T* data;            ///< Pointer to the pixel data.
    Size size;          ///< Dimensions of the image.
    int32_t byteStride; ///< Stride in bytes between consecutive rows. Can be negative.
    int32_t components; ///< Number of color components per pixel.

    ImageData() noexcept                 = default;
    ImageData(const ImageData&) noexcept = default;

    /**
     * @brief Implicit conversion to StridedData<T>.
     */
    operator StridedData<T>() const {
        return StridedData<T>{
            data,
            byteStride,
        };
    }

    /**
     * @brief Constructs an ImageData object.
     *
     * @param data Pointer to the pixel data.
     * @param size Image dimensions.
     * @param byteStride Byte stride between rows.
     * @param components Number of color components per pixel.
     */
    ImageData(T* data, Size size, int32_t byteStride, int32_t components)
        : data(data), size(size), byteStride(byteStride), components(components) {}

    /**
     * @brief Conversion from ImageData<T> to ImageData<const T>.
     */
    ImageData(const ImageData<std::remove_const_t<T>>& other)
        requires(std::is_const_v<T>)
        : data(other.data), size(other.size), byteStride(other.byteStride), components(other.components) {}

    /**
     * @brief Converts the image data to another type.
     *
     * @tparam U The target data type.
     * @return Converted ImageData<U>.
     */
    template <typename U>
    ImageData<U> to() const {
        if (components * sizeof(T) % sizeof(U) != 0) {
            throwException(EArgument("ImageData: invalid conversion"));
        }
        return { reinterpret_cast<U*>(data), size, byteStride,
                 static_cast<int32_t>(components * sizeof(T) / sizeof(U)) };
    }

    /**
     * @brief Copies pixel data from another ImageData object.
     *
     * @param src Source ImageData object.
     */
    void copyFrom(const ImageData<const T>& src) const {
        auto srcLine = src.lineIterator();
        auto dstLine = lineIterator();
        int32_t w    = memoryWidth();
        for (int32_t y = 0; y < size.height; ++y, ++srcLine, ++dstLine) {
            std::copy_n(srcLine.data, w, dstLine.data);
        }
    }

    /**
     * @brief Extracts a subregion of the image.
     *
     * @param rect The rectangle defining the subregion.
     * @return A new ImageData object representing the subregion.
     */
    ImageData subrect(Rectangle rect) const {
        if (rect.intersection(Rectangle{ Point{ 0, 0 }, size }) != rect) {
            throwException(EArgument("ImageData: invalid rectangle passed to subrect"));
        }
        return ImageData{
            pixel(rect.x1, rect.y1),
            rect.size(),
            byteStride,
            components,
        };
    }

    /**
     * @brief Computes the width in memory (in color components, not bytes).
     */
    int32_t memoryWidth() const {
        return size.width * components;
    }

    /**
     * @brief Computes the total memory size of the image in color components.
     */
    size_t memorySize() const {
        return area() * components;
    }

    /**
     * @brief Computes the total number of pixels in the image.
     */
    size_t area() const {
        return static_cast<int64_t>(size.width) * static_cast<int64_t>(size.height);
    }

    /**
     * @brief Computes the total memory size in bytes.
     */
    size_t byteSize() const {
        return sizeof(T) * memorySize();
    }

    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    /**
     * @brief Retrieves a pointer to the start of a given row.
     *
     * @param y The row index.
     * @return Pointer to the first pixel in the specified row.
     */
    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }

    /**
     * @brief Retrieves a pointer to a specific pixel.
     *
     * @tparam U Optional return type (defaults to T).
     * @param x X-coordinate of the pixel.
     * @param y Y-coordinate of the pixel.
     * @return Pointer to the specified pixel.
     */
    template <typename U = T>
    U* pixel(int32_t x, int32_t y) const {
        return line(y) + x * components;
    }

    /**
     * @brief Iterator for traversing image rows.
     */
    struct LineIterator {
        T* data;
        int32_t byteStride;

        LineIterator& operator++() {
            reinterpret_cast<U8*&>(data) += byteStride;
            return *this;
        }

        LineIterator operator++(int) {
            LineIterator copy = *this;
            ++copy;
            return copy;
        }
    };

    /**
     * @brief Returns an iterator to the first row.
     */
    LineIterator lineIterator() const {
        return { data, byteStride };
    }

    /**
     * @brief Returns a reverse iterator to the last row.
     */
    LineIterator lineReverseIterator() const {
        return { line(size.height - 1), -byteStride };
    }
};

/**
 * @brief Represents a mapped region within an image.
 */
struct MappedRegion {
    Point origin;                                 ///< Origin point of the mapped region.
    ImageMapFlags flags = ImageMapFlags::Default; ///< Mapping flags.
};

namespace Internal {
template <ImageFormat format>
struct PixelOf {
    using Type = Pixel<toPixelType(format), toPixelFormat(format)>;
};

template <ImageFormat format>
    requires(toPixelFormat(format) == PixelFormat::Unknown)
struct PixelOf<format> {
    using Type = PixelTypeOf<toPixelType(format)>;
};
} // namespace Internal

template <ImageFormat format>
using PixelOf = typename Internal::PixelOf<format>::Type;

/**
 * @brief Provides controlled access to image data with specified format and access mode.
 * @tparam ImageFmt The format of the image.
 * @tparam Mode The access mode (read, write, or read-write).
 */
template <ImageFormat ImageFmt, AccessMode Mode>
struct ImageAccess {
    ImageAccess()                               = delete;
    ImageAccess(const ImageAccess&)             = delete;
    ImageAccess& operator=(const ImageAccess&)  = delete;
    ImageAccess& operator=(ImageAccess&&)       = delete;

    /**
     * @brief Pixel format and type information derived from ImageFmt.
     */
    constexpr static PixelFormat FmtPixelFormat = toPixelFormat(ImageFmt);
    constexpr static PixelType FmtPixelType     = toPixelType(ImageFmt);
    constexpr static bool pixelTypeKnown =
        (FmtPixelFormat != PixelFormat::Unknown && FmtPixelType != PixelType::Unknown);

    /**
     * @brief Constructs an ImageAccess object by moving from another instance.
     */
    ImageAccess(ImageAccess&& other) : m_data{}, m_mapped{}, m_commit{}, m_format{} {
        swap(other);
    }

    using ComponentType = PixelTypeOf<FmtPixelType>;   ///< Component type derived from pixel type.
    using StorageType   = PixelOf<ImageFmt>;           ///< Storage type based on image format.
    using value_type    = ConstIfR<StorageType, Mode>; ///< Value type based on access mode.
    using reference     = value_type&;                 ///< Reference to pixel value.
    using pointer       = value_type*;                 ///< Pointer to pixel data.

    /**
     * @brief Functor for handling unmapping of image data.
     */
    struct UnmapFn {
        using Fn = void (*)(void*, ImageData<value_type>&, MappedRegion&);
        Fn fn;
        void* self;

        void operator()(ImageData<value_type>& data, MappedRegion& mapped) {
            fn(self, data, mapped);
        }
    };

    /**
     * @brief Constructs an ImageAccess from mapped image data.
     * @param dataMapped A tuple containing the image data and mapped region.
     * @param commit The unmapping function to be called on destruction.
     * @param format The image format.
     */
    ImageAccess(const std::tuple<ImageData<value_type>, MappedRegion>& dataMapped, UnmapFn commit,
                ImageFormat format)
        : ImageAccess(std::get<0>(dataMapped), std::get<1>(dataMapped), commit, format) {}

    /**
     * @brief Constructs ImageAccess with given data, mapped region, commit function, and format.
     * @param data The image data.
     * @param mapped The mapped region of the image.
     * @param commit The commit function.
     * @param format The image format.
     */
    ImageAccess(const ImageData<value_type>& data, const MappedRegion& mapped, UnmapFn commit,
                ImageFormat format)
        : m_data(data), m_mapped(mapped), m_commit(std::move(commit)), m_format(format) {}

    /**
     * @brief Swaps the contents of this instance with another.
     * @param other The other ImageAccess instance.
     */
    void swap(ImageAccess& other) {
        std::swap(m_data, other.m_data);
        std::swap(m_mapped, other.m_mapped);
        std::swap(m_commit, other.m_commit);
        std::swap(m_format, other.m_format);
    }

    using LineIterator = typename ImageData<value_type>::LineIterator; ///< Line iterator for image traversal.

    /**
     * @brief Throws a range error exception.
     * @param str The error message.
     */
    [[noreturn]] static void throwRangeError(const std::string& str) {
        throwException(ERange(str.c_str()));
    }

    /**
     * @brief Accesses pixel data at the specified coordinates.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @return Reference to the pixel data.
     */
    reference operator()(int32_t x, int32_t y) const
        requires(FmtPixelType != PixelType::Unknown)
    {
#ifndef NDEBUG
        if (x < 0 || x >= width())
            throwRangeError(
                fmt::format("operator(): invalid coordinate {}x{} (size={}x{})", x, y, width(), height()));
#endif
        return line(y)[x];
    }

    /**
     * @brief Returns the size of the image.
     * @return The image size.
     */
    Size size() const {
        return m_data.size;
    }

    /**
     * @brief Returns the width of the image.
     * @return The image width.
     */
    int32_t width() const {
        return m_data.size.width;
    }

    /**
     * @brief Returns the height of the image.
     * @return The image height.
     */
    int32_t height() const {
        return m_data.size.height;
    }

    /**
     * @brief Returns the image width in memory (in color components, not bytes).
     * @return The memory width.
     */
    int32_t memoryWidth() const {
        return m_data.memoryWidth();
    }

    /**
     * @brief Copies data from another ImageAccess.
     * @param src The source ImageAccess instance.
     */
    template <ImageFormat SrcImageFmt, AccessMode SrcMode>
    void copyFrom(const ImageAccess<SrcImageFmt, SrcMode>& src)
        requires(Mode != AccessMode::R)
    {
        return copyFrom<SrcImageFmt>(src.m_data);
    }

    /// @brief Destructor that commits any pending changes.
    ~ImageAccess() {
        m_commit(m_data, m_mapped);
    }

    /**
     * @brief Returns a pointer to the image data.
     * @return Pointer to the image data.
     */
    pointer data() const {
        return m_data.data;
    }

    /**
     * @brief Returns the byte stride of the image.
     * @return The byte stride.
     */
    int32_t byteStride() const {
        return m_data.byteStride;
    }

    /**
     * @brief Returns the total size of the image in color components.
     * @return The memory size in color components.
     */
    size_t memorySize() const {
        return m_data.memorySize();
    }

    /**
     * @brief Returns the byte size of the image.
     * @return The byte size of the image.
     */
    size_t byteSize() const {
        return m_data.byteSize();
    }

    /**
     * @brief Returns the format of the image.
     * @return The image format.
     */
    ImageFormat format() const {
        return m_format;
    }

    /**
     * @brief Returns the pixel type of the image.
     * @return The pixel type.
     */
    PixelType pixelType() const {
        return toPixelType(m_format);
    }

    /**
     * @brief Returns the pixel format of the image.
     * @return The pixel format.
     */
    PixelFormat pixelFormat() const {
        return toPixelFormat(m_format);
    }

    /**
     * @brief Returns the number of components in a pixel (1-4).
     * @return The number of components.
     */
    int32_t components() const {
        return m_data.components;
    }

    /**
     * @brief Returns an iterator for traversing the lines of the image.
     * @return Line iterator.
     */
    LineIterator lineIterator() const {
        return m_data.lineIterator();
    }

    /**
     * @brief Returns a reverse iterator for traversing the lines of the image.
     * @return Line reverse iterator.
     */
    LineIterator lineReverseIterator() const {
        return m_data.lineReverseIterator();
    }

    /**
     * @brief Retrieves a pointer to the start of a specific line.
     * @param y The y-coordinate of the line.
     * @return Pointer to the start of the line.
     */
    pointer line(int32_t y) const {
#ifndef NDEBUG
        if (y < 0 || y >= height())
            throwRangeError(fmt::format("line(): invalid line index {} (height={})", y, height()));
#endif
        return m_data.line(y);
    }

    /**
     * @brief Writes the image data to the contiguous memory buffer.
     * @param data The memory buffer to write to.
     * @param flipY Whether to flip the image vertically.
     */
    void writeTo(BytesMutableView data, bool flipY = false) const {
#ifndef NDEBUG
        if (data.size() != sizeof(StorageType) * m_data.memorySize())
            throwRangeError(fmt::format("writeTo(): invalid size {} (required={})", data.size(),
                                        sizeof(StorageType) * m_data.memorySize()));
#endif
        LineIterator l   = lineIterator();
        StorageType* dst = reinterpret_cast<StorageType*>(data.data());
        int32_t w        = m_data.memoryWidth();
        int32_t dstStep  = w;
        if (flipY) {
            dst += w * (height() - 1);
            dstStep = -dstStep;
        }
        w *= sizeof(StorageType);
        for (int32_t y = 0; y < height(); ++y, ++l, dst += dstStep)
            memcpy(dst, l.data, w);
    }

    /**
     * @brief Reads the image data from the contiguous memory buffer.
     * @param data The memory buffer to read from.
     * @param flipY Whether to flip the image vertically.
     */
    void readFrom(BytesView data, bool flipY = false) const
        requires(Mode != AccessMode::R)
    {
#ifndef NDEBUG
        if (data.size() != sizeof(StorageType) * m_data.memorySize())
            throwRangeError(fmt::format("readFrom(): invalid size {} (required={})", data.size(),
                                        sizeof(StorageType) * m_data.memorySize()));
#endif
        LineIterator l         = lineIterator();
        const StorageType* src = reinterpret_cast<const StorageType*>(data.data());
        int32_t w              = m_data.memoryWidth();
        int32_t srcStep        = w;
        if (flipY) {
            src += w * (height() - 1);
            srcStep = -srcStep;
        }
        w *= sizeof(StorageType);
        for (int32_t y = 0; y < height(); ++y, ++l, src += srcStep)
            memcpy(l.data, src, w);
    }

    /**
     * @brief Checks if the image data is contiguous in memory.
     * @return True if the data is contiguous, false otherwise.
     */
    bool isContiguous() const {
        return m_data.stride == m_data.size.width;
    }

    /**
     * @brief Checks if the image is stored top-down.
     * @return True if the image is top-down, false otherwise.
     */
    bool isTopDown() const {
        return m_data.stride > 0;
    }

    /**
     * @brief Clears the image with a specified fill color.
     * @param fillColor The color to fill the image with.
     */
    void clear(ColorW fillColor)
        requires(Mode != AccessMode::R)
    {
        forPixels([fillColor](int32_t, int32_t, auto& pix) {
            colorToPixel(pix, fillColor);
        });
    }

    /**
     * @brief Flips the image along the specified axis.
     * @param axis The axis to flip along.
     */
    void flip(FlipAxis axis) const
        requires(Mode != AccessMode::R)
    {
        switch (axis) {
        case FlipAxis::X:
            for (int32_t y = 0; y < m_data.size.height; ++y) {
                value_type* l = line(y);
                for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                    swapItem(l, x1, l, x2);
                }
            }
            break;
        case FlipAxis::Y:
            for (int32_t y1 = 0, y2 = m_data.size.height - 1; y1 < y2; ++y1, --y2) {
                value_type* l1 = line(y1);
                value_type* l2 = line(y2);
                for (int32_t x = 0; x < m_data.size.width; ++x) {
                    swapItem(l1, x, l2, x);
                }
            }
            break;
        case FlipAxis::Both:
            for (int32_t y1 = 0, y2 = m_data.size.height - 1; y1 <= y2; ++y1, --y2) {
                if (y1 != y2) {
                    value_type* l1 = line(y1);
                    value_type* l2 = line(y2);
                    for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                        swapItem(l1, x1, l2, x2);
                        swapItem(l1, x2, l2, x1);
                    }
                } else {
                    value_type* l = line(y1);
                    for (int32_t x1 = 0, x2 = m_data.size.width - 1; x1 < x2; ++x1, --x2) {
                        swapItem(l, x1, l, x2);
                    }
                }
            }
            break;
        }
    }

    /**
     * @brief Iterates over the pixels of the image and applies the provided function.
     *
     * This function allows applying a custom operation on each pixel of the image. It takes
     * into account the image format and pixel type, and adapts the iteration accordingly.
     *
     * @tparam Hint The image format hint (default is `ImageFmt`).
     * @param fn The function to apply to each pixel. The function must take the pixel's x and y coordinates
     *           along with a reference to the pixel's value.
     */
    template <ImageFormat Hint = ImageFmt, typename Fn>
    void forPixels(Fn&& fn) {
        constexpr PixelType typeHint     = toPixelType(Hint);
        constexpr PixelFormat typeFormat = toPixelFormat(Hint);

        // If pixel type is unknown, we deduce it dynamically based on the image format.
        if constexpr (typeHint == PixelType::Unknown) {
            DO_PIX_TYP(pixelType(), return forPixels<imageFormat(TYP, typeFormat)>(std::forward<Fn>(fn)););
        }
        // If pixel format is unknown, we deduce it dynamically based on the pixel type.
        else if constexpr (typeFormat == PixelFormat::Unknown) {
            DO_PIX_FMT(pixelFormat(), return forPixels<imageFormat(typeHint, FMT)>(std::forward<Fn>(fn)););
        } else {
            // Convert the image data to the appropriate pixel type and format, then iterate over it.
            auto data = m_data.template to<Pixel<typeHint, typeFormat>>();
            static_assert(typeHint != PixelType::Unknown);
            static_assert(typeFormat != PixelFormat::Unknown);
            auto l    = data.lineIterator();
            int32_t w = m_data.size.width;

            // Iterate over each pixel in the image and apply the function.
            for (int32_t y = 0; y < m_data.size.height; ++y, ++l) {
                for (int32_t x = 0; x < w; ++x) {
                    fn(x, y, l.data[x]);
                }
            }
        }
    }

    /**
     * @brief Premultiplies the alpha channel for each pixel in the image.
     *
     * This function modifies the image's color data by premultiplying the alpha value for each pixel.
     * The alpha premultiplication is only performed if the image has an alpha channel.
     *
     * This function is only available when the access mode is not read-only (`Mode != AccessMode::R`).
     */
    void premultiplyAlpha()
        requires(Mode != AccessMode::R)
    {
        // Check if the image format supports alpha and is not a purely alpha format.
        if (pixelAlpha(pixelFormat()) != PixelFlagAlpha::None && pixelFormat() != PixelFormat::Alpha) {
            // Iterate over each pixel and premultiply the alpha value.
            forPixels([](int32_t, int32_t, auto& pix) {
                ColorW color;
                pixelToColor(color, pix);    // Convert pixel to color
                color = color.premultiply(); // Premultiply alpha
                colorToPixel(pix, color);    // Convert back to pixel format
            });
        }
    }

    /**
     * @brief Unpremultiplies the alpha channel for each pixel in the image.
     *
     * This function modifies the image's color data by unpremultiplying the alpha value for each pixel.
     * The alpha unpremultiplication is only performed if the image has an alpha channel.
     *
     * This function is only available when the access mode is not read-only (`Mode != AccessMode::R`).
     */
    void unpremultiplyAlpha()
        requires(Mode != AccessMode::R)
    {
        // Check if the image format supports alpha and is not a purely alpha format.
        if (pixelAlpha(pixelFormat()) != PixelFlagAlpha::None && pixelFormat() != PixelFormat::Alpha) {
            // Iterate over each pixel and unpremultiply the alpha value.
            forPixels([](int32_t, int32_t, auto& pix) {
                ColorW color;
                pixelToColor(color, pix);      // Convert pixel to color
                color = color.unpremultiply(); // Unpremultiply alpha
                colorToPixel(pix, color);      // Convert back to pixel format
            });
        }
    }

    /**
     * @brief Retrieves the image data.
     * @return The image data.
     */
    const ImageData<value_type>& imageData() const {
        return m_data;
    }

    template <ImageFormat format, AccessMode>
    friend struct ImageAccess;

    /**
     * @brief Copies data from the source image.
     * @param src The source image data.
     */
    template <ImageFormat SrcImageFmt = ImageFmt>
    void copyFrom(const ImageData<const PixelOf<SrcImageFmt>>& src)
        requires(Mode != AccessMode::R)
    {
#ifndef NDEBUG
        if (src.size != m_data.size) {
            throwRangeError(fmt::format("copyFrom: source size = {}x{}, target size = {}x{}", src.size.x,
                                        src.size.y, m_data.size.x, m_data.size.y));
        }
        if (src.components != m_data.components) {
            throwRangeError(fmt::format("copyFrom: source components = {}, target components = {}",
                                        src.components, m_data.components));
        }
#endif
        if constexpr (SrcImageFmt == ImageFmt) {
            m_data.copyFrom(src);
        } else {
            forPixels([](int32_t x, int32_t y, auto& pix) {

            });
        }
    }

private:
    ImageData<value_type> m_data;
    MappedRegion m_mapped;
    UnmapFn m_commit;
    ImageFormat m_format;

    /**
     * @brief Swaps two pixel items in the image.
     * @param a Pointer to the first pixel.
     * @param ax Index of the first pixel in the row.
     * @param b Pointer to the second pixel.
     * @param bx Index of the second pixel in the row.
     */
    void swapItem(value_type* a, int32_t ax, value_type* b, int32_t bx) const {
        if constexpr (Mode != AccessMode::R) {
            a += ax * m_data.components;
            b += bx * m_data.components;
            for (int32_t i = 0; i < m_data.components; ++i) {
                std::swap(a[i], b[i]);
            }
        }
    }
};

class Image;

namespace Internal {
struct ImageBackend {
    virtual ~ImageBackend()                             = 0;
    /// Can transfer image data from backend if the image is changed on GPU since previous call to update
    virtual void begin(AccessMode mode, Rectangle rect) = 0;
    /// Can transfer image data to backend
    virtual void end(AccessMode mode, Rectangle rect)   = 0;
};

ImageBackend* getBackend(RC<Image> image);
void setBackend(RC<Image> image, ImageBackend* backend);
} // namespace Internal

/**
 * @brief Allocates memory for image data with the specified size, components, and alignment.
 *
 * This function allocates memory for an image with the given width, height, and number of components
 * per pixel. The memory is aligned based on the specified `strideAlignment`. It ensures that the image
 * size is valid, with a maximum width or height less than 65536. It also computes the necessary byte stride
 * and allocates memory accordingly.
 *
 * @tparam T The color component type (e.g., `uint8_t`, `float`).
 * @param size The size of the image (width and height).
 * @param components The number of components per pixel (1-4, default is 1).
 * @param strideAlignment The alignment of the stride in bytes (default is 1).
 * @return The allocated `ImageData<T>` structure containing the image data.
 * @throws EArgument If the image size is too large (width or height >= 65536).
 */
template <typename T>
inline ImageData<T> allocateImageData(Size size, int32_t components = 1, int32_t strideAlignment = 1) {
    // Check if the image size is valid
    if (std::max(size.width, size.height) >= 65536) {
        throwException(EArgument("Invalid size for image data: {}x{}", size.x, size.y));
    }

    // Compute the byte stride with alignment
    size_t byteStride = alignUp(size.width * sizeof(T) * components, strideAlignment);

    // Allocate and return the image data structure
    return ImageData<T>{
        alignedAlloc<T>(size.height * byteStride),
        size,
        int32_t(byteStride),
        components,
    };
}

/**
 * @brief Deallocates memory used by the image data.
 *
 * This function frees the memory allocated for the image data by calling the aligned free function.
 *
 * @tparam T The pixel type (e.g., `uint8_t`, `float`).
 * @param data The `ImageData<T>` structure containing the image data to be deallocated.
 */
template <typename T>
inline void deallocateImageData(const ImageData<T>& data) {
    alignedFree(data.data);
}

class Image : public std::enable_shared_from_this<Image> {
public:
    Image()                        = delete;
    Image(const Image&)            = delete;
    Image(Image&&)                 = delete;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&)      = delete;

    /**
     * @brief Returns the width of the image.
     *
     * @return The width of the image in pixels.
     */
    int32_t width() const noexcept {
        return m_data.size.width;
    }

    /**
     * @brief Returns the height of the image.
     *
     * @return The height of the image in pixels.
     */
    int32_t height() const noexcept {
        return m_data.size.height;
    }

    /**
     * @brief Returns the size of the image.
     *
     * @return The size of the image in pixels.
     */
    Size size() const noexcept {
        return m_data.size;
    }

    /**
     * @brief Returns the bounds of the image.
     *
     * This function returns a `Rectangle` that represents the bounds of the image, starting from the origin
     * (0, 0) to the image size.
     *
     * @return A `Rectangle` representing the bounds of the image.
     */
    Rectangle bounds() const noexcept {
        return Rectangle{ Point{ 0, 0 }, size() };
    }

    /**
     * @brief Returns the total byte size of the image data.
     *
     * @return The total byte size of the image data.
     */
    size_t byteSize() const noexcept {
        return m_data.byteSize();
    }

    /**
     * @brief Returns the pixel type of the image.
     *
     * @return The pixel type of the image.
     */
    PixelType pixelType() const noexcept {
        return m_type;
    }

    /**
     * @brief Returns the image format.
     *
     * @return The image format of the image.
     */
    ImageFormat format() const noexcept {
        return imageFormat(m_type, m_format);
    }

    /**
     * @brief Returns the pixel format of the image.
     *
     * @return The pixel format of the image.
     */
    PixelFormat pixelFormat() const noexcept {
        return m_format;
    }

    /**
     * @brief Returns the number of color components per pixel in the image (1-4).
     *
     * @return The number of color components per pixel.
     */
    int32_t componentsPerPixel() const noexcept {
        return pixelComponents(pixelFormat());
    }

    /**
     * @brief Returns the number of bytes per pixel.
     *
     * This function returns the number of bytes required to store a single pixel based on the pixel type and
     * pixel format.
     *
     * @return The number of bytes per pixel.
     */
    int32_t bytesPerPixel() const noexcept {
        return pixelSize(pixelType(), pixelFormat());
    }

    /**
     * @brief Checks if the image is greyscale.
     *
     * @return `true` if the image is greyscale, otherwise `false`.
     */
    bool isGreyscale() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::Greyscale;
    }

    /**
     * @brief Checks if the image is a color image.
     *
     * @return `true` if the image is a color image, otherwise `false`.
     */
    bool isColor() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::RGB;
    }

    /**
     * @brief Checks if the image has an alpha channel.
     *
     * @return `true` if the image has an alpha channel, otherwise `false`.
     */
    bool hasAlpha() const noexcept {
        return pixelAlpha(pixelFormat()) != PixelFlagAlpha::None;
    }

    /**
     * @brief Checks if the image contains only alpha data.
     *
     * @return `true` if the image is alpha-only, otherwise `false`.
     */
    bool isAlphaOnly() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::None;
    }

    /**
     * @brief Checks if the image is linear (non-gamma encoded).
     *
     * @return `true` if the image is linear, otherwise `false`.
     */
    bool isLinear() const noexcept {
        return pixelType() != PixelType::U8Gamma;
    }

    /**
     * @brief Maps the image for reading.
     *
     * Maps the image data for reading with the specified format.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @return An `ImageAccess` object for reading.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::R> mapRead() const {
        return map<format, AccessMode::R>(data(), this->bounds(), m_backend.get(), this->format());
    }

    /**
     * @brief Maps the image for writing.
     *
     * Maps the image data for writing with the specified format.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @return An `ImageAccess` object for writing.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::W> mapWrite() {
        return map<format, AccessMode::W>(data(), this->bounds(), m_backend.get(), this->format());
    }

    /**
     * @brief Maps the image for read and write.
     *
     * Maps the image data for both reading and writing with the specified format.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @return An `ImageAccess` object for read and write access.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::RW> mapReadWrite() {
        return map<format, AccessMode::RW>(data(), this->bounds(), m_backend.get(), this->format());
    }

    /**
     * @brief Maps a rectangular region of the image for reading.
     *
     * Maps the specified rectangular region of the image for reading.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @param rect The rectangular region to map.
     * @return An `ImageAccess` object for reading the region.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::R> mapRead(Rectangle rect) const {
        return map<format, AccessMode::R>(data(), rect, m_backend.get(), this->format());
    }

    /**
     * @brief Maps a rectangular region of the image for writing.
     *
     * Maps the specified rectangular region of the image for writing.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @param rect The rectangular region to map.
     * @return An `ImageAccess` object for writing the region.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::W> mapWrite(Rectangle rect) {
        return this->map<format, AccessMode::W>(data(), rect, m_backend.get(), this->format());
    }

    /**
     * @brief Maps a rectangular region of the image for read and write.
     *
     * Maps the specified rectangular region of the image for both reading and writing.
     *
     * @tparam format The image format (default is `ImageFormat::Unknown`).
     * @param rect The rectangular region to map.
     * @return An `ImageAccess` object for read and write access to the region.
     */
    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::RW> mapReadWrite(Rectangle rect) {
        return map<format, AccessMode::RW>(data(), rect, m_backend.get(), this->format());
    }

    /**
     * @brief Clears the image with a specified color.
     *
     * Clears the entire image by writing the specified color value to it.
     *
     * @param value The color to clear the image with.
     */
    void clear(ColorW value) {
        auto w = mapWrite();
        w.clear(value);
    }

    /**
     * @brief Copies data from another image.
     *
     * Copies a rectangular region of data from the source image to the destination image.
     *
     * @param source The source image.
     * @param sourceRect The region to copy from the source.
     * @param destRect The region to copy to in the destination.
     */
    void copyFrom(const RC<const Image>& source, Rectangle sourceRect, Rectangle destRect) {
        auto r = source->mapRead(sourceRect);
        auto w = mapWrite(destRect);
        w.copyFrom(r);
    }

    /**
     * @brief Copies data from another image (full bounds).
     *
     * Copies data from the source image's bounds to the destination image's bounds.
     *
     * @param source The source image.
     */
    void copyFrom(const RC<const Image>& source) {
        return copyFrom(source, source->bounds(), this->bounds());
    }

    /**
     * @brief Returns the image data.
     *
     * Returns the image data as an `ImageData<UntypedPixel>` object.
     *
     * @return The image data.
     */
    ImageData<UntypedPixel> data() const noexcept {
        return m_data;
    }

    /**
     * @brief Creates an image with the specified size, pixel type, and pixel format, and allocates memory for
     * it.
     *
     * This constructor creates an 8-bit RGBA image by default with a gamma-corrected sRGB color space.
     *
     * @param size The size of the image.
     * @param format The image format (default is `ImageFormat::RGBA`).
     */
    explicit Image(Size size, ImageFormat format = ImageFormat::RGBA)
        : Image(allocateImageData<UntypedPixel>(size, pixelSize(toPixelType(format), toPixelFormat(format))),
                format, &deallocateImageData<UntypedPixel>) {}

    /**
     * @brief Constructs an image with the given size, pixel format, and fills it with a color.
     *
     * This constructor creates an image with the specified size and pixel format, then fills the image with
     * the provided `fillColor`.
     *
     * @param size The size of the image.
     * @param format The pixel format of the image.
     * @param fillColor The color to fill the image with.
     */
    explicit Image(Size size, ImageFormat format, ColorW fillColor) : Image(size, format) {
        auto w = mapWrite();
        w.forPixels([fillColor](int32_t, int32_t, auto& pix) {
            colorToPixel(pix, fillColor);
        });
    }

    /**
     * @brief Constructs an image by referencing existing data (no data copy).
     *
     * This constructor creates an image by referencing existing data, without copying it. The caller is
     * responsible for managing the lifetime of the data.
     *
     * @param data Pointer to the existing image data.
     * @param size The size of the image.
     * @param byteStride The byte stride between rows of pixels.
     * @param format The pixel format of the image.
     */
    explicit Image(void* data, Size size, int32_t byteStride, ImageFormat format)
        : Image(ImageData<UntypedPixel>{ reinterpret_cast<UntypedPixel*>(data), size, byteStride,
                                         pixelComponents(toPixelFormat(format)) },
                format, nullptr) {}

    /**
     * @brief Destructor to clean up the image.
     *
     * This destructor deallocates the image data if a deleter is provided.
     */
    ~Image() {
        if (m_deleter)
            m_deleter(m_data);
    }

    /**
     * @brief Creates a copy of the image.
     *
     * Creates a new `Image` object with the same size and format as the original image. Optionally, copies
     * the pixel data from the original image.
     *
     * @param copyPixels Whether to copy the pixel data (default is `true`).
     * @return A new `RC<Image>` containing the copied image.
     */
    RC<Image> copy(bool copyPixels = true) const {
        RC<Image> result = rcnew Image(size(), format());
        if (copyPixels) {
            result->copyFrom(this->shared_from_this());
        }
        return result;
    }

protected:
    using ImageDataDeleter = void (*)(const ImageData<UntypedPixel>& data);

    Image(ImageData<UntypedPixel> data, ImageFormat format, ImageDataDeleter deleter,
          std::unique_ptr<Internal::ImageBackend> backend = nullptr)
        : m_data(std::move(data)), m_type(toPixelType(format)), m_format(toPixelFormat(format)),
          m_deleter(deleter), m_backend(std::move(backend)) {}

    template <ImageFormat requestedFormat, AccessMode Mode>
    static ImageAccess<requestedFormat, Mode> map(const ImageData<UntypedPixel>& data, Rectangle rect,
                                                  Internal::ImageBackend* backend, ImageFormat actualFormat) {
        using ImgAcc = ImageAccess<requestedFormat, Mode>;
        using T      = typename ImgAcc::value_type;
        if (backend)
            backend->begin(Mode, rect);
        if (!imageFormatCompatible(requestedFormat, actualFormat)) {
            throwException(EImageError("Cannot map {} image to {} data", actualFormat, requestedFormat));
        }
        return ImgAcc(data.template to<T>().subrect(rect), MappedRegion{ rect.p1 },
                      typename ImgAcc::UnmapFn{ &unmap<Mode, T>, backend }, actualFormat);
    }

    template <AccessMode M, typename T>
    static void unmap(void* backend_, ImageData<ConstIfR<T, M>>& data, MappedRegion& mapped) {
        Internal::ImageBackend* backend = static_cast<Internal::ImageBackend*>(backend_);
        if (backend)
            backend->end(M, Rectangle{ mapped.origin, data.size });
    }

    ImageData<UntypedPixel> m_data;
    PixelType m_type;
    PixelFormat m_format;
    ImageDataDeleter m_deleter;

    friend Internal::ImageBackend* Internal::getBackend(RC<Image> image);
    friend void Internal::setBackend(RC<Image> image, Internal::ImageBackend* backend);
    mutable std::unique_ptr<Internal::ImageBackend> m_backend;
};

namespace Internal {
inline ImageBackend* getBackend(RC<Image> image) {
    return image->m_backend.get();
}

inline void setBackend(RC<Image> image, ImageBackend* backend) {
    image->m_backend.reset(backend);
}

} // namespace Internal

} // namespace Brisk
