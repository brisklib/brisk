# Working with Graphics

Drawing 2D content may be necessary in the following cases:

- Painting with a `Viewport` widget
- Painting custom widgets
- Offscreen painting to an `Image`

In all these cases, drawing is performed using the `Canvas` class.

The `Canvas` class in the Brisk library provides a high-level interface for rendering 2D graphical elements, such as shapes, text, and images. It abstracts low-level rendering APIs, making it easier to create rich, interactive graphics. This tutorial guides you through the key features of the `Canvas` class, from basic drawing operations to advanced state management and performance optimizations.

`Canvas` provides methods for drawing various graphical elements, including paths, rectangles, ellipses, text, and images.

## Basic Drawing

### Drawing Paths and Shapes

Using the `Path` class, you can build a shape consisting of rectangles, ellipses, lines, Bezier curves, and more. Once the path is complete, you can render it by stroking the outline (`strokePath`), filling the interior (`fillPath`), or both (`drawPath`).

```c++
Path path;
path.moveTo(0, 0);
path.lineTo(100, 100);
canvas.strokePath(path);  // Draws the line
```

`Canvas` offers convenience methods for drawing rectangles and ellipses, with options for rounded corners and squircle shapes.

```c++
canvas.fillRect(RectangleF(10, 10, 50, 50), CornersF(5.f)); // Rounded corners
canvas.strokeEllipse(RectangleF(70, 70, 30, 30)); // Ellipse
```

### Drawing Text

`Canvas` supports text rendering with customizable fonts and alignment options. Use `fillText` to render text at a specific position or within a rectangle.

```c++
canvas.setFont(Font("Arial", 16));
canvas.fillText("Hello, World!", PointF(100, 100));
// Center in rectangle
canvas.fillText("Hello, World!", RectangleF(50, 50, 150, 150), PointF{0.5f, 0.5f});
```

### Drawing Images

Images can be drawn within a specified rectangle, with optional transformations (e.g., rotation, scaling) and sampler modes.

```c++
Rc<Image> image = ...;  // Assume image is loaded
canvas.drawImage(RectangleF(200, 200, 100, 100), image);
```

To apply transformations, pass a `Matrix`:

```c++
Matrix transform = Matrix::rotation(45.f) * Matrix::translation(150, 150);
canvas.drawImage(RectangleF(0, 0, 100, 100), image, transform);
```

### Setting Drawing Parameters

You can use the following `Canvas` functions to control how paths are drawn:

#### Stroke

- `setStrokePaint`/`setStrokeColor`
- `setStrokeWidth`
- `setMiterLimit`
- `setJoinStyle`
- `setCapStyle`

#### Fill

- `setFillRule`
- `setFillPaint`/`setFillColor`

#### Blending

- `setOpacity`

### Managing Canvas State

`Canvas` maintains a state that includes the transformation matrix, clip rectangle, paints, and other drawing parameters. You can save and restore the state to manage complex drawing operations.

#### Saving and Restoring State

Use `save` to save the current state and `restore` to revert to the last saved state.

```c++
canvas.save();
// Modify state (e.g., change color, transform)
canvas.setStrokeColor(ColorW::Red);
canvas.strokeRect(RectangleF(10, 10, 50, 50));
canvas.restore();  // Restores previous state
```

For automatic state restoration, use the `StateSaver` class, which restores the state when it goes out of scope.

```c++
{
    auto&& state = canvas.saveState();
    state->setStrokeColor(ColorW::Blue);
    canvas.strokeRect(RectangleF(70, 70, 50, 50));
}  // State is automatically restored here
```

#### Using `StateSaver` and `ClipRectSaver`

Similarly, `ClipRectSaver` can be used to manage the clip rectangle.

```c++
{
    auto&& clip = canvas.saveClipRect();
    clip->setClipRect(RectangleF(50, 50, 100, 100));
    canvas.fillRect(RectangleF(0, 0, 200, 200));  // Only the clipped area is filled
}  // Clip rectangle is restored here
```

## Example: Drawing with `Viewport` Widget

```c++
rcnew Viewport{
    [](Canvas& canvas, Rectangle rect) {
        // Static initialization of an image rendered from an SVG representation of "cat"
        // with a size of 256x256 pixels.
        static Rc<Image> img =
            SvgImage(Resources::loadText("cat.svg")).render(Size{ idp(256), idp(256) });

        // Draws a rectangle on the canvas at position 'rect' with no fill color (transparent)
        // and a stroke color of amber and a stroke width of 1 pixel.
        canvas.setStrokeColor(Palette::Standard::amber);
        canvas.setStrokeWidth(1);
        canvas.strokeRect(rect);

        // Creates a rectangle 'frect' based on 'rect' for further operations.
        // 'angle' is a static float variable initialized at 0 and incremented by 0.2 in every render
        // cycle, giving rotation over time. The canvas transform rotates the rectangle around its
        // center.
        RectangleF frect   = rect;
        static float angle = 0.f;
        angle += 0.2f;
        canvas.transform(Matrix{}.rotate(angle, frect.at(0.5f, 0.5f)));

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
        Gradient gradient{ GradientType::Linear, rect.at(0.25, 0.25), rect.at(0.75, 0.75) };
        gradient.addStop(0.f, Palette::Standard::red);
        gradient.addStop(1.f, Palette::Standard::green);

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

        // Set the fill color to lime green and set the font with a size of 48
        // pixels. Draw the text "Brisk" centered inside the rectangle 'frect'.
        canvas.setFillColor(Palette::Standard::lime);
        canvas.setFont(Font{ Font::Default, 48_dp });
        canvas.fillText("Brisk", frect.at(0.5f, 0.5f));
    },
    dimensions = { 256, 256 },
},
```

## Advanced Scenarios

### Disabling SDF

Brisk internally uses SDF (Signed Distance Fields) for simple shapes to improve rendering performance. SDF graphics look almost identical to analytical rasterization, but if you need a 100% match, you can disable SDF by passing `CanvasFlag::None` when creating `Canvas`:

```c++
Canvas canvas(context, CanvasFlags::None);
```

### Linear Color

By default, Brisk operates in an “sRGB-unaware” mode, performing linear color blending directly in gamma-corrected sRGB space. While this approach isn’t physically accurate, it aligns with the behavior of many major graphics libraries and meets common user expectations.

To enable physically accurate blending, set the global variable `linearColor` to `true`. This switches Brisk to linear color mode, where blending occurs in linear space and conversion to sRGB happens only at the final output stage.

### Stateless Drawing

For greater control, you can use the stateless versions of drawing functions, which allow you to specify all parameters explicitly without altering the canvas state.

```c++
Paint strokePaint = ColorW::Red;
StrokeParams params = { 2.f, CapStyle::Round, JoinStyle::Round };
Matrix matrix = Matrix::identity();
RectangleF clipRect = RectangleF(0, 0, 200, 200);
float opacity = 1.f;
canvas.strokePath(path, strokePaint, params, matrix, clipRect, opacity);
```
