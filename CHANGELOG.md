# Brisk Changlog

## 0.9.8

### Renames

- **C++**
  - Standardized capitalization for abbreviations, treating them as normal words (e.g., `HTTPResponse` → `HttpResponse`).
  - `GUIApplication` to `GuiApplication` and `GUIWindow` to `GuiWindow`.
  - `separateRenderThread` to `separateUiThread`.
  - `Palette::light_green` to `Palette::lightGreen` and similar color names.
  - `ItemList` to `Menu`.
  - `BindingObject` to `BindableObject`.
  - `KeyCode::Kp*` to `KeyCode::Num*`.
  - `Os*Handle` to `Native*Handle`.
  - `setMinimumMaximumSize` to `setMinimumMaximumSizes`.
  - `wordWrap` function to `textWordWrap`.
- **CMake**
  - `APPLE_BUNDLE` to `APPLE_BUNDLE_ID`.
- **Directories**
  - Some directories were also renamed to reflect changes in class naming. If you encounter issues with pulling updates, clone the repository into a separate directory.

### Added

- Added app- and vendor-specific folders to the `DefaultFolder` enum.
- Enabled access to the WebGPU API from Brisk applications.
- Introduced `WebGpuWidget` for displaying 3D content within a widget.
- Added new example: WebGPU demo.
- Improved uniform scaling support in `Matrix`.
- Added `_fmt` user-defined literal for string formatting (e.g., `"x={}"_fmt(x)`).
- Implemented automatic signed distance field optimizations for `Canvas`.
- Enabled drawing rectangles with individual border radii.
- Added `LinearGradient` and `RadialGradient` classes.
- Introduced texture blur functionality.
- Added style variables for focus frame and hint.
- Enabled measurement of GPU execution time.
- Added support for premultiplying alpha during decoding.
- Allowed disabling RTTI for Brisk.
- Allowed disabling exceptions for Brisk.
- Supported reversed subpixel order.
- Enabled retrieval of the display associated with a window.
- Added ability to create a render device for a specific display.
- Supported rendering UI to an offscreen buffer.
- Added option to skip frames when no UI changes occur.
- Provided full support for `Length` debug visualization.
- Added new example: Rendering to an external window.
- Added new example: Splash screen.
- Introduced shadow spread and shadow offset properties.
- Allowed use of a custom `main` function.
- Added a safe nullable pointer wrapper.
- Added new example: Rendering GUI to an external window.
- Supported nested menus and improved menu accessibility.
- Added `cpuUsage` function.
- Introduced component actions.
- Enabled application-wide shortcut support.
- Added `Widget::selected` property.
- Introduced `ListBox` widget.
- Enabled retrieval of native texture from `Image`.
- Added new example: Dashboard.

### Changed

- Updated documentation for improved clarity and completeness.
- Refactored `SIMD<>` for better optimization.
- Made D3D11 backend optional on Win32 when WebGPU is enabled.
- Changed `ImageRenderTarget` to use BGRA color format (previously RGBA).
- Updated `Gradient` to use premultiplied colors.
- Set `ColorW` as the default color type.
- Made `Gradient` a stack-allocated object.
- Removed `RawCanvas` in favor of using `Canvas` universally.
- Added new renderer debug visualization.
- Improved angle gradient antialiasing at the 360°/0° boundary.
- Updated Google Dawn to version 7080.
- Optimized window rendering.
- Refactored color spaces for better consistency.
- Optimized texture blitting.
- Made `InputQueue` explicitly stored in `WidgetTree`.
- Enabled receiving non-client mouse clicks.
- Improved focus handling: if a widget does not accept focus, the first descendant that does is selected.
- Split `overflow` property into `overflowScroll` and `contentOverflow`.
- Removed the following classes and functions:
  - `PerformanceStatistics`
  - `DeferredCallback`
  - `threadScheduler`

### Fixed

- Reduced blinking when closing a modal window.
- Preserved order in `VisualGroup`.
- Fixed widget border rendering issues.
- Corrected `incbin.h` for 32-bit Windows ABI compatibility.
- Allowed closing component windows from any thread.
- Ensured both host and target tools are built during cross-compilation.
- Prevented window centering unless its size is changed.

## 0.9.7

A purely bug-fix release.

- Updated ICU data loading code to resolve issues on macOS ARM64.
- Fixed a WebGPU memory leak.
- Ensured the D3D11 render context is flushed after every batch.
- Increased the minimum required CMake version to 3.22.
- Fixed `imageResize`.

## 0.9.6

❗ indicates a breaking change.

### Added

- Added support for SVG fonts🖋️💻.
- Added support for color emojis🔤😊.
- Resource bundling has been rewritten from scratch. Apps can now override Brisk resources, such as supplying their own fonts.
- Support for viewport-relative units (vw/vh/vmax/vmin)
- ❗ Buffered GUI mode has been introduced. The GUI is rendered to an offscreen texture, tracking the updated rectangles each frame. The texture is then blitted to the screen, significantly improving performance in cases where GUI updates are infrequent. This mode can be switched off for a specific window or globally.
- Lucide icons have been updated to latest version.
- Ability to override `ItemList` visible property.
- `WidgetTree` can now be used without a `GUIWindow` and `InputQueue`.
- New algorithm for shadow rendering by Evan Wallace.
- A new `ColorW` type has been introduced, capable of storing extended-range sRGB values in a compact form.
- `storeWidget` function to store the weak pointer to the widget to an external variable during tree building.
- `string_view` versions of the `trim`, `ltrim` and `rtrim` functions.
- `BRISK_ASSUME` macro that maps to `__builtin_assume` on supported compilers.
- `SVGImage::renderTo` to render to an existing `Image`.
- Resources are now packed into a binary using the `.incbin` assembly instruction on supported compilers for improved performance.
- HTML SAX parser added.
- Very basic HTML support in the `Text` widget via `textOption = TextOptions::HTML`.
- Stateless functions in `Canvas` class.
- Added the ability to select the internationalization implementation: basic or ICU-based. Apps can now choose the implementation without rebuilding the Brisk binaries.
- Visual tests for widgets.

### Changed

- ❗ A new implementation of widget clipping has been introduced. Widget painting can now be clipped to the parent's bounds, the parent's clip rectangle, or the widget's own bounds.
- ❗ `DialogComponent::result` is now a `Property` instead of a plain field.  
  Use `result.get()` to retrieve the boolean value or rely on implicit conversion.  
  The same applies to the `value`, `prompt`, `text`, and `icon` fields in derived dialogs.
- ❗ `buttonColor` has been renamed to `mainColor`.
- ❗ The `layoutOptions` property of the `Text` widget has been renamed to `textOptions`, and the `LayoutOptions` type is now `TextOptions`.
- ❗ The `createWindow` function should now be overridden instead of `makeWindow` to use a custom window class for the component.
- ❗ A new `isHintVisible` property controls whether the widget's hint is visible.  
- ❗ The `description` property has been removed in favor of the `hint` property.
- ❗ New font size default (12 pixels).
- ❗ The `reflection` static field is now used to store field information (previously `Reflection`).
- ❗ Style variables (e.g., `selectedColor`) are now indexed by string hash instead of a linear index, making it easier to create custom variables.
- `ComboBox` and `PopupBox` mouse interaction fixes.
- `ImplicitContext` is now convertible to `bool`.
- The descender value of Lucide icons has been adjusted to improve their alignment within text.
- The `Knob` and `Slider` colors can be changed by assigning the `selectedColor` style variable. Hover and pressed styles are preserved.
- `Showcase` app updated.
- The `ReflectionOf` utility class has been introduced to provide reflection information for external types.
- ❗ Shell-related functions moved to the `Shell` class: `Shell::showMessage`, `Shell::openURLInBrowser`, `Shell::showDialog`, etc.
- ❗ Clipboard-related functions moved to the `Clipboard` class: `Resources::getText`, etc.
- ❗ Resource-related functions moved to the `Resources` class: `Resources::load`, etc.
- ❗ `InplacePtr` and `PossiblyShared` types are removed.
- ❗ `uiThread` has been renamed to `uiScheduler` to match `mainScheduler`.
- ❗ `std::optional` and `std::nullopt` are no more included in Brisk namespaces. Use full qualified names instead.
- ❗ File IO functions now use `uint64_t` instead of `uintmax_t`.
- ❗ Several renames have been made to ensure style consistency (`bytes` → `Bytes`, `bytes_view` → `BytesView`).
- ❗ `Bytes`, `BytesView`, and other related types now use `std::byte` instead of `uint8_t`.
- ❗ The lifetime of `BindingObject` is now accessed via the `lifetime` function. Replace occurrences of `m_lifetime` with `lifetime()` in your code.
- ❗ Font name constants moved to the `Font` class: `Fonts::Monospace` → `Font::Monospace`.
- Documentation files have been updated.
- ❗ Fonts are now set by their names, not IDs. Examples: `fontFamily = "Lato"`, `Font{"Noto", 16}`.
- Performance optimizations.
- New `WrapAnywhere` flag in `TextOptions`.
- ❗ Aliases to `std` types removed.

### Fixed

- Numerous fixes and improvements.
- `BRISK_SCRIPTS_DIR` is now cached in `CMakeCache.txt`, resolving issues when Brisk is included using `add_subdirectory`.
- `Rectangle::empty` has been fixed to return `true` for `{INT_MAX, INT_MAX, INT_MIN, INT_MIN}`.
- `BackStrikedText` now uses opacity for painting.
- Fixed contrast ratio calculation, ensuring that light text color is preferred on certain backgrounds.
- `findIntegralType` fixes.
- Brisk now supports spaces around commas in the font list ("Lato, Noto").

## 0.9.5

### Added

- Scrolling for every Widget in #20
- Windows ARM64 support in #21

## 0.9.4

This is a bug-fix release that includes improvements to exception safety, memory alignment, and the resolution of several bugs.

## 0.9.3

* Updated the showcase demo app with additional features
* Fixed screenshot capture issues caused by nested event loops
* Added `""_Text` UDL as an alias for `new Text{""}`
* Introduced a `transform` function that accepts multiple `Value`s and a lambda
* Refactored package building process
* Reduced the size of release binaries
* Expanded and improved documentation
* Updated CxxDox to the latest version
* Enabled testing of demo app building in GitHub Actions
* Disabled inlining enforcement in debug mode
* Applied layout fixes
* Prevented letter spacing from disabling kerning
* Refactored `TextEditor` and `Pages` components
* Added properties for `ScrollBox`
* Various fixes and general improvements

## 0.9.2

### Added
* New `BRISK_ICU` CMake option to control ICU inclusion
* Added support for multiline text editors
* Added `wordWrap` property for the `Text` widget to control word breaking
* Added `dynamicFocus` property for the `Item` widget
* Added support for *-windows-static vcpkg triplets
* New `fontFeatures` property
* Enhanced OS dialogs (file and folder dialogs, message boxes)
* Updated vcpkg to 2024.09.23
* Wayland support for Linux
* Temporarily disabling style transitions

### Changed
* Updated example projects
* Optimized dependency size
* Improved Linux support
* Major update to the font engine
* Improved text rendering
* Enhanced `.natvis` support
* Stylesheet updates
* Documentation improvements
* Text widget: improved caching
* vcpkg: Use Clang if available

### Fixed
* Fixed `trim` function
* Fixed `drawImage` coordinates
* Fixed WebP reading issues
* Fixed selected state behavior
* Improved keyboard focus behavior
* Various performance optimizations
