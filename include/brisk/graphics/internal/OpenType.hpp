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
 */                                                                                                          \
#pragma once

#include <cstdint>

#include <fmt/format.h>

namespace Brisk {

/**
 * @brief Packs four characters into a 32-bit OpenType feature tag,
 * similar to HarfBuzz's @c HB_TAG macro.
 */
constexpr uint32_t otTag(char c1, char c2, char c3, char c4) {
    return ((uint32_t(c1) & 0xFF) << 24) | ((uint32_t(c2) & 0xFF) << 16) | ((uint32_t(c3) & 0xFF) << 8) |
           (uint32_t(c4) & 0xFF);
}

/**
 * @enum OpenTypeFeature
 * @brief Enumerates various OpenType font features.
 *
 * Each enumerator holds the corresponding 4-byte OpenType feature tag
 * (as produced by otTag), so it can be passed directly to shaping APIs.
 */
enum class OpenTypeFeature : uint32_t {
    aalt  = otTag('a', 'a', 'l', 't'), /**< Access All Alternates */
    abvf  = otTag('a', 'b', 'v', 'f'), /**< Above-base Forms */
    abvm  = otTag('a', 'b', 'v', 'm'), /**< Above-base Mark Positioning */
    abvs  = otTag('a', 'b', 'v', 's'), /**< Above-base Substitutions */
    afrc  = otTag('a', 'f', 'r', 'c'), /**< Alternative Fractions */
    akhn  = otTag('a', 'k', 'h', 'n'), /**< Akhand - Forms a conjunct */
    blwf  = otTag('b', 'l', 'w', 'f'), /**< Below-base Forms */
    blwm  = otTag('b', 'l', 'w', 'm'), /**< Below-base Mark Positioning */
    blws  = otTag('b', 'l', 'w', 's'), /**< Below-base Substitutions */
    calt  = otTag('c', 'a', 'l', 't'), /**< Contextual Alternates */
    case_ = otTag('c', 'a', 's', 'e'), /**< Case-Sensitive Forms */
    ccmp  = otTag('c', 'c', 'm', 'p'), /**< Glyph Composition/Decomposition */
    cfar  = otTag('c', 'f', 'a', 'r'), /**< Conjunct Form After Ro */
    chws  = otTag('c', 'h', 'w', 's'), /**< Contextual Half-width Spacing */
    cjct  = otTag('c', 'j', 'c', 't'), /**< Conjunct Forms */
    clig  = otTag('c', 'l', 'i', 'g'), /**< Contextual Ligatures */
    cpct  = otTag('c', 'p', 'c', 't'), /**< Centered CJK Punctuation */
    cpsp  = otTag('c', 'p', 's', 'p'), /**< Capital Spacing */
    cswh  = otTag('c', 's', 'w', 'h'), /**< Contextual Swash */
    curs  = otTag('c', 'u', 'r', 's'), /**< Cursive Positioning */
    cv01  = otTag('c', 'v', '0', '1'), /**< Character Variants (1) */
    cv02  = otTag('c', 'v', '0', '2'), /**< Character Variants (2) */
#ifndef DOCUMENTATION
    cv03 = otTag('c', 'v', '0', '3'),
    cv04 = otTag('c', 'v', '0', '4'),
    cv05 = otTag('c', 'v', '0', '5'),
    cv06 = otTag('c', 'v', '0', '6'),
    cv07 = otTag('c', 'v', '0', '7'),
    cv08 = otTag('c', 'v', '0', '8'),
    cv09 = otTag('c', 'v', '0', '9'),
    cv10 = otTag('c', 'v', '1', '0'),
    cv11 = otTag('c', 'v', '1', '1'),
    cv12 = otTag('c', 'v', '1', '2'),
    cv13 = otTag('c', 'v', '1', '3'),
    cv14 = otTag('c', 'v', '1', '4'),
    cv15 = otTag('c', 'v', '1', '5'),
    cv16 = otTag('c', 'v', '1', '6'),
    cv17 = otTag('c', 'v', '1', '7'),
    cv18 = otTag('c', 'v', '1', '8'),
    cv19 = otTag('c', 'v', '1', '9'),
    cv20 = otTag('c', 'v', '2', '0'),
    cv21 = otTag('c', 'v', '2', '1'),
    cv22 = otTag('c', 'v', '2', '2'),
    cv23 = otTag('c', 'v', '2', '3'),
    cv24 = otTag('c', 'v', '2', '4'),
    cv25 = otTag('c', 'v', '2', '5'),
    cv26 = otTag('c', 'v', '2', '6'),
    cv27 = otTag('c', 'v', '2', '7'),
    cv28 = otTag('c', 'v', '2', '8'),
    cv29 = otTag('c', 'v', '2', '9'),
    cv30 = otTag('c', 'v', '3', '0'),
    cv31 = otTag('c', 'v', '3', '1'),
    cv32 = otTag('c', 'v', '3', '2'),
    cv33 = otTag('c', 'v', '3', '3'),
    cv34 = otTag('c', 'v', '3', '4'),
    cv35 = otTag('c', 'v', '3', '5'),
    cv36 = otTag('c', 'v', '3', '6'),
    cv37 = otTag('c', 'v', '3', '7'),
    cv38 = otTag('c', 'v', '3', '8'),
    cv39 = otTag('c', 'v', '3', '9'),
    cv40 = otTag('c', 'v', '4', '0'),
    cv41 = otTag('c', 'v', '4', '1'),
    cv42 = otTag('c', 'v', '4', '2'),
    cv43 = otTag('c', 'v', '4', '3'),
    cv44 = otTag('c', 'v', '4', '4'),
    cv45 = otTag('c', 'v', '4', '5'),
    cv46 = otTag('c', 'v', '4', '6'),
    cv47 = otTag('c', 'v', '4', '7'),
    cv48 = otTag('c', 'v', '4', '8'),
    cv49 = otTag('c', 'v', '4', '9'),
    cv50 = otTag('c', 'v', '5', '0'),
    cv51 = otTag('c', 'v', '5', '1'),
    cv52 = otTag('c', 'v', '5', '2'),
    cv53 = otTag('c', 'v', '5', '3'),
    cv54 = otTag('c', 'v', '5', '4'),
    cv55 = otTag('c', 'v', '5', '5'),
    cv56 = otTag('c', 'v', '5', '6'),
    cv57 = otTag('c', 'v', '5', '7'),
    cv58 = otTag('c', 'v', '5', '8'),
    cv59 = otTag('c', 'v', '5', '9'),
    cv60 = otTag('c', 'v', '6', '0'),
    cv61 = otTag('c', 'v', '6', '1'),
    cv62 = otTag('c', 'v', '6', '2'),
    cv63 = otTag('c', 'v', '6', '3'),
    cv64 = otTag('c', 'v', '6', '4'),
    cv65 = otTag('c', 'v', '6', '5'),
    cv66 = otTag('c', 'v', '6', '6'),
    cv67 = otTag('c', 'v', '6', '7'),
    cv68 = otTag('c', 'v', '6', '8'),
    cv69 = otTag('c', 'v', '6', '9'),
    cv70 = otTag('c', 'v', '7', '0'),
    cv71 = otTag('c', 'v', '7', '1'),
    cv72 = otTag('c', 'v', '7', '2'),
    cv73 = otTag('c', 'v', '7', '3'),
    cv74 = otTag('c', 'v', '7', '4'),
    cv75 = otTag('c', 'v', '7', '5'),
    cv76 = otTag('c', 'v', '7', '6'),
    cv77 = otTag('c', 'v', '7', '7'),
    cv78 = otTag('c', 'v', '7', '8'),
    cv79 = otTag('c', 'v', '7', '9'),
    cv80 = otTag('c', 'v', '8', '0'),
    cv81 = otTag('c', 'v', '8', '1'),
    cv82 = otTag('c', 'v', '8', '2'),
    cv83 = otTag('c', 'v', '8', '3'),
    cv84 = otTag('c', 'v', '8', '4'),
    cv85 = otTag('c', 'v', '8', '5'),
    cv86 = otTag('c', 'v', '8', '6'),
    cv87 = otTag('c', 'v', '8', '7'),
    cv88 = otTag('c', 'v', '8', '8'),
    cv89 = otTag('c', 'v', '8', '9'),
    cv90 = otTag('c', 'v', '9', '0'),
    cv91 = otTag('c', 'v', '9', '1'),
    cv92 = otTag('c', 'v', '9', '2'),
    cv93 = otTag('c', 'v', '9', '3'),
    cv94 = otTag('c', 'v', '9', '4'),
    cv95 = otTag('c', 'v', '9', '5'),
    cv96 = otTag('c', 'v', '9', '6'),
    cv97 = otTag('c', 'v', '9', '7'),
    cv98 = otTag('c', 'v', '9', '8'),
    cv99 = otTag('c', 'v', '9', '9'), /**< Character Variants (99) */
#endif
    c2pc = otTag('c', '2', 'p', 'c'), /**< Petite Capitals From Capitals */
    c2sc = otTag('c', '2', 's', 'c'), /**< Small Capitals From Capitals */
    dist = otTag('d', 'i', 's', 't'), /**< Distances */
    dlig = otTag('d', 'l', 'i', 'g'), /**< Discretionary Ligatures */
    dnom = otTag('d', 'n', 'o', 'm'), /**< Denominators */
    dtls = otTag('d', 't', 'l', 's'), /**< Dotless Forms */
    expt = otTag('e', 'x', 'p', 't'), /**< Expert Forms */
    falt = otTag('f', 'a', 'l', 't'), /**< Final Alternates */
    fin2 = otTag('f', 'i', 'n', '2'), /**< Terminal Forms #2 */
    fin3 = otTag('f', 'i', 'n', '3'), /**< Terminal Forms #3 */
    fina = otTag('f', 'i', 'n', 'a'), /**< Terminal Forms */
    flac = otTag('f', 'l', 'a', 'c'), /**< Flattened accent forms */
    frac = otTag('f', 'r', 'a', 'c'), /**< Fractions */
    fwid = otTag('f', 'w', 'i', 'd'), /**< Full Width */
    half = otTag('h', 'a', 'l', 'f'), /**< Half Forms */
    haln = otTag('h', 'a', 'l', 'n'), /**< Halant Forms */
    halt = otTag('h', 'a', 'l', 't'), /**< Alternate Half Width */
    hist = otTag('h', 'i', 's', 't'), /**< Historical Forms */
    hkna = otTag('h', 'k', 'n', 'a'), /**< Horizontal Kana Alternates */
    hlig = otTag('h', 'l', 'i', 'g'), /**< Historical Ligatures */
    hngl = otTag('h', 'n', 'g', 'l'), /**< Hangul */
    hojo = otTag('h', 'o', 'j', 'o'), /**< Hojo Kanji Forms */
    hwid = otTag('h', 'w', 'i', 'd'), /**< Half Width */
    init = otTag('i', 'n', 'i', 't'), /**< Initial Forms */
    isol = otTag('i', 's', 'o', 'l'), /**< Isolated Forms */
    ital = otTag('i', 't', 'a', 'l'), /**< Italics */
    jalt = otTag('j', 'a', 'l', 't'), /**< Justification Alternates */
    jp78 = otTag('j', 'p', '7', '8'), /**< Japanese Forms 1978 */
    jp83 = otTag('j', 'p', '8', '3'), /**< Japanese Forms 1983 */
    jp90 = otTag('j', 'p', '9', '0'), /**< Japanese Forms 1990 */
    jp04 = otTag('j', 'p', '0', '4'), /**< Japanese Forms 2004 */
    kern = otTag('k', 'e', 'r', 'n'), /**< Kerning */
    lfbd = otTag('l', 'f', 'b', 'd'), /**< Left Bounds */
    liga = otTag('l', 'i', 'g', 'a'), /**< Standard Ligatures */
    ljmo = otTag('l', 'j', 'm', 'o'), /**< Leading Jamo Forms */
    lnum = otTag('l', 'n', 'u', 'm'), /**< Lining Figures */
    locl = otTag('l', 'o', 'c', 'l'), /**< Localized Forms */
    ltra = otTag('l', 't', 'r', 'a'), /**< Left-to-Right Alternates */
    ltrm = otTag('l', 't', 'r', 'm'), /**< Left-to-Right Mirrored Forms */
    mark = otTag('m', 'a', 'r', 'k'), /**< Mark Positioning */
    med2 = otTag('m', 'e', 'd', '2'), /**< Medial Forms #2 */
    medi = otTag('m', 'e', 'd', 'i'), /**< Medial Forms */
    mgrk = otTag('m', 'g', 'r', 'k'), /**< Mathematical Greek */
    mkmk = otTag('m', 'k', 'm', 'k'), /**< Mark-to-Mark Positioning */
    mset = otTag('m', 's', 'e', 't'), /**< Mark Positioning via Substitution */
    nalt = otTag('n', 'a', 'l', 't'), /**< Alternate Annotation Forms */
    nlck = otTag('n', 'l', 'c', 'k'), /**< NLC Kanji Forms */
    nukt = otTag('n', 'u', 'k', 't'), /**< Nukta Forms */
    numr = otTag('n', 'u', 'm', 'r'), /**< Numerators */
    onum = otTag('o', 'n', 'u', 'm'), /**< Oldstyle Figures */
    opbd = otTag('o', 'p', 'b', 'd'), /**< Optical Bounds */
    ordn = otTag('o', 'r', 'd', 'n'), /**< Ordinals */
    ornm = otTag('o', 'r', 'n', 'm'), /**< Ornaments */
    palt = otTag('p', 'a', 'l', 't'), /**< Proportional Alternate Width */
    pcap = otTag('p', 'c', 'a', 'p'), /**< Petite Capitals */
    pkna = otTag('p', 'k', 'n', 'a'), /**< Proportional Kana */
    pnum = otTag('p', 'n', 'u', 'm'), /**< Proportional Figures */
    pref = otTag('p', 'r', 'e', 'f'), /**< Pre-Base Forms */
    pres = otTag('p', 'r', 'e', 's'), /**< Pre-Below Substitutions */
    pstf = otTag('p', 's', 't', 'f'), /**< Post-Base Forms */
    psts = otTag('p', 's', 't', 's'), /**< Post-Below Substitutions */
    pwid = otTag('p', 'w', 'i', 'd'), /**< Proportional Widths */
    qwid = otTag('q', 'w', 'i', 'd'), /**< Quarter Widths */
    rand = otTag('r', 'a', 'n', 'd'), /**< Randomize */
    rclt = otTag('r', 'c', 'l', 't'), /**< Required Contextual Alternates */
    rkrf = otTag('r', 'k', 'r', 'f'), /**< Rakar Forms */
    rlig = otTag('r', 'l', 'i', 'g'), /**< Required Ligatures */
    rphf = otTag('r', 'p', 'h', 'f'), /**< Reph Forms */
    rtbd = otTag('r', 't', 'b', 'd'), /**< Right Bounds */
    rtla = otTag('r', 't', 'l', 'a'), /**< Right-to-Left Alternates */
    rtlm = otTag('r', 't', 'l', 'm'), /**< Right-to-Left Mirrored Forms */
    ruby = otTag('r', 'u', 'b', 'y'), /**< Ruby Notation Forms */
    rvrn = otTag('r', 'v', 'r', 'n'), /**< Required Variation Alternates */
    salt = otTag('s', 'a', 'l', 't'), /**< Stylistic Alternates */
    sinf = otTag('s', 'i', 'n', 'f'), /**< Scientific Inferiors */
    size = otTag('s', 'i', 'z', 'e'), /**< Optical Size */
    smcp = otTag('s', 'm', 'c', 'p'), /**< Small Capitals */
    smpl = otTag('s', 'm', 'p', 'l'), /**< Simplified Forms */
    ss01 = otTag('s', 's', '0', '1'), /**< Stylistic Set 1 */
    ss02 = otTag('s', 's', '0', '2'), /**< Stylistic Set 2 */
    ss03 = otTag('s', 's', '0', '3'), /**< Stylistic Set 3 */
#ifndef DOCUMENTATION
    ss04 = otTag('s', 's', '0', '4'), /**< Stylistic Set 4 */
    ss05 = otTag('s', 's', '0', '5'), /**< Stylistic Set 5 */
    ss06 = otTag('s', 's', '0', '6'), /**< Stylistic Set 6 */
    ss07 = otTag('s', 's', '0', '7'), /**< Stylistic Set 7 */
    ss08 = otTag('s', 's', '0', '8'), /**< Stylistic Set 8 */
    ss09 = otTag('s', 's', '0', '9'), /**< Stylistic Set 9 */
    ss10 = otTag('s', 's', '1', '0'), /**< Stylistic Set 10 */
    ss11 = otTag('s', 's', '1', '1'), /**< Stylistic Set 11 */
    ss12 = otTag('s', 's', '1', '2'), /**< Stylistic Set 12 */
    ss13 = otTag('s', 's', '1', '3'), /**< Stylistic Set 13 */
    ss14 = otTag('s', 's', '1', '4'), /**< Stylistic Set 14 */
    ss15 = otTag('s', 's', '1', '5'), /**< Stylistic Set 15 */
    ss16 = otTag('s', 's', '1', '6'), /**< Stylistic Set 16 */
    ss17 = otTag('s', 's', '1', '7'), /**< Stylistic Set 17 */
    ss18 = otTag('s', 's', '1', '8'), /**< Stylistic Set 18 */
    ss19 = otTag('s', 's', '1', '9'), /**< Stylistic Set 19 */
    ss20 = otTag('s', 's', '2', '0'), /**< Stylistic Set 20 */
#endif
    ssty = otTag('s', 's', 't', 'y'), /**< Script Style */
    stch = otTag('s', 't', 'c', 'h'), /**< Stretching Glyph Deformation */
    subs = otTag('s', 'u', 'b', 's'), /**< Subscript */
    sups = otTag('s', 'u', 'p', 's'), /**< Superscript */
    swsh = otTag('s', 'w', 's', 'h'), /**< Swash */
    titl = otTag('t', 'i', 't', 'l'), /**< Titling Alternates */
    tjmo = otTag('t', 'j', 'm', 'o'), /**< Trailing Jamo Forms */
    tnam = otTag('t', 'n', 'a', 'm'), /**< Traditional Name Forms */
    tnum = otTag('t', 'n', 'u', 'm'), /**< Tabular Figures */
    trad = otTag('t', 'r', 'a', 'd'), /**< Traditional Forms */
    twid = otTag('t', 'w', 'i', 'd'), /**< Third Widths */
    unic = otTag('u', 'n', 'i', 'c'), /**< Unicase */
    valt = otTag('v', 'a', 'l', 't'), /**< Alternate Vertical Metrics */
    vatu = otTag('v', 'a', 't', 'u'), /**< Vattu Variants */
    vchw = otTag('v', 'c', 'h', 'w'), /**< Vertical Counterparts to Half-width */
    vert = otTag('v', 'e', 'r', 't'), /**< Vertical Writing */
    vhal = otTag('v', 'h', 'a', 'l'), /**< Vertical Alternate Half-width */
    vjmo = otTag('v', 'j', 'm', 'o'), /**< Vertical Jamo Forms */
    vkna = otTag('v', 'k', 'n', 'a'), /**< Vertical Kana Alternates */
    vkrn = otTag('v', 'k', 'r', 'n'), /**< Vertical Kerning */
    vpal = otTag('v', 'p', 'a', 'l'), /**< Proportional Alternate Vertical Metrics */
    vrt2 = otTag('v', 'r', 't', '2'), /**< Vertical Alternates and Rotation */
    vrtr = otTag('v', 'r', 't', 'r'), /**< Vertical Alternates for Rotation */
    zero = otTag('z', 'e', 'r', 'o')  /**< Slashed Zero */
};

std::string openTypeFeatureToString(OpenTypeFeature feat);

} // namespace Brisk

template <typename Char>
struct fmt::formatter<Brisk::OpenTypeFeature, Char> : fmt::formatter<std::basic_string<Char>, Char> {
    template <typename FormatContext>
    auto format(const Brisk::OpenTypeFeature& val, FormatContext& ctx) const {
        return formatter<std::basic_string<Char>, Char>::format(Brisk::openTypeFeatureToString(val), ctx);
    }
};
