# Building Brisk and Dependencies from Source

If you’ve already set up the prebuilt binaries, you can skip this article entirely.

## Installing build prerequisites

[See Installing Prerequisites](prerequisites.md) first to install packages needed to use Brisk.

=== "macOS"

    To install additional system dependencies needed to build Brisk from source, use the following command:

    ```bash
    brew install cmake git ninja autoconf automake autoconf-archive
    ```

=== "Linux"

    To install additional system dependencies needed to build Brisk from source on Ubuntu-derived distro, use this command:

    ```bash
    sudo apt-get install ninja-build mesa-vulkan-drivers vulkan-tools wget xorg-dev libgl-dev libgl1-mesa-dev libvulkan-dev autoconf autoconf-archive libxrandr-dev libxinerama-dev libxcursor-dev mesa-common-dev libx11-xcb-dev libwayland-dev libxkbcommon-dev
    ```

## Fetching the Brisk Source Code

### Latest Version

The `main` branch contains the latest features and stable code. New code is merged into `main` only after passing all tests, ensuring this branch represents the most up-to-date, stable version of the Brisk library.

To clone the `main` branch into a `brisk` subdirectory:

```bash
git clone https://github.com/brisklib/brisk.git brisk
```

I you’ve already cloned the Brisk repository and want to update it to the latest version:

```bash
cd path/to/brisk/repo
git pull
```

Alternatively, but not recommended, you can download the sources as a `.zip` archive directly from GitHub: https://github.com/brisklib/brisk/archive/refs/heads/main.zip

In this case, updating requires downloading a new archive and replacing the directory contents.

## Building Brisk (Prebuilt Dependencies)

!!! tip "Prebuilt Binaries"
    Building Brisk and its dependencies may take a significant amount of time on your system.
    Alternatively, you can download prebuilt binaries from our build server (see [Prebuilt Binaries](prebuilt_binaries.md)).

To acquire the prebuilt dependencies, execute the following command in the Brisk repository directory:

=== "macOS"

    ```bash
    cmake -DVCPKG_TARGET_TRIPLET=uni-osx -P acquire-deps.cmake
    ```

=== "Linux"

    ```bash
    cmake -DVCPKG_TARGET_TRIPLET=x64-linux -P acquire-deps.cmake
    ```

=== "Windows"

    ```bash
    cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -P acquire-deps.cmake
    ```

The recommended way to build the Brisk library is with Ninja:

=== "macOS"

    ```bash
    cd path/to/brisk/repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=uni-osx -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=uni-osx -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

=== "Linux"

    ```bash
    cd path/to/brisk/repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

=== "Windows"

    ```powershell
    cd path\to\brisk\repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=OFF -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_TOOLCHAIN_FILE=vcpkg_exported/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

You can replace `<TRIPLET>` with one of valid [triplets](../advanced/triplets.md#list-of-supported-triplets).

`CMAKE_INSTALL_PREFIX` specifies the directory where the static libraries and header files will be placed after building.

On Windows, these commands should be executed in the Visual Studio Developer Command Prompt.

After executing the above commands, you’ll get the following new directories in the Brisk root directory:

1. `dist` — stores Brisk’s static libraries and headers.
2. `vcpkg_exported` — contains minimal vcpkg installation required for Brisk.
3. `vcpkg_installed` — contains all dependencies needed for building and linking applications with Brisk.

These directories are relocatable, so you can move them to another directory or computer if needed.

## Building Brisk And Dependencies

!!! tip "Prebuilt Binaries"
    Building Brisk and its dependencies may take a significant amount of time on your system.
    Alternatively, you can download prebuilt binaries from our build server (see [Prebuilt Binaries](prebuilt_binaries.md)).

To build Brisk dependencies the latest version of Vcpkg is required.

=== "macOS"

    ```bash
    cd path/to/brisk/repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=uni-osx -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=uni-osx -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

=== "Linux"

    ```bash
    cd path/to/brisk/repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

=== "Windows"

    ```powershell
    cd path\to\brisk\repo
    cmake -GNinja -S . -B build-release -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release --target install
    cmake -GNinja -S . -B build-debug -DVCPKG_MANIFEST_INSTALL=ON -DCMAKE_INSTALL_PREFIX=dist -DVCPKG_TARGET_TRIPLET=x64-windows-static-md -DCMAKE_TOOLCHAIN_FILE=<vcpkg-root-directory>/scripts/buildsystems/vcpkg.cmake -DVCPKG_INSTALLED_DIR=vcpkg_installed -DCMAKE_BUILD_TYPE=Debug
    cmake --build build-debug --target install
    ```

Replace `<vcpkg-root-directory>` with the path to the Vcpkg root directory.

After executing the above commands, you’ll get the following new directories in the Brisk root directory:

1. `dist` — stores Brisk’s static libraries and headers.
2. `vcpkg_installed` — contains all dependencies needed for building and linking applications with Brisk.

## CMake Options

You can enable/disable options when configuring Brisk in CMake by adding `-DOPTION=ON` or `-DOPTION=OFF` to the command line.

- `BRISK_WEBGPU` enables building WebGPU graphics backend. Default value is `ON` on macOS/Linux and `OFF` on Windows.
- `BRISK_D3D11` enables building D3D11 graphics backend on Windows. Default value is `ON` on Windows.
- `BRISK_TESTS` and `BRISK_EXAMPLES` control whether the Brisk tests and examples will be compiled or not. Default is `ON` in standalone mode and `OFF` when used with `add_subdirectory`.

## Building Brisk with Your Project (`add_subdirectory`)

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
