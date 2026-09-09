#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <brisk/core/internal/FunctionRef.hpp>

#include "Types.hpp"

namespace Brisk::TextEngine {

constexpr inline unsigned kHorizontalOversampling = 16;

struct FontDef;

namespace detail {
class FontInstance;

inline std::filesystem::path findFontsDirectory() {
    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (!ec) {
        const std::array<std::filesystem::path, 6> candidates = {
            cwd / "fonts",
            cwd / "resources" / "fonts",
            cwd / ".." / "fonts",
            cwd / ".." / "resources" / "fonts",
            cwd / ".." / ".." / "fonts",
            cwd / ".." / ".." / "resources" / "fonts",
        };
        for (const std::filesystem::path& candidate : candidates) {
            ec.clear();
            if (std::filesystem::is_directory(candidate, ec)) {
                return candidate;
            }
        }
    }
    return "fonts";
}
} // namespace detail

/**
 * @brief Shared identity and ownership of one variation, pixel size, and load policy.
 *
 * Copies are cheap reference-counted handles. Font resources remain alive while any
 * handle, including one stored by PreparedDocument, refers to the instance.
 */
using FontHandle = std::shared_ptr<const detail::FontInstance>;

/**
 * @brief Opaque handles to the currently active FreeType and HarfBuzz font objects.
 *
 * Returned by activate() after the font's size has been made current. The pointers
 * are owned by the FontDatabase and are only valid until the next activate() call.
 */
struct ActiveFont {
    /// Opaque pointer to the active FreeType face (FT_Face).
    void* ftFace{};
    /// Opaque pointer to the active HarfBuzz font (hb_font_t), sized to match ftFace.
    void* hbFont{};
    /// FreeType load flags that shaping/rasterization should pass to FT_Load_Glyph.
    int32_t loadFlags{};
    /// True when every printable ASCII codepoint (U+0020..U+007E) is present.
    bool asciiRangePresent{};
    /// Stable process-wide identity of the font instance, for rasterization caches.
    uint64_t instanceId{};
};

/**
 * @brief Counters describing the current state and lifetime activity of the font cache.
 */
struct FontCacheStats {
    /// Number of distinct font faces currently cached.
    size_t faceCount{};
    /// Number of FT_Size objects (pixel sizes) currently cached across all faces.
    size_t sizeCount{};
    /// Number of times a cached size had to be (re)initialized for a new pixel size.
    size_t sizeInitializations{};
    /// Number of hb_font_t objects currently cached.
    size_t harfBuzzFontCount{};
    /// Number of times FT_New_Face was invoked (including scans and probes).
    size_t ftNewFaceCalls{};
    /// Number of times FT_Done_Face was invoked.
    size_t ftDoneFaceCalls{};
};

/**
 * @brief Abstract interface for resolving, activating, and rasterizing fonts.
 *
 * The database and layout pipeline are single-threaded. Implementations cache faces,
 * pixel sizes, and HarfBuzz fonts internally; activate() centralizes FT_Size activation
 * and guarantees that the returned hb_font_t's size is active.
 */
class FontDatabase {
public:
    /// Destroys the database. All outstanding FontHandles keep their resources alive.
    virtual ~FontDatabase()                                                                      = default;

    /// Resolves a font from a structured font definition.
    /// @param fontDef The family, style, weight, size, and variations to match.
    /// @return A handle to the resolved font instance, or a null handle if no match.
    [[nodiscard]] virtual FontHandle resolveFont(const FontDef& fontDef) const                   = 0;

    /// Resolves a font by its individual attributes.
    /// @param familyName Case-insensitive family name (aliases are honored).
    /// @param style Requested font style (normal, italic, ...).
    /// @param weight Requested font weight; the closest available match is used.
    /// @param fontSize Requested size in layout units.
    /// @param variations Optional list of variation axis settings for variable fonts.
    /// @param hinting Glyph hinting policy; affects the FreeType load flags used for
    ///                shaping and rasterization.
    /// @return A handle to the resolved font instance, or a null handle if no match.
    [[nodiscard]] virtual FontHandle resolveFont(std::string_view familyName, FontStyle style,
                                                 FontWeight weight, LayoutUnit fontSize,
                                                 std::span<const FontVariation> variations = {},
                                                 Hinting hinting = Hinting::Auto) const          = 0;

    /// Makes the font's pixel size current for shaping and rasterization.
    /// @param fontHandle A handle previously returned by resolveFont().
    /// @return Opaque FreeType/HarfBuzz pointers valid until the next activate() call.
    [[nodiscard]] virtual ActiveFont activate(const FontHandle& fontHandle) const                = 0;

    /// Returns a snapshot of the cache counters for this database.
    [[nodiscard]] virtual FontCacheStats cacheStats() const noexcept                             = 0;

    /// Registers every face of a font file (TTF/OTF/TTC) directly from borrowed memory.
    /// The buffer is NOT copied: faces reference it in place, so the caller must keep
    /// the bytes alive and unmodified for the lifetime of the database (and of any
    /// FontHandle resolved from it).
    /// @param data Borrowed font file bytes; must remain alive and unmodified for the
    ///             lifetime of the database and any FontHandle resolved from it.
    /// @return The family names of the registered faces, in face order (a collection
    ///         file yields one entry per face); empty if the data is not a readable
    ///         font file.
    [[nodiscard]] virtual std::vector<std::string> registerFont(std::span<const std::byte> data) = 0;

    /// Registers an alias that resolves to an existing family name. Family matching
    /// is case-insensitive. A later alias may rebind the same name.
    /// @param existingFamily An already-registered family name the alias resolves to.
    /// @param alias The alias name to register.
    /// @return False if existingFamily is not registered (the alias is then not
    ///         stored); true otherwise.
    [[nodiscard]] virtual bool addAlias(std::string_view existingFamily, std::string_view alias) = 0;

    /// Scans a directory for font files (TTF/OTF/TTC) and registers every face found.
    /// Non-recursive: only regular files directly inside the directory are considered.
    /// Files that cannot be opened as fonts are silently skipped. Previously registered
    /// fonts remain registered; the combined set is re-sorted by family name afterwards.
    /// @param directory The directory to scan; a non-directory path is ignored.
    virtual void scanDirectory(const std::filesystem::path& directory)                           = 0;
};
} // namespace Brisk::TextEngine
