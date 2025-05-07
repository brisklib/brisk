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
3. `icon.png` – Brisk icon (must be in PNG format, with a minimum size of 512x512). Download an example icon from https://github.com/brisklib/brisk-helloworld/raw/main/icon.png.

### CMakeLists.txt

Create a `CMakeLists.txt` file in your project directory with the following contents:

```cmake
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

```cpp
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

Place an icon named `icon.png` in your project directory. You can download an example from [this link](https://github.com/brisklib/brisk-helloworld/raw/main/icon.png).

## Step 2: Build the Application

With all files in place, you can now build the project.

1. Open a terminal in your project directory.
2. Run the following commands to create a build directory, configure the project, set the required CMake variables, and build the executable:

   ```bash
   mkdir build
   cmake -S . -B build \
     -DCMAKE_PREFIX_PATH=<path-to-brisk/lib/cmake> \
     -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg/scripts/buildsystems/vcpkg.cmake> \
     -DVCPKG_INSTALLED_DIR=<path-to-vcpkg-installed>
   cmake --build build
   ```

   For Windows you should also set the triplet: `-DVCPKG_TARGET_TRIPLET=x64-windows-static-md`.

   Replace `path-to-brisk/lib/cmake` with the actual path to the `lib/cmake` directory inside the Brisk package, `<path-to-vcpkg/scripts/buildsystems/vcpkg.cmake>` with the path to the Vcpkg toolchain file, and `<path-to-vcpkg-installed>` with the path to your Vcpkg `installed` directory.

3. After a successful build, you should have an executable named `main` in the build directory.

## Step 3: Run the Application

Execute the `main` file to run the application. You should see a window displaying "Hello, World" text with a "Quit" button that closes the application when clicked.

Congratulations! You've built your first "Hello, World" application with Brisk.
