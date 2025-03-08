#include <brisk/graphics/ImageTransform.hpp>
#include <fmt/format.h>
#include <brisk/core/Bytes.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/core/IO.hpp>
#include <brisk/core/internal/Filesystem.hpp>

#include <png.h>
#include <windows.h>

using namespace Brisk;

// Define the ICO structures
#pragma pack(push, 1) // Ensure no padding in the structures

// ICONDIR structure (icon file header)
struct ICONDIR {
    uint16_t idReserved; // Reserved (must be 0)
    uint16_t idType;     // Resource Type (1 for icons)
    uint16_t idCount;    // Number of icons in the file
};

// ICONDIRENTRY structure (each icon's directory entry)
struct ICONDIRENTRY {
    uint8_t bWidth;         // Width of the image
    uint8_t bHeight;        // Height of the image
    uint8_t bColorCount;    // Number of colors (0 if 32-bit)
    uint8_t bReserved;      // Reserved (must be 0)
    uint16_t wPlanes;       // Color Planes
    uint16_t wBitCount;     // Bits per pixel
    uint32_t dwBytesInRes;  // Image data size
    uint32_t dwImageOffset; // Offset of the image data from the start of the file
};

#pragma pack(pop)

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        fmt::println("Usage: icowriter.exe <input_image> <output_ico>");
        fmt::println(
            "Only square 32-bit PNG files are accepted as input. The size must be 256x256 pixels or larger.");
        return 1;
    }

    fs::path input  = argv[1];
    fs::path output = argv[2];

    auto bytes      = readBytes(input);
    if (!bytes) {
        fmt::println("Cannot read the input file");
        return 2;
    }

    auto imageR = pngDecode(*bytes, ImageFormat::BGRA);
    if (!imageR) {
        fmt::println("Cannot decode the input file as a PNG image");
        return 3;
    }
    RC<Image> image = (*imageR);
    if (image->width() != image->height() || image->width() < 256 || image->height() < 256) {
        fmt::println("The image is not square or is less than 256x256. The size is {}x{}", image->width(),
                     image->height());
        return 4;
    }

    auto out = openFileForWriting(output);
    if (!out) {
        fmt::println("Cannot open file for writing: {}", output.string());
        return 5;
    }

    std::vector<uint32_t> sizes{ 256, 128, 64, 48, 32, 24, 16 };
    const uint32_t pngMinimumSize = 64;

    ICONDIR iconDir{};
    iconDir.idReserved = 0;
    iconDir.idType     = 1;
    iconDir.idCount    = sizes.size();

    if (!(*out)->writeAll(asBytesView(iconDir))) {
        fmt::println("Cannot write ico header");
        return 6;
    }

    std::vector<Bytes> data(sizes.size());

    uint32_t offset = static_cast<uint32_t>(sizeof(ICONDIR) + sizes.size() * sizeof(ICONDIRENTRY));

    for (int i = 0; i < sizes.size(); ++i) {
        uint32_t size     = sizes[i];
        RC<Image> resized = imageResize(image, Size(size, size), ResizingFilter::Mitchell);
        ICONDIRENTRY entry;
        entry.bWidth      = static_cast<uint8_t>(size); // wrapping 256 to 0 is ok
        entry.bHeight     = static_cast<uint8_t>(size);
        entry.bColorCount = 0;
        entry.bReserved   = 0;
        entry.wPlanes     = 1;
        entry.wBitCount   = 32;
        if (size < pngMinimumSize) {
            // DIB Bitmap
            data[i] = Bytes(size * size * 4);
            auto r  = resized->mapRead<ImageFormat::BGRA>();
            // Export BGRA data
            r.writeTo(data[i], true);
            entry.dwBytesInRes = static_cast<uint32_t>(sizeof(BITMAPINFOHEADER) + data[i].size() +
                                                       (((size + 31u) & ~31u) >> 3) * size);
        } else {
            // PNG
            data[i]            = pngEncode(resized);
            entry.dwBytesInRes = static_cast<uint32_t>(data[i].size());
        }
        entry.dwImageOffset = offset;
        offset += entry.dwBytesInRes;
        if (!(*out)->writeAll(asBytesView(entry))) {
            fmt::println("Cannot write ico entry");
            return 6;
        }
    }
    for (int i = 0; i < sizes.size(); ++i) {
        uint32_t size = sizes[i];
        if (size < pngMinimumSize) {
            // DIB Bitmap
            BITMAPINFOHEADER bmpHeader{};
            bmpHeader.biSize          = sizeof(BITMAPINFOHEADER);
            bmpHeader.biWidth         = size;
            bmpHeader.biHeight        = 2 * size;
            bmpHeader.biPlanes        = 1;
            bmpHeader.biBitCount      = 32;
            bmpHeader.biCompression   = 0;
            bmpHeader.biSizeImage     = data[i].size();
            bmpHeader.biXPelsPerMeter = 3779; // 96 dpi
            bmpHeader.biYPelsPerMeter = 3779; // 96 dpi
            if (!(*out)->writeAll(asBytesView(bmpHeader))) {
                fmt::println("Cannot write ico bmp header");
                return 6;
            }
        }
        if (!(*out)->writeAll(data[i])) {
            fmt::println("Cannot write ico data");
            return 6;
        }
        if (size < pngMinimumSize) {
            // write mask
            Bytes mask((((size + 31u) & ~31u) >> 3) * size, std::byte{ 0 });
            if (!(*out)->writeAll(mask)) {
                fmt::println("Cannot write ico mask");
                return 6;
            }
        }
    }
    fmt::println("ICO file has been written successfully");
}
