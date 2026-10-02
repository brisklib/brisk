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
 * Brisk is dual-licensed under the GNU General Public License version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the terms of the GNU GPL, you may purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <brisk/application/Args.hpp>
#include <brisk/core/Io.hpp>
#include <brisk/graphics/ImageFormats.hpp>
#include <brisk/graphics/Offscreen.hpp>
#include <brisk/graphics/Palette.hpp>

namespace {

using namespace Brisk;

constexpr std::string_view fontAlias = "SpecimenFont";

struct Options {
    fs::path fontPath;
    std::optional<size_t> fontIndex;
    std::string fontName;
    fs::path outputPath = "font-specimen.png";
    int scale           = 2;
    float gamma         = 1.f;
    bool lcd            = false;
    bool hinting        = false;
    bool dark           = false;
};

void printUsage() {
    std::fprintf(stderr, "Usage: font-specimen --font=font_file[:face_index] [--scale=1|2|4] [--gamma=value] "
                         "[--lcd] [--hinting] [--dark] "
                         "[output.png]\n");
}

bool parseFontSpec(std::string_view value, fs::path& path, std::optional<size_t>& faceIndex) {
    const size_t separator = value.rfind(':');
    if (separator == std::string_view::npos) {
        path = std::string(value);
        return !value.empty();
    }

    const std::string_view suffix = value.substr(separator + 1);
    const std::string_view base   = value.substr(0, separator);
    if (base.empty() || suffix.empty()) {
        return false;
    }

    const fs::path candidate{ std::string(base) };
    std::string extension = candidate.extension().string();
    for (char& c : extension) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (extension != ".ttc") {
        path = std::string(value);
        return !value.empty();
    }

    if (!std::all_of(suffix.begin(), suffix.end(), [](unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        return false;
    }

    size_t parsed     = 0;
    const auto result = std::from_chars(suffix.data(), suffix.data() + suffix.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != suffix.data() + suffix.size())
        return false;
    path      = candidate;
    faceIndex = parsed;
    return true;
}

bool parseScale(std::string_view value, int& scale) {
    int parsed        = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
        return false;
    if (parsed != 1 && parsed != 2 && parsed != 4)
        return false;
    scale = parsed;
    return true;
}

bool parseGamma(std::string_view value, float& gamma) {
    std::string input{ value };
    char* end          = nullptr;
    const float parsed = std::strtof(input.c_str(), &end);
    if (end != input.c_str() + input.size() || !std::isfinite(parsed) || parsed <= 0.f) {
        return false;
    }
    gamma = parsed;
    return true;
}

bool parseOptions(Options& options) {
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string_view argument = args[i];
        if (argument.starts_with("--font=")) {
            if (!parseFontSpec(argument.substr(7), options.fontPath, options.fontIndex))
                return false;
        } else if (argument.starts_with("--scale=")) {
            if (!parseScale(argument.substr(8), options.scale))
                return false;
        } else if (argument.starts_with("--gamma=")) {
            if (!parseGamma(argument.substr(8), options.gamma))
                return false;
        } else if (argument == "--lcd") {
            options.lcd = true;
        } else if (argument == "--hinting") {
            options.hinting = true;
        } else if (argument == "--dark") {
            options.dark = true;
        } else if (argument.starts_with("--")) {
            return false;
        } else if (options.outputPath == fs::path("font-specimen.png")) {
            options.outputPath = std::string(argument);
        } else {
            return false;
        }
    }
    return !options.fontPath.empty();
}

struct Paragraph {
    std::string text;
    Color color;
    Font font;
    TextLayoutOptions layoutOptions;
    float topMargin;
    float bottomMargin;
    TextLayout layout;
};

Font specimenFont(const Options& options, float size) {
    Font font{ std::string(fontAlias), size };
    font.hinting = options.hinting ? FontHinting::Auto : FontHinting::Disable;
    return font;
}

TextLayoutOptions paragraphLayout(float width, float firstLineIndent = 0.f) {
    return TextLayoutOptions{
        .maxLineWidth    = width,
        .firstLineIndent = firstLineIndent,
    };
}

Paragraph makeParagraph(std::string_view text, Color color, Font font, TextLayoutOptions layoutOptions,
                        float margin) {
    return Paragraph{
        .text          = std::string(text),
        .color         = color,
        .font          = std::move(font),
        .layoutOptions = layoutOptions,
        .topMargin     = margin,
        .bottomMargin  = margin,
    };
}

constexpr std::string_view paragraph1 =
    "Typography gives language a visible voice. This specimen shows the character of the font at several "
    "sizes, with ordinary words, punctuation, and numerals arranged for comparison.";
constexpr std::string_view paragraph2 =
    "A good typeface remains comfortable to read in a quiet paragraph and expressive in a large "
    "headline. "
    "Notice the rhythm, proportions, spacing, and the shapes of its most familiar letters.";
constexpr std::string_view paragraph3 =
    "Sed ut perspiciatis, unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, "
    "totam rem aperiam eaque ipsa, quae ab illo inventore veritatis et quasi architecto beatae vitae "
    "dicta sunt, explicabo. Nemo enim ipsam voluptatem, quia voluptas sit, aspernatur aut odit aut "
    "fugit, sed quia consequuntur magni dolores eos, qui ratione voluptatem sequi nesciunt, neque porro "
    "quisquam est, qui dolorem ipsum, quia dolor sit amet consectetur adipisci velit, sed quia non "
    "numquam eius modi tempora incidunt, ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim "
    "ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid "
    "ex ea commodi consequatur? Quis autem vel eum iure reprehenderit, qui in ea voluptate velit esse, "
    "quam nihil molestiae consequatur, vel illum, qui dolorem eum fugiat, quo voluptas nulla pariatur?";
constexpr std::string_view paragraph4 =
    "At vero eos et accusamus et iusto odio dignissimos ducimus, qui blanditiis praesentium voluptatum "
    "deleniti atque corrupti, quos dolores et quas molestias excepturi sint, obcaecati cupiditate non "
    "provident, similique sunt in culpa, qui officia deserunt mollitia animi, id est laborum et dolorum "
    "fuga. Et harum quidem reruum facilis est et expedita distinctio. Nam libero tempore, cum soluta "
    "nobis est eligendi optio, cumque nihil impedit, quo minus id, quod maxime placeat facere possimus, "
    "omnis voluptas assumenda est, omnis dolor repellendus. Temporibus autem quibusdam et aut officiis "
    "debitis aut rerum necessitatibus saepe eveniet, ut et voluptates repudiandae sint et molestiae non "
    "recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus "
    "maiores alias consequatur aut perferendis doloribus asperiores repellat.";

std::vector<Paragraph> buildParagraphs(const Options& options, float width, float margin) {
    const float s            = static_cast<float>(options.scale);
    const Color foreground   = options.dark ? Palette::white : Palette::black;
    const Color secondary    = options.dark ? Color{ 190, 190, 190 } : Color{ 90, 90, 90 };
    const float contentWidth = width - margin * 2.f;

    std::vector<Paragraph> paragraphs;
    paragraphs.reserve(9);

    paragraphs.push_back(makeParagraph(options.fontName, foreground, specimenFont(options, 64.f * s),
                                       paragraphLayout(contentWidth), 24.f * s));
    paragraphs.push_back(makeParagraph("abcdefghijklmnopqrstuvwxyz", foreground,
                                       specimenFont(options, 32.f * s), paragraphLayout(contentWidth),
                                       8.f * s));
    paragraphs.push_back(makeParagraph("ABCDEFGHIJKLMNOPQRSTUVWXYZ", foreground,
                                       specimenFont(options, 32.f * s), paragraphLayout(contentWidth),
                                       8.f * s));
    paragraphs.push_back(makeParagraph("0123456789  !? @#$% &*() [] {} /\\ +-=;:,.", foreground,
                                       specimenFont(options, 32.f * s), paragraphLayout(contentWidth),
                                       24.f * s));
    paragraphs.push_back(makeParagraph(paragraph1, foreground, specimenFont(options, 25.f * s),
                                       paragraphLayout(contentWidth, 50.f * s), 32.f * s));
    paragraphs.push_back(makeParagraph(paragraph2, foreground, specimenFont(options, 20.f * s),
                                       paragraphLayout(contentWidth, 40.f * s), 32.f * s));
    paragraphs.push_back(makeParagraph(paragraph3, foreground, specimenFont(options, 18.f * s),
                                       paragraphLayout(contentWidth, 36.f * s), 32.f * s));
    paragraphs.push_back(makeParagraph(paragraph4, foreground, specimenFont(options, 14.f * s),
                                       paragraphLayout(contentWidth, 28.f * s), 32.f * s));
    paragraphs.push_back(
        makeParagraph(fmt::format("scale {}  •  gamma {}  •  LCD {}  •  hinting {}", options.scale,
                                  options.gamma, options.lcd ? "on" : "off", options.hinting ? "on" : "off"),
                      secondary, specimenFont(options, 16.f * s), paragraphLayout(contentWidth), 0.f));

    for (Paragraph& paragraph : paragraphs) {
        paragraph.layout = fonts->shapeText(paragraph.font, TextWithOptions{ paragraph.text })
                               .layout(paragraph.layoutOptions);
    }
    return paragraphs;
}

float specimenHeight(const std::vector<Paragraph>& paragraphs, float outerMargin) {
    float height      = 0.f;
    float previousGap = outerMargin;
    for (const Paragraph& paragraph : paragraphs) {
        height += std::max(previousGap, paragraph.topMargin);
        height += paragraph.layout.bounds().height();
        previousGap = paragraph.bottomMargin;
    }
    return height + std::max(previousGap, outerMargin);
}

void drawSpecimen(Canvas& canvas, const Options& options, int width, int height,
                  const std::vector<Paragraph>& paragraphs) {
    const float s          = static_cast<float>(options.scale);
    const Color background = options.dark ? Palette::black : Palette::white;
    const float margin     = 48.f * s;

    canvas.setFillColor(background);
    canvas.fillRect(RectangleF{ 0.f, 0.f, static_cast<float>(width), static_cast<float>(height) });
    canvas.setSubpixelTextRendering(options.lcd);

    float y           = 0.f;
    float previousGap = margin;
    for (const Paragraph& paragraph : paragraphs) {
        // Adjacent vertical margins collapse, just like margins between HTML blocks.
        y += std::max(previousGap, paragraph.topMargin);
        canvas.setFillColor(paragraph.color);
        canvas.setFont(paragraph.font);
        canvas.fillText(PointF{ margin, y }, paragraph.layout);
        y += paragraph.layout.bounds().height();
        previousGap = paragraph.bottomMargin;
    }
}

} // namespace

int briskMain() {
    using namespace Brisk;

    Options options;
    if (!parseOptions(options)) {
        printUsage();
        return 2;
    }

    auto status = fonts->addFontFromFile(options.fontPath, std::string(fontAlias), options.fontIndex);
    if (!status) {
        std::fprintf(stderr, "Cannot load font '%s': %s\n", options.fontPath.string().c_str(),
                     fmt::format("{}", status.error()).c_str());
        return 1;
    }
    options.fontName      = status->front();

    const int width       = 1024 * options.scale;
    const float margin    = 48.f * static_cast<float>(options.scale);
    const auto paragraphs = buildParagraphs(options, static_cast<float>(width), margin);
    const int height      = static_cast<int>(std::ceil(specimenHeight(paragraphs, margin)));

    OffscreenCanvas offscreen{ Size{ width, height }, static_cast<float>(options.scale),
                               VisualSettings{ .gamma = options.gamma, .subPixelText = options.lcd } };
    drawSpecimen(offscreen.canvas(), options, width, height, paragraphs);

    Rc<Image> image = offscreen.render();
    if (!image) {
        std::fprintf(stderr, "Could not render font specimen\n");
        return 1;
    }

    const Bytes png = pngEncode(std::move(image));
    if (png.empty()) {
        std::fprintf(stderr, "Could not encode PNG\n");
        return 1;
    }
    if (auto status = writeBytes(options.outputPath, png); !status) {
        std::fprintf(stderr, "Cannot write '%s': %s\n", options.outputPath.string().c_str(),
                     fmt::format("{}", status.error()).c_str());
        return 1;
    }

    return 0;
}
