# "Hello, World" GUI Application with Brisk

This tutorial will guide you through creating a basic "Hello, World" application using the Brisk framework.

## Prerequisites

- A functioning C++20 compiler
- Brisk library binary package and its dependencies (either downloaded or built)

Refer to the [Getting Started](../index.md#getting-started) tutorial for detailed instructions.

## Step 1: Create the Project Files

The "Hello, World" project will consist of three main files:

1. `CMakeLists.txt` – Project setup
2. `main.cpp` – Application code
3. `icon.png` – Brisk icon (must be in PNG format, with a minimum size of 512x512). Download an example icon from [https://github.com/brisklib/brisk-helloworld/raw/main/icon.png](https://github.com/brisklib/brisk-helloworld/raw/main/icon.png){target="_blank"}.

### CMakeLists.txt

Create a `CMakeLists.txt` file in your project directory with the following contents:

```cmake title="CMakeLists.txt"
# Minimum supported CMake version is 3.22
cmake_minimum_required(VERSION 3.22)

project(HelloWorldApp)

# Locate Brisk libraries and headers
find_package(Brisk CONFIG REQUIRED)

# Define application metadata
brisk_metadata(
    VENDOR "Brisk"                     # Vendor or company name
    NAME "HelloWorldApp"               # Application name
    DESCRIPTION "Brisk Hello World"    # Short application description
    VERSION "0.1.0"                    # Version number
    COPYRIGHT "© 2025 Brisk"           # Copyright information
    ICON ${CMAKE_SOURCE_DIR}/icon.png  # Path to the icon (PNG)
    APPLE_BUNDLE_ID com.brisklib.helloworld # Apple bundle identifier
)

# Create an executable target 'main' from main.cpp
add_executable(main main.cpp)

# Link necessary Brisk libraries to 'main'
target_link_libraries(main PRIVATE Brisk::Widgets Brisk::Executable)

# Set up the executable 'main' with Brisk icons, metadata, and startup/shutdown code
brisk_setup_executable(main)
```

### main.cpp

Create a `main.cpp` file in the same directory with the following code:

```cpp title="main.cpp"
#include <brisk/gui/Component.hpp>
#include <brisk/gui/GuiApplication.hpp>
#include <brisk/widgets/Graphene.hpp>
#include <brisk/widgets/Button.hpp>
#include <brisk/widgets/Layouts.hpp>
#include <brisk/widgets/Text.hpp>

using namespace Brisk;

// Root component of the application, inherits from Brisk's Component class
class RootComponent : public Component {
public:
    // Builds the UI layout for the component
    Rc<Widget> build() final {
        return rcnew VLayout{
            stylesheet = Graphene::stylesheet(), // Apply the default stylesheet
            Graphene::darkColors(),              // Use dark color scheme
            gapRow = 8_px,                       // Set vertical gap between elements
            alignItems = AlignItems::Center,     // Align child widgets to the center
            justifyContent = Justify::Center,    // Center the layout in the parent
            rcnew Text{"Hello, world"},          // Display a text widget with "Hello, world"
            rcnew Button{
                rcnew Text{"Quit"},              // Button label
                onClick = lifetime() | []() {    // Quit the application on button click
                    windowApplication->quit();
                },
            },
        };
    }
};

// Entry point of the Brisk application
int briskMain() {
    GuiApplication application; // Create the GUI application
    return application.run(createComponent<RootComponent>()); // Run with RootComponent as main component
}
```

### icon.png

Place an icon named `icon.png` in your project directory. You can download an example from the [Brisk repository](https://github.com/brisklib/brisk-helloworld/raw/main/icon.png).

## Step 2: Build the Application

With all files in place, you can now build the project.

=== "macOS"
    === "Generating Xcode project"
        ```bash
        mkdir build
        cmake -GXcode -S . -B build \
            -DCMAKE_PREFIX_PATH=<path-to-brisk>/lib/cmake \
            -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake \
            -DVCPKG_INSTALLED_DIR=<path-to-installed> \
            -DVCPKG_TARGET_TRIPLET=uni-osx \
            -DVCPKG_HOST_TRIPLET=uni-osx
        ```

        The project files will be placed in the `build` directory and can be opened in Xcode.

    === "Building using Ninja"
        ```bash
        mkdir build
        cmake -GNinja -S . -B build \
            -DCMAKE_PREFIX_PATH=<path-to-brisk>/lib/cmake \
            -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake \
            -DVCPKG_INSTALLED_DIR=<path-to-installed> \
            -DVCPKG_TARGET_TRIPLET=uni-osx \
            -DVCPKG_HOST_TRIPLET=uni-osx
        cmake --build build
        ```

=== "Linux"

    === "Building using Ninja"
        ```bash
        mkdir build
        cmake -GNinja -S . -B build \
            -DCMAKE_PREFIX_PATH=<path-to-brisk>/lib/cmake \
            -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake \
            -DVCPKG_INSTALLED_DIR=<path-to-installed> \
            -DVCPKG_TARGET_TRIPLET=x64-linux \
            -DVCPKG_HOST_TRIPLET=x64-linux
        cmake --build build
        ```

=== "Windows"

    === "Generating Visual Studio project"
        Use PowerShell to execute the commands below.

        ```powershell
        mkdir build
        cmake -G"Visual Studio 17 2022"  -S . -B build `
            -DCMAKE_PREFIX_PATH=<path-to-brisk>\lib\cmake `
            -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>\scripts\buildsystems\vcpkg.cmake `
            -DVCPKG_INSTALLED_DIR=<path-to-installed> `
            -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
            -DVCPKG_HOST_TRIPLET=x64-windows-static-md
        ```

        The project files will be placed in the `build` directory and can be opened in Visual Studio.

    === "Building using Ninja"
        Use the Visual Studio Developer Command Prompt (PowerShell) to execute the commands below.

        ```powershell
        mkdir build
        cmake -GNinja -S . -B build `
            -DCMAKE_PREFIX_PATH=<path-to-brisk>/lib/cmake `
            -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake `
            -DVCPKG_INSTALLED_DIR=<path-to-installed> `
            -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
            -DVCPKG_HOST_TRIPLET=x64-windows-static-md
        cmake --build build
        ```

### Path Substitution

Depending on how you set up Brisk and its dependencies, replace the following placeholder paths accordingly:

#### **Using Prebuilt Dependencies & Brisk Binaries**

* Replace `path-to-brisk` with the directory where the `Brisk-Prebuilt-...tar.xz` archive was extracted.
* Replace `path-to-vcpkg` with the directory containing Vcpkg. If Vcpkg is not installed globally, use the directory where `Brisk-Dependencies-...tar.xz` was extracted.
* `path-to-installed` should point to the `installed` subdirectory inside the extracted `Brisk-Dependencies-...tar.xz` directory.

#### **Using Prebuilt Dependencies but Building Brisk from Source**

* Replace `path-to-brisk` with the `CMAKE_INSTALL_PREFIX` path you used (usually the `dist` subdirectory in the Brisk repository).
* Replace `path-to-vcpkg` with the Vcpkg directory. If not installed globally, use the `vcpkg_exported` directory inside the Brisk repository.
* `path-to-installed` should point to the `vcpkg_installed` directory within the Brisk repository.

#### **Building Brisk and Dependencies from Source**

* Replace `path-to-brisk` with the `CMAKE_INSTALL_PREFIX` path (usually the `dist` subdirectory in the Brisk repository).
* Replace `path-to-vcpkg` with the Vcpkg directory. If Vcpkg is not installed globally, Brisk should have downloaded and installed a local copy in the repository during the first build.
* `path-to-installed` should point to the `vcpkg_installed` directory within the Brisk repository.

After a successful build, the `main` executable should be available in the build directory.


## Step 3: Run the Application

Execute the `main` file to run the application. You should see a window displaying "Hello, World" text with a "Quit" button that closes the application when clicked.

Congratulations! You've built your first "Hello, World" application with Brisk.
