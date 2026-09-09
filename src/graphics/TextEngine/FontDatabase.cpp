#include "text_layout/FontDatabase.hpp"

#include <ft2build.h>
#include <hb-ft.h>
#include <hb-ot.h>

#include <brisk/core/internal/InlineVector.hpp>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_MULTIPLE_MASTERS_H
#include FT_SIZES_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Brisk::TextEngine {

namespace {

// OpenType features that HarfBuzz enables by default (or that a font
// commonly ships) and that are capable of changing glyph selection
// (GSUB) or glyph positioning (GPOS) even for a plain Latin/Common
// script run:
//
//   ccmp - glyph composition/decomposition
//   liga - standard ligatures (fi, fl, ffi, ...)
//   clig - contextual ligatures
//   rlig - required ligatures
//   calt - contextual alternates
//   rclt - required contextual alternates
//   locl - localized forms
//   kern - pair kerning (GPOS)
//   mark - mark-to-base attachment
//   mkmk - mark-to-mark attachment
//   curs - cursive attachment
//   dist - minimum distance adjustment
//   rvrn - required variation alternates (variable fonts)
//
// Presence of ANY of these for the "latn" or "DFLT" script is treated
// as disqualifying, regardless of whether it actually touches an
// ASCII glyph. This is deliberately conservative: a false negative
// (falling back to full HarfBuzz shaping unnecessarily) just costs
// some CPU. A false positive (wrongly taking the fast path) would
// produce a visible, hard-to-diagnose shaping bug. Given the huge
// asymmetry in cost, we always round down to "not safe" when unsure.
inline constexpr std::array<const char*, 13> shapingSensitiveFeatures = {
    "ccmp", "liga", "clig", "rlig", "calt", "rclt", "locl", "kern", "mark", "mkmk", "curs", "dist", "rvrn",
};

inline bool tableHasFeatureForScript(hb_face_t* hbFace, hb_tag_t tableTag, hb_tag_t featureTag) {
    static constexpr hb_tag_t scriptsToCheck[] = {
        HB_TAG('l', 'a', 't', 'n'),
        HB_TAG('D', 'F', 'L', 'T'),
    };

    for (hb_tag_t scriptTag : scriptsToCheck) {
        unsigned int scriptIndex = 0;
        if (!hb_ot_layout_table_find_script(hbFace, tableTag, scriptTag, &scriptIndex)) {
            continue; // font has no entry for this script in this table
        }

        // Check the script's default LangSys (what an unlabeled ASCII
        // run resolves to — we don't have a specific BCP-47 language
        // tag at the text-classification stage).
        unsigned int featureIndex = 0;
        if (hb_ot_layout_language_find_feature(hbFace, tableTag, scriptIndex,
                                               HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX, featureTag,
                                               &featureIndex)) {
            return true;
        }
    }
    return false;
}

// Legacy pre-OpenType TrueType 'kern' table. HarfBuzz uses it as a
// kerning fallback when GPOS has no applicable pair-kerning lookups.
// If it exists and is non-empty, ASCII pair spacing may still be
// adjusted even though no GPOS feature fired.
inline bool hasLegacyKernTable(FT_Face ftFace) {
    if (!FT_HAS_KERNING(ftFace)) {
        return false;
    }

    FT_ULong len = 0;
    // Passing a null buffer makes FreeType just report the table
    // length without copying data.
    FT_Error err = FT_Load_Sfnt_Table(ftFace, FT_MAKE_TAG('k', 'e', 'r', 'n'), 0, nullptr, &len);
    return err == FT_Err_Ok && len > 0;
}

// Returns true if it is safe to render ASCII text (0x20-0x7E) for
// `ftFace` using a direct cmap+advance path (one glyph per codepoint,
// hmtx advance) instead of full HarfBuzz shaping. Call this ONCE per
// loaded font (cache the bool alongside the font object); do not call
// per-frame or per-string.
inline bool isAsciiFastShapingSafe(FT_Face ftFace) {
    if (ftFace == nullptr) {
        return false;
    }

    if (hasLegacyKernTable(ftFace)) {
        return false;
    }

    // hb_ft_face_create_referenced keeps its own refcount on ftFace;
    // it does not take ownership away from FreeType.
    hb_face_t* hbFace = hb_ft_face_create_referenced(ftFace);
    if (hbFace == nullptr) {
        return false;
    }

    bool sensitiveFeatureFound = false;
    for (const char* featureStr : shapingSensitiveFeatures) {
        hb_tag_t featureTag = HB_TAG(featureStr[0], featureStr[1], featureStr[2], featureStr[3]);
        if (tableHasFeatureForScript(hbFace, HB_OT_TAG_GSUB, featureTag) ||
            tableHasFeatureForScript(hbFace, HB_OT_TAG_GPOS, featureTag)) {
            sensitiveFeatureFound = true;
            break;
        }
    }

    hb_face_destroy(hbFace);
    return !sensitiveFeatureFound;
}

constexpr Range<char32_t> kAsciiRange{ 0x20, 0x7F }; // 0x7F is exclusive

inline bool fontHasGlyphs(FT_Face ftFace, Range<char32_t> range) {
    if (ftFace == nullptr) {
        return false;
    }

    // FT_Get_Char_Index is a direct cmap lookup. Enumerating the complete cmap
    // with FT_Get_First_Char/FT_Get_Next_Char would usually inspect thousands
    // of entries just to answer this small, fixed ASCII query.
    const FT_ULong first = static_cast<FT_ULong>(range.min);
    const FT_ULong last  = static_cast<FT_ULong>(range.max);
    for (FT_ULong codepoint = first; codepoint < last; ++codepoint) {
        if (FT_Get_Char_Index(ftFace, codepoint) == 0) [[unlikely]] {
            return false;
        }
    }
    return true;
}

/// Converts a hinting policy to the FreeType load flags to use for glyph loading.
constexpr FT_Int32 hintingLoadFlags(Hinting hinting) {
    switch (hinting) {
    case Hinting::Disable:
        return FT_LOAD_NO_HINTING;
    case Hinting::Enable:
        return FT_LOAD_DEFAULT;
    case Hinting::Auto:
    default:
        return FT_LOAD_TARGET_LIGHT;
    }
}

constexpr size_t kMaxVariationAxes = 12;

/// FT_Done_Face calls from FaceEntry destructors, which may run after the owning
/// database has been destroyed (handles can outlive it via PreparedDocument).
size_t s_ftDoneFaceCalls           = 0;

// Branchless ASCII lowercase (only touches 'A'-'Z', leaves everything else untouched).
[[nodiscard]] inline char ascii_to_lower(char c) noexcept {
    unsigned char uc       = static_cast<unsigned char>(c);
    unsigned char is_upper = static_cast<unsigned char>(uc - 'A') < 26u;
    return static_cast<char>(uc | (is_upper << 5)); // OR with 0x20 if uppercase
}

// strcmp-style three-way compare, case-insensitive, ASCII-only.
// Returns <0, 0, >0 like strcmp.
[[nodiscard]] inline int compare_ci(std::string_view a, std::string_view b) noexcept {
    const char* pa      = a.data();
    const char* pb      = b.data();
    const std::size_t n = a.size() < b.size() ? a.size() : b.size();

    for (std::size_t i = 0; i < n; ++i) {
        unsigned char ca = static_cast<unsigned char>(ascii_to_lower(pa[i]));
        unsigned char cb = static_cast<unsigned char>(ascii_to_lower(pb[i]));
        if (ca != cb)
            return static_cast<int>(ca) - static_cast<int>(cb);
    }
    // Common prefix equal -> shorter string is "less".
    return static_cast<int>(a.size()) - static_cast<int>(b.size());
}

// Convenience predicate for use with std::sort / std::map / etc.
[[nodiscard]] inline bool less_ci(std::string_view a, std::string_view b) noexcept {
    return compare_ci(a, b) < 0;
}

[[nodiscard]] inline bool equal_ci(std::string_view a, std::string_view b) noexcept {
    return compare_ci(a, b) == 0;
}

struct VariationCoordinate {
    FT_ULong tag{};
    FT_Fixed value{};
    auto operator<=>(const VariationCoordinate&) const = default;
};

using NormalizedVariations = inline_vector<VariationCoordinate, kMaxVariationAxes>;

struct FontFileRecord {
    uint32_t id{};
    std::filesystem::path path;
    /// Borrowed font bytes for memory-registered fonts (registerFont). When set,
    /// faces are opened with FT_New_Memory_Face and the caller must keep the
    /// buffer alive and unmodified for as long as the database is used.
    const std::byte* data{};
    size_t dataSize{};
    FT_Long faceIndex{};
    std::string familyName;
    bool italic{};
    uint16_t weight{};
    bool asciiFastShapingSafe{};
    bool asciiRangePresent{};

    struct VariationAxis {
        FT_ULong tag{};
        FT_Fixed minimum{};
        FT_Fixed defaultValue{};
        FT_Fixed maximum{};
    };

    inline_vector<VariationAxis, kMaxVariationAxes> variationAxes;
};

struct FaceKey {
    uint32_t fileId{};
    FT_Long faceIndex{};
    NormalizedVariations variations;
    auto operator<=>(const FaceKey&) const = default;
};

struct SizeKey {
    FT_F26Dot6 pixels{};
    FT_Int32 loadFlags{};
    auto operator<=>(const SizeKey&) const = default;
};

struct LibraryOwner {
    FT_Library library{};
    std::shared_ptr<void> sharedLibrary;

    ~LibraryOwner() {
        // A shared library is owned by the caller. Keeping sharedLibrary alive
        // is the reference-counting mechanism for the external FT_Library;
        // only independently-created libraries are destroyed here.
        if (sharedLibrary == nullptr && library != nullptr) {
            FT_Done_FreeType(library);
        }
    }

    explicit LibraryOwner() = default;

    explicit LibraryOwner(std::shared_ptr<void> owner, void* sharedLibrary)
        : library(static_cast<FT_Library>(sharedLibrary)), sharedLibrary(std::move(owner)) {}
};

struct FaceEntry {
    std::shared_ptr<LibraryOwner> libraryOwner;
    FT_Face ftFace{};
    FT_Size activeSize{};

    ~FaceEntry() {
        if (ftFace != nullptr) {
            FT_Done_Face(ftFace);
            ++s_ftDoneFaceCalls;
        }
    }
};

} // namespace

namespace detail {

/// Allocates a stable process-wide identity for a font instance.
uint64_t nextFontInstanceId() {
    static std::atomic<uint64_t> next{ 1 };
    return next.fetch_add(1, std::memory_order_relaxed);
}

class FontInstance {
public:
    std::shared_ptr<FaceEntry> face;
    FT_Size ftSize{};
    hb_font_t* hbFont{};
    FT_F26Dot6 pixels{};
    FT_Int32 loadFlags{};
    bool asciiRangePresent{};
    uint64_t instanceId{};

    ~FontInstance() {
        if (face != nullptr && face->activeSize == ftSize) {
            face->activeSize = nullptr;
        }
        if (hbFont != nullptr) {
            hb_font_destroy(hbFont);
        }
        if (ftSize != nullptr) {
            FT_Done_Size(ftSize);
        }
    }
};

} // namespace detail

class DefaultFontDatabase final : public FontDatabase {
public:
    explicit DefaultFontDatabase(std::shared_ptr<void> owner = {}, void* sharedLibrary = nullptr) {
        if (owner != nullptr && sharedLibrary != nullptr) {
            m_libraryOwner = std::make_shared<LibraryOwner>(std::move(owner), sharedLibrary);
            return;
        }
        m_libraryOwner = std::make_shared<LibraryOwner>();
        FT_Init_FreeType(&m_libraryOwner->library);
    }

    ~DefaultFontDatabase() override {
        m_recentInstances = {};
        m_instanceLookup.clear();
        m_faces.clear();
        m_libraryOwner.reset();
    }

    FontHandle resolveFont(const FontDef& fontDef) const override {
        for (std::string_view family : fontDef.familyNames) {
            if (FontHandle handle = resolveFont(family, fontDef.style, fontDef.weight, fontDef.fontSize,
                                                fontDef.variations, fontDef.hinting)) {
                return handle;
            }
        }
        return {};
    }

    FontHandle resolveFont(std::string_view familyName, FontStyle style, FontWeight weight,
                           LayoutUnit fontSize, std::span<const FontVariation> variations,
                           Hinting hinting) const override {
        if (m_libraryOwner == nullptr || m_libraryOwner->library == nullptr || fontSize <= kZero) {
            return {};
        }

        const FontFileRecord* record = matchFont(familyName, style, weight);
        if (record == nullptr) {
            return {};
        }

        const auto normalized = normalizeVariations(*record, variations);
        if (!normalized) {
            return {};
        }

        FaceKey faceKey{ record->id, record->faceIndex, *normalized };
        std::shared_ptr<FaceEntry> face = getOrCreateFace(faceKey, *record);
        if (face == nullptr) {
            return {};
        }

        const FT_F26Dot6 pixels = static_cast<FT_F26Dot6>(to26Dot6(fontSize));
        if (pixels <= 0) {
            return {};
        }
        const SizeKey sizeKey{ pixels, hintingLoadFlags(hinting) };
        const auto instanceKey = std::make_pair(faceKey, sizeKey);
        if (auto found = m_instanceLookup.find(instanceKey); found != m_instanceLookup.end()) {
            if (FontHandle handle = found->second.lock()) {
                remember(handle);
                return handle;
            }
            m_instanceLookup.erase(found);
        }

        FontHandle handle = createInstance(std::move(face), sizeKey, record->asciiRangePresent);
        if (!handle) {
            return {};
        }
        m_instanceLookup.emplace(instanceKey, handle);
        remember(handle);
        return handle;
    }

    ActiveFont activate(const FontHandle& handle) const override {
        if (!belongsToThisDatabase(handle)) {
            return {};
        }
        FaceEntry& face = *handle->face;
        if (face.activeSize != handle->ftSize) {
            if (FT_Activate_Size(handle->ftSize) != 0) {
                return {};
            }
            face.activeSize = handle->ftSize;
            // hb-ft derives scale/state from FT_Face. A cached size switch is the only
            // GUI state change, so synchronize this font only when that switch occurs.
            hb_ft_font_changed(handle->hbFont);
        }
        return ActiveFont{ static_cast<void*>(face.ftFace), static_cast<void*>(handle->hbFont),
                           static_cast<int32_t>(handle->loadFlags), handle->asciiRangePresent,
                           handle->instanceId };
    }

    FontCacheStats cacheStats() const noexcept override {
        size_t liveFaces = 0;
        for (const auto& [key, face] : m_faces) {
            liveFaces += !face.expired();
        }
        size_t liveInstances = 0;
        for (const auto& [key, instance] : m_instanceLookup) {
            liveInstances += !instance.expired();
        }
        return FontCacheStats{ liveFaces,     liveInstances,    m_sizeInitializations,
                               liveInstances, m_ftNewFaceCalls, m_ftDoneFaceCalls + s_ftDoneFaceCalls };
    }

    /// Registers all faces of an in-memory font file without copying the bytes.
    std::vector<std::string> registerFont(std::span<const std::byte> data) override {
        std::vector<std::string> families;
        if (m_libraryOwner == nullptr || m_libraryOwner->library == nullptr || data.empty()) {
            return families;
        }

        FT_Face first = nullptr;
        ++m_ftNewFaceCalls;
        if (FT_New_Memory_Face(m_libraryOwner->library, reinterpret_cast<const FT_Byte*>(data.data()),
                               static_cast<FT_Long>(data.size()), 0, &first) != 0) {
            return families;
        }
        const FT_Long faceCount = first->num_faces;
        FT_Done_Face(first);
        ++m_ftDoneFaceCalls;

        for (FT_Long faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
            FT_Face face = nullptr;
            ++m_ftNewFaceCalls;
            if (FT_New_Memory_Face(m_libraryOwner->library, reinterpret_cast<const FT_Byte*>(data.data()),
                                   static_cast<FT_Long>(data.size()), faceIndex, &face) != 0) {
                continue;
            }
            FontFileRecord record;
            record.id        = ++m_uniqueFontFileId;
            record.data      = data.data();
            record.dataSize  = data.size();
            record.faceIndex = faceIndex;
            if (face->family_name != nullptr) {
                record.familyName = face->family_name;
                families.push_back(record.familyName);
            }
            record.italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
            TT_OS2* os2   = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
            record.weight = os2 != nullptr && os2->version != 0xFFFF
                                ? static_cast<uint16_t>(os2->usWeightClass)
                                : ((face->style_flags & FT_STYLE_FLAG_BOLD) != 0
                                       ? static_cast<uint16_t>(FontWeight::Bold)
                                       : static_cast<uint16_t>(FontWeight::Regular));
            FT_MM_Var* mm = nullptr;
            if (FT_Get_MM_Var(face, &mm) == 0 && mm != nullptr) {
                const FT_UInt axisCount = std::min(mm->num_axis, static_cast<FT_UInt>(kMaxVariationAxes));
                for (FT_UInt i = 0; i < axisCount; ++i) {
                    record.variationAxes.push_back(
                        { mm->axis[i].tag, mm->axis[i].minimum, mm->axis[i].def, mm->axis[i].maximum });
                }
                FT_Done_MM_Var(m_libraryOwner->library, mm);
            }
            record.asciiFastShapingSafe = isAsciiFastShapingSafe(face);
            record.asciiRangePresent    = fontHasGlyphs(face, kAsciiRange);
            FT_Done_Face(face);
            ++m_ftDoneFaceCalls;
            m_fontFiles.push_back(std::move(record));
        }

        if (faceCount > 0) {
            std::sort(m_fontFiles.begin(), m_fontFiles.end(),
                      [](const FontFileRecord& a, const FontFileRecord& b) {
                          return less_ci(a.familyName, b.familyName);
                      });
        }
        return families;
    }

    static int matchScore(const FontFileRecord& record, FontStyle style, FontWeight weight) {
        const int styleScore = record.italic == (style == FontStyle::Italic) ? 2000 : 0;
        const int distance   = std::abs(static_cast<int>(record.weight) - static_cast<int>(weight));
        return styleScore + 1000 - std::min(distance, 1000);
    }

    const FontFileRecord* matchFont(std::string_view family, FontStyle style, FontWeight weight) const {
        struct FontFileRecordComparator {
            bool operator()(const FontFileRecord& record, std::string_view name) const noexcept {
                return less_ci(record.familyName, name);
            }

            bool operator()(std::string_view name, const FontFileRecord& record) const noexcept {
                return less_ci(name, record.familyName);
            }
        };

        const auto [rangeBegin, rangeEnd] =
            std::equal_range(m_fontFiles.begin(), m_fontFiles.end(), family, FontFileRecordComparator{});

        if (rangeBegin == rangeEnd) {
            // Not a registered family: follow aliases (one hop; aliases always point at
            // real families, so no cycle can occur).
            if (const auto it = findAlias(family); it != m_aliases.end()) {
                const auto [aliasBegin, aliasEnd] = std::equal_range(m_fontFiles.begin(), m_fontFiles.end(),
                                                                     it->second, FontFileRecordComparator{});
                return bestInRange(aliasBegin, aliasEnd, style, weight);
            }
            return nullptr;
        }

        return bestInRange(rangeBegin, rangeEnd, style, weight);
    }

    template <typename Iterator>
    const FontFileRecord* bestInRange(Iterator first, Iterator last, FontStyle style,
                                      FontWeight weight) const {
        const FontFileRecord* best = nullptr;
        int bestScore              = -1;
        for (auto it = first; it != last; ++it) {
            const int score = matchScore(*it, style, weight);
            if (score > bestScore) {
                best      = &*it;
                bestScore = score;
            }
        }
        return best;
    }

    struct CaseInsensitiveLess {
        bool operator()(const std::string& a, const std::string& b) const noexcept {
            return less_ci(a, b);
        }
    };

    using AliasMap = std::map<std::string, std::string, CaseInsensitiveLess>;

    AliasMap::const_iterator findAlias(std::string_view family) const {
        return m_aliases.find(std::string(family));
    }

    bool addAlias(std::string_view existingFamily, std::string_view alias) override {
        if (m_libraryOwner == nullptr || m_libraryOwner->library == nullptr) {
            return false;
        }

        struct FontFileRecordComparator {
            bool operator()(const FontFileRecord& record, std::string_view name) const noexcept {
                return less_ci(record.familyName, name);
            }

            bool operator()(std::string_view name, const FontFileRecord& record) const noexcept {
                return less_ci(name, record.familyName);
            }
        };

        const auto [begin, end] = std::equal_range(m_fontFiles.begin(), m_fontFiles.end(), existingFamily,
                                                   FontFileRecordComparator{});
        if (begin == end) {
            return false;
        }
        m_aliases.insert_or_assign(std::string(alias), std::string(begin->familyName));
        return true;
    }

    std::optional<NormalizedVariations> normalizeVariations(const FontFileRecord& record,
                                                            std::span<const FontVariation> requested) const {
        if (record.variationAxes.empty()) {
            return requested.empty() ? std::optional<NormalizedVariations>(NormalizedVariations{})
                                     : std::nullopt;
        }

        NormalizedVariations normalized;
        for (const FontFileRecord::VariationAxis& axis : record.variationAxes) {
            normalized.push_back({ axis.tag, axis.defaultValue });
        }

        inline_vector<FT_ULong, kMaxVariationAxes> seen;
        bool valid = true;
        for (const FontVariation& variation : requested) {
            const FT_ULong tag = variation.axisTag;
            if (std::find(seen.begin(), seen.end(), tag) != seen.end()) {
                valid = false;
                break;
            }
            seen.push_back(tag);
            const auto* axesBegin = record.variationAxes.data();
            const auto* axesEnd   = record.variationAxes.end();
            const auto* axis =
                std::find_if(axesBegin, axesEnd, [tag](const FontFileRecord::VariationAxis& candidate) {
                    return candidate.tag == tag;
                });
            if (axis == axesEnd) {
                valid = false;
                break;
            }
            const double scaled = static_cast<double>(toFloat(variation.value)) * 65536.0;
            if (!std::isfinite(scaled) || scaled < axis->minimum || scaled > axis->maximum) {
                valid = false;
                break;
            }
            normalized[static_cast<size_t>(axis - axesBegin)].value =
                static_cast<FT_Fixed>(std::llround(scaled));
        }

        std::sort(normalized.begin(), normalized.end());
        return valid ? std::optional<NormalizedVariations>(normalized) : std::nullopt;
    }

    std::shared_ptr<FaceEntry> getOrCreateFace(const FaceKey& key, const FontFileRecord& record) const {
        if (auto found = m_faces.find(key); found != m_faces.end()) {
            if (std::shared_ptr<FaceEntry> face = found->second.lock()) {
                return face;
            }
            m_faces.erase(found);
        }

        auto entry          = std::make_shared<FaceEntry>();
        entry->libraryOwner = m_libraryOwner;
        ++m_ftNewFaceCalls;
        const FT_Error openError =
            record.data != nullptr
                ? FT_New_Memory_Face(m_libraryOwner->library, reinterpret_cast<const FT_Byte*>(record.data),
                                     static_cast<FT_Long>(record.dataSize), key.faceIndex, &entry->ftFace)
                : FT_New_Face(m_libraryOwner->library, record.path.string().c_str(), key.faceIndex,
                              &entry->ftFace);
        if (openError != 0) {
            return {};
        }
        FT_Matrix matrix{ static_cast<FT_Fixed>(0x10000 / kHorizontalOversampling), 0, 0, 0x10000 };
        FT_Set_Transform(entry->ftFace, &matrix, nullptr);
        if (!key.variations.empty()) {
            FT_MM_Var* mm = nullptr;
            if (FT_Get_MM_Var(entry->ftFace, &mm) != 0 || mm == nullptr) {
                return {};
            }
            std::vector<FT_Fixed> coordinates(mm->num_axis);
            bool valid = true;
            for (FT_UInt i = 0; i < mm->num_axis; ++i) {
                auto coordinate = std::find_if(key.variations.begin(), key.variations.end(),
                                               [&](const VariationCoordinate& item) {
                                                   return item.tag == mm->axis[i].tag;
                                               });
                if (coordinate == key.variations.end()) {
                    valid = false;
                    break;
                }
                coordinates[i] = coordinate->value;
            }
            FT_Done_MM_Var(m_libraryOwner->library, mm);
            if (!valid ||
                FT_Set_Var_Design_Coordinates(entry->ftFace, static_cast<FT_UInt>(coordinates.size()),
                                              coordinates.data()) != 0) {
                return {};
            }
        }

        m_faces.emplace(key, entry);
        return entry;
    }

    FontHandle createInstance(std::shared_ptr<FaceEntry> face, const SizeKey& key,
                              bool asciiRangePresent) const {
        auto instance                = std::make_shared<detail::FontInstance>();
        instance->face               = std::move(face);
        instance->pixels             = key.pixels;
        instance->loadFlags          = key.loadFlags;
        instance->asciiRangePresent  = asciiRangePresent;
        instance->instanceId         = detail::nextFontInstanceId();
        const FT_Error newError      = FT_New_Size(instance->face->ftFace, &instance->ftSize);
        const FT_Error activateError = newError == 0 ? FT_Activate_Size(instance->ftSize) : newError;
        if (newError != 0 || activateError != 0) {
            return {};
        }
        instance->face->activeSize = instance->ftSize;
        FT_Size_RequestRec request{ FT_SIZE_REQUEST_TYPE_NOMINAL,
                                    static_cast<FT_Long>(key.pixels * kHorizontalOversampling), key.pixels, 0,
                                    0 };
        const FT_Error requestError = FT_Request_Size(instance->face->ftFace, &request);
        if (requestError != 0) {
            instance->face->activeSize = nullptr;
            return {};
        }
        instance->hbFont = hb_ft_font_create_referenced(instance->face->ftFace);
        if (instance->hbFont == nullptr) {
            instance->face->activeSize = nullptr;
            return {};
        }
        hb_ft_font_set_load_flags(instance->hbFont, key.loadFlags);
        ++m_sizeInitializations;
        return instance;
    }

    bool belongsToThisDatabase(const FontHandle& handle) const noexcept {
        return handle != nullptr && handle->face != nullptr && handle->face->libraryOwner == m_libraryOwner;
    }

    void remember(const FontHandle& handle) const {
        m_recentInstances[m_nextRecentInstance] = handle;
        m_nextRecentInstance                    = (m_nextRecentInstance + 1) % m_recentInstances.size();
    }

public:
    void scanDirectory(const std::filesystem::path& directory) override {
        std::error_code ec;
        if (!std::filesystem::is_directory(directory, ec)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::string ext = entry.path().extension().string();
            if (!equal_ci(ext, ".ttf") && !equal_ci(ext, ".otf") && !equal_ci(ext, ".ttc")) {
                continue;
            }

            ec.clear();
            const std::filesystem::path canonical = std::filesystem::weakly_canonical(entry.path(), ec);
            const std::filesystem::path path      = ec ? entry.path() : canonical;
            FT_Face first                         = nullptr;
            ++m_ftNewFaceCalls;
            if (FT_New_Face(m_libraryOwner->library, path.string().c_str(), 0, &first) != 0) {
                continue;
            }
            const FT_Long faceCount = first->num_faces;
            FT_Done_Face(first);
            ++m_ftDoneFaceCalls;

            for (FT_Long faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
                FT_Face face = nullptr;
                ++m_ftNewFaceCalls;
                if (FT_New_Face(m_libraryOwner->library, path.string().c_str(), faceIndex, &face) != 0) {
                    continue;
                }
                FontFileRecord record;
                record.id        = ++m_uniqueFontFileId;
                record.path      = path;
                record.faceIndex = faceIndex;
                if (face->family_name != nullptr) {
                    record.familyName = face->family_name;
                }
                record.italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
                TT_OS2* os2   = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
                record.weight = os2 != nullptr && os2->version != 0xFFFF
                                    ? static_cast<uint16_t>(os2->usWeightClass)
                                    : ((face->style_flags & FT_STYLE_FLAG_BOLD) != 0
                                           ? static_cast<uint16_t>(FontWeight::Bold)
                                           : static_cast<uint16_t>(FontWeight::Regular));
                FT_MM_Var* mm = nullptr;
                if (FT_Get_MM_Var(face, &mm) == 0 && mm != nullptr) {
                    const FT_UInt axisCount = std::min(mm->num_axis, static_cast<FT_UInt>(kMaxVariationAxes));
                    for (FT_UInt i = 0; i < axisCount; ++i) {
                        record.variationAxes.push_back(
                            { mm->axis[i].tag, mm->axis[i].minimum, mm->axis[i].def, mm->axis[i].maximum });
                    }
                    FT_Done_MM_Var(m_libraryOwner->library, mm);
                }
                record.asciiFastShapingSafe = isAsciiFastShapingSafe(face);
                record.asciiRangePresent    = fontHasGlyphs(face, kAsciiRange);
                FT_Done_Face(face);
                ++m_ftDoneFaceCalls;
                m_fontFiles.push_back(std::move(record));
            }
        }

        std::sort(m_fontFiles.begin(), m_fontFiles.end(),
                  [](const FontFileRecord& a, const FontFileRecord& b) {
                      return less_ci(a.familyName, b.familyName);
                  });
    }

    static constexpr size_t kRecentInstanceCount = 8;
    std::shared_ptr<LibraryOwner> m_libraryOwner;
    std::vector<FontFileRecord> m_fontFiles;
    /// Alias name -> canonical family name (as registered in m_fontFiles).
    AliasMap m_aliases;
    mutable std::map<FaceKey, std::weak_ptr<FaceEntry>> m_faces;
    mutable std::map<std::pair<FaceKey, SizeKey>, std::weak_ptr<const detail::FontInstance>> m_instanceLookup;
    mutable std::array<FontHandle, kRecentInstanceCount> m_recentInstances{};
    mutable size_t m_nextRecentInstance{};
    mutable size_t m_sizeInitializations{};
    mutable size_t m_ftNewFaceCalls{};
    mutable size_t m_ftDoneFaceCalls{};
    mutable uint64_t m_uniqueFontFileId{};
};

std::shared_ptr<const FontDatabase> getDefaultFontDatabase() {
    static const auto database = [] {
        auto db = std::make_shared<DefaultFontDatabase>();
        db->scanDirectory(detail::findFontsDirectory());
        return db;
    }();
    return database;
}

std::shared_ptr<FontDatabase> createFontDatabase() {
    return std::make_shared<DefaultFontDatabase>();
}

std::shared_ptr<FontDatabase> createFontDatabase(std::shared_ptr<void> owner, void* library) {
    return std::make_shared<DefaultFontDatabase>(std::move(owner), library);
}

} // namespace Brisk::TextEngine
