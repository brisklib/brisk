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
#include <brisk/graphics/Html.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <map>
#include <brisk/core/Log.hpp>
#include <brisk/core/Time.hpp>
#include <brisk/core/Utilities.hpp>
#include <brisk/core/internal/Lock.hpp>
#include <brisk/core/internal/Fixed.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/core/Text.hpp>

#include <numeric>
#include <utf8proc.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <lunasvg.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include FT_SIZES_H
#include FT_TRUETYPE_TABLES_H
#include FT_OTSVG_H
#include FT_MODULE_H

namespace Brisk {

void uncompressICUData();

static std::string_view freeTypeError(FT_Error err) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)                                                                                 \
    case e:                                                                                                  \
        return s;
#define FT_ERROR_START_LIST switch (err) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
    return "(unknown)";
}

[[noreturn]] static void handleFTErr(FT_Error err) {
    throwException(EFreeType("FreeType Error: {}", freeTypeError(err)));
}

static void handleFTErrSoft(FT_Error err) {
    LOG_ERROR(freetype, "FreeType Error: {}", freeTypeError(err));
}

#define HANDLE_FT_ERROR(expression)                                                                          \
    do {                                                                                                     \
        FT_Error error_ = expression;                                                                        \
        if (error_) {                                                                                        \
            handleFTErr(error_);                                                                             \
        }                                                                                                    \
    } while (0)

#define HANDLE_FT_ERROR_SOFT(expression, ...)                                                                \
    do {                                                                                                     \
        FT_Error error_ = expression;                                                                        \
        if (error_) {                                                                                        \
            handleFTErrSoft(error_);                                                                         \
            __VA_ARGS__;                                                                                     \
        }                                                                                                    \
    } while (0)

struct hb_buffer_deleter {
    void operator()(hb_buffer_t* ptr) {
        hb_buffer_destroy(ptr);
    }
};

#define HRES 64
#define HRESf 64.f
#define DPI 72

#define HORIZONTAL_OVERSAMPLING 64

using Internal::FTFixed;

inline FTFixed toFixed16(float value) {
    return std::lround(value * 0x10000);
}

inline FTFixed toFixed6(float value) {
    return std::lround(value * 64);
}

inline float fromFixed16(FTFixed value) {
    return static_cast<float>(value) / 0x10000;
}

inline float fromFixed6(FTFixed value) {
    return static_cast<float>(value) / 64;
}

namespace Internal {

struct GlyphCacheKey {
    FTFixed fontSize;
    uint32_t glyphIndex;

    bool operator==(const GlyphCacheKey&) const noexcept = default;
};

static_assert(std::has_unique_object_representations_v<GlyphCacheKey>);

static GlyphCacheKey glyphCacheKey(float fontSize, uint32_t glyphIndex) {
    return { toFixed6(fontSize), glyphIndex };
}

const static InclusiveRange<float> nullRange{ HUGE_VALF, -HUGE_VALF };

struct FontFace {
    FontManager* manager;
    FontFlags flags;
    FT_Face face;
    hb_font_t* hb_font;
    Bytes bytes;

    bool isSvg() const noexcept {
        return (flags && FontFlags::EnableColor) && FT_HAS_SVG(face);
    }

    struct GlyphDataAndTime : GlyphData {
        double time;
    };

    struct SizeData {
        FT_Size ftSize;
        FontMetrics metrics;
    };

    std::unordered_map<GlyphCacheKey, GlyphDataAndTime, FastHash> cache;
    std::map<uint32_t, SizeData> sizes;
    FT_Fixed xHeight                     = 0;
    FT_Fixed capHeight                   = 0;
    int hscale                           = 1;

    FontFace(const FontFace&)            = delete;
    FontFace(FontFace&&)                 = delete;
    FontFace& operator=(const FontFace&) = delete;
    FontFace& operator=(FontFace&&)      = delete;

    ~FontFace() {
        clearCache();
        if (hb_font)
            hb_font_destroy(hb_font);
        for (auto& s : sizes) {
            HANDLE_FT_ERROR_SOFT(FT_Done_Size(s.second.ftSize), continue);
        }
        HANDLE_FT_ERROR_SOFT(FT_Done_Face(face), return);
    }

    explicit FontFace(FontManager* manager, BytesView data, bool makeCopy, FontFlags flags)
        : manager(manager), flags(flags) {
        if (makeCopy) {
            bytes = Bytes(data.begin(), data.end());
            data  = bytes;
        }
        HANDLE_FT_ERROR(FT_New_Memory_Face(static_cast<FT_Library>(manager->m_ft_library),
                                           (const FT_Byte*)data.data(), data.size(), 0, &face));
        HANDLE_FT_ERROR(FT_Select_Charmap(face, FT_ENCODING_UNICODE));

        hscale           = isSvg() ? 1 : manager->m_hscale;

        FT_Matrix matrix = { toFixed16(1.0f / HORIZONTAL_OVERSAMPLING * hscale), toFixed16(0), toFixed16(0),
                             toFixed16(1.0f) };
        FT_Set_Transform(face, &matrix, NULL);
        TT_OS2* os2 = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
        if (os2) {
            if (os2->version >= 2) {
                xHeight   = os2->sxHeight;
                capHeight = os2->sCapHeight;
            }
        }

        // just for HarfBuzz that requires FT_Size to be set
        setSize(toFixed6(10));

        hb_font = hb_ft_font_create_referenced(face);
    }

    std::string_view familyName() const {
        return face->family_name;
    }

    std::string_view styleName() const {
        return face->style_name;
    }

    bool setSize(uint32_t sz) {
        HANDLE_FT_ERROR(FT_Set_Char_Size(face, sz, 0, DPI * HORIZONTAL_OVERSAMPLING, DPI));
        return true;
    }

    SizeData lookupSize(float fontSize) {
        uint32_t sz = toFixed6(fontSize);

        auto it     = sizes.find(sz);
        if (it == sizes.end()) {
            FT_Size ftSize;
            HANDLE_FT_ERROR(FT_New_Size(face, &ftSize));
            HANDLE_FT_ERROR(FT_Activate_Size(ftSize));
            if (!setSize(sz))
                return { nullptr, {} };

            hb_ft_font_changed(hb_font);

            float spaceAdvanceX = getGlyphAdvance(codepointToGlyph(U' '));
            FontMetrics metrics{
                fontSize,
                fromFixed6(ftSize->metrics.ascender),
                fromFixed6(ftSize->metrics.descender),
                fromFixed6(ftSize->metrics.height),
                spaceAdvanceX,
                fromFixed6(ftSize->metrics.height) * 0.0625f,
                xHeight * fontSize / face->units_per_EM,
                capHeight * fontSize / face->units_per_EM,
            };

            it = sizes.insert(it, std::pair{ sz, SizeData{ ftSize, metrics } });
        } else {
            if (it->second.ftSize != face->size) {
                HANDLE_FT_ERROR(FT_Activate_Size(it->second.ftSize));
                hb_ft_font_changed(hb_font);
            }
        }
        return it->second;
    }

    void clearCache() {
        cache.clear();
    }

    GlyphId codepointToGlyph(char32_t codepoint) const {
        return FT_Get_Char_Index(face, (FT_ULong)(codepoint));
    }

    int garbageCollectCache(double maximumTime) {
        int numRemoved = 0;
        double time    = currentTime();
        for (auto it = cache.begin(); it != cache.end();) {
            if (time - it->second.time > maximumTime) {
                it = cache.erase(it);
                ++numRemoved;
            } else {
                ++it;
            }
        }
        return numRemoved;
    }

    std::optional<GlyphData> loadGlyphCached(float fontSize, GlyphId glyphIndex) {
        if (auto it = cache.find(glyphCacheKey(fontSize, glyphIndex)); it != cache.end()) {
            it->second.time = currentTime();
            return it->second;
        } else {
            std::ignore                   = lookupSize(fontSize);
            std::optional<GlyphData> data = loadGlyph(glyphIndex);
            if (!data.has_value())
                return std::nullopt;

            it              = cache.insert(it, std::pair<Internal::GlyphCacheKey, GlyphDataAndTime>{
                                      glyphCacheKey(fontSize, glyphIndex),
                                      GlyphDataAndTime{ static_cast<GlyphData&&>(std::move(*data)), 0.f } });
            it->second.time = currentTime();
            return it->second;
        }
    }

    float getGlyphAdvance(GlyphId glyphIndex) {
        FT_Int32 ftFlags = FT_LOAD_DEFAULT | FT_LOAD_TARGET_LIGHT;
        if (flags && FontFlags::DisableHinting) {
            ftFlags |= FT_LOAD_NO_HINTING;
        } else {
            ftFlags |= FT_LOAD_FORCE_AUTOHINT;
        }
        HANDLE_FT_ERROR_SOFT(FT_Load_Glyph(face, glyphIndex, ftFlags), return 0.f);

        FT_GlyphSlot slot = face->glyph;

        return fromFixed6(slot->advance.x) / float(hscale);
    }

    std::optional<GlyphData> loadGlyph(GlyphId glyphIndex) {
        FT_Int32 ftFlags;
        if (isSvg()) {
            ftFlags = FT_LOAD_TARGET_LIGHT | FT_LOAD_SVG_ONLY | FT_LOAD_COLOR;
        } else {
            ftFlags = FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT;
        }
        if (flags && FontFlags::DisableHinting) {
            ftFlags |= FT_LOAD_NO_HINTING;
        } else {
            ftFlags |= FT_LOAD_FORCE_AUTOHINT;
        }

        FT_Error err = FT_Load_Glyph(face, glyphIndex, ftFlags);
        if (err == FT_Err_Invalid_Glyph_Index || err == FT_Err_Invalid_Argument) {
            return std::nullopt;
        }
        HANDLE_FT_ERROR(err);

        if (isSvg()) {
            if (face->glyph->format != FT_GLYPH_FORMAT_SVG) {
                LOG_WARN(font, "Cannot load svg glyph #{} from a SVG font {}", glyphIndex, familyName());
                return std::nullopt;
            }
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        }

        FT_GlyphSlot slot = face->glyph;
        if (slot->advance.y != 0)
            return std::nullopt;

        GlyphData glyph;
        glyph.offset_x = slot->bitmap_left / float(hscale);
        glyph.offset_y = slot->bitmap_top;
        glyph.size.x   = slot->bitmap.width;
        glyph.size.y   = slot->bitmap.rows;
        unsigned comp  = slot->bitmap.pixel_mode == FT_PIXEL_MODE_BGRA ? 4 : 1;
        glyph.sprite   = makeSprite(Size(glyph.size.x * comp, glyph.size.y));

        if (!glyph.size.empty()) {
            if (slot->bitmap.pitch == glyph.size.x * comp) {
                memcpy(glyph.sprite->data(), slot->bitmap.buffer, glyph.size.area() * comp);
            } else {
                for (int i = 0; i < glyph.size.y; ++i) {
                    memcpy(glyph.sprite->data() + i * glyph.size.x * comp,
                           slot->bitmap.buffer + i * slot->bitmap.pitch, glyph.size.x);
                }
            }
        }
        glyph.advance_x = fromFixed6(slot->advance.x) / float(hscale);
        return glyph;
    }
};

struct Caret {
    TextOptions options;
    float tabStep = 1.f;
    float x       = 0;
    float y       = 0;
    int t         = 0;

    PointF pt() const {
        return { x, y };
    }

    void advance(char32_t ch, const GlyphData* g) {
        switch (ch) {
        case U'\t':
            x = std::max(x, ++t * tabStep);
            break;
        default:
            if (g) {
                x += g->advance_x;
            }
            break;
        }
    }
};

} // namespace Internal

using namespace Internal;

namespace {

struct SvgGlyphState {
    std::unique_ptr<lunasvg::Document> doc;
    lunasvg::Box bbox;
};

FT_Error svg_port_init(FT_Pointer* state) {
    *state = new SvgGlyphState{};
    return FT_Err_Ok;
}

void svg_port_free(FT_Pointer* state) {
    delete reinterpret_cast<SvgGlyphState*>(*state);
}

FT_Error svg_port_render(FT_GlyphSlot slot, FT_Pointer* state) {
    lunasvg::Bitmap bmp(slot->bitmap.buffer, slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch);

    bmp.clear(0x00000000u);

    auto& svgState = *reinterpret_cast<SvgGlyphState*>(*state);
    lunasvg::Matrix mat;
    mat.translate(-svgState.bbox.x, -svgState.bbox.y);
    svgState.doc->render(bmp, mat);

    uint8_t* pixels = slot->bitmap.buffer;
    for (int i = 0; i < slot->bitmap.rows; ++i) {
        for (int j = 0; j < slot->bitmap.width; ++j) {
            std::swap(pixels[j * 4 + 0], pixels[j * 4 + 2]);
        }
        pixels += slot->bitmap.pitch;
    }

    // Rc<Image> img           = rcnew Image(slot->bitmap.buffer, Size(slot->bitmap.width, slot->bitmap.rows),
    //   slot->bitmap.pitch, ImageFormat::RGBA);

    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    slot->bitmap.num_grays  = 256;
    slot->format            = FT_GLYPH_FORMAT_BITMAP;

    return FT_Err_Ok;
}

static RectangleF viewBoxToRect(const std::string& str) {
    std::array<float, 4> values;
    int num       = 0;
    const char* p = str.c_str();
    char* end     = nullptr;
    for (float f = std::strtof(p, &end); p != end; f = std::strtof(p, &end)) {
        values[num++] = f;
        if (num == 4)
            break;
        p = end;
        while (*p && std::isspace(*p))
            ++p;
        if (*p && *p == ',')
            ++p;
    }
    return RectangleF(values[0], values[1], values[0] + values[2], values[1] + values[3]);
}

FT_Error svg_port_preset_slot(FT_GlyphSlot slot, FT_Bool cache, FT_Pointer* state) {
    FT_SVG_Document document = (FT_SVG_Document)slot->other;
    FT_Size_Metrics metrics  = document->metrics;

    FT_UShort units_per_EM   = document->units_per_EM;

    std::string svg((const char*)document->svg_document, document->svg_document_length);

    auto& svgState                         = *reinterpret_cast<SvgGlyphState*>(*state);

    std::unique_ptr<lunasvg::Document> doc = lunasvg::Document::loadFromData(svg);

    auto root                              = doc->rootElement();
    std::string attr_viewBox               = root.getAttribute("viewBox");
    std::string attr_width                 = root.getAttribute("width");
    std::string attr_height                = root.getAttribute("height");

    Size dimensions;
    PointF offset{ 0, 0 };

    if (!attr_viewBox.empty()) {
        RectangleF vbox = viewBoxToRect(attr_viewBox);
        dimensions      = vbox.size();
        offset          = vbox.p1;
    } else if (!attr_width.empty() && !attr_height.empty()) {
        dimensions.width  = atoi(attr_width.c_str());
        dimensions.height = atoi(attr_height.c_str());

        if (dimensions == Size{ 1, 1 }) {
            dimensions.width  = units_per_EM;
            dimensions.height = units_per_EM;
        }
    } else {
        dimensions.width  = units_per_EM;
        dimensions.height = units_per_EM;
    }

    lunasvg::Matrix mat;
    SizeF svgScale = SizeF(metrics.x_ppem, metrics.y_ppem) / SizeF(dimensions);
    mat.scale(svgScale.x, svgScale.y);
    mat.transform(+(double)document->transform.xx / (1 << 16),                         //
                  -(double)document->transform.xy / (1 << 16),                         //
                  -(double)document->transform.yx / (1 << 16),                         //
                  +(double)document->transform.yy / (1 << 16),                         //
                  +(double)document->delta.x / 64 * dimensions.width / metrics.x_ppem, //
                  -(double)document->delta.y / 64 * dimensions.height / metrics.y_ppem //
    );
    mat.translate(-offset.x, -offset.y);
    doc->setMatrix(mat);

    auto box                = doc->box();
    slot->bitmap_left       = std::floor(box.x);
    slot->bitmap_top        = -std::floor(box.y);
    slot->bitmap.rows       = std::ceil(box.y + box.h) - -slot->bitmap_top;
    slot->bitmap.width      = std::ceil(box.x + box.w) - slot->bitmap_left;
    slot->bitmap.pitch      = slot->bitmap.width * 4;
    slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

    if (cache) {
        svgState.doc  = std::move(doc);
        svgState.bbox = box;
    }

    return FT_Err_Ok;
}

SVG_RendererHooks svgHooks = {
    (SVG_Lib_Init_Func)svg_port_init,
    (SVG_Lib_Free_Func)svg_port_free,
    (SVG_Lib_Render_Func)svg_port_render,
    (SVG_Lib_Preset_Slot_Func)svg_port_preset_slot,
};
} // namespace

FontManager::FontManager(std::recursive_mutex* mutex, int hscale, uint32_t cacheTimeMs)
    : m_lock(mutex), m_hscale(hscale), m_cacheTimeMs(cacheTimeMs) {
    HANDLE_FT_ERROR(FT_Init_FreeType(&reinterpret_cast<FT_Library&>(m_ft_library)));

    FT_Module mod = FT_Get_Module(reinterpret_cast<FT_Library&>(m_ft_library), "ot-svg");
    if (!mod) {
        LOG_ERROR(svg, "ot-svg module is not found");
    }

    HANDLE_FT_ERROR(
        FT_Property_Set(reinterpret_cast<FT_Library&>(m_ft_library), "ot-svg", "svg-hooks", &svgHooks));
}

FontManager::~FontManager() {
    m_fonts.clear(); // Free FT_Face’s before calling FT_Done_FreeType
    HANDLE_FT_ERROR(FT_Done_FreeType(static_cast<FT_Library>(m_ft_library)));
}

std::vector<std::string_view> FontManager::fontList(std::string_view ff) const {
    std::vector<std::string_view> list = split(ff, ',');
    for (std::string_view& sv : list) {
        sv = trim(sv);
    }
    return list;
}

FontFace* FontManager::findFontByKey(FontKey fontKey) const {
    for (;;) {
        auto it = m_fonts.find(fontKey);
        if (it != m_fonts.end()) {
            return it->second.get();
        }
        if (std::get<1>(fontKey) != FontStyle::Normal)
            std::get<1>(fontKey) = FontStyle::Normal;
        else if (std::get<2>(fontKey) != FontWeight::Regular)
            std::get<2>(fontKey) = FontWeight::Regular;
        else
            break;
    }
    return nullptr;
}

std::pair<FontFace*, GlyphId> FontManager::lookupCodepoint(const Font& font, char32_t codepoint,
                                                           bool fallbackToUndef) const {
    if (codepoint < U' ')
        return { nullptr, UINT32_MAX };
    auto list = fontList(font.fontFamily);
    for (int offset = 0; offset < list.size(); ++offset) {
        if (list[offset].empty())
            continue;
        FontFace* face = findFontByKey(FontKey{ list[offset], font.style, font.weight });
        if (face) {
            GlyphId id = FT_Get_Char_Index(face->face, (FT_ULong)(codepoint));
            if (id != 0)
                return { face, id };
        }
    }
    if (fallbackToUndef) {
        FontFace* face = findFontByKey(FontKey{ list[0], font.style, font.weight });
        if (face) {
            return { face, 0 };
        }
    }

    return { nullptr, UINT32_MAX };
}

Internal::FontFace* FontManager::lookup(const Font& font) const {
    auto list = fontList(font.fontFamily);
    return findFontByKey(FontKey{ list[0], font.style, font.weight });
}

FontManager::FontKey FontManager::faceToKey(Internal::FontFace* face) const {
    lock_quard_cond lk(m_lock);
    for (const auto& f : m_fonts) {
        if (face == f.second.get())
            return f.first;
    }
    return {};
}

void FontManager::addFontAlias(std::string_view newFontFamily, std::string_view existingFontFamily) {
    lock_quard_cond lk(m_lock);
    SmallVector<std::pair<FontKey, Rc<FontFace>>, 1> aliasesToAdd;
    for (const auto& f : m_fonts) {
        if (std::get<0>(f.first) == existingFontFamily) {
            FontKey k      = f.first;
            std::get<0>(k) = newFontFamily;
            aliasesToAdd.push_back(std::pair{ k, f.second });
        }
    }
    for (auto& f : aliasesToAdd) {
        m_fonts.insert_or_assign(std::move(f.first), std::move(f.second));
    }
}

void FontManager::addFont(std::string fontFamily, FontStyle style, FontWeight weight, BytesView data,
                          bool makeCopy, FontFlags flags) {
    lock_quard_cond lk(m_lock);
    FontKey key{ std::move(fontFamily), style, weight };
    auto fontFace = rcnew FontFace(this, data, makeCopy, flags);
    m_fonts.insert_or_assign(key, fontFace);
    if (fontFace->familyName() != std::get<0>(key)) {
        // Register alias with real font name
        m_fonts.insert_or_assign(FontKey{ fontFace->familyName(), style, weight }, std::move(fontFace));
    }
}

status<IoError> FontManager::addFontFromFile(std::string fontFamily, FontStyle style, FontWeight weight,
                                             const fs::path& path) {
    lock_quard_cond lk(m_lock);
    expected<Bytes, IoError> b = readBytes(path);
    if (b) {
        addFont(std::move(fontFamily), style, weight, *b);
        return {};
    }
    return unexpected(b.error());
}

static bool cmpi(std::string_view a, std::string_view b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

static bool isFontExt(std::string_view ext) {
    return cmpi(ext, ".ttf") || cmpi(ext, ".otf");
}

static std::optional<OsFont> fontQuickInfo(FT_Library library, const fs::path& path) {
    FT_Face face;
    FT_Error err = FT_New_Face(library, path.string().c_str(), 0, &face);
    if (err)
        return std::nullopt;
    SCOPE_EXIT {
        FT_Done_Face(face);
    };

    OsFont font;
    font.path   = path;
    font.weight = FontWeight::Regular;
    font.style  = FontStyle::Normal;
    if (face->family_name == nullptr)
        return std::nullopt;
    font.family = face->family_name;
    if (face->style_name != nullptr) {
        std::string_view styleName = face->style_name;
        std::vector<std::string_view> extraStyles;
        for (std::string_view s : split(styleName, ' ')) {
            if (cmpi(s, "Thin") || cmpi(s, "UltraLight")) {
                font.weight = FontWeight::Thin;
            } else if (cmpi(s, "ExtraLight")) {
                font.weight = FontWeight::ExtraLight;
            } else if (cmpi(s, "Light") || cmpi(s, "SemiLight")) {
                font.weight = FontWeight::Light;
            } else if (cmpi(s, "Regular") || cmpi(s, "Normal") || cmpi(s, "Book")) {
                font.weight = FontWeight::Regular;
            } else if (cmpi(s, "Medium") || cmpi(s, "Roman")) {
                font.weight = FontWeight::Medium;
            } else if (cmpi(s, "SemiBold") || cmpi(s, "DemiBold") || cmpi(s, "Demi")) {
                font.weight = FontWeight::SemiBold;
            } else if (cmpi(s, "Bold")) {
                font.weight = FontWeight::Bold;
            } else if (cmpi(s, "ExtraBold") || cmpi(s, "Heavy")) {
                font.weight = FontWeight::ExtraBold;
            } else if (cmpi(s, "Black")) {
                font.weight = FontWeight::Black;
            } else if (cmpi(s, "Italic") || cmpi(s, "Oblique")) {
                font.style = FontStyle::Italic;
            } else {
                if (!s.empty()) {
                    extraStyles.push_back(s);
                }
            }
        }
        font.styleName = join(extraStyles, ' ');
    }

    return font;
}

std::vector<OsFont> FontManager::installedFonts(bool rescan) const {
    lock_quard_cond lk(m_lock);
    if (m_osFonts.empty() || rescan) {
        m_osFonts.clear();
        for (fs::path path : fontFolders()) {
            for (auto f : fs::directory_iterator(path)) {
                if (f.is_regular_file() && isFontExt(f.path().extension().string())) {
                    if (std::optional<OsFont> fontInfo =
                            fontQuickInfo(static_cast<FT_Library>(m_ft_library), f.path())) {
                        m_osFonts.push_back(std::move(*fontInfo));
                    }
                }
            }
        }
    }
    return m_osFonts;
}

bool FontManager::addSystemFont(std::string fontFamily) {
    lock_quard_cond lk(m_lock);
    fs::path path = fontFolders().front();
#ifdef BRISK_WINDOWS
    return addFontFromFile(fontFamily, FontStyle::Normal, FontWeight::Regular, path / "segoeui.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Italic, FontWeight::Regular, path / "segoeuii.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Normal, FontWeight::Bold, path / "segoeuib.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Italic, FontWeight::Bold, path / "segoeuiz.ttf");
#elif defined BRISK_MACOS
    return addFontFromFile(fontFamily, FontStyle::Normal, FontWeight::Regular, path / "SFNS.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Italic, FontWeight::Regular, path / "SFNSItalic.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Normal, FontWeight::Bold, path / "SFNS.ttf") &&
           addFontFromFile(fontFamily, FontStyle::Italic, FontWeight::Bold, path / "SFNSItalic.ttf");
#else
    return false;
#endif
}

bool FontManager::addFontByName(std::string fontFamily, std::string_view fontName) {
    lock_quard_cond lk(m_lock);
    std::ignore = installedFonts();
    int num     = 0;
    for (const auto& f : m_osFonts) {
        if (f.family == fontName && f.styleName.empty()) {
            if (!addFontFromFile(fontFamily, f.style, f.weight, f.path))
                return false;
            ++num;
        }
    }
    return num > 0;
}

std::vector<FontStyleAndWeight> FontManager::fontFamilyStyles(std::string_view font) const {
    lock_quard_cond lk(m_lock);
    std::vector<FontStyleAndWeight> result;
    for (const auto& f : m_fonts) {
        if (std::get<0>(f.first) == font) {
            result.push_back(FontStyleAndWeight{ std::get<1>(f.first), std::get<2>(f.first) });
        }
    }
    return result;
}

bool FontManager::hasCodepoint(const Font& font, char32_t codepoint) const {
    lock_quard_cond lk(m_lock);
    return lookupCodepoint(font, codepoint, false).first != nullptr;
}

FontMetrics FontManager::metrics(const Font& font) const {
    lock_quard_cond lk(m_lock);
    return getMetrics(font);
}

FontMetrics FontManager::getMetrics(const Font& font) const {
    if (FontFace* ff = lookup(font)) {
        if (FontFace::SizeData sz = ff->lookupSize(font.fontSize); sz.ftSize) {
            return sz.metrics;
        }
    }
    return FontMetrics{};
}

namespace Internal {

// Split text into runs of the same direction
std::vector<TextRun> splitTextRuns(std::u32string_view text, TextDirection defaultDirection) {
    std::vector<TextRun> textRuns;
    auto bidi = bidiTextIterator(text, defaultDirection);
    while (auto f = bidi->next()) {
        TextRun r{
            .direction   = f->direction,
            .begin       = f->codepointRange.min,
            .end         = f->codepointRange.max,
            .visualOrder = f->visualOrder,
            .fontIndex   = 0,
            .face        = nullptr,
        };
        textRuns.push_back(r);
    }
    return textRuns;
}

template <typename Fn>
static std::vector<TextRun> splitRuns(std::u32string_view text, std::vector<TextRun> textRuns, Fn&& fn) {
    std::vector<TextRun> newTextRuns;
    if (textRuns.empty())
        return newTextRuns;

    for (const TextRun& t : textRuns) {
        if (t.end == t.begin)
            continue;
        auto value                        = fn(text[t.begin]);
        uint32_t start                    = t.begin;
        std::vector<TextRun>::iterator it = newTextRuns.end();

        for (uint32_t i = t.begin + 1; i < t.end; ++i) {
            auto newValue = fn(text[i]);
            if (newValue != value) {
                // TODO: correct t.order
                it = newTextRuns.insert(it,
                                        TextRun{ t.direction, start, i, t.visualOrder, t.fontIndex, t.face });
                if (t.direction == TextDirection::LTR) {
                    ++it;
                }
                start = i;
                value = newValue;
            }
        }
        // TODO: correct t.order
        it = newTextRuns.insert(it, TextRun{ t.direction, start, t.end, t.visualOrder, t.fontIndex, t.face });
    }

    return newTextRuns;
}
} // namespace Internal

static bool isControlCode(char32_t code) {
    utf8proc_category_t cat = utf8proc_category(code);
    return cat == UTF8PROC_CATEGORY_CC || cat == UTF8PROC_CATEGORY_ZL || cat == UTF8PROC_CATEGORY_ZP;
}

// Split at control characters
std::vector<Internal::TextRun> FontManager::splitControls(
    std::u32string_view text, const std::vector<Internal::TextRun>& textRuns) const {
    uint32_t counter = 0;
    return Internal::splitRuns(text, textRuns, [&](char32_t ch) -> char32_t {
        return isControlCode(ch) ? ++counter : 0;
    });
}

#if 0
// Assign fonts to text runs
std::vector<TextRun> FontManager::assignFontsToTextRuns(std::u32string_view text, std::span<const FontAndColor> fonts,
                                  std::span<const uint32_t> offsets,
                                                        const std::vector<TextRun>& textRuns) const {

    return Internal::splitRuns(text, textRuns, [&](char32_t ch) -> Internal::FontFace* {
        return lookupCodepoint(font, ch, true).first;
    });
}
#else
// Assign fonts to text runs
std::vector<TextRun> FontManager::assignFontsToTextRuns(std::u32string_view text,
                                                        std::span<const FontAndColor> fonts,
                                                        std::span<const uint32_t> offsets,
                                                        const std::vector<TextRun>& textRuns) const {
    std::vector<TextRun> newTextRuns;
    newTextRuns.reserve(textRuns.size());
    for (const TextRun& t : textRuns) {
        if (t.end == t.begin)
            continue;

        const Font& font                  = fonts[t.fontIndex].font;

        Internal::FontFace* face          = lookupCodepoint(font, text[t.begin], true).first;
        uint32_t start                    = t.begin;
        std::vector<TextRun>::iterator it = newTextRuns.end();

        for (uint32_t i = t.begin + 1; i < t.end; ++i) {
            char32_t codepoint          = text[i];
            Internal::FontFace* newFace = lookupCodepoint(font, codepoint, true).first;
            if (newFace != face) {
                // TODO: correct t.visualOrder
                it = newTextRuns.insert(it,
                                        TextRun{ t.direction, start, i, t.visualOrder, t.fontIndex, face });
                if (t.direction == TextDirection::LTR) {
                    ++it;
                }
                start = i;
                face  = newFace;
            }
        }
        // TODO: correct t.visualOrder
        it = newTextRuns.insert(it, TextRun{ t.direction, start, t.end, t.visualOrder, t.fontIndex, face });
    }
    return newTextRuns;
}
#endif

static void fixCaret(GlyphList::iterator first, GlyphList::iterator last) {
    float min_left             = HUGE_VALF;
    float max_right            = -HUGE_VALF;
    GlyphList::iterator nfirst = first;
    for (; nfirst != last; ++nfirst) {
        min_left  = std::min(min_left, nfirst->left_caret);
        max_right = std::max(max_right, nfirst->right_caret);
    }
    for (; first != last; ++first) {
        first->left_caret  = min_left;
        first->right_caret = max_right;
    }
}

static constexpr uint8_t operator+(OpenTypeFeature feat) {
    return static_cast<uint8_t>(feat);
}

static bool isPrintable(char32_t ch) {
    switch (utf8proc_category(ch)) {
    case UTF8PROC_CATEGORY_ZS:
    case UTF8PROC_CATEGORY_ZL:
    case UTF8PROC_CATEGORY_ZP:
    case UTF8PROC_CATEGORY_CC:
    case UTF8PROC_CATEGORY_CF:
        return false;
    default:
        return true;
    }
}

static AscenderDescender calcAscDesc(float lineHeight, const FontMetrics& metrics) {
    const float halfGap = (metrics.height * lineHeight - metrics.ascender + metrics.descender) * 0.5f;
    return { metrics.ascender + halfGap, -metrics.descender + halfGap };
}

static AscenderDescender spanAscDesc(std::span<const GlyphRun> runs) {
    AscenderDescender result{ 0, 0 };
    for (const GlyphRun& r : runs) {
        result = max(result, r.ascDesc());
    }
    return result;
}

PreparedText FontManager::shapeRuns(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                    std::span<const uint32_t> offsets,
                                    const std::vector<TextRun>& textRuns) const {
    PreparedText shaped;
    shaped.options            = text.options;
    shaped.graphemeBoundaries = textBreakPositions(text.text, TextBreakMode::Grapheme);
    std::vector<uint32_t> textBreaks;
    if (!(text.options && TextOptions::SingleLine))
        textBreaks = textBreakPositions(text.text, TextBreakMode::Line);

    std::unique_ptr<hb_buffer_t, hb_buffer_deleter> hb_buffer;
    hb_buffer.reset(hb_buffer_create());

    for (const TextRun& t : textRuns) {
        const Font& font = fonts[t.fontIndex].font;

        PointF caret{ 0.f, 0.f };
        GlyphRun run;
        run.face          = t.face;
        run.fontSize      = font.fontSize;
        run.tabWidth      = font.tabWidth;
        run.lineHeight    = font.lineHeight;
        run.visualOrder   = t.visualOrder;
        run.decoration    = font.textDecoration;
        run.direction     = t.direction;
        run.verticalAlign = font.verticalAlign;
        run.metrics       = getMetrics(font);
        run.color         = fonts[t.fontIndex].color;

        if (isControlCode(text.text[t.begin])) {
            for (int32_t i = t.begin; i < t.end; ++i) {
                Glyph g;
                g.glyph      = 0;
                g.codepoint  = text.text[i];
                g.begin_char = i;
                g.end_char   = i + 1;
                g.dir        = t.direction;
                g.flags      = GlyphFlags::IsControl;
                run.glyphs.push_back(std::move(g));
            }
            shaped.runs.push_back(std::move(run));
            shaped.visualOrder.push_back(shaped.visualOrder.size());
            run = {};
            continue;
        }
        if (t.face == nullptr)
            continue;

        hb_buffer_reset(hb_buffer.get());

        hb_buffer_add_codepoints(hb_buffer.get(), (const uint32_t*)text.text.data(), text.text.size(),
                                 t.begin, t.end - t.begin);
        hb_buffer_set_direction(hb_buffer.get(),
                                t.direction == TextDirection::LTR ? HB_DIRECTION_LTR : HB_DIRECTION_RTL);
        hb_buffer_guess_segment_properties(hb_buffer.get());

        std::vector<hb_feature_t> features;
        bool kernSet = false;
        for (OpenTypeFeatureFlag feat : font.features) {
            if (feat.feature == OpenTypeFeature::kern) {
                kernSet = true;
            }
            features.push_back(hb_feature_t{
                openTypeFeatures[+feat.feature],
                feat.enabled ? 1u : 0u,
                HB_FEATURE_GLOBAL_START,
                HB_FEATURE_GLOBAL_END,
            });
        }
        FontFlags flags = t.face->flags;
        if (font.letterSpacing > 0) {
            flags |= FontFlags::DisableLigatures;
        }
        if (!kernSet && (flags && FontFlags::DisableKerning)) {
            features.push_back(hb_feature_t{
                openTypeFeatures[+OpenTypeFeature::kern],
                0u,
                HB_FEATURE_GLOBAL_START,
                HB_FEATURE_GLOBAL_END,
            });
        }
        if ((flags && FontFlags::DisableLigatures)) {
            using enum OpenTypeFeature;
            for (OpenTypeFeature feature : { liga, clig, dlig }) {
                features.push_back(hb_feature_t{
                    openTypeFeatures[+feature],
                    0u,
                    HB_FEATURE_GLOBAL_START,
                    HB_FEATURE_GLOBAL_END,
                });
            }
        }

        std::ignore = t.face->lookupSize(font.fontSize);

        hb_shape(t.face->hb_font, hb_buffer.get(), features.data(), features.size());

        unsigned int len               = hb_buffer_get_length(hb_buffer.get());
        hb_glyph_info_t* info          = hb_buffer_get_glyph_infos(hb_buffer.get(), nullptr);
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(hb_buffer.get(), nullptr);

        int run_start                  = run.glyphs.size();
        run.glyphs.reserve(run.glyphs.size() + len);
        uint32_t cluster = UINT32_MAX;
        for (uint32_t i = 0; i < len; i++) {
            bool breakAllowed =
                (hb_glyph_info_get_glyph_flags((info + i)) & HB_GLYPH_FLAG_UNSAFE_TO_BREAK) == 0;
            bool newCluster = info[i].cluster != cluster;
            if (i != 0 && newCluster) {
                if (breakAllowed) {
                    caret.x += font.letterSpacing;
                    if (utf8proc_category(text.text[info[i].cluster]) == UTF8PROC_CATEGORY_ZS) {
                        caret.x += font.wordSpacing;
                    }
                }
            }
            cluster = info[i].cluster;
            Glyph g;
            g.glyph     = info[i].codepoint;
            g.codepoint = text.text[info[i].cluster];
            toggle(g.flags, GlyphFlags::IsPrintable, isPrintable(g.codepoint));
            g.pos.x      = caret.x + fromFixed6(positions[i].x_offset) / HORIZONTAL_OVERSAMPLING;
            g.pos.y      = caret.y - fromFixed6(positions[i].y_offset);
            g.left_caret = caret.x;
            g.begin_char = g.end_char = info[i].cluster;
            caret.x += fromFixed6(positions[i].x_advance) / HORIZONTAL_OVERSAMPLING;
            g.right_caret    = caret.x;
            g.dir            = t.direction;
            bool atLineBreak = false;
            if (newCluster) {
                auto it     = std::lower_bound(textBreaks.begin(), textBreaks.end(), info[i].cluster);
                atLineBreak = it != textBreaks.end() && *it == info[i].cluster;
            }
            toggle(g.flags, GlyphFlags::SafeToBreak, breakAllowed);
            toggle(g.flags, GlyphFlags::AtLineBreak, atLineBreak);
            run.glyphs.push_back(std::move(g));
        }
        uint32_t prev_begin_char   = UINT32_MAX;
        uint32_t end_character     = t.end;
        uint32_t new_end_character = t.end;
        if (t.direction == TextDirection::LTR) {
            for (int i = run.glyphs.size() - 1; i >= run_start; --i) {
                if (run.glyphs[i].begin_char != prev_begin_char) {
                    end_character   = new_end_character;
                    prev_begin_char = run.glyphs[i].begin_char;
                }
                run.glyphs[i].end_char = end_character;
                new_end_character      = run.glyphs[i].begin_char;
            }
        } else {
            for (int i = run_start; i < run.glyphs.size(); ++i) {
                if (run.glyphs[i].begin_char != prev_begin_char) {
                    end_character   = new_end_character;
                    prev_begin_char = run.glyphs[i].begin_char;
                }
                run.glyphs[i].end_char = end_character;
                new_end_character      = run.glyphs[i].begin_char;
            }
        }

        int start = run_start;
        int i;
        for (i = run_start + 1; i < run.glyphs.size(); i++) {
            if (run.glyphs[i - 1].begin_char != run.glyphs[i].begin_char) {
                if (i - start > 1)
                    fixCaret(run.glyphs.begin() + start, run.glyphs.begin() + i);
                start = i;
            }
        }
        if (i - start > 1)
            fixCaret(run.glyphs.begin() + start, run.glyphs.begin() + i);

        BRISK_ASSERT(!run.glyphs.empty());
        shaped.runs.push_back(std::move(run));
        shaped.visualOrder.push_back(shaped.visualOrder.size());
        run = {};
    }
    shaped.lines.push_back(PreparedText::GlyphLine{
        Range{ 0u, uint32_t(shaped.runs.size()) },
        Range{ 0u, uint32_t(shaped.graphemeBoundaries.size()) },
        shaped.runs.empty() ? calcAscDesc(fonts.front().font.lineHeight, getMetrics(fonts.front().font))
                            : spanAscDesc(shaped.runs),
        0.f,
    });

    return shaped;
}

PreparedText FontManager::prepare(const Font& font, const TextWithOptions& text, float width) const {
    lock_quard_cond lk(m_lock);
    if (!text.richText.empty()) {
        RichText richText = text.richText;
        richText.setBaseFont(font);
        return doPrepare(text, richText.fonts, richText.offsets, width);
    } else {
        return doPrepare(text, one(FontAndColor{ font }), {}, width);
    }
}

RectangleF FontManager::bounds(const Font& font, const TextWithOptions& text,
                               GlyphRunBounds boundsType) const {
    if (!text.richText.empty()) {
        RichText richText = text.richText;
        richText.setBaseFont(font);
        return bounds(text, richText.fonts, richText.offsets, boundsType);
    } else {
        return bounds(text, one(FontAndColor{ font }), {}, boundsType);
    }
}

PreparedText FontManager::prepare(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                  std::span<const uint32_t> offsets, float width) const {
    BRISK_ASSERT_MSG("The number of fonts and offsets do not match", fonts.size() == offsets.size() + 1);
    lock_quard_cond lk(m_lock);
    return doPrepare(text, fonts, offsets, width);
}

void FontManager::testRender(Rc<Image> image, const PreparedText& prepared, Point origin,
                             TestRenderFlags flags, std::initializer_list<int> xlines,
                             std::initializer_list<int> ylines) const {
    lock_quard_cond lk(m_lock);
    auto w = image->mapWrite<ImageFormat::Greyscale_U8Gamma>();
    if (flags && TestRenderFlags::TextBounds) {
        RectangleF rect;
        rect = prepared.bounds(GlyphRunBounds::Alignment);
        rect = rect.withOffset(origin);
        for (int32_t y = std::floor(rect.y1); y < std::ceil(rect.y2); ++y) {
            if (y < 0 || y >= w.height())
                continue;
            PixelGreyscale8* l = w.line(y);
            for (int32_t x = std::floor(rect.x1); x < std::ceil(rect.x2); ++x) {
                if (x < 0 || x >= w.width())
                    continue;
                l[x] = std::max(l[x] - 16, 0);
            }
        }
    }
    for (int32_t x : xlines) {
        for (int32_t y = 0; y < w.height(); ++y)
            w.line(y)[x] = PixelGreyscale8{ 128 };
    }
    for (int32_t y : ylines) {
        PixelGreyscale8* l = w.line(y);
        std::fill_n(l, w.width(), PixelGreyscale8{ 128 });
    }

    for (uint32_t ri = 0; ri < prepared.runs.size(); ++ri) {
        const GlyphRun& run = prepared.runVisual(ri);

        for (size_t i = 0; i < run.glyphs.size(); ++i) {
            const Glyph& g                = run.glyphs[i];
            std::optional<GlyphData> data = g.load(run);

            if (data && data->sprite) {
                BytesView v = data->sprite->bytes();
                if (v.empty())
                    continue;
                for (int32_t y = 0; y < data->size.height; ++y) {
                    int32_t yy = std::lround(origin.y - +data->offset_y + (g.pos + run.position).y + y);
                    if (yy < 0 || yy >= w.height())
                        continue;
                    PixelGreyscale8* l = w.line(yy);
                    for (int32_t x = 0; x < data->size.width; ++x) {
                        int32_t xx = std::lround(origin.x + (g.pos + run.position).x + data->offset_x + x);
                        if (xx < 0 || xx >= w.width())
                            continue;
                        uint8_t value = static_cast<uint8_t>(v[x + y * data->size.width]);
                        if (flags && TestRenderFlags::Fade)
                            value /= 2;
                        if (flags && TestRenderFlags::GlyphBounds)
                            value = std::min(value + 10, 255);
                        l[xx] = std::max(l[xx] - value, 0);
                    }
                }
            }
        }
    }
}

const size_t shapeCacheSizeLow  = 190;
const size_t shapeCacheSizeHigh = 210;

PreparedText FontManager::doShapeCached(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                        std::span<const uint32_t> offsets) const {
    BRISK_ASSERT_MSG("The number of fonts and offsets do not match", fonts.size() == offsets.size() + 1);
#if 1
    // TODO: reenable cache
    return doShape(text, fonts, offsets);
#else
    Internal::ShapingCacheKey key{ font, text };
    ++m_cacheCounter;
    if (auto it = m_shapeCache.find(key); it != m_shapeCache.end()) {
        it->second.counter = m_cacheCounter;
        return it->second.runs;
    } else {
        if (m_shapeCache.size() > shapeCacheSizeHigh) {
            for (auto it = m_shapeCache.begin(); it != m_shapeCache.end();) {
                if (it->second.counter < m_cacheCounter - (shapeCacheSizeHigh - shapeCacheSizeLow)) {
                    it = m_shapeCache.erase(it);
                } else {
                    ++it;
                }
            }
        }

        PreparedText shaped = doShape(text, fonts, offsets);
        m_shapeCache.insert(it, std::make_pair(key, ShapeCacheEntry{ shaped, m_cacheCounter }));
        return shaped;
    }
#endif
}

PreparedText FontManager::doShape(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                  std::span<const uint32_t> offsets) const {
    BRISK_ASSERT_MSG("The number of fonts and offsets do not match", fonts.size() == offsets.size() + 1);
    std::vector<TextRun> textRuns = splitTextRuns(text.text, text.defaultDirection);

    uint32_t fontIndex            = 0;
    for (size_t i = 0; i < textRuns.size(); ++i) {
        textRuns[i].fontIndex = fontIndex;
        if (!offsets.empty() && offsets.front() < textRuns[i].end) {
            if (offsets.front() > textRuns[i].begin) {
                textRuns.insert(textRuns.begin() + i, textRuns[i]);
                textRuns[i].end = textRuns[i + 1].begin = offsets.front();
                ++fontIndex;
            }
            offsets = offsets.subspan(1);
        }
    }

    textRuns = assignFontsToTextRuns(text.text, fonts, offsets, textRuns);
    textRuns = splitControls(text.text, textRuns);
    return shapeRuns(text, fonts, offsets, textRuns);
}

PreparedText FontManager::doPrepare(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                                    std::span<const uint32_t> offsets, float width) const {
    BRISK_ASSERT_MSG("The number of fonts and offsets do not match", fonts.size() == offsets.size() + 1);
    PreparedText shaped = doShapeCached(text, fonts, offsets);
    return std::move(shaped).wrap(width, text.options && TextOptions::WrapAnywhere);
}

RectangleF FontManager::bounds(const TextWithOptions& text, std::span<const FontAndColor> fonts,
                               std::span<const uint32_t> offsets, GlyphRunBounds boundsType) const {
    BRISK_ASSERT_MSG("The number of fonts and offsets do not match", fonts.size() == offsets.size() + 1);
    lock_quard_cond lk(m_lock);
    PreparedText run = doPrepare(text, fonts, offsets);
    return run.bounds(boundsType);
}

void FontManager::garbageCollectCache() {
    lock_quard_cond lk(m_lock);
    for (auto& ff : m_fonts) {
        ff.second->garbageCollectCache(0.5);
    }
}

namespace Internal {

float Glyph::caretForDirection(bool inverse) const {
    if ((dir == TextDirection::LTR) != inverse)
        return right_caret;
    else
        return left_caret;
}

std::optional<GlyphData> Glyph::load(const GlyphRun& run) const {
    if (!run.face || glyph == UINT32_MAX)
        return std::nullopt;
    return run.face->loadGlyphCached(run.fontSize, glyph);
}

} // namespace Internal

static RectangleF spanBounds(std::span<const GlyphRun> runs, GlyphRunBounds boundsType) {
    RectangleF result{ HUGE_VALF, HUGE_VALF, -HUGE_VALF, -HUGE_VALF };
    for (const GlyphRun& r : runs) {
        result = result.union_(r.bounds(boundsType));
    }
    return result;
}

template <typename T>
static bool isSubspanOf(std::span<T> subspan, std::span<T> origSpan) {
    return subspan.data() >= origSpan.data() &&
           subspan.data() + subspan.size() <= origSpan.data() + origSpan.size();
}

int32_t PreparedText::yToLine(float y) const {
    if (lines.empty() || y < lines.front().baseline - lines.front().ascDesc.ascender) {
        return -1;
    }
    auto it = std::lower_bound(lines.begin(), lines.end(), y, [](const GlyphLine& line, float y) {
        return line.baseline + line.ascDesc.descender < y;
    });
    return std::distance(lines.begin(), it);
}

uint32_t PreparedText::caretToGrapheme(uint32_t line, float x) const {
    float distance    = HUGE_VALF;
    uint32_t grapheme = UINT32_MAX;
    for (uint32_t i : lines[line].graphemeRange) {
        float newDistance = std::abs(caretPositions[i] - x);
        if (newDistance < distance) {
            grapheme = i;
            distance = newDistance;
        }
    }
    return grapheme;
}

uint32_t PreparedText::caretToGrapheme(PointF pt) const {
    if (lines.empty() || !hasCaretData())
        return UINT32_MAX;
    int32_t line = yToLine(pt.y);
    if (line < 0) {
        return 0;
    }
    if (line >= lines.size()) {
        return graphemeBoundaries.size() - 1;
    }
    return caretToGrapheme(line, pt.x);
}

PointF PreparedText::graphemeToCaret(uint32_t graphemeIndex) const {
    uint32_t line = graphemeToLine(graphemeIndex);
    if (line != UINT32_MAX) {
        return PointF(caretPositions[graphemeIndex], lines[line].baseline);
    }
    return PointF{};
}

uint32_t PreparedText::graphemeToLine(uint32_t graphemeIndex) const {
    if (lines.empty() || !hasCaretData())
        return UINT32_MAX;
    auto it = std::lower_bound(lines.begin(), lines.end(), graphemeIndex,
                               [this](const GlyphLine& line, uint32_t graphemeIndex) {
                                   if (line.runRange.max == 0)
                                       return true;
                                   uint32_t firstGrapheme = line.graphemeRange.min;
                                   return firstGrapheme <= graphemeIndex;
                               });
    if (it == lines.begin()) {
        return 0;
    }
    --it;
    return std::distance(lines.begin(), it);
}

PointF PreparedText::alignLines(float alignment_x, float alignment_y) {
    if (runs.empty()) {
        BRISK_ASSERT(!lines.empty());
        return { 0, -lines.front().ascDesc.height() * alignment_y + lines.front().ascDesc.ascender };
    }
    RectangleF bounds = this->bounds(GlyphRunBounds::Text);
    for (GlyphLine& line : lines) {
        auto lineSpan         = std::span{ runs }.subspan(line.runRange.min, line.runRange.distance());
        RectangleF lineBounds = spanBounds(lineSpan, GlyphRunBounds::Alignment);
        for (GlyphRun& run : lineSpan) {
            run.position.x = run.position.x - lineBounds.x1 - lineBounds.width() * alignment_x;
        }
    }
    float y2 = -bounds.height() * alignment_y - bounds.y1;
    return PointF(0, y2);
}

PointF PreparedText::alignLines(PointF alignment) {
    return alignLines(alignment.x, alignment.y);
}

InclusiveRange<float> GlyphRun::textVRange() const {
#if 0
    return { -metrics.ascender, -metrics.descender };
#else
    AscenderDescender ascDesc = this->ascDesc();
    return { -ascDesc.ascender, ascDesc.descender };
#endif
}

AscenderDescender GlyphRun::ascDesc() const {
    BRISK_ASSERT(!glyphs.empty());
    return calcAscDesc(lineHeight, metrics);
}

float GlyphRun::lastCaret() const noexcept {
    return direction == TextDirection::LTR ? glyphs.back().right_caret : glyphs.front().left_caret;
}

float GlyphRun::firstCaret() const noexcept {
    return direction == TextDirection::LTR ? glyphs.front().left_caret : glyphs.back().right_caret;
}

int GlyphRun::hscale() const noexcept {
    return face ? face->hscale : 1;
}

bool GlyphRun::hasColor() const noexcept {
    return face && face->isSvg();
}

RectangleF GlyphRun::bounds(GlyphRunBounds boundsType) const {
    updateRanges();
    const InclusiveRange<float> vRange = this->textVRange();
    InclusiveRange<float> range;
    switch (boundsType) {
    case GlyphRunBounds::Text:
        range = textHRange;
        break;
    case GlyphRunBounds::Alignment:
        range = alignmentHRange;
        break;
    case GlyphRunBounds::Printable:
        range = printableHRange;
        break;
    default:
        BRISK_UNREACHABLE();
    }
    return RectangleF{ range.min, vRange.min, range.max, vRange.max }.withOffset(position);
}

SizeF GlyphRun::size(GlyphRunBounds boundsType) const {
    return bounds(boundsType).size();
}

Range<uint32_t> GlyphRun::charRange() const {
    BRISK_ASSERT(!glyphs.empty());
    return Range<uint32_t>{
        std::min(glyphs.front().begin_char, glyphs.back().begin_char),
        std::max(glyphs.front().end_char, glyphs.back().end_char),
    };
}

GlyphFlags GlyphRun::flags() const {
    BRISK_ASSERT(!glyphs.empty());
    return glyphs.front().flags;
}

GlyphRun GlyphRun::breakAt(float width, bool allowEmpty, bool wrapAnywhere) & {
    BRISK_ASSERT(!glyphs.empty());
    // Returns the widest glyph run that fits within the specified width
    // and removes the corresponding glyphs from *this.

    Internal::GlyphList glyphs = std::move(this->glyphs);
    GlyphRun result            = *this;

    if (!glyphs.empty()) {
        if (direction == TextDirection::LTR) {
            // Handle left-to-right (LTR) text direction
            // Set the maximum allowed right caret position based on the width.
            float limit  = glyphs.front().left_caret + width;
            int breakPos = allowEmpty ? 0 : -1;

            // Find the position to break the glyph run.
            for (int i = allowEmpty ? 0 : 1; i < glyphs.size(); ++i) {
                // Check if the glyph has the line-break flag.
                if (wrapAnywhere || (glyphs[i].flags && GlyphFlags::AtLineBreak)) {
                    breakPos = i;
                }
                // Stop if the next glyph exceeds the width and a break point has been found.
                if ((glyphs[i].flags && GlyphFlags::IsPrintable) && glyphs[i].right_caret > limit &&
                    breakPos >= 0) {
                    break;
                }
            }

            // If no valid break position found, use the entire glyph list.
            if (breakPos == -1) {
                breakPos = glyphs.size();
            }

            // If break position is zero, no splitting is needed.
            if (breakPos == 0) {
                this->glyphs = std::move(glyphs);
                return result;
            }

            // Resize result glyph run to hold the glyphs that fit within the width.
            result.glyphs.resize(breakPos);
            std::copy_n(std::make_move_iterator(glyphs.begin()), breakPos, result.glyphs.begin());

            for (int i = result.glyphs.size() - 1; i >= 0; --i) {
                if (utf8proc_category(result.glyphs[i].codepoint) == UTF8PROC_CATEGORY_ZS) {
                    result.glyphs[i].flags |= GlyphFlags::IsCompactedWhitespace;
                } else {
                    break;
                }
            }

            // Remove the processed glyphs from the original list.
            glyphs.erase(glyphs.begin(), glyphs.begin() + breakPos);
            this->glyphs = std::move(glyphs);

            // Adjust the positions of the remaining glyphs to maintain relative alignment.
            if (!this->glyphs.empty()) {
                const float offset = this->glyphs.front().left_caret;
                for (auto& g : this->glyphs) {
                    g.left_caret -= offset;
                    g.right_caret -= offset;
                    g.pos.x -= offset;
                }
            }

        } else {
            // Handle right-to-left (RTL) text direction
            // Set the maximum allowed left caret position based on the width.
            float limit  = glyphs.back().right_caret - width;
            int breakPos = allowEmpty ? 0 : -1;

            // Find the position to break the glyph run.
            for (int i = allowEmpty ? 0 : 1; i < glyphs.size(); ++i) {
                // Check if the glyph has the line-break flag (iterate in reverse order).
                if (wrapAnywhere || (glyphs[glyphs.size() - 1 - i].flags && GlyphFlags::AtLineBreak)) {
                    breakPos = i;
                }
                // Stop if the next glyph exceeds the width and a break point has been found.
                if ((glyphs[glyphs.size() - 1 - i].flags && GlyphFlags::IsPrintable) &&
                    glyphs[glyphs.size() - 1 - i].left_caret < limit && breakPos >= 0) {
                    break;
                }
            }

            // If no valid break position found, use the entire glyph list.
            if (breakPos == -1) {
                breakPos = glyphs.size();
            }

            // If break position is zero, no splitting is needed.
            if (breakPos == 0) {
                this->glyphs = std::move(glyphs);
                return result;
            }

            // Resize result glyph run to hold the glyphs that fit within the width.
            result.glyphs.resize(breakPos);
            std::copy_n(std::make_move_iterator(glyphs.end() - breakPos), breakPos, result.glyphs.begin());

            for (int i = 0; i < result.glyphs.size(); ++i) {
                if (utf8proc_category(result.glyphs[i].codepoint) == UTF8PROC_CATEGORY_ZS) {
                    result.glyphs[i].flags |= GlyphFlags::IsCompactedWhitespace;
                } else {
                    break;
                }
            }

            // Remove the processed glyphs from the original list.
            glyphs.erase(glyphs.end() - breakPos, glyphs.end());
            this->glyphs = std::move(glyphs);

            // Adjust the positions of the resulting glyphs to maintain relative alignment.
            if (!result.glyphs.empty()) {
                const float offset = result.glyphs.front().left_caret;
                for (auto& g : result.glyphs) {
                    g.left_caret -= offset;
                    g.right_caret -= offset;
                    g.pos.x -= offset;
                }
            }
        }

        // Invalidate cached ranges for both original and result glyph runs.
        this->invalidateRanges();
        result.invalidateRanges();
    }

    return result;
}

void GlyphRun::invalidateRanges() {
    rangesValid = false;
}

void GlyphRun::updateRanges() const {
    if (rangesValid)
        return;
    textHRange      = nullRange;
    alignmentHRange = nullRange;
    printableHRange = nullRange;

    for (int i = 0; i < glyphs.size(); ++i) {
        InclusiveRange<float> h{ glyphs[i].left_caret, glyphs[i].right_caret };
        textHRange = textHRange.union_(h);
        if (!(glyphs[i].flags && GlyphFlags::IsCompactedWhitespace)) {
            alignmentHRange = alignmentHRange.union_(h);
        }
        if (glyphs[i].flags && GlyphFlags::IsPrintable) {
            printableHRange = printableHRange.union_(h);
        }
    }

    if (textHRange == nullRange)
        textHRange = { 0.f, 0.f };
    if (alignmentHRange == nullRange)
        alignmentHRange = { 0.f, 0.f };
    if (printableHRange == nullRange)
        printableHRange = { 0.f, 0.f };
    rangesValid = true;
}

Font Font::operator()(FontWeight weight) const {
    Font result   = *this;
    result.weight = weight;
    return result;
}

Font Font::operator()(FontStyle style) const {
    Font result  = *this;
    result.style = style;
    return result;
}

Font Font::operator()(float fontSize) const {
    Font result     = *this;
    result.fontSize = fontSize;
    return result;
}

Font Font::operator()(std::string fontFamily) const {
    Font result       = *this;
    result.fontFamily = std::move(fontFamily);
    return result;
}

enum class ExtractLineResult {
    NewLine,
    End,
    MaxWidthReached,
};

/**
 * @brief Extracts a line of text from the input `GlyphRuns` and populates the output.
 *
 * This function processes glyph runs from the `input` to fit within the specified `maxWidth`.
 * It either moves entire glyph runs or splits them if necessary, considering line-breaking
 * and wrapping rules.
 *
 * @param ascDesc Reference to `AscenderDescender` object, updated with the maximum ascender and descender
 * values of the extracted line.
 * @param output Reference to the `GlyphRuns` object where the extracted line will be stored.
 * @param input Reference to the `GlyphRuns` object containing the remaining text to be processed.
 * @param maxWidth Maximum width for the line. If `std::isinf(maxWidth)` is true, the line has no width
 * constraint.
 * @param wrapAnywhere Boolean flag indicating if wrapping can occur at any position within a glyph run.
 */
[[nodiscard]] static ExtractLineResult extractLine(AscenderDescender& ascDesc, GlyphRuns& output,
                                                   GlyphRuns& input, float maxWidth, bool wrapAnywhere) {
    float remainingWidth = maxWidth;
    bool isInf           = std::isinf(maxWidth);

    bool lineIsEmpty     = true;
    while (!input.empty()) {
        GlyphRun& run = input.front();
        ascDesc       = max(ascDesc, run.ascDesc());

        if (run.glyphs.front().codepoint == U'\n') {
            input.erase(input.begin());
            return ExtractLineResult::NewLine;
        }
        run.updateRanges();

        if (isInf || run.printableHRange.max <= remainingWidth) {
            // The run fits on the current line
            if (!isInf)
                remainingWidth -= run.size(GlyphRunBounds::Text).width;
            BRISK_ASSERT(!run.glyphs.empty());
            output.push_back(std::move(run));
            input.erase(input.begin());
            lineIsEmpty = false;
            if (remainingWidth <= 0)
                return ExtractLineResult::MaxWidthReached;
        } else {
            // The run doesn't fit on the current line
            // Try to split run
            GlyphRun partial = run.breakAt(remainingWidth, !lineIsEmpty, wrapAnywhere);
            if (run.glyphs.empty()) {
                input.erase(input.begin());
            }
            if (partial.glyphs.empty()) {
                return ExtractLineResult::MaxWidthReached;
            } else {
                remainingWidth -= partial.size(GlyphRunBounds::Text).width;
                BRISK_ASSERT(!partial.glyphs.empty());
                output.push_back(std::move(partial));
                lineIsEmpty = false;
                if (remainingWidth <= 0)
                    return ExtractLineResult::MaxWidthReached;
            }
        }
    }
    return ExtractLineResult::End;
}

static void formatLine(std::span<const uint32_t> input, std::span<GlyphRun> runs, float y) {
    if (input.empty())
        return;
    auto& first    = runs[input.front()];
    float tabWidth = first.tabWidth;
    Caret caret{ .options = TextOptions::SingleLine,
                 .tabStep = tabWidth * runs[input.front()].metrics.spaceAdvanceX };

    float xOffset = first.bounds(GlyphRunBounds::Text).x1 - first.bounds(GlyphRunBounds::Alignment).x1;

    for (uint32_t ri : input) {
        GlyphRun& run = runs[ri];
        if (run.glyphs.front().flags && GlyphFlags::IsControl) {
            // control-only runs
            run.position = caret.pt() + PointF(xOffset, y);
            float offset = caret.x;
            for (Glyph& g : run.glyphs) {
                g.left_caret = caret.pt().x - offset;
                caret.advance(g.codepoint, nullptr);
                g.right_caret = caret.pt().x - offset;
            }
            run.invalidateRanges();
        } else {
            run.position = caret.pt() + PointF{ xOffset, y - run.verticalAlign };
            caret.x += run.bounds(GlyphRunBounds::Text).width();
        }
    }
}

static void sortVisualOrder(std::span<uint32_t> indices, std::span<const GlyphRun> runs) {
    std::stable_sort(indices.begin(), indices.end(), [runs](uint32_t a, uint32_t b) {
        return runs[a].visualOrder < runs[b].visualOrder;
    });
}

static bool hasControlRuns(std::span<const GlyphRun> runs) {
    for (const GlyphRun& gr : runs) {
        if (gr.flags() && GlyphFlags::IsControl) {
            return true;
        }
    }
    return false;
}

PreparedText PreparedText::wrap(float maxWidth, bool wrapAnywhere) && {
    PreparedText result;
    BRISK_ASSERT(visualOrder.size() == runs.size());
    result.graphemeBoundaries = std::move(graphemeBoundaries);
    if (options && TextOptions::SingleLine || std::isinf(maxWidth) && !hasControlRuns(runs) || runs.empty()) {
        result.runs = std::move(runs);
        result.visualOrder.resize(result.runs.size());
        std::iota(result.visualOrder.begin(), result.visualOrder.end(), 0u);
        sortVisualOrder(result.visualOrder, result.runs);
        formatLine(result.visualOrder, result.runs, 0);
        result.lines = std::move(lines);
    } else {
        float y     = 0;
        bool repeat = true;
        AscenderDescender ascDesc{ 0, 0 };
        while (repeat) {
            GlyphLine line{};
            size_t oldSize         = result.runs.size();
            line.graphemeRange.min = runs.empty() ? result.graphemeBoundaries.size() - 1
                                                  : result.characterToGrapheme(runs.front().charRange().min);
            ExtractLineResult extractResult =
                extractLine(line.ascDesc, result.runs, runs, maxWidth, wrapAnywhere);
            line.graphemeRange.max = runs.empty() ? result.graphemeBoundaries.size() - 1
                                                  : result.characterToGrapheme(runs.front().charRange().min);
            repeat                 = !runs.empty() || extractResult == ExtractLineResult::NewLine;
            if (!repeat) {
                ++line.graphemeRange.max;
            }
            if (line.ascDesc.height() == 0) {
                line.ascDesc = ascDesc;
            }
            BRISK_ASSERT(!line.graphemeRange.empty());
            if (!result.lines.empty())
                y += line.ascDesc.ascender;
            line.runRange = Range<size_t>{ oldSize, result.runs.size() };
            line.baseline = y;
            if (result.runs.size() > oldSize) {
                result.visualOrder.resize(result.runs.size());
                std::iota(result.visualOrder.begin() + oldSize, result.visualOrder.end(),
                          static_cast<uint32_t>(oldSize));

                std::span<uint32_t> lineOrder = std::span{ result.visualOrder }.subspan(oldSize);
                sortVisualOrder(lineOrder, result.runs);
                formatLine(lineOrder, result.runs, line.baseline);
            }
            result.lines.push_back(line);
            y += line.ascDesc.descender;
            if (line.ascDesc.height() > 0) {
                ascDesc = line.ascDesc;
            }
        }
    }

    return result;
}

PreparedText PreparedText::wrap(float maxWidth, bool wrapAnywhere) const& {
    return PreparedText(*this).wrap(maxWidth, wrapAnywhere);
}

RectangleF PreparedText::bounds(GlyphRunBounds boundsType) const {
    RectangleF result = spanBounds(runs, boundsType);
    result.y1         = lines.front().baseline - lines.front().ascDesc.ascender;
    result.y2         = lines.back().baseline + lines.back().ascDesc.descender;
    return result;
}

bool PreparedText::hasCaretData() const noexcept {
    return !caretPositions.empty();
}

void PreparedText::updateCaretData() {
    BRISK_ASSERT(graphemeBoundaries.size() >= 1);
    caretPositions.clear();
    ranges.clear();

    caretPositions.resize(graphemeBoundaries.size(), NAN);
    ranges.resize(graphemeBoundaries.size() - 1, { 0.f, 0.f });
    if (graphemeBoundaries.size() == 1) {
        caretPositions.front() = 0;
        return;
    }
    for (int32_t line = lines.size() - 1; line >= 0; --line) {
        if (lines[line].runRange.empty()) {
            for (uint32_t caret : lines[line].graphemeRange) {
                if (std::isnan(caretPositions[caret])) {
                    caretPositions[caret] = 0;
                }
            }
            continue;
        }
        for (uint32_t ri : lines[line].runRange) {
            const GlyphRun& run = runs[ri];
            for (uint32_t gi = 0; gi < run.glyphs.size(); ++gi) {
                const Glyph& g         = run.glyphs[gi];
                uint32_t firstGrapheme = characterToGrapheme(g.begin_char);
                uint32_t lastGrapheme  = characterToGrapheme(g.end_char - 1);
                float beginCaret       = g.caretForDirection(true);
                float endCaret         = g.caretForDirection(false);
                uint32_t numGraphemes  = lastGrapheme - firstGrapheme + 1;
                for (uint32_t grapheme = firstGrapheme; grapheme <= lastGrapheme; ++grapheme) {
                    float leftFrac  = float(grapheme - firstGrapheme) / numGraphemes;
                    float rightFrac = float(grapheme - firstGrapheme + 1) / numGraphemes;
                    if (std::isnan(caretPositions[grapheme]))
                        caretPositions[grapheme] = mix(leftFrac, beginCaret, endCaret) + run.position.x;
                    if (std::isnan(caretPositions[grapheme + 1]))
                        caretPositions[grapheme + 1] = mix(rightFrac, beginCaret, endCaret) + run.position.x;

                    ranges[grapheme].min = mix(leftFrac, g.left_caret, g.right_caret) + run.position.x;
                    ranges[grapheme].max = mix(rightFrac, g.left_caret, g.right_caret) + run.position.x;
                }
            }
        }
    }
    float prev = 0;
    for (size_t i = 0; i < caretPositions.size(); ++i) {
        if (std::isnan(caretPositions[i]))
            caretPositions[i] = prev;
        else
            prev = caretPositions[i];
    }
}

GlyphRun& PreparedText::runVisual(uint32_t index) {
    return runs[visualOrder[index]];
}

const GlyphRun& PreparedText::runVisual(uint32_t index) const {
    return runs[visualOrder[index]];
}

Range<uint32_t> PreparedText::graphemeToCharacters(uint32_t graphemeIndex) const {
    return { graphemeToCharacter(graphemeIndex), graphemeToCharacter(graphemeIndex + 1) };
}

uint32_t PreparedText::graphemeToCharacter(uint32_t graphemeIndex) const {
    return graphemeBoundaries[std::min(graphemeIndex, (uint32_t)graphemeBoundaries.size() - 1u)];
}

uint32_t PreparedText::characterToGrapheme(uint32_t charIndex) const {
    if (charIndex == 0)
        return 0;
    return std::upper_bound(graphemeBoundaries.begin(), graphemeBoundaries.end(), charIndex) -
           graphemeBoundaries.begin() - 1;
}

float FontMetrics::linegap() const noexcept {
    return height - ascender + descender;
}

float FontMetrics::underlineOffset() const noexcept {
    return -descender * 0.5f;
}

float FontMetrics::overlineOffset() const noexcept {
    return -ascender * 0.84375f;
}

float FontMetrics::lineThroughOffset() const noexcept {
    return (underlineOffset() + overlineOffset()) * 0.5f;
}

float FontMetrics::vertBounds() const noexcept {
    return -descender + ascender;
}

TextWithOptions::TextWithOptions(std::string_view text, TextOptions options, TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(text);
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = utf8ToUtf32(text);
    }
}

TextWithOptions::TextWithOptions(std::u16string_view text, TextOptions options,
                                 TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf16ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = utf16ToUtf32(text);
    }
}

TextWithOptions::TextWithOptions(std::u32string_view text, TextOptions options,
                                 TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf32ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = text;
    }
}

TextWithOptions::TextWithOptions(std::u32string text, TextOptions options, TextDirection defaultDirection) {
    this->options          = options & ~TextOptions::Html;
    this->defaultDirection = defaultDirection;
    if (options && TextOptions::Html) {
        auto rich = RichText::fromHtml(utf32ToUtf8(text));
        if (rich)
            std::tie(this->text, this->richText) = *rich;
    } else {
        this->text = std::move(text);
    }
}

namespace Internal {

static Font overrideFont(const Font& base, Font&& font, FontFormatFlags flags) {
    font.letterSpacing = base.letterSpacing;
    font.wordSpacing   = base.wordSpacing;
    font.verticalAlign = base.verticalAlign;
    font.tabWidth      = base.tabWidth;
    font.lineHeight    = base.lineHeight;
    if (!(flags && FontFormatFlags::Family))
        font.fontFamily = base.fontFamily;
    if (!(flags && FontFormatFlags::Size))
        font.fontSize = base.fontSize;
    else if (flags && FontFormatFlags::SizeIsRelative) {
        font.fontSize = base.fontSize * font.fontSize;
    }
    if (!(flags && FontFormatFlags::Weight))
        font.weight = base.weight;
    if (!(flags && FontFormatFlags::Style))
        font.style = base.style;
    if (!(flags && FontFormatFlags::TextDecoration))
        font.textDecoration = base.textDecoration;
    return font;
}

void RichText::setBaseFont(const Font& font) {
    BRISK_ASSERT(fonts.size() == flags.size());
    for (size_t i = 0; i < fonts.size(); ++i) {
        fonts[i].font = overrideFont(font, std::move(fonts[i].font), flags[i]);
    }
    flags.clear();
}

struct FontFormatEx : FontAndColor {
    FontFormatFlags flags                      = FontFormatFlags::None;

    bool operator==(const FontFormatEx&) const = default;
};

struct Visitor final : public HtmlSax {
    std::u32string text;
    RichText richText;
    std::vector<FontFormatEx> fontStack{
        FontFormatEx{},
    };
    std::string_view tag;
    std::string_view attr;
    std::string attrValue;

    void closeDocument() {
        if (!richText.offsets.empty())
            richText.offsets.pop_back();
    }

    void openTag(std::string_view tagName) {
        tag = tagName;
        fontStack.push_back(fontStack.back());

        if (tagName == "b"sv || tagName == "strong"sv) {
            fontStack.back().font.weight = FontWeight::Bold;
            fontStack.back().flags |= FontFormatFlags::Weight;
        } else if (tagName == "i"sv || tagName == "em"sv) {
            fontStack.back().font.style = FontStyle::Italic;
            fontStack.back().flags |= FontFormatFlags::Style;
        } else if (tagName == "big"sv) {
            if (fontStack.back().flags && FontFormatFlags::Size) {
                fontStack.back().font.fontSize *= 2.f;
            } else {
                fontStack.back().font.fontSize = 2.f;
            }
            fontStack.back().flags |= FontFormatFlags::Size;
            fontStack.back().flags |= FontFormatFlags::SizeIsRelative;
        } else if (tagName == "small"sv) {
            if (fontStack.back().flags && FontFormatFlags::Size) {
                fontStack.back().font.fontSize *= 0.5f;
            } else {
                fontStack.back().font.fontSize = 0.5f;
            }
            fontStack.back().flags |= FontFormatFlags::Size;
            fontStack.back().flags |= FontFormatFlags::SizeIsRelative;
        } else if (tagName == "s"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::LineThrough;
            fontStack.back().flags |= FontFormatFlags::TextDecoration;
        } else if (tagName == "u"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::Underline;
            fontStack.back().flags |= FontFormatFlags::TextDecoration;
        } else if (tagName == "br"sv) {
            emitText(U"\n");
        } else if (tagName == "code"sv || tagName == "kbd"sv) {
            fontStack.back().font.fontFamily = Font::Monospace;
            fontStack.back().flags |= FontFormatFlags::Family;
        }
    }

    void closeTag() {
        fontStack.pop_back();
    }

    void attrName(std::string_view name) {
        attr = name;
    }

    void attrValueFragment(std::string_view value) {
        attrValue += value;
    }

    void attrFinished() {
        if (tag == "font" && attr == "color") {
            fontStack.back().color = parseHtmlColor(attrValue);
        }
        if (tag == "font" && attr == "face") {
            fontStack.back().font.fontFamily = attrValue;
            fontStack.back().flags |= FontFormatFlags::Family;
        }
        if (tag == "font" && attr == "size") {
            float val = strtof(attrValue.c_str(), nullptr);
            if (val != 0)
                fontStack.back().font.fontSize = val;
        }
        attrValue = {};
    }

    void emitText(std::u32string_view str) {
        FontFormatEx newFont = fontStack.back();
        text += str;
        // OPTIMIZE: Avoid comparison
        if (richText.fonts.empty() || newFont != richText.fonts.back()) {
            richText.flags.push_back(newFont.flags);
            richText.fonts.push_back(std::move(newFont));
            richText.offsets.push_back(text.size());
        } else if (!richText.offsets.empty()) {
            richText.offsets.back() = text.size();
        }
    }

    void textFragment(std::string_view text) {
        emitText(utf8ToUtf32(text));
    }
};

std::optional<std::pair<std::u32string, RichText>> RichText::fromHtml(std::string_view html) {
    Visitor visitor;
    if (parseHtml(html, &visitor)) {
        return std::pair{ std::move(visitor.text), std::move(visitor.richText) };
    } else {
        return std::nullopt;
    }
}
} // namespace Internal

std::recursive_mutex fontMutex;

std::optional<FontManager> fonts(std::in_place, &fontMutex, 3, 5000);

} // namespace Brisk
