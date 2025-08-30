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

#include "Mask.hpp"

namespace Brisk {

namespace Internal {

Internal::SparseMask sparseMaskFromDense(const DenseMask& bitmap) {
    SparseMask result;
    Rectangle bounds = bitmap.bounds;

    PatchMerger merger(result.patches, result.patchData, result.bounds);
    merger.reserve(bounds.size().area() / 16 + 1);

    size_t stride = bitmap.stride / 4;

    const Rectangle patchBounds{ bounds.x1 / 4, bounds.y1 / 4, (bounds.x2 + 3) / 4, (bounds.y2 + 3) / 4 };

    // Iterate over the image in screen-aligned 4x4 patches:
    for (uint32_t y = patchBounds.y1; y < patchBounds.y2; y++) {
        const uint8_t* line = bitmap.line(y * 4 - bounds.y1);

        for (uint32_t x = patchBounds.x1; x < patchBounds.x2; x++) {
            PatchData patchData;
            const uint32_t* pixels = reinterpret_cast<const uint32_t*>(line + x * 4 - bounds.x1);
            patchData.data_u32[0]  = *pixels;
            patchData.data_u32[1]  = *(pixels + stride);
            patchData.data_u32[2]  = *(pixels + 2 * stride);
            patchData.data_u32[3]  = *(pixels + 3 * stride);

            if (patchData.data_u32[0] != 0 || patchData.data_u32[1] != 0 || patchData.data_u32[2] != 0 ||
                patchData.data_u32[3] != 0) [[likely]] {
                merger.add(uint16_t(x), uint16_t(y), 1, patchData);
            }
        }
    }
    return result;
}

inline PatchData coverageOp(MaskOp op, const PatchData& a, const PatchData& b) {
    PatchData result;
    coverageOp<16>(op, result.data_u8, a.data_u8, b.data_u8);
    return result;
}

inline SparseMask mergeMasks(const SparseMask& a, const SparseMask& b) {
    SparseMask result;
    result.patches   = a.patches;
    result.patchData = a.patchData;
    std::map<PatchData, uint32_t> lookup;
    PatchMerger merger(result.patches, result.patchData, result.bounds);

    for (const Patch& patch : b.patches) {
        const PatchData& data = b.patchData[patch.offset];
        merger.add(patch.x(), patch.y(), patch.len(), data);
    }
    return result;
}

SparseMask maskOp(MaskOp op, const SparseMask& left, const SparseMask& right) {
    if (left.empty() && right.empty()) [[unlikely]] {
        return {}; // empty mask
    }
    if (left.isRectangle() && right.isRectangle()) [[unlikely]] {
        // if both are rectangles, we can use the rectangle operation
        std::optional<RectangleF> result = rectangleOp(op, left.rectangle, right.rectangle);
        if (result)
            return SparseMask(*result);
    }
    if (left.isRectangle() && right.isRectangle()) [[unlikely]] {
        return maskOp(op, left.toSparse(), right.toSparse());
    }
    if (left.isRectangle() && !right.isRectangle()) [[unlikely]] {
        return maskOp(op, left.toSparse(), right);
    }
    if (!left.isRectangle() && right.isRectangle()) [[unlikely]] {
        return maskOp(op, left, right.toSparse());
    }

    BRISK_ASSERT(left.isSparse());
    BRISK_ASSERT(right.isSparse());

    bool singleLeft  = boolOp(op, true, false);
    bool singleRight = boolOp(op, false, true);

    if (left.empty()) [[unlikely]] {
        if (singleRight) {
            return right;
        } else {
            return {};
        }
    }
    if (right.empty()) [[unlikely]] {
        if (singleLeft) {
            return left;
        } else {
            return {};
        }
    }

    if (!left.intersects(right)) [[unlikely]] {
        // if two rle are disjoint
        switch (op) {
        case MaskOp::And:
            return {};
        case MaskOp::AndNot:
            return left;
        case MaskOp::Or:
        case MaskOp::Xor: {
            // merge the two rle objects
            if (left.patches.front() < right.patches.front())
                return mergeMasks(left, right);
            else
                return mergeMasks(right, left);
        }
        default:
            BRISK_UNREACHABLE();
        }
    }

    SparseMask result;
    PatchMerger merger(result.patches, result.patchData, result.bounds);

    struct PatchIterator {
        std::span<const Patch> list;
        uint8_t index;

        PatchIterator(std::span<const Patch> list) : list(list), index(0) {}

        bool empty() const {
            return list.empty();
        }

        Patch front() const {
            if (list.front().len() == 1)
                return list.front();
            return Patch(list.front().x() + index, list.front().y(), 1, list.front().offset);
        }

        Patch partial() const {
            return Patch(list.front().x() + index, list.front().y(), list.front().len() - index,
                         list.front().offset);
        }

        void next(uint8_t len = 1) {
            index += len;
            if (index >= list.front().len()) {
                index = 0;
                list  = list.subspan(1);
            }
        }
    };

    PatchIterator l{ left.patches };
    PatchIterator r{ right.patches };

    for (; !l.empty() && !r.empty();) {
        const Patch lFront = l.front();
        const Patch rFront = r.front();

        if (lFront < rFront) {
            // left patch is before right patch
            if (singleLeft) {
                const PatchData& data = left.patchData[l.list.front().offset];
                merger.add(lFront.x(), lFront.y(), 1, data);
            }
            l.next();
        } else if (rFront < lFront) {
            // right patch is before left patch
            if (singleRight) {
                const PatchData& data = right.patchData[r.list.front().offset];
                merger.add(rFront.x(), rFront.y(), 1, data);
            }
            r.next();
        } else {
            // patches are equal
            const PatchData& dataLeft  = left.patchData[l.list.front().offset];
            const PatchData& dataRight = right.patchData[r.list.front().offset];
            PatchData dataResult       = coverageOp(op, dataLeft, dataRight);
            if (!dataResult.empty())
                merger.add(lFront.x(), lFront.y(), 1, dataResult);
            l.next();
            r.next();
        }
    }

    if (l.empty()) {
        if (singleRight) {
            // left is empty, so we can add right
            if (r.index > 0) {
                Patch rightPartial = r.partial();
                merger.add(rightPartial.x(), rightPartial.y(), rightPartial.len(),
                           right.patchData[rightPartial.offset]);
                r.next(rightPartial.len());
            }

            merger.add(r.list, right.patchData);
        }
    } else { // if (r.empty())
        if (singleLeft) {
            // right is empty, so we can add left
            if (l.index > 0) {
                Patch leftPartial = l.partial();
                merger.add(leftPartial.x(), leftPartial.y(), leftPartial.len(),
                           left.patchData[leftPartial.offset]);
                l.next(leftPartial.len());
            }
            merger.add(l.list, left.patchData);
        }
    }
    return result;
}
} // namespace Internal
} // namespace Brisk
