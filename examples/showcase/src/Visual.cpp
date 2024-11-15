#include "Visual.hpp"
#include <brisk/gui/Styles.hpp>
#include <brisk/graphics/Palette.hpp>
#include <resources/cat.hpp>
#include <resources/hot_air_balloons.hpp>
#include <resources/countries_json.hpp>

namespace Brisk {

const std::string loremIpsumShort =
    "Sed ut perspiciatis, unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, "
    "totam rem aperiam eaque ipsa, quae ab illo inventore veritatis et quasi architecto beatae vitae dicta "
    "sunt, explicabo. Nemo enim ipsam voluptatem, quia voluptas sit, aspernatur aut odit aut fugit, sed quia "
    "consequuntur magni dolores eos, qui ratione voluptatem sequi nesciunt, neque porro quisquam est, qui "
    "do.";

const NameValueOrderedList<TextAlign> textAlignList{
    { "Left", TextAlign::Start },
    { "Center", TextAlign::Center },
    { "Right", TextAlign::End },
};

RC<Widget> ShowcaseVisual::build(RC<Notifications> notifications) {

    static const Rules cell = Rules{
        layout  = Layout::Horizontal,
        padding = { 16, 5 },
    };
    static const Rules headerCell = Rules{
        layout = Layout::Horizontal, fontFamily = Lato,   fontWeight = FontWeight::Bold,
        color = 0x808080_rgb,        padding = { 16, 5 },
    };

    return rcnew VLayout{
        flexGrow = 1,
        padding  = 16_apx,
        gapRow   = 8_apx,

        new Text{ "Text (widgets/Text.hpp)", classes = { "section-header" } },

        new VLayout{
            gapRow = 4_apx,
            new Text{ "Simple text" },
            new Text{ "Multi-line\ntext" },
            new Text{ "Text with color = Palette::Standard::fuchsia, fontWeight = FontWeight::Bold",
                      color = Palette::Standard::fuchsia, fontWeight = FontWeight::Bold },
            new Text{ "Text with textAutoSize = TextAutoSize::FitWidth (Resize the window to make the text"
                      "size fit the width)",
                      height = 50_apx, textAutoSize = TextAutoSize::FitWidth },
        },

        new Text{ "wordWrap = true", classes = { "section-header" } },

        new VLayout{
            new HLayout{
                new Text{ "Text alignment: " },
                new ComboBox{
                    Value{ &m_textAlign },
                    notManaged(&textAlignList),
                    width = 110_apx,
                },
                new Text{ "Font size: " },

                new Slider{ value = Value{ &m_fontSize }, minimum = 0.25f, maximum = 4.f, width = 300_apx },
            },
            new Text{ loremIpsumShort, wordWrap = true, textAlign = Value{ &m_textAlign }, marginTop = 10_apx,
                      fontSize = Value{ &m_fontSize }.transform([](float v) {
                          return v * 100_perc;
                      }),
                      fontFamily = Lato },
        },

        new Text{ "Viewport (widgets/Viewport.hpp)", classes = { "section-header" } },

        new Viewport{
            [](Canvas& canvas, Rectangle rect) {
                // Static initialization of an image rendered from an SVG representation of "cat"
                // with a size of 256x256 pixels.
                static RC<Image> img = SVGImage(toStringView(cat())).render(Size{ idp(256), idp(256) });

                // Draws a rectangle on the canvas at position 'rect' with no fill color (transparent)
                // and a stroke color of amber and a stroke width of 1 pixel.
                canvas.raw().drawRectangle(rect, 0.f, 0.f, fillColor = Palette::transparent,
                                           strokeColor = Palette::Standard::amber, strokeWidth = 1);

                // Creates a rectangle 'frect' based on 'rect' for further operations.
                // 'angle' is a static float variable initialized at 0 and incremented by 0.2 in every render
                // cycle, giving rotation over time. The canvas transform rotates the rectangle around its
                // center.
                RectangleF frect   = rect;
                static float angle = 0.f;
                angle += 0.2f;
                canvas.transform(Matrix2D{}.rotate(angle, frect.at(0.5f, 0.5f)));

                // Set the fill color to red and draw a small ellipse centered at 0.25f, 0.25f of the
                // rectangle's size, aligning to the center with dimensions of 12 pixels.
                canvas.setFillColor(Palette::Standard::red);
                canvas.fillEllipse(frect.at(0.25f, 0.25f).alignedRect({ 12_dp, 12_dp }, { 0.5f, 0.5f }));

                // Calculate the center point and radius for a circular path.
                // 'r' is half the width of the rectangle, and 'c' is the center point.
                // A lambda function 'pt' computes the points on the circumference of the circle, using an
                // angle in radians.
                float r = rect.width() / 2.f;
                PointF c{ r, r };
                c       = c + frect.p1;
                auto pt = [c, r](float a) {
                    a *= std::numbers::pi_v<float> * 2;                    // Convert angle to radians.
                    return c + PointF{ std::cos(a) * r, std::sin(a) * r }; // Calculate a point on the circle.
                };

                // Create a path 'p' and define a star-like shape by connecting points on the circle at
                // different angles.
                Path p;
                p.moveTo(pt(0));       // Start at the first point.
                p.lineTo(pt(0));       // Draw lines between subsequent points.
                p.lineTo(pt(3.f / 8)); // 3/8 of the full circle.
                p.lineTo(pt(6.f / 8)); // 6/8 of the full circle.
                p.lineTo(pt(9.f / 8)); // Continue around the circle.
                p.lineTo(pt(12.f / 8));
                p.lineTo(pt(15.f / 8));
                p.lineTo(pt(18.f / 8));
                p.lineTo(pt(21.f / 8));
                p.close(); // Close the path, creating the shape.

                // Set the fill rule to Winding, which determines how the shape's interior is filled.
                canvas.setFillRule(FillRule::Winding);

                // Create a linear gradient from one corner to another and define two color stops
                // (red at 0% and green at 100%).
                GradientPtr gradient =
                    rcnew Gradient{ GradientType::Linear, rect.at(0.25, 0.25), rect.at(0.75, 0.75) };
                gradient->addStop(0.f, Palette::Standard::red);
                gradient->addStop(1.f, Palette::Standard::green);

                // Apply the gradient as the fill paint for the path and fill the shape 'p' with this
                // gradient.
                canvas.setFillPaint(gradient);
                canvas.fillPath(p);

                // Set the stroke color to blue, stroke width to 10 pixels, and a dash pattern of 40 pixels
                // on, 20 pixels off. Use a miter join style for sharp corners. Then, stroke the path 'p' to
                // outline it.
                canvas.setStrokeColor(Palette::Standard::blue);
                canvas.setStrokeWidth(10.f);
                canvas.setDashArray({ 40.f, 20.f });
                canvas.setJoinStyle(JoinStyle::Miter);
                canvas.strokePath(p);

                // Set the fill color to lime green and set the font to "Lato" with a size of 48
                // pixels. Draw the text "Brisk" centered inside the rectangle 'frect'.
                canvas.setFillColor(Palette::Standard::lime);
                canvas.setFont(Font{ Lato, 48_dp });
                canvas.fillText("Brisk", frect.at(0.5f, 0.5f));
            },
            nullptr,
            dimensions = { 256, 256 },
        },

        new Text{ "Spinner (widgets/Spinner.hpp)", classes = { "section-header" } },

        new HLayout{
            new Spinner{
                dimensions = { 40_apx, 40_apx },
                active     = Value{ &m_active },
            },
            gapColumn = 10_apx,
            new CheckBox{ value = Value{ &m_active }, new Text{ "Active" } },
        },

        new Text{ "Progress (widgets/Progress.hpp)", classes = { "section-header" } },

        new HLayout{
            new Progress{
                value      = Value{ &m_progress },
                minimum    = 0,
                maximum    = 100,
                dimensions = { 400_apx, 20_apx },
            },
            gapColumn = 10_apx,
            new CheckBox{ value = Value{ &m_progressActive }, new Text{ "Active" } },
        },

        new Text{ "ImageView (widgets/ImageView.hpp)", classes = { "section-header" } },

        new HLayout{
            new ImageView{ hot_air_balloons(), dimensions = { 180_apx, 120_apx } },
        },

        new Text{ "SVGImageView (widgets/ImageView.hpp)", classes = { "section-header" } },

        new HLayout{
            new SVGImageView{ toStringView(cat()), dimensions = { 120_apx, 120_apx } },
        },

        new Text{ "Table (widgets/Table.hpp)", classes = { "section-header" } },

        new VScrollBox{
            height = 400_apx,
            new Table{
                flexGrow        = 1,
                backgroundColor = 0xFFFFFF'10_rgba,
                new TableHeader{
                    new TableCell{ headerCell, new Text{ "Country" } },
                    new TableCell{ headerCell, new Text{ "Capital" } },
                    new TableCell{ headerCell, new Text{ "Population" }, justifyContent = Justify::FlexEnd },
                    new TableCell{ headerCell, new Text{ "Area (km²)" }, justifyContent = Justify::FlexEnd },
                },
                Builder([](Widget* target) {
                    JsonArray countries = Json::fromJson(std::string(toStringView(countries_json())))
                                              .value()              // Assume it's valid JSON
                                              .access<JsonArray>(); // Assume it's array
                    std::sort(countries.begin(), countries.end(), [](Json a, Json b) {
                        return a.access<JsonObject>()["population"].to<int64_t>().value_or(0) >
                               b.access<JsonObject>()["population"].to<int64_t>().value_or(0);
                    });
                    for (Json country : countries) {
                        JsonObject& obj = country.access<JsonObject>();
                        target->apply(new TableRow{
                            new TableCell{ cell, new Text{ obj["country"].to<std::string>().value_or("") } },
                            new TableCell{ cell, new Text{ obj["capital"].to<std::string>().value_or("") } },
                            new TableCell{
                                cell, new Text{ fmt::to_string(obj["population"].to<int64_t>().value_or(0)) },
                                justifyContent = Justify::FlexEnd },
                            new TableCell{ cell,
                                           new Text{ fmt::to_string(obj["area"].to<int64_t>().value_or(0)) },
                                           justifyContent = Justify::FlexEnd },
                        });
                    }
                }),
            },
        },

        new Table{
            flexGrow = 1,
            Builder{
                [this](Widget* target) {
                    for (Row& row : m_rows) {
                        target->apply(new TableRow{
                            new TableCell{ cell, new Text{ row.firstName } },
                            new TableCell{ cell, new Text{ row.lastName } },
                            new TableCell{ cell, new ComboBox{ value = Value{ &row.index },
                                                               new ItemList{
                                                                   new Text{ "UX/UI Designer" },
                                                                   new Text{ "Project Manager" },
                                                                   new Text{ "Software Engineer" },
                                                                   new Text{ "Software Developer" },
                                                               } } },
                            new TableCell{ cell, new CheckBox{ value = Value{ &row.checkBox },
                                                               new Text{ "Full access" } } },
                        });
                    }
                },
            },
        },

        new Text{ "Hint", classes = { "section-header" } },

        new HLayout{
            new Text{
                "Hej, verden",
                isHintExclusive = true,
                hint            = Value{ &m_hintActive }.transform([](bool v) -> std::string {
                    return v ? "Hello, world" : "";
                }),
            },
            gapColumn = 10_apx,
            new CheckBox{ value = Value{ &m_hintActive }, new Text{ "Show hint" } },
        },

    };
}

ShowcaseVisual::ShowcaseVisual() {
    bindings->listen(Value{ &frameStartTime },
                     WithLifetime{
                         [this]() {
                             if (m_progressActive)
                                 bindings->assign(m_progress, std::fmod(m_progress + 0.2f, 100.f));
                         },
                         toBindingAddress(this),
                     });
}
} // namespace Brisk
