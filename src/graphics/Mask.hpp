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

#pragma once

#include <memory>
#include <cstdint>
#include <brisk/core/Memory.hpp>

#include <brisk/graphics/Geometry.hpp>
#include <brisk/graphics/Path.hpp>

namespace Brisk {

namespace Internal {

struct DenseMask {
    size_t stride;
    size_t rows;
    std::unique_ptr<uint8_t[]> data;
    Rectangle bounds;

    DenseMask() : data(nullptr), bounds{} {}

    DenseMask(Rectangle bounds) {
        if (bounds.size().longestSide() >= 16384) {
            throwException(
                EGeometryError("Requested image render target size is too large: {}", bounds.size()));
        }
        stride = alignUp(bounds.width(), 4) + 4;
        rows   = bounds.height() + 3 + 3;
        data   = std::make_unique<uint8_t[]>(stride * rows + 4);
        bounds = bounds, memset(data.get(), 0, stride * rows + 4);
    }

    uint8_t* line(int32_t y) const {
        return data.get() + (y + 3) * stride + 4;
    }
};

inline BRISK_ALWAYS_INLINE uint32_t div255(uint32_t n) {
    return (n + 1 + (n >> 8)) >> 8;
}

inline BRISK_ALWAYS_INLINE void blendRow(uint8_t* dst, uint8_t src, uint32_t len) {
    if (src == 0)
        return;
    if (src >= 255) {
        memset(dst, 255, len);
        return;
    }
    for (uint32_t j = 0; j < len; ++j) {
        *dst = *dst + div255(src * (255 - *dst));
        ++dst;
    }
}

struct alignas(8) PatchData {

    union {
        uint64_t data_u64[2];
        uint32_t data_u32[4];
        uint16_t data_u16[8];
        uint8_t data_u8[16];
    };

    static const PatchData filled;

    static PatchData fromBits(uint16_t bits) {
        PatchData result;
        for (size_t i = 0; i < 16; ++i) {
            result.data_u8[i] = bits & (1 << (15 - i)) ? 255 : 0;
        }
        return result;
    }

    bool operator==(const PatchData& other) const noexcept {
        return data_u64[0] == other.data_u64[0] && data_u64[1] == other.data_u64[1];
    }

    bool operator<(const PatchData& other) const noexcept {
        return data_u64[0] < other.data_u64[0] ||
               (data_u64[0] == other.data_u64[0] && data_u64[1] < other.data_u64[1]);
    }

    bool empty() const noexcept {
        return data_u64[0] == 0 && data_u64[1] == 0;
    }
};

inline const PatchData PatchData::filled = { .data_u64 = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF } };

struct Patch {
    uint32_t coord;  // screen-aligned
    uint32_t offset; // index in patchData

    Patch(uint16_t x, uint16_t y, uint8_t len, uint32_t offset)
        : coord((uint32_t(x) & 0xFFF) | ((uint32_t(y) & 0xFFF) << 12) | (uint32_t(len) << 24)),
          offset(offset) {}

    uint16_t x() const noexcept {
        return (coord & 0xFFF);
    }

    uint16_t y() const noexcept {
        return ((coord >> 12) & 0xFFF);
    }

    uint8_t len() const noexcept {
        return (coord >> 24) & 0xFF;
    }

    bool operator==(const Patch& other) const noexcept {
        return coord == other.coord;
    }

    bool operator<(const Patch& other) const noexcept {
        uint32_t xy       = coord & 0xFFFFFF;
        uint32_t other_xy = other.coord & 0xFFFFFF;
        return xy < other_xy;
    }
};

struct PatchDataHash {
    size_t operator()(const PatchData& data) const noexcept {
        if constexpr (sizeof(size_t) == 8) {
            return std::hash<uint64_t>()(data.data_u64[0]) ^ std::hash<uint64_t>()(data.data_u64[1]);
        } else {
            return std::hash<uint32_t>()(data.data_u32[0]) ^ std::hash<uint32_t>()(data.data_u32[1]) ^
                   std::hash<uint32_t>()(data.data_u32[2]) ^ std::hash<uint32_t>()(data.data_u32[3]);
        }
    }
};

static_assert(sizeof(Patch) == 8, "Patch size must be 8 bytes");

struct PatchMerger {
    std::vector<Patch>& patches;
    std::vector<PatchData>& patchData;
    Rectangle& bounds;

    PatchMerger(std::vector<Patch>& patches, std::vector<PatchData>& patchData, Rectangle& bounds)
        : patches(patches), patchData(patchData), bounds(bounds) {}

    void reserve(size_t size) {
        patches.reserve(size / 2);
        patchData.reserve((size + 7) / 8);
    }

    BRISK_INLINE void add(uint16_t x, uint16_t y, uint8_t len, const PatchData& data) {
        // Extend the last patch if it matches
        if (!patches.empty()) [[likely]] {
            auto& back       = patches.back();
            uint16_t back_x  = back.x();
            uint16_t back_y  = back.y();
            uint8_t back_len = back.len();

            if (x == back_x + back_len && y == back_y && back_len + len < 0x100u &&
                patchData[back.offset] == data) [[unlikely]] {
                // merge with previous patch
                back      = Patch(back_x, back_y, back_len + len, back.offset);
                bounds.x2 = std::max(bounds.x2, int32_t(x + len));
                return;
            }
        }

        uint32_t offset;
        offset = uint32_t(patchData.size());
        patchData.push_back(data);
        patches.emplace_back(x, y, len, offset);
        bounds.x1 = std::min(bounds.x1, int32_t(x));
        bounds.x2 = std::max(bounds.x2, int32_t(x + len));
        bounds.y1 = std::min(bounds.y1, int32_t(y));
        bounds.y2 = std::max(bounds.y2, int32_t(y + 1));
    }

    void add(std::span<const Patch> sourcePatches, const std::vector<PatchData>& sourcePatchData) {
        for (const Patch& patch : sourcePatches) {
            const PatchData& data = sourcePatchData[patch.offset];
            add(patch.x(), patch.y(), patch.len(), data);
        }
    }
};

SparseMask sparseMaskFromDense(const DenseMask& bitmap);

SparseMask maskOp(MaskOp op, const SparseMask& left, const SparseMask& right);

} // namespace Internal
} // namespace Brisk
