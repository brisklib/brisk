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
#include "Atlas.hpp"
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/Log.hpp>

namespace Brisk {

SpriteAtlas::SpriteAtlas(uint32_t size, uint32_t maxSize, uint32_t sizeIncrement, std::recursive_mutex* mutex)
    : m_size(size), m_maxSize(maxSize), m_sizeIncrement(sizeIncrement), m_lock(mutex), m_data(size, 0),
      m_alloc(new Allocator(size)) {}

FlatAllocatorStat SpriteAtlas::stat() const {
    lock_quard_cond lk(m_lock);
    return m_alloc->stat();
}

size_t SpriteAtlas::numSprites() const {
    return m_numSprites;
}

bool SpriteAtlas::canAdd(size_t size) {
    bool canAlloc = m_alloc->canAllocate(size);
    while (!canAlloc) {
        if (!grow()) {
            return false;
        }
        canAlloc = m_alloc->canAllocate(size);
    }
    return true;
}

SpriteOffset SpriteAtlas::add(BytesView data, bool allowGrow) {
    Allocator::offset_type offset = m_alloc->allocate(data.size());
    while (offset == Allocator::null()) {
        if (!allowGrow)
            return spriteNull;
        if (!grow()) {
            return spriteNull;
        }
        offset = m_alloc->allocate(data.size());
    }
    memcpy(m_data.data() + offset, data.data(), data.size());
    ++changed;
    ++m_numSprites;
    return SpriteOffset(offset) / alignment;
}

void SpriteAtlas::remove(SpriteOffset sprite, size_t size) {
    if (sprite == spriteNull)
        return;
    memset(m_data.data() + sprite * alignment, 0, size);
    m_alloc->free(sprite * alignment, size);
    --m_numSprites;
    ++changed;
}

bool SpriteAtlas::grow() {
    if (m_size == m_maxSize) {
        return false;
    }
    m_size += m_sizeIncrement;
    m_alloc->grow(m_size);
    m_data.resize(m_size, 0);
    ++changed;
    return true;
}

SpriteOffset SpriteAtlas::addEntry(Rc<SpriteResource> sprite, uint64_t firstGeneration,
                                   uint64_t currentGeneration) {
    lock_quard_cond lk(m_lock);
    auto it = m_sprites.find(sprite->id);
    // Check if the resource is already in the atlas
    if (it != m_sprites.end()) {
        // Update its generation
        it->second.generation = currentGeneration;
        return it->second.offset;
    }

    bool allowGrow      = false;

    SpriteOffset offset = add(sprite->bytes(), allowGrow);
    while (offset == spriteNull) {
        if (!removeOutdated(firstGeneration)) {
            // Cannot remove any more sprites, but there is still no space for a new sprite
            if (allowGrow)
                return spriteNull;
            allowGrow = true;
        }
        offset = add(sprite->bytes(), allowGrow);
    }
    BRISK_ASSERT(offset != spriteNull);
    m_sprites.insert(
        it, std::pair{ sprite->id, SpriteNode{ offset, uint32_t(sprite->size.area()), currentGeneration } });
    return offset;
}

bool SpriteAtlas::removeOutdated(uint64_t generation) {
    for (auto it = m_sprites.begin(); it != m_sprites.end(); ++it) {
        if (it->second.generation < generation) {
            remove(it->second.offset, it->second.size);
            m_sprites.erase(it);
            return true;
        }
    }
    return false;
}

const std::vector<uint8_t>& SpriteAtlas::data() const {
    return m_data;
}

uint32_t SpriteAtlas::size() const {
    return m_size;
}

uint32_t SpriteAtlas::maxSize() const {
    return m_maxSize;
}

uint32_t SpriteAtlas::sizeIncrement() const {
    return m_sizeIncrement;
}

GradientAtlas::GradientAtlas(uint32_t slots, std::recursive_mutex* mutex)
    : m_slots(slots, 0), m_data(slots), m_lock(mutex) {}

bool GradientAtlas::removeOutdated(uint64_t generation) {
    for (auto it = m_gradients.begin(); it != m_gradients.end(); ++it) {
        if (it->second.generation < generation) {
            remove(it->second.index);
            m_gradients.erase(it);
            return true;
        }
    }
    return false;
}

bool GradientAtlas::canAdd() {
    auto it = std::find(m_slots.begin(), m_slots.end(), 0);
    return it != m_slots.end();
}

GradientIndex GradientAtlas::add(const GradientData& data) {
    auto it = std::find(m_slots.begin(), m_slots.end(), 0);
    if (it == m_slots.end())
        return gradientNull;
    GradientIndex index = it - m_slots.begin();
    m_slots[index]      = 1;
    m_data[index]       = data;
    ++changed;
    return index;
}

void GradientAtlas::remove(GradientIndex index) {
    m_slots[index] = 0;
    m_data[index]  = {};
    ++changed;
}

uint32_t GradientAtlas::size() const noexcept {
    return static_cast<uint32_t>(m_data.size());
}

GradientIndex GradientAtlas::addEntry(Rc<GradientResource> gradient, uint64_t firstGeneration,
                                      uint64_t currentGeneration) {
    lock_quard_cond lk(m_lock);
    auto it = m_gradients.find(gradient->id);
    // Check if the resource is already in the atlas
    if (it != m_gradients.end()) {
        // Update its generation
        it->second.generation = currentGeneration;
        return it->second.index;
    }

    SpriteOffset offset = add(gradient->data);
    while (offset == gradientNull) {
        if (!removeOutdated(firstGeneration)) {
            // Cannot remove any more gradients, but there is still no space for a new gradient
            return gradientNull;
        }
        offset = add(gradient->data);
    }
    m_gradients.insert(it, std::pair{ gradient->id, GradientNode{ offset, currentGeneration } });
    return offset;
}
} // namespace Brisk
