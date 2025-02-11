#pragma once

#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Debug.hpp>
#include <brisk/core/RC.hpp>
#include <brisk/graphics/Geometry.hpp>
#include <cstdint>

namespace Brisk {

struct SpriteResource {
    uint64_t id;
    Size size;

    std::byte* data() noexcept {
        return std::launder(reinterpret_cast<std::byte*>(this)) + sizeof(SpriteResource);
    }

    const std::byte* data() const noexcept {
        return std::launder(reinterpret_cast<const std::byte*>(this)) + sizeof(SpriteResource);
    }

    BytesView bytes() const noexcept {
        return { data(), static_cast<size_t>(size.area()) };
    }

    BytesMutableView bytes() noexcept {
        return { data(), static_cast<size_t>(size.area()) };
    }
};

inline RC<SpriteResource> makeSprite(Size size) {
    std::byte* ptr         = (std::byte*)::malloc(sizeof(SpriteResource) + size.area());
    SpriteResource* sprite = new (ptr) SpriteResource{ autoincremented<SpriteResource, uint64_t>(), size };
    return std::shared_ptr<SpriteResource>(sprite, [](SpriteResource* ptr) {
        ::free(reinterpret_cast<std::byte*>(ptr));
    });
}

inline RC<SpriteResource> makeSprite(Size size, BytesView bytes) {
    BRISK_ASSERT(size.area() == bytes.size());
    RC<SpriteResource> result = makeSprite(size);
    memcpy(result->data(), bytes.data(), bytes.size());
    return result;
}

} // namespace Brisk
