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

#include <brisk/graphics/Fonts.hpp>

namespace Brisk {

namespace Internal {

/**
 * @brief Private bridge data for rasterizing a new-engine glyph into Brisk sprites.
 */
struct TextLayoutGlyphBitmap {
    Size size;
    uint32_t height{};
    Rc<SpriteResource> sprite;
    uint32_t glyphIndex = 0;
    int offsetX         = 0;
    int offsetY         = 0;
    bool color          = false;
    int horizontalScale = 1;
};

/** @brief Renderer-neutral glyph data emitted by the private document renderer. */
struct TextLayoutGlyph {
    PointF position;
    TextLayoutGlyphBitmap bitmap;
    std::optional<Color> color;
};

/** @brief Renderer-neutral text decoration data emitted by the private document renderer. */
struct TextLayoutDecoration {
    PointF start;
    PointF end;
    float underlineOffset     = 0.f;
    float overlineOffset      = 0.f;
    float lineThroughOffset   = 0.f;
    float thickness           = 0.f;
    TextDecoration decoration = TextDecoration::None;
    std::optional<Color> color;
};

enum class GlyphRenderMode : uint8_t {
    Mask,
    Color,
};

struct GlyphCacheKey {
    uint64_t fontInstanceId{};
    uint32_t glyphId{};

    bool operator==(const GlyphCacheKey&) const noexcept = default;
};

struct CachedGlyph {
    Size size;
    uint32_t height{};
    Rc<SpriteResource> sprite;
    int offsetX{};
    int offsetY{};
    GlyphRenderMode renderMode = GlyphRenderMode::Mask;
    int horizontalScale        = 1;
};

struct GlyphCacheStats {
    uint64_t hits{};
    uint64_t misses{};
};

class GlyphCache {
public:
    virtual ~GlyphCache() = default;
    virtual std::optional<CachedGlyph> getOrCreate(const GlyphCacheKey& key,
                                                   function_ref<std::optional<CachedGlyph>()> factory) = 0;
    virtual GlyphCacheStats getCacheStats() const noexcept                                             = 0;
    virtual void setMemoryBudget(size_t bytes)                                                         = 0;
    virtual void clear()                                                                               = 0;
};

void loadTextLayoutGlyphRun(const ShapedText& shapedText, uint32_t preparedRunIndex, GlyphCache* glyphCache,
                            function_ref<void(uint32_t, const TextLayoutGlyphBitmap&)> onGlyph);
void forEachTextLayoutGlyph(const TextLayout& layout, PointF origin,
                            function_ref<void(const TextLayoutGlyph&)> onGlyph);
void forEachTextLayoutDecoration(const TextLayout& layout, PointF origin,
                                 function_ref<void(const TextLayoutDecoration&)> onDecoration);
void textLayoutSelectionRects(const TextLayout& layout, Range<uint32_t> characterSelection,
                              function_ref<void(const ::Brisk::TextSelectionRect&)> onRect);
void renderPreparedDocument(Rc<Image> image, Point origin, const ShapedText& shapedText,
                            const TextLayout& layout);

} // namespace Internal

struct PimplAccessor {
    template <typename Class>
    static auto* getImpl(const Class& obj) {
        return obj.m_impl.get();
    }
};

} // namespace Brisk
