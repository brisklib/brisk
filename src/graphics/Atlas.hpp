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
#include <brisk/core/internal/Generation.hpp>
#include <brisk/core/Rc.hpp>
#include "FlatAllocator.hpp"
#include <mutex>
#include <brisk/graphics/internal/Sprites.hpp>
#include <brisk/graphics/Gradients.hpp>
#include <map>

namespace Brisk {

using GradientIndex                         = int32_t;

constexpr inline GradientIndex gradientNull = static_cast<GradientIndex>(-1);

/**
 * @brief Represents an atlas for managing gradients.
 *
 * The GradientAtlas class provides functionalities to add, remove, and manage gradients
 * within a fixed number of slots. It tracks the allocation and deallocation of slots
 * to store gradient data.
 */
class GradientAtlas final {
public:
    /**
     * @brief Constructs a GradientAtlas with the specified number of slots.
     *
     * @param slots The total number of slots available in the atlas for storing gradients.
     * @param mutex A pointer to a mutex for thread-safe operations (may be nullptr).
     */
    explicit GradientAtlas(uint32_t slots, std::recursive_mutex* mutex);

    /**
     * @brief Adds a gradient resource to the atlas.
     *
     * This function adds a new gradient to the atlas and sets its generation to `currentGeneration`.
     * Resources with a generation less than `firstGeneration` may be removed to make space.
     *
     * @param gradient The `GradientResource` to add.
     * @param firstGeneration Minimum generation value required to keep resources in the atlas.
     * @param currentGeneration The generation value for the newly added resource.
     *
     * @return A `GradientIndex` representing the index of the added gradient. Returns `gradientNull`
     *         if no space is available while preserving resources with generation >= `firstGeneration`.
     *
     * @note If `gradientNull` is returned, no changed are made to the atlas.
     */
    GradientIndex addEntry(Rc<GradientResource> gradient, uint64_t firstGeneration,
                           uint64_t currentGeneration);

    /**
     * @brief Gets the size of the gradient atlas specified in constructor.
     *
     * @return The number of slots.
     */
    uint32_t size() const noexcept;

    /**
     * @brief Retrieves a view of the gradient data currently stored in the atlas.
     *
     * @note To safely use the data returned by this method, GradientAtlas's mutex must be locked.
     * @return A read-only span of the gradient data stored in the atlas.
     */
    std::span<const GradientData> data() const noexcept {
        return m_data;
    }

    Generation changed; ///< Represents whether the atlas has changed.

private:
    std::vector<uint8_t> m_slots;     ///< Vector tracking the status of each slot (0 - free, 1 - used).
    std::vector<GradientData> m_data; ///< Vector holding the gradient data for each occupied slot.
    std::recursive_mutex* m_lock;

    struct GradientNode {
        GradientIndex index; ///< The index of the gradient within the atlas.
        uint64_t generation; ///< The generation identifier for the gradient.
    };

    std::map<uint64_t, GradientNode> m_gradients; ///< Map of gradients stored in the atlas.

    /**
     * @brief Removes a gradient from the atlas if its generation is less than `generation`.
     *
     * @param generation The current generation identifier.
     * @return True if an old gradient was removed, false otherwise.
     */
    bool removeOutdated(uint64_t generation);

    /**
     * @brief Checks if there is a free slot available to add a new gradient.
     *
     * @return True if a slot is available, false otherwise.
     */
    bool canAdd();

    /**
     * @brief Allocates a slot for a gradient and stores its data.
     *
     * @param data The data of the gradient to be stored.
     * @return The index of the allocated slot, or gradientNull if no slots are available.
     */
    GradientIndex add(const GradientData& data);

    /**
     * @brief Deallocates a slot and removes the gradient data.
     *
     * @param index The index of the gradient to be removed.
     */
    void remove(GradientIndex index);
};

/// @brief Aligned offset in atlas. Multiply by Atlas::alignment to get byte offset
using SpriteOffset                       = int32_t;

constexpr inline SpriteOffset spriteNull = static_cast<SpriteOffset>(-1);

/**
 * @brief Represents a SpriteAtlas used for managing sprites in a flat memory buffer.
 *
 * The Atlas class provides functionalities to add, remove and manage sprites.
 * It handles memory allocation, deallocation and resizing of the internal buffer as necessary.
 */
class SpriteAtlas final {
public:
    /**
     * @brief Constructs a SpriteAtlas with the specified parameters.
     *
     * @param size Initial size of the atlas.
     * @param maxSize Maximum allowed size of the atlas.
     * @param sizeIncrement The amount by which the atlas size increases when needed.
     * @param mutex A pointer to a mutex for thread-safe operations (may be nullptr).
     */
    explicit SpriteAtlas(uint32_t size, uint32_t maxSize, uint32_t sizeIncrement,
                         std::recursive_mutex* mutex);

    /**
     * @brief Adds a sprite resource to the atlas.
     *
     * This function adds a new sprite to the atlas and sets its generation to `currentGeneration`.
     * Resources with a generation less than `firstGeneration` may be removed to make space.
     *
     * @param sprite The `SpriteResource` to add.
     * @param firstGeneration Minimum generation value required to keep resources in the atlas.
     * @param currentGeneration The generation value for the newly added resource.
     *
     * @return A `SpriteOffset` representing the offset of the added sprite. Returns `spriteNull`
     *         if no space is available while preserving resources with generation >= `firstGeneration`.
     *
     * @note If `spriteNull` is returned, no changed are made to the atlas.
     */
    SpriteOffset addEntry(Rc<SpriteResource> sprite, uint64_t firstGeneration, uint64_t currentGeneration);

    /**
     * @brief Gets the current data stored in the atlas.
     *
     * @return A const reference to the vector containing the atlas data.
     */
    const std::vector<uint8_t>& data() const;

    /**
     * @brief Gets the current size of the atlas.
     *
     * @return The current size of the atlas in bytes.
     */
    uint32_t size() const;

    /**
     * @brief Gets the maximum allowed size of the atlas.
     *
     * @return The maximum size of the atlas in bytes.
     */
    uint32_t maxSize() const;

    /**
     * @brief Gets the size increment of the atlas.
     *
     * @return The size increment of the atlas in bytes.
     */
    uint32_t sizeIncrement() const;

    /**
     * @brief Gets the current statistics of the flat allocator.
     *
     * @return The statistics of the flat allocator.
     */
    FlatAllocatorStat stat() const;

    /**
     * @brief Gets the number of sprites currently stored in the atlas.
     *
     * @return The number of sprites in the atlas.
     */
    size_t numSprites() const;

    /// Alignment for sprite data within the atlas.
    constexpr static size_t alignment = 8;

    Generation changed;

private:
    uint32_t m_size;
    uint32_t m_maxSize;
    uint32_t m_sizeIncrement;
    std::recursive_mutex* m_lock;
    std::vector<uint8_t> m_data;
    size_t m_numSprites = 0;
    using Allocator     = FlatAllocator<uint32_t, alignment>;
    std::unique_ptr<Allocator> m_alloc;

    struct SpriteNode {
        SpriteOffset offset; ///< The offset of the sprite within the atlas.
        uint32_t size;       ///< The size of the sprite.
        uint64_t generation; ///< The generation identifier for the sprite.
    };

    std::map<uint64_t, SpriteNode> m_sprites;

    /**
     * @brief Removes a sprite from the atlas if its generation is less than `generation`.
     *
     * @param generation The current generation identifier.
     * @return True if a sprite was removed, false otherwise.
     */
    bool removeOutdated(uint64_t generation);

    /**
     * @brief Attempts to resize the atlas to make more space.
     *
     * @return True if the resize was successful, false otherwise.
     */
    bool grow();

    /**
     * @brief Tests whether a block of the specified size can be added to the atlas.
     *
     * @param size The size of the block to be added in bytes.
     * @return True if the block can be added, false otherwise.
     */
    bool canAdd(size_t size);

    /**
     * @brief Allocates a block and sets its content.
     *
     * @param data The data to be stored in the allocated block.
     * @return The offset of the allocated block within the atlas, or spriteNull if the atlas is full.
     */
    SpriteOffset add(BytesView data, bool allowGrow);

    /**
     * @brief Deallocates a block and zeroes its content.
     *
     * @param sprite The offset of the sprite to be removed.
     * @param size The size of the block to be deallocated in bytes.
     */
    void remove(SpriteOffset sprite, size_t size);
};

} // namespace Brisk
