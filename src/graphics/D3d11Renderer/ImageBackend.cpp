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
 */
#include "ImageBackend.hpp"
#include <brisk/core/Utilities.hpp>

namespace Brisk {

ImageBackendD3d11* getOrCreateBackend(Rc<RenderDeviceD3d11> device, Rc<Image> image, bool uploadImage,
                                      bool renderTarget) {
    if (!image)
        return nullptr;
    Internal::ImageBackend* imageBackend = Internal::getBackend(image);

    ImageBackendD3d11* backend           = static_cast<ImageBackendD3d11*>(imageBackend);
    if (backend && imageBackend->device()->backend() == RendererBackend::D3d11)
        return backend;

    ImageBackendD3d11* newBackend =
        new ImageBackendD3d11(std::move(device), image.get(), uploadImage, renderTarget);
    Internal::setBackend(image, newBackend);
    return newBackend;
}

ImageBackendD3d11::ImageBackendD3d11(Rc<RenderDeviceD3d11> device, Image* image, bool uploadImage,
                                     bool renderTarget)
    : m_device(std::move(device)), m_image(image) {
    D3D11_TEXTURE2D_DESC tex = texDesc(
        dxFormatTypeless(m_image->pixelType(), m_image->pixelFormat()), image->size(), 1, D3D11_USAGE_DEFAULT,
        renderTarget ? D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE : D3D11_BIND_SHADER_RESOURCE);
    if (renderTarget) {
        tex.CPUAccessFlags = 0;
    }
    HRESULT hr = m_device->m_device->CreateTexture2D(&tex, nullptr, m_texture.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);

    if (uploadImage) {
        writeToGpu(m_image->data(), Point{ 0, 0 });
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{}; // zero-initialize
    PixelType pixType           = Internal::fixPixelType(m_image->pixelType());
    srvDesc.Format              = dxFormat(pixType, m_image->pixelFormat());
    srvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    hr                          = m_device->m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc,
                                                                               m_srv.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);
}

void ImageBackendD3d11::begin(AccessMode mode, Rectangle rect) {
    if (mode != AccessMode::W) {
        readFromGpu(m_image->data().subrect(rect), rect.p1);
    }
}

void ImageBackendD3d11::end(AccessMode mode, Rectangle rect) {
    if (mode != AccessMode::R) {
        writeToGpu(m_image->data().subrect(rect), rect.p1);
    }
}

void ImageBackendD3d11::invalidate() {
    m_invalidated = true;
}

void ImageBackendD3d11::readFromGpu(const ImageData<UntypedPixel>& data, Point origin) {
    D3D11_TEXTURE2D_DESC texDesc;
    m_texture.Get()->GetDesc(&texDesc);
    texDesc.Width          = data.size.width;
    texDesc.Height         = data.size.height;
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Usage          = D3D11_USAGE_STAGING;
    texDesc.BindFlags      = 0;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texDesc.MiscFlags      = 0;
    ComPtr<ID3D11Texture2D> tmp;
    HRESULT hr = m_device->m_device->CreateTexture2D(&texDesc, nullptr, tmp.ReleaseAndGetAddressOf());
    CHECK_HRESULT(hr, return);

    CD3D11_BOX box(origin.x, origin.y, 0, origin.x + data.size.width, origin.y + data.size.height, 1);
    m_device->m_context->CopySubresourceRegion(tmp.Get(), 0, 0, 0, 0, m_texture.Get(), 0, &box);

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_device->m_context->Map(tmp.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    CHECK_HRESULT(hr, return);
    SCOPE_EXIT {
        m_device->m_context->Unmap(tmp.Get(), 0);
    };

    ImageData<UntypedPixel> srcData;
    srcData.size       = data.size;
    srcData.components = data.components;
    srcData.data       = reinterpret_cast<UntypedPixel*>(mapped.pData);
    srcData.byteStride = mapped.RowPitch;
    data.copyFrom(srcData);
}

void ImageBackendD3d11::writeToGpu(const ImageData<UntypedPixel>& data, Point origin) {
    D3D11_BOX box;
    box.left   = origin.x;
    box.top    = origin.y;
    box.front  = 0;
    box.right  = origin.x + data.size.x;
    box.bottom = origin.y + data.size.y;
    box.back   = 1;
    m_device->m_context->UpdateSubresource(m_texture.Get(), 0, &box, data.data, data.byteStride, 0);
}

Rc<RenderDevice> ImageBackendD3d11::device() const noexcept {
    return m_device;
}
} // namespace Brisk
