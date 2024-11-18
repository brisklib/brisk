#pragma once

#include <cstdint>
#include <fmt/format.h>

namespace Brisk {

/**
 * @enum OpenTypeFeature
 * @brief Enumerates various OpenType font features.
 *
 * @note Use openTypeFeatures to map to uint32_t.
 */
enum class OpenTypeFeature : uint8_t {
    aalt,  /**< Access All Alternates */
    abvf,  /**< Above-base Forms */
    abvm,  /**< Above-base Mark Positioning */
    abvs,  /**< Above-base Substitutions */
    afrc,  /**< Alternative Fractions */
    akhn,  /**< Akhand - Forms a conjunct */
    blwf,  /**< Below-base Forms */
    blwm,  /**< Below-base Mark Positioning */
    blws,  /**< Below-base Substitutions */
    calt,  /**< Contextual Alternates */
    case_, /**< Case-Sensitive Forms */
    ccmp,  /**< Glyph Composition/Decomposition */
    cfar,  /**< Conjunct Form After Ro */
    chws,  /**< Contextual Half-width Spacing */
    cjct,  /**< Conjunct Forms */
    clig,  /**< Contextual Ligatures */
    cpct,  /**< Centered CJK Punctuation */
    cpsp,  /**< Capital Spacing */
    cswh,  /**< Contextual Swash */
    curs,  /**< Cursive Positioning */
    cv01,  /**< Character Variants (1) */
    cv02,  /**< Character Variants (2) */
#ifndef DOCUMENTATION
    cv03,
    cv04,
    cv05,
    cv06,
    cv07,
    cv08,
    cv09,
    cv10,
    cv11,
    cv12,
    cv13,
    cv14,
    cv15,
    cv16,
    cv17,
    cv18,
    cv19,
    cv20,
    cv21,
    cv22,
    cv23,
    cv24,
    cv25,
    cv26,
    cv27,
    cv28,
    cv29,
    cv30,
    cv31,
    cv32,
    cv33,
    cv34,
    cv35,
    cv36,
    cv37,
    cv38,
    cv39,
    cv40,
    cv41,
    cv42,
    cv43,
    cv44,
    cv45,
    cv46,
    cv47,
    cv48,
    cv49,
    cv50,
    cv51,
    cv52,
    cv53,
    cv54,
    cv55,
    cv56,
    cv57,
    cv58,
    cv59,
    cv60,
    cv61,
    cv62,
    cv63,
    cv64,
    cv65,
    cv66,
    cv67,
    cv68,
    cv69,
    cv70,
    cv71,
    cv72,
    cv73,
    cv74,
    cv75,
    cv76,
    cv77,
    cv78,
    cv79,
    cv80,
    cv81,
    cv82,
    cv83,
    cv84,
    cv85,
    cv86,
    cv87,
    cv88,
    cv89,
    cv90,
    cv91,
    cv92,
    cv93,
    cv94,
    cv95,
    cv96,
    cv97,
    cv98,
    cv99, /**< Character Variants (99) */
#endif
    c2pc, /**< Petite Capitals From Capitals */
    c2sc, /**< Small Capitals From Capitals */
    dist, /**< Distances */
    dlig, /**< Discretionary Ligatures */
    dnom, /**< Denominators */
    dtls, /**< Dotless Forms */
    expt, /**< Expert Forms */
    falt, /**< Final Alternates */
    fin2, /**< Terminal Forms #2 */
    fin3, /**< Terminal Forms #3 */
    fina, /**< Terminal Forms */
    flac, /**< Flattened accent forms */
    frac, /**< Fractions */
    fwid, /**< Full Width */
    half, /**< Half Forms */
    haln, /**< Halant Forms */
    halt, /**< Alternate Half Width */
    hist, /**< Historical Forms */
    hkna, /**< Horizontal Kana Alternates */
    hlig, /**< Historical Ligatures */
    hngl, /**< Hangul */
    hojo, /**< Hojo Kanji Forms */
    hwid, /**< Half Width */
    init, /**< Initial Forms */
    isol, /**< Isolated Forms */
    ital, /**< Italics */
    jalt, /**< Justification Alternates */
    jp78, /**< Japanese Forms 1978 */
    jp83, /**< Japanese Forms 1983 */
    jp90, /**< Japanese Forms 1990 */
    jp04, /**< Japanese Forms 2004 */
    kern, /**< Kerning */
    lfbd, /**< Left Bounds */
    liga, /**< Standard Ligatures */
    ljmo, /**< Leading Jamo Forms */
    lnum, /**< Lining Figures */
    locl, /**< Localized Forms */
    ltra, /**< Left-to-Right Alternates */
    ltrm, /**< Left-to-Right Mirrored Forms */
    mark, /**< Mark Positioning */
    med2, /**< Medial Forms #2 */
    medi, /**< Medial Forms */
    mgrk, /**< Mathematical Greek */
    mkmk, /**< Mark-to-Mark Positioning */
    mset, /**< Mark Positioning via Substitution */
    nalt, /**< Alternate Annotation Forms */
    nlck, /**< NLC Kanji Forms */
    nukt, /**< Nukta Forms */
    numr, /**< Numerators */
    onum, /**< Oldstyle Figures */
    opbd, /**< Optical Bounds */
    ordn, /**< Ordinals */
    ornm, /**< Ornaments */
    palt, /**< Proportional Alternate Width */
    pcap, /**< Petite Capitals */
    pkna, /**< Proportional Kana */
    pnum, /**< Proportional Figures */
    pref, /**< Pre-Base Forms */
    pres, /**< Pre-Below Substitutions */
    pstf, /**< Post-Base Forms */
    psts, /**< Post-Below Substitutions */
    pwid, /**< Proportional Widths */
    qwid, /**< Quarter Widths */
    rand, /**< Randomize */
    rclt, /**< Required Contextual Alternates */
    rkrf, /**< Rakar Forms */
    rlig, /**< Required Ligatures */
    rphf, /**< Reph Forms */
    rtbd, /**< Right Bounds */
    rtla, /**< Right-to-Left Alternates */
    rtlm, /**< Right-to-Left Mirrored Forms */
    ruby, /**< Ruby Notation Forms */
    rvrn, /**< Required Variation Alternates */
    salt, /**< Stylistic Alternates */
    sinf, /**< Scientific Inferiors */
    size, /**< Optical Size */
    smcp, /**< Small Capitals */
    smpl, /**< Simplified Forms */
    ss01, /**< Stylistic Set 1 */
    ss02, /**< Stylistic Set 2 */
    ss03, /**< Stylistic Set 3 */
#ifndef DOCUMENTATION
    ss04, /**< Stylistic Set 4 */
    ss05, /**< Stylistic Set 5 */
    ss06, /**< Stylistic Set 6 */
    ss07, /**< Stylistic Set 7 */
    ss08, /**< Stylistic Set 8 */
    ss09, /**< Stylistic Set 9 */
    ss10, /**< Stylistic Set 10 */
    ss11, /**< Stylistic Set 11 */
    ss12, /**< Stylistic Set 12 */
    ss13, /**< Stylistic Set 13 */
    ss14, /**< Stylistic Set 14 */
    ss15, /**< Stylistic Set 15 */
    ss16, /**< Stylistic Set 16 */
    ss17, /**< Stylistic Set 17 */
    ss18, /**< Stylistic Set 18 */
    ss19, /**< Stylistic Set 19 */
    ss20, /**< Stylistic Set 20 */
#endif
    ssty, /**< Script Style */
    stch, /**< Stretching Glyph Deformation */
    subs, /**< Subscript */
    sups, /**< Superscript */
    swsh, /**< Swash */
    titl, /**< Titling Alternates */
    tjmo, /**< Trailing Jamo Forms */
    tnam, /**< Traditional Name Forms */
    tnum, /**< Tabular Figures */
    trad, /**< Traditional Forms */
    twid, /**< Third Widths */
    unic, /**< Unicase */
    valt, /**< Alternate Vertical Metrics */
    vatu, /**< Vattu Variants */
    vchw, /**< Vertical Counterparts to Half-width */
    vert, /**< Vertical Writing */
    vhal, /**< Vertical Alternate Half-width */
    vjmo, /**< Vertical Jamo Forms */
    vkna, /**< Vertical Kana Alternates */
    vkrn, /**< Vertical Kerning */
    vpal, /**< Proportional Alternate Vertical Metrics */
    vrt2, /**< Vertical Alternates and Rotation */
    vrtr, /**< Vertical Alternates for Rotation */
    zero  /**< Slashed Zero */
};

extern const uint32_t openTypeFeatures[241];

std::string openTypeFeatureToString(OpenTypeFeature feat);

} // namespace Brisk

template <typename Char>
struct fmt::formatter<Brisk::OpenTypeFeature, Char> : fmt::formatter<std::basic_string<Char>, Char> {
    template <typename FormatContext>
    auto format(const Brisk::OpenTypeFeature& val, FormatContext& ctx) const {
        return formatter<std::basic_string<Char>, Char>::format(Brisk::openTypeFeatureToString(val), ctx);
    }
};
