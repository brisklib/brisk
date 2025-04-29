# Brisk: A Modern C++ GUI Framework

## Introduction to Brisk

Brisk is an advanced, open-source C++ GUI framework originally developed for a proprietary project. It has been released under the GPLv2+ license, with commercial licensing options available for closed-source applications.

Brisk requires a C++20-compliant compiler and uses vcpkg for dependency management, with prebuilt binaries also available. It currently supports macOS, Linux, and Windows, with plans to extend support to mobile platforms.

## Reactive Features

Brisk brings modern UI paradigms to C++ by streamlining complex workflows without compromising performance. Its flexible binding system integrates smoothly with standard C++ data structures, requiring minimal boilerplate. This enables effortless integration with existing codebases and efficient UI updates based on data changes.

The framework follows a declarative approach to UI development, which simplifies both UI construction and appearance definition. This approach is similar to modern UI frameworks for web and mobile platforms, such as React, SwiftUI, and Jetpack Compose.

Despite C++'s limited reflection and introspection capabilities, Brisk employs innovative techniques to maintain flexibility and ease of use while avoiding runtime overhead.

While Brisk applications typically adopt the **MVVM** (Model-View-ViewModel) pattern, the framework is flexible enough to support a variety of architectural approaches.

The built-in binding system supports advanced value transformations, custom functions with dynamic inputs, type conversions, multithreading, and customizable scheduling.

## Declarative C++ UI Definition

Brisk eliminates the need for markup languages in defining user interfaces. Instead, widgets and their properties are described directly in C++ code using a **declarative style**.

For example:
```c++
// Label with text defined at creation
Rc<Widget> makeLabel(std::string text) {
    return rcnew Widget{
        padding = 4_apx,
        classes = { "label" },
        rcnew Text{ std::move(text) },
    };
}
```

`padding` and `classes` are properties of the `Widget` class.

Here’s an example of creating a `Slider` widget, accompanied by a `Text` widget that displays its current value. The slider binds to a `value` variable, while the `Text` widget listens for changes and updates the displayed text accordingly.

```c++
// Slider with a dynamic value display
Rc<Widget> makeSlider(float& value) {
    return rcnew HLayout{
        rcnew Slider{
            // Bind the value of the slider to the provided 'value' variable.
            value         = Value{ &value },
            minimum       = 0.f,
            maximum       = 100.f,
            width         = 250_px,

            // Define a hintFormatter that formats the tooltip text
            // as "x=value" with one decimal place.
            hintFormatter = "x={:.1f}",
            borderColor   = 0x00e1f6_rgb,
        },

        gapColumn = 10_px,

        // Create a Text widget to display the value of the slider.
        rcnew Text{
            // The text is dynamically generated based on the slider value.
            text = Value{ &value }.transform([](float v) {
                return fmt::format("Value: {:.1f}", v);
            }),
        },
    };
}
```

### Layout

Brisk supports the **CSS Flexbox** layout model, enabling flexible and responsive widget arrangements. This ensures efficient alignment and spacing of widgets with minimal configuration.

## Powerful Binding System

Brisk's data binding system revolves around the `Property` and `Value` abstractions. Any C++ value (such as `int`, `std::string`, or custom classes) can be wrapped in a `Value` structure and used as a source or target for binding, provided that the memory is registered with the binding system.

In the example below, the `Text` widget binds its `text` property to a `temperature` value, while its `color` property responds dynamically to temperature changes. The `hidden` property toggles every half-second based on the global `frameStartTime` variable, creating a blinking effect.

```c++
// Temperature widget with dynamic text and color
float temperature = 16.f;

Rc<Widget> makeTemperatureWidget() {
    return rcnew Text{
        text = Value{ &temperature }.transform([](float t){
            return fmt::format("{:.1f}°C", t);
        }),

        color = Value{ &temperature }.transform([](float t){
            return t >= 40.f ? Palette::red : Palette::green;
        }),

        hidden = Value{ &frameStartTime }.transform([](double time) {
            return time - std::floor(time) < 0.5;
        }),
    };
}
```

Brisk's binding system is highly flexible. It supports value transformations, multithreading, and low-level bindings for greater control over data synchronization and task scheduling.

## Dynamic Widget Trees with Builders

In addition to supporting dynamic widget properties, Brisk allows the entire widget tree to be regenerated dynamically through the `Builder` concept. A `Builder` is a function that populates a target widget with dynamically created child widgets. Brisk automatically re-evaluates builders when a bound value changes, efficiently updating the widget tree.

```c++
// Dynamically created widget tree
static int count = 1;

Rc<Widget> makeTree() {
    return rcnew VLayout{
        rcnew Text{ "Squares:" },
        Builder{ [](Widget* target){
            for (int i = 1; i <= count; ++i) {
                target->apply(rcnew Text{ fmt::format("{}^2 = {}", i, i * i) });
            }
        }},
        depends = Value{ &count },

        rcnew Button{
            rcnew Text{ "Next" },
            onClick = [](){
                bindings->assign(count, count + 1);
            },
        },
    };
}
```

## GPU-Accelerated Graphics

Brisk includes a custom graphics engine optimized for high-performance rendering. It leverages platform-specific 3D APIs, such as D3D11, Metal, and Vulkan, to provide GPU-accelerated graphics. This is especially beneficial for applications with complex graphical user interfaces.

Brisk employs Signed Distance Field (SDF) techniques for efficient rendering of simple shapes, ideal for widget drawing. Additionally, Brisk supports loading various image formats, including SVG (Scalable Vector Graphics) rasterization.

Here’s an example of rendering a rounded rectangle using Brisk’s graphics API:

```c++
// Rendering a rounded rectangle
void renderBox(RenderContext& context, Rectangle rect, float radius) {
    const float strokeWidth = 2.f;

    // Create a canvas object to draw on, using the provided render context.
    Canvas canvas(context);
    Path path;
    path.addRoundRect(rect, radius);

    canvas.setStrokeWidth(strokeWidth);
    canvas.setStrokeColor(Palette::black);

    // Create a linear gradient and add colors.
    Gradient grad(GradientType::Linear, rect.at(0.1f, 0.1f),
                  rect.at(0.9f, 0.9f));
    grad.addStop(0.f, Palette::Standard::green);
    grad.addStop(1.f, Palette::Standard::red);

    canvas.setFillPaint(grad);
    canvas.fillPath(path);
    canvas.strokePath(path);
}
```

## Text Handling

Brisk uses UTF-8 encoding internally and provides utilities for converting between UTF-8, UTF-16, and UTF-32. It also supports Unicode normalization, grapheme splitting, and proper line breaking. Brisk fully supports Left-to-Right (LTR), Right-to-Left (RTL), and bidirectional text, as well as complex text shaping with OpenType features.

For example, the `Text` widget is highly configurable with respect to font features and text formatting:

```c++
// Configuring text with OpenType features
rcnew Text{
    text           = Value{ &m_text },
    fontSize       = 40,
    fontFamily     = "Lato",
    fontFeatures =
        OpenTypeFeatureFlags{
            { OpenTypeFeature::salt, false },
            { OpenTypeFeature::liga, true },
            { OpenTypeFeature::kern, true },
        },
    letterSpacing  = 0,
    wordSpacing    = 2_px,
    textDecoration = TextDecoration::Underline,
}
```

## Additional Examples

```c++
// Example of using Switch, ComboBox, and dynamic widget creation
const NameValueOrderedList<TextAlign> textAlignList{ { "Start", TextAlign::Start },
                                                     { "Center", TextAlign::Center },
                                                     { "End", TextAlign::End } };

class Example : public Component {
public:
    Rc<Widget> build() final {
        // rcnew Widget{...} is equivalent to std::shared_ptr<Widget>(new Widget{...})
        return rcnew Widget{
            layout = Layout::Vertical,
            rcnew Text{
                "Switch (widgets/Switch.hpp)",
                classes = { "section-header" }, // Widgets can be styled using stylesheets
            },

            rcnew HLayout{
                rcnew Widget{
                    rcnew Switch{
                        // Bind the switch value to the m_toggled variable (bidirectional)
                        value = Value{ &m_toggled },
                        rcnew Text{ "Switch" },
                    },
                },
                gapColumn = 10_apx, // CSS Flex-like properties
                rcnew Text{
                    text = Value{ &m_label }, // Text may be dynamic
                    visible =
                        Value{ &m_toggled }, // The Switch widget controls the visibility of this text widget
                },
            },

            // Button widget
            rcnew Button{
                rcnew Text{ "Click" },
                // Using lifetime() ensures that callbacks will be detached once the Component is deleted
                onClick = lifetime() |
                          [this]() {
                              // Notify bindings about the change
                    bindings->assign(m_label) = "Updated text";
                },
            },

            // ComboBox widget
            rcnew ComboBox{
                Value{ &m_textAlignment },  // Bind ComboBox value to an enumeration
                notManaged(&textAlignList), // Pass the list of name-value pairs to populate the ComboBox
            },

            // The Builder creates widgets dynamically whenever needed
            Builder([this](Widget* target) {
                for (int i = 0; i < m_number; ++i) {
                    target->apply(rcnew Widget{
                        dimensions = { 40_apx, 40_apx },
                    });
                }
            }),
            depends = Value{ &m_number }, // Instructs to rebuild this if m_number changes
        };
    }

private:
    bool m_toggled            = false;
    TextAlign m_textAlignment = TextAlign::Start;
    std::string m_label       = "OK";
    float m_progress          = 0;
    int m_number              = 0;
};
```
