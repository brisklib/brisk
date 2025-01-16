#include <string>
#include <string_view>
#include <brisk/core/internal/SmallVector.hpp>
#include <brisk/core/Log.hpp>
#include <brisk/graphics/Fonts.hpp>
#include <tinyxml2.h>

namespace Brisk {

using namespace tinyxml2;

namespace Internal {

class Html : public XMLDocument {
public:
    using XMLDocument::XMLDocument;
};

namespace {

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

class RichTextBuilder final : public XMLVisitor {
public:
    RichText richText;
    std::vector<FontAndColor> fontStack;
    bool lastNodeIsText = true;

    explicit RichTextBuilder(const Font& font) {
        fontStack.push_back({ font, std::nullopt });
    }

    bool VisitExit(const XMLDocument& /*doc*/) {
        richText.offsets.pop_back();
        return true;
    }

    void emitText(std::u32string_view text) {
        FontAndColor newFont = fontStack.back();
        richText.text += text;
        if (richText.fonts.empty() || newFont != richText.fonts.back()) {
            richText.fonts.push_back(std::move(newFont));
            richText.offsets.push_back(richText.text.size());
        } else if (!richText.offsets.empty()) {
            richText.offsets.back() = richText.text.size();
        }
    }

    bool Visit(const XMLText& text) {
        emitText(utf8ToUtf32(text.Value()));
        lastNodeIsText = true;
        return true;
    }

    bool VisitEnter(const XMLElement& element, const XMLAttribute* /*firstAttribute*/) {
        if (!lastNodeIsText) {
            emitText(U" ");
        }
        fontStack.push_back(fontStack.back());
        if (element.Value() == "b"sv || element.Value() == "strong"sv) {
            fontStack.back().font.weight = FontWeight::Bold;
        } else if (element.Value() == "i"sv || element.Value() == "em"sv) {
            fontStack.back().font.style = FontStyle::Italic;
        } else if (element.Value() == "small"sv) {
            fontStack.back().font.fontSize *= 0.5f;
        } else if (element.Value() == "s"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::LineThrough;
        } else if (element.Value() == "u"sv) {
            fontStack.back().font.textDecoration |= TextDecoration::Underline;
        } else if (element.Value() == "big"sv) {
            fontStack.back().font.fontSize *= 2.0f;
        } else if (element.Value() == "br"sv) {
            emitText(U"\n");
        } else if (element.Value() == "font"sv) {
            if (const char* color = element.Attribute("color")) {
                fontStack.back().color = parseHtmlColor(color);
            }
            if (float size = element.FloatAttribute("size", 0.f); size != 0.f) {
                fontStack.back().font.fontSize = size;
            }
        }
        return true;
    }

    bool VisitExit(const XMLElement& element) {
        fontStack.pop_back();
        lastNodeIsText = false;
        return true;
    }
};
} // namespace

std::optional<Color> parseHtmlColor(std::string_view colorText) {
    if (colorText.starts_with('#')) {
        if (colorText.size() == 1 + 6) { // RRGGBB
            uint8_t rgb[3];
            if (fromHex(rgb, colorText.substr(1)) != 3)
                return std::nullopt;
            return Color(rgb[0], rgb[1], rgb[2]);
        } else if (colorText.size() == 1 + 8) { // RRGGBBAA
            uint8_t rgba[4];
            if (fromHex(rgba, colorText.substr(1)) != 4)
                return std::nullopt;
            return Color(rgba[0], rgba[1], rgba[2], rgba[3]);
        } else if (colorText.size() == 1 + 3) { // RGB
            const char expanded[6] = { colorText[1], colorText[1], colorText[2],
                                       colorText[2], colorText[3], colorText[3] };
            uint8_t rgb[3];
            if (fromHex(rgb, std::string_view(std::data(expanded), std::size(expanded))) != 3)
                return std::nullopt;
            return Color(rgb[0], rgb[1], rgb[2]);
        } else if (colorText.size() == 1 + 4) { // RGBA
            const char expanded[8] = { colorText[1], colorText[1], colorText[2], colorText[2],
                                       colorText[3], colorText[3], colorText[4], colorText[4] };
            uint8_t rgba[4];
            if (fromHex(rgba, std::string_view(std::data(expanded), std::size(expanded))) != 4)
                return std::nullopt;
            return Color(rgba[0], rgba[1], rgba[2], rgba[3]);
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

std::shared_ptr<Html> parseHtml(std::string_view html) {
    std::shared_ptr<Html> doc(new Html(true, PRESERVE_WHITESPACE));
    auto err = doc->Parse(fmt::format("<html>{}</html>", html).c_str());
    if (err != XMLError::XML_SUCCESS) {
        LOG_DEBUG(xml, "xml parse error {}", Html::ErrorIDToName(err));
        return nullptr;
    }
    return doc;
}

RichText processHtml(std::shared_ptr<Html> html, const Font& defaultFont) {
    RichTextBuilder builder{ defaultFont };
    html->Accept(&builder);
    return std::move(builder.richText);
}
}; // namespace Internal

} // namespace Brisk
