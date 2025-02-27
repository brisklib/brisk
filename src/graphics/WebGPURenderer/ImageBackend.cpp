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
 */
#include "ImageBackend.hpp"
#include <brisk/core/Utilities.hpp>
#include <brisk/core/Log.hpp>

namespace Brisk {

ImageBackendWebGPU* getOrCreateBackend(RC<RenderDeviceWebGPU> device, RC<Image> image, bool uploadImage,
                                       bool renderTarget) {
    ImageBackendWebGPU* backend = dynamic_cast<ImageBackendWebGPU*>(Internal::getBackend(image));
    if (backend)
        return backend;
    ImageBackendWebGPU* newBackend = new ImageBackendWebGPU(device, image.get(), uploadImage, renderTarget);
    Internal::setBackend(image, newBackend);
    return newBackend;
}

ImageBackendWebGPU::ImageBackendWebGPU(RC<RenderDeviceWebGPU> device, Image* image, bool uploadImage,
                                       bool renderTarget)
    : m_device(std::move(device)), m_image(image) {
    Size size                = image->size();

    wgpu::TextureUsage usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
    if (renderTarget) {
        usage |= wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
    }

    const wgpu::TextureDescriptor descriptor{
        .usage  = usage,
        .size   = wgpu::Extent3D{ uint32_t(size.x), uint32_t(size.y) },
        .format = wgFormat(Internal::fixPixelType(image->pixelType()), image->pixelFormat()),
    };
    m_texture     = m_device->m_device.CreateTexture(&descriptor);
    m_textureView = m_texture.CreateView();

    if (uploadImage) {
        writeToGPU(m_image->data(), Point{ 0, 0 });
    }
}

void ImageBackendWebGPU::begin(AccessMode mode, Rectangle rect) {
    if (mode != AccessMode::W) {
        readFromGPU(m_image->data().subrect(rect), rect.p1);
    }
}

void ImageBackendWebGPU::end(AccessMode mode, Rectangle rect) {
    if (mode != AccessMode::R) {
        writeToGPU(m_image->data().subrect(rect), rect.p1);
    }
}

void ImageBackendWebGPU::invalidate() {
    m_invalidated = true;
}

void ImageBackendWebGPU::readFromGPU(const ImageData<UntypedPixel>& data, Point origin) {
    constexpr int wgpuBufferAlignment = 256;
    wgpu::BufferDescriptor bufDesc{};
    bufDesc.usage         = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    int32_t alignedStride = alignUp(data.memoryWidth(), wgpuBufferAlignment);
    bufDesc.size          = alignedStride * data.size.height;
    wgpu::Buffer buffer   = m_device->m_device.CreateBuffer(&bufDesc);

    auto encoder          = m_device->m_device.CreateCommandEncoder();
    wgpu::ImageCopyTexture source{};
    source.texture  = m_texture;
    source.origin.x = origin.x;
    source.origin.y = origin.y;
    source.origin.z = 0;
    source.mipLevel = 0;
    wgpu::ImageCopyBuffer destination{};
    destination.buffer             = buffer;
    destination.layout.bytesPerRow = alignedStride;
    wgpu::Extent3D copySize{ uint32_t(data.size.width), uint32_t(data.size.height) };
    encoder.CopyTextureToBuffer(&source, &destination, &copySize);
    wgpu::CommandBuffer commands = encoder.Finish();
    m_device->m_device.GetQueue().Submit(1, &commands);

    wgpu::FutureWaitInfo future{};
    future.future           = buffer.MapAsync(wgpu::MapMode::Read, 0, bufDesc.size,
                                              wgpu::BufferMapCallbackInfo{
                                                  .mode = wgpu::CallbackMode::AllowProcessEvents,
                                                  .callback =
                                            [](WGPUBufferMapAsyncStatus status, void* userdata) {

                                            },
                                    });
    static bool longTimeout = std::getenv("WGPU_LONG_TIMEOUT");
    auto time               = std::chrono::high_resolution_clock::now();
    //  Timeout is 2 minutes or 5 seconds
    std::chrono::nanoseconds timeout{ longTimeout ? 120'000'000'000 : 5'000'000'000 };
    wgpu::WaitStatus status = m_device->m_instance.WaitAny(1, &future, timeout.count());
    if (status == wgpu::WaitStatus::Success) {
        const UntypedPixel* bufferData =
            reinterpret_cast<const UntypedPixel*>(buffer.GetConstMappedRange(0, bufDesc.size));
        data.copyFrom(ImageData<const UntypedPixel>{ bufferData, data.size, alignedStride, data.components });
        buffer.Unmap();
    } else {
        auto dur = std::chrono::high_resolution_clock::now() - time;
        LOG_ERROR(wgpu, "WaitAny for MapAsync failed: {:08X} after {} (timeout={})", (uint32_t)status,
                  std::chrono::duration_cast<std::chrono::microseconds>(dur),
                  std::chrono::duration_cast<std::chrono::microseconds>(timeout));
    }
}

void ImageBackendWebGPU::writeToGPU(const ImageData<UntypedPixel>& data, Point origin) {
    wgpu::ImageCopyTexture destination;
    destination.texture  = m_texture;
    destination.origin.x = origin.x;
    destination.origin.y = origin.y;
    wgpu::TextureDataLayout source;
    source.bytesPerRow = data.byteStride;
    wgpu::Extent3D writeSize{ uint32_t(data.size.width), uint32_t(data.size.height) };
    m_device->m_device.GetQueue().WriteTexture(&destination, data.data, data.byteSize(), &source, &writeSize);
}

} // namespace Brisk
