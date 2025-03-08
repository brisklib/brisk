#include <brisk/graphics/Html.hpp>
#include <brisk/core/Encoding.hpp>
#include <tao/pegtl.hpp>
#include <charconv>

namespace Brisk {

namespace Internal {

namespace {

std::array<std::pair<std::string_view, std::string_view>, 6> charNames{ {
    { "amp", "&" },
    { "apos", "'" },
    { "gt", ">" },
    { "lt", "<" },
    { "nbsp", "\xA0" },
    { "quot", "\"" },
} };

namespace Grammar {

using namespace tao::pegtl;
using tao::pegtl::eof;
using tao::pegtl::one;

struct WS : plus<one<' ', '\t', '\f', '\n', '\r'>> {};

struct OptWS : star<one<' ', '\t', '\f', '\n', '\r'>> {};

struct TagName : plus<ranges<'A', 'Z', 'a', 'z', '0', '9'>> {};

struct OpenTagName : TagName {};

struct CloseTagName : TagName {};

struct CharName : plus<ranges<'A', 'Z', 'a', 'z', '0', '9'>> {};

struct CharCode : plus<digit> {};

struct CharHexCode : plus<xdigit> {};

struct CharRef
    : seq<one<'&'>, sor<CharName, seq<one<'#'>, CharCode>, seq<one<'#'>, one<'x', 'X'>, CharHexCode>>,
          one<';'>> {};

template <char q>
struct PlainAttrValueQuoted : plus<not_one<q, '&', '<', '>'>> {};

template <char q>
struct QuotedContent : star<sor<CharRef, PlainAttrValueQuoted<q>>> {};

template <char q>
struct Quoted : seq<one<q>, QuotedContent<q>, one<q>> {};

struct AttrName : plus<not_one<' ', '\t', '\f', '\n', '\r', '"', '\x27', '>', '/', '=', '&'>> {};

struct PlainAttrValueUnquoted
    : plus<not_one<' ', '\t', '\f', '\n', '\r', '"', '\x27', '<', '>', '/', '=', '`', '&'>> {};

struct Unquoted : plus<sor<CharRef, PlainAttrValueUnquoted>> {};

struct AttrValue : sor<Unquoted, Quoted<'"'>, Quoted<'\''>> {};

struct Attribute : seq<AttrName, OptWS, opt<one<'='>, OptWS, AttrValue>> {};

struct PlainText : plus<not_one<'<', '&'>> {};

struct Text : plus<sor<CharRef, PlainText>> {};

struct Content;

struct SelfCloseTag : one<'>'> {};

struct EndOpenTag : one<'>'> {};

struct Element
    : seq<one<'<'>, OpenTagName, opt<OptWS, list<Attribute, WS>>, //
          if_then_else<one<'/'>, SelfCloseTag,
                       seq<EndOpenTag, Content, one<'<'>, one<'/'>, CloseTagName, OptWS, one<'>'>>>> {};

struct Content : star<sor<Element, Text>> {};

struct Document : seq<Content, eof> {};

enum class SAXMode {
    Text,
    Attribute,
};

template <typename Rule>
struct Action : nothing<Rule> {};

template <>
struct Action<EndOpenTag> {
    static void apply0(SAXMode& mode, HTMLSAX* sax) {
        mode = SAXMode::Text;
    }
};

template <>
struct Action<Text> {
    static void apply0(SAXMode& mode, HTMLSAX* sax) {
        sax->textFinished();
    }
};

template <>
struct Action<Attribute> {
    static void apply0(SAXMode& mode, HTMLSAX* sax) {
        sax->attrFinished();
    }
};

template <>
struct Action<OpenTagName> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        sax->openTag(str);
        mode = SAXMode::Attribute;
    }
};

template <>
struct Action<SelfCloseTag> {
    static void apply0(SAXMode& mode, HTMLSAX* sax) {
        sax->closeTag();
        mode = SAXMode::Text;
    }
};

template <>
struct Action<PlainText> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        sax->textFragment(str);
    }
};

template <>
struct Action<PlainAttrValueUnquoted> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        sax->attrValueFragment(str);
    }
};

template <char q>
struct Action<PlainAttrValueQuoted<q>> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        sax->attrValueFragment(str);
    }
};

template <>
struct Action<AttrName> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        sax->attrName(str);
    }
};

template <>
struct Action<CharName> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        if (mode == SAXMode::Attribute)
            sax->attrValueFragment(htmlDecodeChar(str));
        else
            sax->textFragment(htmlDecodeChar(str));
    }
};

template <>
struct Action<CharCode> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        uint32_t value;
        std::from_chars(str.data(), str.data() + str.size(), value, 10);
        if (mode == SAXMode::Attribute)
            sax->attrValueFragment(Utf8Character{ value });
        else
            sax->textFragment(Utf8Character{ value });
    }
};

template <>
struct Action<CharHexCode> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        std::string_view str = in.string_view();
        uint32_t value;
        std::from_chars(str.data(), str.data() + str.size(), value, 16);
        if (mode == SAXMode::Attribute)
            sax->attrValueFragment(Utf8Character{ value });
        else
            sax->textFragment(Utf8Character{ value });
    }
};

template <>
struct Action<CloseTagName> {
    template <typename ActionInput>
    static void apply(const ActionInput& in, SAXMode& mode, HTMLSAX* sax) {
        sax->closeTag();
    }
};

} // namespace Grammar

std::array<std::pair<std::string_view, Color>, 149> colorNames{ {
    { "aliceblue", 0xf0f8ff_rgb },
    { "antiquewhite", 0xfaebd7_rgb },
    { "aqua", 0x00ffff_rgb },
    { "aquamarine", 0x7fffd4_rgb },
    { "azure", 0xf0ffff_rgb },
    { "beige", 0xf5f5dc_rgb },
    { "bisque", 0xffe4c4_rgb },
    { "black", 0x000000_rgb },
    { "blanchedalmond", 0xffebcd_rgb },
    { "blue", 0x0000ff_rgb },
    { "blueviolet", 0x8a2be2_rgb },
    { "brown", 0xa52a2a_rgb },
    { "burlywood", 0xdeb887_rgb },
    { "cadetblue", 0x5f9ea0_rgb },
    { "chartreuse", 0x7fff00_rgb },
    { "chocolate", 0xd2691e_rgb },
    { "coral", 0xff7f50_rgb },
    { "cornflowerblue", 0x6495ed_rgb },
    { "cornsilk", 0xfff8dc_rgb },
    { "crimson", 0xdc143c_rgb },
    { "cyan", 0x00ffff_rgb },
    { "darkblue", 0x00008b_rgb },
    { "darkcyan", 0x008b8b_rgb },
    { "darkgoldenrod", 0xb8860b_rgb },
    { "darkgray", 0xa9a9a9_rgb },
    { "darkgreen", 0x006400_rgb },
    { "darkgrey", 0xa9a9a9_rgb },
    { "darkkhaki", 0xbdb76b_rgb },
    { "darkmagenta", 0x8b008b_rgb },
    { "darkolivegreen", 0x556b2f_rgb },
    { "darkorange", 0xff8c00_rgb },
    { "darkorchid", 0x9932cc_rgb },
    { "darkred", 0x8b0000_rgb },
    { "darksalmon", 0xe9967a_rgb },
    { "darkseagreen", 0x8fbc8f_rgb },
    { "darkslateblue", 0x483d8b_rgb },
    { "darkslategray", 0x2f4f4f_rgb },
    { "darkslategrey", 0x2f4f4f_rgb },
    { "darkturquoise", 0x00ced1_rgb },
    { "darkviolet", 0x9400d3_rgb },
    { "deeppink", 0xff1493_rgb },
    { "deepskyblue", 0x00bfff_rgb },
    { "dimgray", 0x696969_rgb },
    { "dimgrey", 0x696969_rgb },
    { "dodgerblue", 0x1e90ff_rgb },
    { "firebrick", 0xb22222_rgb },
    { "floralwhite", 0xfffaf0_rgb },
    { "forestgreen", 0x228b22_rgb },
    { "fuchsia", 0xff00ff_rgb },
    { "gainsboro", 0xdcdcdc_rgb },
    { "ghostwhite", 0xf8f8ff_rgb },
    { "gold", 0xffd700_rgb },
    { "goldenrod", 0xdaa520_rgb },
    { "gray", 0x808080_rgb },
    { "green", 0x008000_rgb },
    { "greenyellow", 0xadff2f_rgb },
    { "grey", 0x808080_rgb },
    { "honeydew", 0xf0fff0_rgb },
    { "hotpink", 0xff69b4_rgb },
    { "indianred", 0xcd5c5c_rgb },
    { "indigo", 0x4b0082_rgb },
    { "ivory", 0xfffff0_rgb },
    { "khaki", 0xf0e68c_rgb },
    { "lavender", 0xe6e6fa_rgb },
    { "lavenderblush", 0xfff0f5_rgb },
    { "lawngreen", 0x7cfc00_rgb },
    { "lemonchiffon", 0xfffacd_rgb },
    { "lightblue", 0xadd8e6_rgb },
    { "lightcoral", 0xf08080_rgb },
    { "lightcyan", 0xe0ffff_rgb },
    { "lightgoldenrodyellow", 0xfafad2_rgb },
    { "lightgray", 0xd3d3d3_rgb },
    { "lightgreen", 0x90ee90_rgb },
    { "lightgrey", 0xd3d3d3_rgb },
    { "lightpink", 0xffb6c1_rgb },
    { "lightsalmon", 0xffa07a_rgb },
    { "lightseagreen", 0x20b2aa_rgb },
    { "lightskyblue", 0x87cefa_rgb },
    { "lightslategray", 0x778899_rgb },
    { "lightslategrey", 0x778899_rgb },
    { "lightsteelblue", 0xb0c4de_rgb },
    { "lightyellow", 0xffffe0_rgb },
    { "lime", 0x00ff00_rgb },
    { "limegreen", 0x32cd32_rgb },
    { "linen", 0xfaf0e6_rgb },
    { "magenta", 0xff00ff_rgb },
    { "maroon", 0x800000_rgb },
    { "mediumaquamarine", 0x66cdaa_rgb },
    { "mediumblue", 0x0000cd_rgb },
    { "mediumorchid", 0xba55d3_rgb },
    { "mediumpurple", 0x9370db_rgb },
    { "mediumseagreen", 0x3cb371_rgb },
    { "mediumslateblue", 0x7b68ee_rgb },
    { "mediumspringgreen", 0x00fa9a_rgb },
    { "mediumturquoise", 0x48d1cc_rgb },
    { "mediumvioletred", 0xc71585_rgb },
    { "midnightblue", 0x191970_rgb },
    { "mintcream", 0xf5fffa_rgb },
    { "mistyrose", 0xffe4e1_rgb },
    { "moccasin", 0xffe4b5_rgb },
    { "navajowhite", 0xffdead_rgb },
    { "navy", 0x000080_rgb },
    { "oldlace", 0xfdf5e6_rgb },
    { "olive", 0x808000_rgb },
    { "olivedrab", 0x6b8e23_rgb },
    { "orange", 0xffa500_rgb },
    { "orangered", 0xff4500_rgb },
    { "orchid", 0xda70d6_rgb },
    { "palegoldenrod", 0xeee8aa_rgb },
    { "palegreen", 0x98fb98_rgb },
    { "paleturquoise", 0xafeeee_rgb },
    { "palevioletred", 0xdb7093_rgb },
    { "papayawhip", 0xffefd5_rgb },
    { "peachpuff", 0xffdab9_rgb },
    { "peru", 0xcd853f_rgb },
    { "pink", 0xffc0cb_rgb },
    { "plum", 0xdda0dd_rgb },
    { "powderblue", 0xb0e0e6_rgb },
    { "purple", 0x800080_rgb },
    { "rebeccapurple", 0x663399_rgb },
    { "red", 0xff0000_rgb },
    { "rosybrown", 0xbc8f8f_rgb },
    { "royalblue", 0x4169e1_rgb },
    { "saddlebrown", 0x8b4513_rgb },
    { "salmon", 0xfa8072_rgb },
    { "sandybrown", 0xf4a460_rgb },
    { "seagreen", 0x2e8b57_rgb },
    { "seashell", 0xfff5ee_rgb },
    { "sienna", 0xa0522d_rgb },
    { "silver", 0xc0c0c0_rgb },
    { "skyblue", 0x87ceeb_rgb },
    { "slateblue", 0x6a5acd_rgb },
    { "slategray", 0x708090_rgb },
    { "slategrey", 0x708090_rgb },
    { "snow", 0xfffafa_rgb },
    { "springgreen", 0x00ff7f_rgb },
    { "steelblue", 0x4682b4_rgb },
    { "tan", 0xd2b48c_rgb },
    { "teal", 0x008080_rgb },
    { "thistle", 0xd8bfd8_rgb },
    { "tomato", 0xff6347_rgb },
    { "transparent", 0x00000000_rgba },
    { "turquoise", 0x40e0d0_rgb },
    { "violet", 0xee82ee_rgb },
    { "wheat", 0xf5deb3_rgb },
    { "white", 0xffffff_rgb },
    { "whitesmoke", 0xf5f5f5_rgb },
    { "yellow", 0xffff00_rgb },
    { "yellowgreen", 0x9acd32_rgb },
} };
} // namespace

} // namespace Internal

using namespace Internal;

bool parseHtml(std::string_view html, HTMLSAX* visitor) {
    tao::pegtl::memory_input input(html.data(), html.size(), "");

    Grammar::SAXMode mode = Grammar::SAXMode::Text;
    visitor->openDocument();
    bool result = tao::pegtl::parse<Grammar::Document, Grammar::Action>(input, mode, visitor);
    visitor->closeDocument();
    return result;
}

std::string_view htmlDecodeChar(std::string_view name) {
    auto it = std::lower_bound(charNames.begin(), charNames.end(), name,
                               [](const auto& pair, std::string_view name) {
                                   return pair.first < name;
                               });
    if (it != charNames.end() && it->first == name)
        return it->second;
    return "";
}

std::optional<Color> parseHtmlColor(std::string_view colorText) {
    if (colorText.starts_with('#')) {
        if (colorText.size() == 1 + 6) { // RRGGBB
            std::byte rgb[3];
            if (fromHex(rgb, colorText.substr(1)) != 3)
                return std::nullopt;
            return Color(uint8_t(rgb[0]), uint8_t(rgb[1]), uint8_t(rgb[2]));
        } else if (colorText.size() == 1 + 8) { // RRGGBBAA
            std::byte rgba[4];
            if (fromHex(rgba, colorText.substr(1)) != 4)
                return std::nullopt;
            return Color(uint8_t(rgba[0]), uint8_t(rgba[1]), uint8_t(rgba[2]), uint8_t(rgba[3]));
        } else if (colorText.size() == 1 + 3) { // RGB
            const char expanded[6] = { colorText[1], colorText[1], colorText[2],
                                       colorText[2], colorText[3], colorText[3] };
            std::byte rgb[3];
            if (fromHex(rgb, std::string_view(std::data(expanded), std::size(expanded))) != 3)
                return std::nullopt;
            return Color(uint8_t(rgb[0]), uint8_t(rgb[1]), uint8_t(rgb[2]));
        } else if (colorText.size() == 1 + 4) { // RGBA
            const char expanded[8] = { colorText[1], colorText[1], colorText[2], colorText[2],
                                       colorText[3], colorText[3], colorText[4], colorText[4] };
            std::byte rgba[4];
            if (fromHex(rgba, std::string_view(std::data(expanded), std::size(expanded))) != 4)
                return std::nullopt;
            return Color(uint8_t(rgba[0]), uint8_t(rgba[1]), uint8_t(rgba[2]), uint8_t(rgba[3]));
        } else {
            return std::nullopt;
        }
    } else {
        auto it = std::lower_bound(colorNames.begin(), colorNames.end(), colorText,
                                   [](const auto& pair, std::string_view name) {
                                       return pair.first < name;
                                   });
        if (it != colorNames.end() && it->first == colorText)
            return it->second;
        return std::nullopt;
    }
}

} // namespace Brisk
