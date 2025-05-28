# Brisk

**Brisk** is a modern, cross-platform C++ GUI framework designed to build responsive, high-performance applications with flexibility and ease.

Initially developed for a graphics-intensive proprietary project with a complex and dynamic GUI.

> [!Note]
> The Brisk library is currently under active development. Breaking changes may occur, and the documentation might not always be up to date. Contributions are always welcome!

Recommended reading:

➡️ [Brisk Design and Feature Overview](https://docs.brisklib.com/about/) ⬅️

[Documentation on docs.brisklib.com](https://docs.brisklib.com)

![Build](https://img.shields.io/github/actions/workflow/status/brisklib/brisk/test.yml?style=flat-square&label=Build)
![License](https://img.shields.io/badge/License-GPL2%2FCommercial-blue.svg?style=flat-square)

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat-square)
![Clang 16+](https://img.shields.io/badge/Clang-16%2B-green.svg?style=flat-square)
![GCC 12+](https://img.shields.io/badge/GCC-12%2B-green.svg?style=flat-square)
![MSVC 2022](https://img.shields.io/badge/MSVC-2022%2B-green.svg?style=flat-square)
![Xcode 14.3+](https://img.shields.io/badge/Xcode-14.3%2B-green.svg?style=flat-square)

### Our other projects

<div align="center">

[**🔴 KFR**](https://github.com/kfrlib/kfr) is a modern C++ DSP library. Includes IIR/FIR filters + filter design, FFT/DFT, Convolution, Resampling. Optimized for all kinds of x86 and ARM architectures. *(GPL/Commercial)*

[**🟢 CxxDox**](https://github.com/kfrlib/cxxdox) — C++ documentation generator. *(MIT)*

</div>

### Key Features 🌟

- **Declarative GUI**: Enables the creation of complex GUIs in C++ using a flexible declarative syntax.
- **Stateful & Stateless Widgets**: Supports both modes, with powerful binding for efficient state management.
- **Stylesheets**: Offers declarative styles to easily modify the application's look and feel.
- **Hardware-Accelerated Graphics**: Backends include D3D11, D3D12, Vulkan, OpenGL, Metal, and WebGPU.
- **Color Processing**: Supports various color spaces and linear color blending for physically accurate rendering.
- **Font support**: OpenType support, advanced shaping for non-European languages, ligatures, rich text formatting, SVG fonts, emojis, and rendering for RTL and bidirectional text.
- **Modular Architecture**: Core, Graphics, Window, GUI, Network, and Widgets modules for versatile application development.

### Code Example

#### Component example

```c++
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
                              *bindings->modify(m_label) = "Updated text";
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

### Screenshots

<img src="docs/docs/images/Brisk-screenshot1.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot2.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot3.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot4.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot5.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot6.png" width="48%" alt="Brisk screenshot"/> <img src="docs/docs/images/Brisk-screenshot7.png" width="48%" alt="Brisk screenshot"/>

### Modules

#### **Core**
   - **Compression & Basic Cryptography**: Provides data compression and basic cryptographic functions, including hashing algorithms.
   - **Dynamic Library Loading**: Loads and interacts with `.dll` or `.so` files dynamically.
   - **String Manipulation**: Handles UTF-8, UTF-16, and UTF-32, with text manipulation utilities.
   - **Stream I/O**: Input/output operations for handling data streams.
   - **Localization Support**: Basic localization and internationalization features for multilingual applications.
   - **Logging**: Built-in logging framework for application diagnostics.
   - **Reflection**: Supports reflection.
   - **Serialization**: Serializes/deserializes data to/from JSON.
   - **App-Global Settings**: Manages global application settings.
   - **Threading**: Provides task queues for multi-threaded applications.
   - **Binding**: Supports value binding, capable of handling multi-threaded environments.

#### **Graphics**
   - **Color & Colorspaces**: Supports working with colors, including various colorspaces.
   - **Geometry**: Provides 2D geometry types like `Rect`, `Point`, `Size`, and 2D matrices for transformations.
   - **Canvas & Path**: Supports drawing with paths and Bézier curves.
   - **SVG Rasterization**: Renders SVG images into raster formats.
   - **Image Processing**: Supports image encoding, decoding, resizing, and manipulation.
   - **Font Handling**: Manages fonts, including loading, rendering, caching, and text layout. Supports advanced text shaping (using HarfBuzz) and SVG fonts (emojis).

#### **Window**
   - **Clipboard**: Provides clipboard access for copy/paste functionality.
   - **OS Dialogs**: Native dialogs for file open/save, folder selection, and message boxes.
   - **Display Information**: Retrieves and manages display/monitor information from the OS.
   - **Buffered rendering** : Brisk supports both buffered rendering, which enables partial repaints, and direct rendering to the window backbuffer.

#### **GUI**
   - **Widgets**: Includes a wide variety of widgets with CSS-style flex layout.
   - **Style Sheets**: Styles your widgets using a stylesheet system that supports property inheritance.
   Viewport-relative units are supported (`vh`, `vw`, `vmin`, `vmax)
   - **Binding Support**: Data-binding between UI elements and application data. Supports transforming values using a function on-the-fly and compound values (e.g., sums of other values).
   - **Stateful and Stateless Modes**: Choose between stateful widgets for persistent state or stateless widgets for easily rebuilding widget subtrees.
   - **Drag-and-Drop**: Supports drag-and-drop within the GUI, with the option to attach a C++ object to represent the dragged data.

#### **Widgets**
   - **Widgets**: Includes buttons, lists, comboboxes, toggle switches, radio buttons, progress bars, sliders, scroll boxes, checkboxes, popup buttons, tabs, tables, spin boxes, dialogs, and more. All public properties are styleable and bindable.
   - **Layouts**: Supports CSS flexbox-style layouts.
   - **Text Editors**: Provides text editing widgets with LTR and RTL text support.
   - **WebGPU**: Ability to render 3D content to the window using the WebGPU API (backed by Google's Dawn), with full support for composition with the UI.

### Requirements ⚙️
- **C++20 Compiler**: Brisk requires a C++20-compatible compiler such as MSVC 2022, Clang, XCode, or GCC.
- **Dependency Management**: Uses [vcpkg](https://github.com/microsoft/vcpkg) to manage dependencies across platforms, simplifying the build process. Alternatively, you can download prebuilt binaries.

### Platform Support

|                     | Windows | macOS | Linux |
|---------------------|---------|-------|-------|
| Core Functionality  | Beta    | Beta  | Beta  |
| Graphics            | Beta    | Beta  | Beta  |
| Window System       | Beta    | Beta  | Beta  |
| Widgets             | Beta    | Beta  | Beta  |
| Application Support | Alpha   | Alpha | N/A   |

#### OS Support

|         | Minimum version                 |
|---------|---------------------------------|
| Windows | Windows 10, Windows Server 2016 |
| macOS   | macOS 11 Big Sur                |
| Linux   | n/a                             |

#### Graphics Backend Support

|         | Backends                        |
|---------|---------------------------------|
| Windows | D3D11 and WebGPU (D3D12/Vulkan) |
| macOS   | WebGPU (Metal)                  |
| Linux   | WebGPU (OpenGL/Vulkan)          |

### Example Projects

The `examples` directory contains projects that showcase how to use the Brisk library.

For a minimal example, check out the [brisk-helloworld](https://github.com/brisklib/brisk-helloworld) repository.

Example Project Binary Size:

| OS          | Static Build, Release (Full Unicode Support)  |
|-------------|-----------------------------------------------|
| Windows x64 | **10.1 MB** (D3D11)                           |
| Linux x64   | **18.2 MB** (WebGPU: Vulkan/OpenGL), stripped |
| macOS x64   | **16.5 MB** (WebGPU: Metal), stripped         |

These sizes do not include embedded resources (such as fonts).

### Development 💻

Brisk is in active development, and we welcome contributions and feedback from the community to improve and expand the toolkit.

The `main` branch contains the latest features and generally passes all built-in tests ✅. Other branches are reserved for feature development and may be force-pushed.

### License 📜

Brisk is licensed under the **GPL v2.0** or later. However, for those who wish to use Brisk in proprietary or closed-source applications, a **commercial license** is also available. For more details on commercial licensing, please contact us at [brisk@brisklib.com](mailto:brisk@brisklib.com).
