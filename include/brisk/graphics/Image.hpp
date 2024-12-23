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

constexpr ImageFormat imageFormat(PixelType type, PixelFormat format) {
    return static_cast<ImageFormat>((+type << 8) | +format);
}

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

constexpr PixelType toPixelType(ImageFormat format) noexcept {
    return static_cast<PixelType>((+format >> 8) & 0xFF);
}

constexpr PixelFormat toPixelFormat(ImageFormat format) noexcept {
    return static_cast<PixelFormat>(+format & 0xFF);
}

constexpr bool pixelFormatCompatible(PixelFormat requestedFormat, PixelFormat actualFormat) noexcept {
    return requestedFormat == actualFormat || requestedFormat == PixelFormat::Unknown;
}

constexpr bool pixelTypeCompatible(PixelType requestedType, PixelType actualType) noexcept {
    return requestedType == actualType || requestedType == PixelType::Unknown;
}

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

template <typename T>
struct StridedData {
    T* data;
    int32_t byteStride;

    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }
};

void convertPixels(PixelFormat dstFmt, StridedData<uint8_t> dst, PixelFormat srcFmt,
                   StridedData<const uint8_t> src, Size size);

enum class AccessMode {
    R,
    W,
    RW,
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

BRISK_FLAGS(ImageMapFlags)

template <typename T>
struct ImageData {
    T* data;
    Size size;
    int32_t byteStride;
    int32_t components;

    ImageData() noexcept                 = default;
    ImageData(const ImageData&) noexcept = default;

    operator StridedData<T>() const {
        return StridedData<T>{
            data,
            byteStride,
        };
    }

    ImageData(T* data, Size size, int32_t byteStride, int32_t components)
        : data(data), size(size), byteStride(byteStride), components(components) {}

    // ImageData<T> -> ImageData<const T> conversion
    ImageData(const ImageData<std::remove_const_t<T>>& other)
        requires(std::is_const_v<T>)
        : data(other.data), size(other.size), byteStride(other.byteStride), components(other.components) {}

    template <typename U>
    ImageData<U> to() const {
        if (components * sizeof(T) % sizeof(U) != 0) {
            throwException(EArgument("ImageData: invalid conversion"));
        }
        return { reinterpret_cast<U*>(data), size, byteStride,
                 static_cast<int32_t>(components * sizeof(T) / sizeof(U)) };
    }

    void copyFrom(const ImageData<const T>& src) const {
        auto srcLine = src.lineIterator();
        auto dstLine = lineIterator();
        int32_t w    = memoryWidth();
        for (int32_t y = 0; y < size.height; ++y, ++srcLine, ++dstLine) {
            std::copy_n(srcLine.data, w, dstLine.data);
        }
    }

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

    int32_t memoryWidth() const {
        return size.width * components;
    }

    size_t memorySize() const {
        return area() * components;
    }

    size_t area() const {
        return static_cast<int64_t>(size.width) * static_cast<int64_t>(size.height);
    }

    size_t byteSize() const {
        return sizeof(T) * memorySize();
    }

    using U8 = std::conditional_t<std::is_const_v<T>, const uint8_t, uint8_t>;

    T* line(int32_t y) const {
        return reinterpret_cast<T*>(reinterpret_cast<U8*>(data) + y * byteStride);
    }

    template <typename U = T>
    U* pixel(int32_t x, int32_t y) const {
        return line(y) + x * components;
    }

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

    LineIterator lineIterator() const {
        return { data, byteStride };
    }

    LineIterator lineReverseIterator() const {
        return { line(size.height - 1), -byteStride };
    }
};

struct MappedRegion {
    Point origin;
    ImageMapFlags flags = ImageMapFlags::Default;
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

template <ImageFormat ImageFmt, AccessMode Mode>
struct ImageAccess {
    ImageAccess()                               = delete;
    ImageAccess(const ImageAccess&)             = delete;
    ImageAccess& operator=(const ImageAccess&)  = delete;
    ImageAccess& operator=(ImageAccess&&)       = delete;

    constexpr static PixelFormat FmtPixelFormat = toPixelFormat(ImageFmt);
    constexpr static PixelType FmtPixelType     = toPixelType(ImageFmt);

    constexpr static bool pixelTypeKnown =
        FmtPixelFormat != PixelFormat::Unknown && FmtPixelType != PixelType::Unknown;

    ImageAccess(ImageAccess&& other) : m_data{}, m_mapped{}, m_commit{}, m_format{} {
        swap(other);
    }

    using ComponentType = PixelTypeOf<FmtPixelType>;
    using StorageType   = PixelOf<ImageFmt>;

    using value_type    = ConstIfR<StorageType, Mode>;
    using reference     = value_type&;
    using pointer       = value_type*;

    struct UnmapFn {
        using Fn = void (*)(void*, ImageData<value_type>&, MappedRegion&);
        Fn fn;
        void* self;

        void operator()(ImageData<value_type>& data, MappedRegion& mapped) {
            fn(self, data, mapped);
        }
    };

    ImageAccess(const std::tuple<ImageData<value_type>, MappedRegion>& dataMapped, UnmapFn commit,
                ImageFormat format)
        : ImageAccess(std::get<0>(dataMapped), std::get<1>(dataMapped), commit, format) {}

    ImageAccess(const ImageData<value_type>& data, const MappedRegion& mapped, UnmapFn commit,
                ImageFormat format)
        : m_data(data), m_mapped(mapped), m_commit(std::move(commit)), m_format(format) {}

    void swap(ImageAccess& other) {
        std::swap(m_data, other.m_data);
        std::swap(m_mapped, other.m_mapped);
        std::swap(m_commit, other.m_commit);
        std::swap(m_format, other.m_format);
    }

    using LineIterator = typename ImageData<value_type>::LineIterator;

    [[noreturn]] static void throwRangeError(const std::string& str) {
        throwException(ERange(str.c_str()));
    }

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

    Size size() const {
        return m_data.size;
    }

    int32_t width() const {
        return m_data.size.width;
    }

    int32_t height() const {
        return m_data.size.height;
    }

    int32_t memoryWidth() const {
        return m_data.memoryWidth();
    }

    template <ImageFormat SrcImageFmt, AccessMode SrcMode>
    void copyFrom(const ImageAccess<SrcImageFmt, SrcMode>& src)
        requires(Mode != AccessMode::R)
    {
        return copyFrom<SrcImageFmt>(src.m_data);
    }

    ~ImageAccess() {
        m_commit(m_data, m_mapped);
    }

    pointer data() const {
        return m_data.data;
    }

    int32_t byteStride() const {
        return m_data.byteStride;
    }

    size_t memorySize() const {
        return m_data.memorySize();
    }

    size_t byteSize() const {
        return m_data.byteSize();
    }

    ImageFormat format() const {
        return m_format;
    }

    PixelType pixelType() const {
        return toPixelType(m_format);
    }

    PixelFormat pixelFormat() const {
        return toPixelFormat(m_format);
    }

    int32_t components() const {
        return m_data.components;
    }

    LineIterator lineIterator() const {
        return m_data.lineIterator();
    }

    LineIterator lineReverseIterator() const {
        return m_data.lineReverseIterator();
    }

    pointer line(int32_t y) const {
#ifndef NDEBUG
        if (y < 0 || y >= height())
            throwRangeError(fmt::format("line(): invalid line index {} (height={})", y, height()));
#endif
        return m_data.line(y);
    }

    void writeTo(bytes_mutable_view data, bool flipY = false) const {
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

    void readFrom(bytes_view data, bool flipY = false) const
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

    bool isContiguous() const {
        return m_data.stride == m_data.size.width;
    }

    bool isTopDown() const {
        return m_data.stride > 0;
    }

    void clear(ColorF fillColor)
        requires(Mode != AccessMode::R)
    {
        forPixels([fillColor](int32_t, int32_t, auto& pix) {
            colorToPixel(pix, fillColor);
        });
    }

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

    template <ImageFormat Hint = ImageFmt, typename Fn>
    void forPixels(Fn&& fn) {
        constexpr PixelType typeHint     = toPixelType(Hint);
        constexpr PixelFormat typeFormat = toPixelFormat(Hint);
        if constexpr (typeHint == PixelType::Unknown) {
            DO_PIX_TYP(pixelType(), return forPixels<imageFormat(TYP, typeFormat)>(std::forward<Fn>(fn)););
        } else if constexpr (typeFormat == PixelFormat::Unknown) {
            DO_PIX_FMT(pixelFormat(), return forPixels<imageFormat(typeHint, FMT)>(std::forward<Fn>(fn)););
        } else {
            auto data = m_data.template to<Pixel<typeHint, typeFormat>>();
            static_assert(typeHint != PixelType::Unknown);
            static_assert(typeFormat != PixelFormat::Unknown);
            auto l    = data.lineIterator();
            int32_t w = m_data.size.width;
            for (int32_t y = 0; y < m_data.size.height; ++y, ++l) {
                for (int32_t x = 0; x < w; ++x) {
                    fn(x, y, l.data[x]);
                }
            }
        }
    }

    void premultiplyAlpha()
        requires(Mode != AccessMode::R)
    {
        if (pixelAlpha(pixelFormat()) != PixelFlagAlpha::None && pixelFormat() != PixelFormat::Alpha) {
            forPixels([](int32_t, int32_t, auto& pix) {
                ColorF color;
                pixelToColor(color, pix);
                color = color.premultiply();
                colorToPixel(pix, color);
            });
        }
    }

    const ImageData<value_type>& imageData() const {
        return m_data;
    }

    template <ImageFormat format, AccessMode>
    friend struct ImageAccess;

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

template <typename T>
inline ImageData<T> allocateImageData(Size size, int32_t components = 1, int32_t strideAlignment = 1) {
    if (std::max(size.width, size.height) >= 65536) {
        throwException(EArgument("Invalid size for image data: {}x{}", size.x, size.y));
    }
    size_t byteStride = alignUp(size.width * sizeof(T) * components, strideAlignment);
    return ImageData<T>{
        alignedAlloc<T>(size.height * byteStride),
        size,
        int32_t(byteStride),
        components,
    };
}

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

    int32_t width() const noexcept {
        return m_data.size.width;
    }

    int32_t height() const noexcept {
        return m_data.size.height;
    }

    Size size() const noexcept {
        return m_data.size;
    }

    Rectangle bounds() const noexcept {
        return Rectangle{ Point{ 0, 0 }, size() };
    }

    size_t byteSize() const noexcept {
        return m_data.byteSize();
    }

    PixelType pixelType() const noexcept {
        return m_type;
    }

    ImageFormat format() const noexcept {
        return imageFormat(m_type, m_format);
    }

    PixelFormat pixelFormat() const noexcept {
        return m_format;
    }

    int32_t componentsPerPixel() const noexcept {
        return pixelComponents(pixelFormat());
    }

    int32_t bytesPerPixel() const noexcept {
        return pixelSize(pixelType(), pixelFormat());
    }

    bool isGreyscale() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::Greyscale;
    }

    bool isColor() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::RGB;
    }

    bool hasAlpha() const noexcept {
        return pixelAlpha(pixelFormat()) != PixelFlagAlpha::None;
    }

    bool isAlphaOnly() const noexcept {
        return pixelColor(pixelFormat()) == PixelFlagColor::None;
    }

    bool isLinear() const noexcept {
        return pixelType() != PixelType::U8Gamma;
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::R> mapRead() const {
        return map<format, AccessMode::R>(data(), this->bounds(), m_backend.get(), this->format());
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::W> mapWrite() {
        return map<format, AccessMode::W>(data(), this->bounds(), m_backend.get(), this->format());
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::RW> mapReadWrite() {
        return map<format, AccessMode::RW>(data(), this->bounds(), m_backend.get(), this->format());
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::R> mapRead(Rectangle rect) const {
        return map<format, AccessMode::R>(data(), rect, m_backend.get(), this->format());
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::W> mapWrite(Rectangle rect) {
        return this->map<format, AccessMode::W>(data(), rect, m_backend.get(), this->format());
    }

    template <ImageFormat format = ImageFormat::Unknown>
    ImageAccess<format, AccessMode::RW> mapReadWrite(Rectangle rect) {
        return map<format, AccessMode::RW>(data(), rect, m_backend.get(), this->format());
    }

    void clear(ColorF value) {
        auto w = mapWrite();
        w.clear(value);
    }

    void copyFrom(const RC<const Image>& source, Rectangle sourceRect, Rectangle destRect) {
        auto r = source->mapRead(sourceRect);
        auto w = mapWrite(destRect);
        w.copyFrom(r);
    }

    void copyFrom(const RC<const Image>& source) {
        return copyFrom(source, source->bounds(), this->bounds());
    }

    ImageData<UntypedPixel> data() const noexcept {
        return m_data;
    }

    /// Create image with given size, pixel type and pixel format and allocates memory for it.
    /// Defaults to creating an 8-bit RGBA image with gamma-corrected sRGB color space
    explicit Image(Size size, ImageFormat format = ImageFormat::RGBA)
        : Image(allocateImageData<UntypedPixel>(size, pixelSize(toPixelType(format), toPixelFormat(format))),
                format, &deallocateImageData<UntypedPixel>) {}

    explicit Image(Size size, ImageFormat format, ColorF fillColor) : Image(size, format) {
        auto w = mapWrite();
        w.forPixels([fillColor](int32_t, int32_t, auto& pix) {
            colorToPixel(pix, fillColor);
        });
    }

    /// Create image with given size, pixel type and pixel format and reference existing data.
    /// Data is not copied. Caller is responsible for data lifetime.
    explicit Image(void* data, Size size, int32_t byteStride, ImageFormat format)
        : Image(ImageData<UntypedPixel>{ reinterpret_cast<UntypedPixel*>(data), size, byteStride,
                                         pixelComponents(toPixelFormat(format)) },
                format, nullptr) {}

    ~Image() {
        if (m_deleter)
            m_deleter(m_data);
    }

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
