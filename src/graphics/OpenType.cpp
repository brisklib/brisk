/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2024 Brisk Developers
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
#include <brisk/graphics/internal/OpenType.hpp>
#include <hb.h>

namespace Brisk {

std::string openTypeFeatureToString(OpenTypeFeature feat) {
    uint32_t val =
        static_cast<uint32_t>(openTypeFeatures[static_cast<std::underlying_type_t<OpenTypeFeature>>(feat)]);
    char tag[4]{
        static_cast<char>((val >> 24) & 0xFF),
        static_cast<char>((val >> 16) & 0xFF),
        static_cast<char>((val >> 8) & 0xFF),
        static_cast<char>((val >> 0) & 0xFF),
    };
    return std::string(tag, 4);
}

constexpr hb_tag_t hb_tag(const char (&str)[5]) noexcept {
    return HB_TAG(str[0], str[1], str[2], str[3]);
}

const uint32_t openTypeFeatures[241] = {
    hb_tag("aalt"), hb_tag("abvf"), hb_tag("abvm"), hb_tag("abvs"), hb_tag("afrc"), hb_tag("akhn"),
    hb_tag("blwf"), hb_tag("blwm"), hb_tag("blws"), hb_tag("calt"), hb_tag("case"), hb_tag("ccmp"),
    hb_tag("cfar"), hb_tag("chws"), hb_tag("cjct"), hb_tag("clig"), hb_tag("cpct"), hb_tag("cpsp"),
    hb_tag("cswh"), hb_tag("curs"), hb_tag("cv01"), hb_tag("cv02"), hb_tag("cv03"), hb_tag("cv04"),
    hb_tag("cv05"), hb_tag("cv06"), hb_tag("cv07"), hb_tag("cv08"), hb_tag("cv09"), hb_tag("cv10"),
    hb_tag("cv11"), hb_tag("cv12"), hb_tag("cv13"), hb_tag("cv14"), hb_tag("cv15"), hb_tag("cv16"),
    hb_tag("cv17"), hb_tag("cv18"), hb_tag("cv19"), hb_tag("cv20"), hb_tag("cv21"), hb_tag("cv22"),
    hb_tag("cv23"), hb_tag("cv24"), hb_tag("cv25"), hb_tag("cv26"), hb_tag("cv27"), hb_tag("cv28"),
    hb_tag("cv29"), hb_tag("cv30"), hb_tag("cv31"), hb_tag("cv32"), hb_tag("cv33"), hb_tag("cv34"),
    hb_tag("cv35"), hb_tag("cv36"), hb_tag("cv37"), hb_tag("cv38"), hb_tag("cv39"), hb_tag("cv40"),
    hb_tag("cv41"), hb_tag("cv42"), hb_tag("cv43"), hb_tag("cv44"), hb_tag("cv45"), hb_tag("cv46"),
    hb_tag("cv47"), hb_tag("cv48"), hb_tag("cv49"), hb_tag("cv50"), hb_tag("cv51"), hb_tag("cv52"),
    hb_tag("cv53"), hb_tag("cv54"), hb_tag("cv55"), hb_tag("cv56"), hb_tag("cv57"), hb_tag("cv58"),
    hb_tag("cv59"), hb_tag("cv60"), hb_tag("cv61"), hb_tag("cv62"), hb_tag("cv63"), hb_tag("cv64"),
    hb_tag("cv65"), hb_tag("cv66"), hb_tag("cv67"), hb_tag("cv68"), hb_tag("cv69"), hb_tag("cv70"),
    hb_tag("cv71"), hb_tag("cv72"), hb_tag("cv73"), hb_tag("cv74"), hb_tag("cv75"), hb_tag("cv76"),
    hb_tag("cv77"), hb_tag("cv78"), hb_tag("cv79"), hb_tag("cv80"), hb_tag("cv81"), hb_tag("cv82"),
    hb_tag("cv83"), hb_tag("cv84"), hb_tag("cv85"), hb_tag("cv86"), hb_tag("cv87"), hb_tag("cv88"),
    hb_tag("cv89"), hb_tag("cv90"), hb_tag("cv91"), hb_tag("cv92"), hb_tag("cv93"), hb_tag("cv94"),
    hb_tag("cv95"), hb_tag("cv96"), hb_tag("cv97"), hb_tag("cv98"), hb_tag("cv99"), hb_tag("c2pc"),
    hb_tag("c2sc"), hb_tag("dist"), hb_tag("dlig"), hb_tag("dnom"), hb_tag("dtls"), hb_tag("expt"),
    hb_tag("falt"), hb_tag("fin2"), hb_tag("fin3"), hb_tag("fina"), hb_tag("flac"), hb_tag("frac"),
    hb_tag("fwid"), hb_tag("half"), hb_tag("haln"), hb_tag("halt"), hb_tag("hist"), hb_tag("hkna"),
    hb_tag("hlig"), hb_tag("hngl"), hb_tag("hojo"), hb_tag("hwid"), hb_tag("init"), hb_tag("isol"),
    hb_tag("ital"), hb_tag("jalt"), hb_tag("jp78"), hb_tag("jp83"), hb_tag("jp90"), hb_tag("jp04"),
    hb_tag("kern"), hb_tag("lfbd"), hb_tag("liga"), hb_tag("ljmo"), hb_tag("lnum"), hb_tag("locl"),
    hb_tag("ltra"), hb_tag("ltrm"), hb_tag("mark"), hb_tag("med2"), hb_tag("medi"), hb_tag("mgrk"),
    hb_tag("mkmk"), hb_tag("mset"), hb_tag("nalt"), hb_tag("nlck"), hb_tag("nukt"), hb_tag("numr"),
    hb_tag("onum"), hb_tag("opbd"), hb_tag("ordn"), hb_tag("ornm"), hb_tag("palt"), hb_tag("pcap"),
    hb_tag("pkna"), hb_tag("pnum"), hb_tag("pref"), hb_tag("pres"), hb_tag("pstf"), hb_tag("psts"),
    hb_tag("pwid"), hb_tag("qwid"), hb_tag("rand"), hb_tag("rclt"), hb_tag("rkrf"), hb_tag("rlig"),
    hb_tag("rphf"), hb_tag("rtbd"), hb_tag("rtla"), hb_tag("rtlm"), hb_tag("ruby"), hb_tag("rvrn"),
    hb_tag("salt"), hb_tag("sinf"), hb_tag("size"), hb_tag("smcp"), hb_tag("smpl"), hb_tag("ss01"),
    hb_tag("ss02"), hb_tag("ss03"), hb_tag("ss04"), hb_tag("ss05"), hb_tag("ss06"), hb_tag("ss07"),
    hb_tag("ss08"), hb_tag("ss09"), hb_tag("ss10"), hb_tag("ss11"), hb_tag("ss12"), hb_tag("ss13"),
    hb_tag("ss14"), hb_tag("ss15"), hb_tag("ss16"), hb_tag("ss17"), hb_tag("ss18"), hb_tag("ss19"),
    hb_tag("ss20"), hb_tag("ssty"), hb_tag("stch"), hb_tag("subs"), hb_tag("sups"), hb_tag("swsh"),
    hb_tag("titl"), hb_tag("tjmo"), hb_tag("tnam"), hb_tag("tnum"), hb_tag("trad"), hb_tag("twid"),
    hb_tag("unic"), hb_tag("valt"), hb_tag("vatu"), hb_tag("vchw"), hb_tag("vert"), hb_tag("vhal"),
    hb_tag("vjmo"), hb_tag("vkna"), hb_tag("vkrn"), hb_tag("vpal"), hb_tag("vrt2"), hb_tag("vrtr"),
    hb_tag("zero"),
};
} // namespace Brisk
