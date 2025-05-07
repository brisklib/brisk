# Getting Started

This tutorial assumes familiarity with Git, CMake, and C++. Additionally, a modern C++ compiler is required. For setup details, refer to [Installing Prerequisites](prerequisites.md).

## Fetching the Brisk Source Code

### Latest Version

The `main` branch contains the latest features and stable code. New code is merged into `main` only after passing all tests, ensuring this branch represents the most up-to-date, stable version of the Brisk library.

To clone the `main` branch into a `brisk` subdirectory:

```bash
git clone https://github.com/brisklib/brisk.git brisk
```

To update to the latest version:

```bash
cd path/to/brisk/repo
git pull
```

Alternatively, you can download the sources as a `.zip` archive directly from GitHub: https://github.com/brisklib/brisk/archive/refs/heads/main.zip

In this case, updating requires downloading a new archive and replacing the directory contents.

## Building Brisk

> [!note]
> You can also download prebuilt binaries from our build server (see [Prebuilt Binaries](prebuilt_binaries.md)). Extract the archive to a directory and skip this section.

The recommended way to build the Brisk library is with Ninja:

```bash
cd path/to/brisk/repo
cmake -GNinja -S . -B build-release -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target install
cmake -GNinja -S . -B build-debug -DCMAKE_INSTALL_PREFIX=dist -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --target install
```

`CMAKE_INSTALL_PREFIX` specifies the directory where the static libraries and header files will be placed after building.

On Windows, these commands should be executed in the Visual Studio Developer Command Prompt.

This command will export all Brisk dependencies to the `vcpkg_exported` directory:
```bash
cd path/to/brisk/repo
vcpkg export --raw --output-dir=. --output=vcpkg_exported
```

> [!note]
> If Vcpkg is not installed globally, Brisk will check out the Vcpkg repository at `path/to/brisk/repo/vcpkg` and use this local copy to build dependencies. In this case, the command above should be modified as follows:
> - For Linux/macOS: `vcpkg/vcpkg export --raw --output-dir=. --output=vcpkg_exported`
> - For Windows: `vcpkg\vcpkg export --raw --output-dir=. --output=vcpkg_exported`

After executing the above commands, you’ll get the following directories:

1. `dist` — stores Brisk’s static libraries and headers.
2. `vcpkg_exported` — contains all dependencies needed for building and linking applications with Brisk.

These directories are relocatable, so you can move them to another directory or computer if needed.

### Vcpkg Triplets

Brisk is tested with the following triplets:

|       | Linux                        | macOS                        | Windows (static runtime)                | Windows (dynamic runtime)                  |
|-------|------------------------------|------------------------------|-----------------------------------------|--------------------------------------------|
| x86   | —                            | —                            | :white_check_mark: x86-windows-static   | :white_check_mark: x86-windows-static-md   |
| x64   | :white_check_mark: x64-linux | :white_check_mark: x64-osx   | :white_check_mark: x64-windows-static   | :white_check_mark: x64-windows-static-md   |
| arm   | :construction:               | :construction:               | :construction:                          | :construction:                             |
| arm64 | :construction:               | :white_check_mark: arm64-osx | :white_check_mark: arm64-windows-static | :white_check_mark: arm64-windows-static-md |

> [!warning]
> Currently, Brisk supports only static linking of dependencies; dynamic linking for Brisk libraries will be available in a future release.

## Configuring Your Project with Brisk

Below is an example of how to use the Brisk library in a CMake project.

```cmake
cmake_minimum_required(VERSION 3.22)
project(yourproject)

# Locate Brisk libraries and headers
find_package(Brisk CONFIG REQUIRED)

# Define application metadata
brisk_metadata(
    VENDOR "Brisk"                     # Vendor or company name
    NAME "Example"                     # Application name
    DESCRIPTION "Brisk example"        # Short application description
    VERSION "0.1.2.3"                  # Version number
    COPYRIGHT "© 2025 Brisk"           # Copyright information
    ICON ${CMAKE_SOURCE_DIR}/icon.png  # Path to the icon (PNG)
    APPLE_BUNDLE_ID com.brisklib.main  # Apple bundle identifier
)

# Create an executable target 'main' from main.cpp
add_executable(main main.cpp)

# Link necessary Brisk libraries to 'main'
target_link_libraries(main PRIVATE Brisk::Widgets Brisk::Executable)

# Set up the executable 'main' with Brisk icons, metadata, and startup/shutdown code
brisk_setup_executable(main)
```

You’ll need to configure CMake to point to the two directories created in the previous steps (or downloaded from the build servers).

Here’s an example for the `vscode-cmake-tools` VSCode extension:

`settings.json`
```json
{
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "<dist>/lib/cmake",
        "CMAKE_TOOLCHAIN_FILE": "<vcpkg_exported>/scripts/buildsystems/vcpkg.cmake",
        "VCPKG_TARGET_TRIPLET": "<triplet>",
        "VCPKG_INSTALLED_DIR": "<dist>/vcpkg/installed"
    }
}
```

Corresponding command line:

```bash
cmake ... -DCMAKE_PREFIX_PATH="<dist>/lib/cmake" -DCMAKE_TOOLCHAIN_FILE="<vcpkg_exported>/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET="<triplet>" -DVCPKG_INSTALLED_DIR="<dist>/vcpkg/installed"
```

Replace `<dist>`, `<vcpkg_exported>`, and `<triplet>` with your actual paths and triplet values.

## `add_subdirectory` Method

Another method for including Brisk in your CMake-based project is by using `add_subdirectory`. This allows for tighter integration and enables automatic rebuilds whenever Brisk sources change. This approach is particularly suitable if you are developing the Brisk library alongside your application.

The `BRISK_DIR` CMake variable must be set to the Brisk source directory.

```cmake
cmake_minimum_required(VERSION 3.22)

# Initialize Brisk CMake modules. This must be done before the first project directive.
include(${BRISK_DIR}/cmake/brisk.cmake)

project(brisk-example)

# Include Brisk source. This must be done after the first project directive.
add_subdirectory(${BRISK_DIR} brisk-bin)

add_executable(main ...)

# Link the Brisk libraries to the 'main' target.
target_link_libraries(main PRIVATE Brisk::Widgets Brisk::Executable ...)

# Perform additional setup tasks specific to the executable target 'main'.
brisk_setup_executable(main)
```
