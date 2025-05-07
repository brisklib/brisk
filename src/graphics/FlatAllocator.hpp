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

#include <algorithm>
#include <limits>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Memory.hpp>
#include <brisk/core/internal/Debug.hpp>

namespace Brisk {

struct FlatAllocatorStat {
    size_t totalSize;
    size_t totalFreeSpace;
    size_t largestFreeBlock;
    size_t numFreeBlocks;
    bool operator==(const FlatAllocatorStat& b) const noexcept = default;
};

enum class FlatAllocatorPolicy {
    AllocateFirst,
    AllocateSmallest,
};

template <typename SizeT = uint32_t, SizeT alignment = 1,
          FlatAllocatorPolicy policy = FlatAllocatorPolicy::AllocateFirst>
class FlatAllocator {
public:
    using size_type   = SizeT;
    using offset_type = SizeT;

    static_assert(std::has_single_bit(alignment));

    explicit FlatAllocator(size_type initialSize)
        : m_size(initialSize), m_freeList{ Block{ 0, initialSize } } {
        BRISK_ASSERT(initialSize < null());
    }

    static constexpr offset_type null() {
        return std::numeric_limits<size_type>::max();
    }

    void grow(size_type newSize) {
        BRISK_ASSERT(newSize > m_size);
        m_freeList.push_back(Block{ m_size, newSize - m_size });
        mergeFreeSpace();
        m_size = newSize;
    }

    size_type totalSize() const {
        return m_size;
    }

    bool canAllocate(size_type size) {
        size = alignUp(size, alignment);
        for (size_t i = 0; i < m_freeList.size(); i++) {
            if (m_freeList[i].size >= size) {
                return true;
            }
        }
        return false;
    }

    offset_type allocate(size_type size) {
        size = alignUp(size, alignment);
        if constexpr (policy == FlatAllocatorPolicy::AllocateFirst) {
            for (size_t i = 0; i < m_freeList.size(); i++) {
                if (m_freeList[i].size >= size) {
                    offset_type p = m_freeList[i].offset;
                    if (m_freeList[i].size == size) {
                        m_freeList.erase(m_freeList.begin() + i);
                    } else {
                        m_freeList[i].offset += size;
                        m_freeList[i].size -= size;
                    }
                    return p;
                }
            }
        } else if constexpr (policy == FlatAllocatorPolicy::AllocateSmallest) {
            size_t bestIndex = SIZE_MAX;
            for (size_t i = 0; i < m_freeList.size(); i++) {
                if (m_freeList[i].size >= size) {
                    if (bestIndex == SIZE_MAX || m_freeList[i].size < m_freeList[bestIndex].size) {
                        bestIndex = i;
                    }
                }
            }
            if (bestIndex != SIZE_MAX) {
                offset_type p = m_freeList[bestIndex].offset;
                if (m_freeList[bestIndex].size == size) {
                    m_freeList.erase(m_freeList.begin() + bestIndex);
                } else {
                    m_freeList[bestIndex].offset += size;
                    m_freeList[bestIndex].size -= size;
                }
                return p;
            }
        }
        return null();
    }

    void free(offset_type ptr, size_type size) {
        size    = alignUp(size, alignment);
        auto it = std::upper_bound(m_freeList.begin(), m_freeList.end(), ptr);
        m_freeList.insert(it, Block{ ptr, size });
        mergeFreeSpace();
    }

    FlatAllocatorStat stat() const {
        size_type total   = 0;
        size_type maximum = 0;
        for (size_t i = 0; i < m_freeList.size(); i++) {
            total += m_freeList[i].size;
            maximum = std::max(maximum, m_freeList[i].size);
        }
        return { m_size, total, maximum, m_freeList.size() };
    }

private:
    struct Block {
        offset_type offset;
        size_type size;

        friend bool operator<(offset_type ptr, const Block& bl) {
            return ptr < bl.offset;
        }
    };

    size_type m_size;
    std::vector<Block> m_freeList; // sorted by offset

    void mergeFreeSpace() {
        if (m_freeList.size() <= 1)
            return;
        for (size_t i = m_freeList.size() - 1; i >= 1; --i) {
            if (m_freeList[i - 1].offset + m_freeList[i - 1].size == m_freeList[i].offset) {
                m_freeList[i - 1].size += m_freeList[i].size;
                m_freeList.erase(m_freeList.begin() + i);
            }
        }
    }
};

} // namespace Brisk
