# Choosing Architecture (Triplets)

Dependency management in Brisk is based on Vcpkg, and [Vcpkg triplets](https://learn.microsoft.com/en-us/vcpkg/concepts/triplets) are used to select the target architecture and environment.

If you're unsure which triplet to choose, the recommendations are as follows:
- `x64-windows-static-md` for Windows
- `x64-linux` for Linux
- `arm64-osx` for ARM-based Macs (or `x64-osx` for Intel-based Macs).

Note that Vcpkg uses different default settings for Windows (`x64-windows`) and macOS (`x64-osx`).

!!! warning "Unsupported triplets"
    The `x64-windows` and `x86-windows` triplets are not supported by Brisk, and a build will fail if this triplet is used.

!!! note "Dependencies linking"
    Currently, Brisk supports only static linking of dependencies; dynamic linking for Brisk libraries will be available in a future release.

## Setting Triplet

1. You can set `VCPKG_DEFAULT_TRIPLET` environment variable globally to override default triplet for your system, or
2. export `VCPKG_TARGET_TRIPLET` environment variable for Brisk build, or
3. set `-DVCPKG_TARGET_TRIPLET=<TRIPLET>` CMake variable when configuring build.

This applies to building Brisk from source and building applications that use the Brisk libraries.

!!! note ""
    Make sure that the triplet used when building the Brisk libraries (and dependencies) matches the triplet used for the application. For prebuilt binaries or dependencies, the triplet is always added to the archive filename (`Brisk-Prebuilt-v0.9.7-arm64-osx.tar.xz`).

For more information, consult the official Vcpkg documentation.

## List of Supported Triplets

Brisk is tested and works on the following triplets:

|       | x86                                    | x64                                    | arm                                    | arm64                                    |
|-------|----------------------------------------|----------------------------------------|----------------------------------------|------------------------------------------|
| Linux | —                                      | :white_check_mark: x64-linux           | :construction:                         | :construction:                           |
| macOS | —                                      | :white_check_mark: x64-osx             | —                                      | :white_check_mark: arm64-osx             |
| Windows (static runtime) | :white_check_mark: x86-windows-static   | :white_check_mark: x64-windows-static  | :construction:                         | :white_check_mark: arm64-windows-static  |
| Windows (dynamic runtime) | :white_check_mark: x86-windows-static-md | :white_check_mark: x64-windows-static-md | :construction:                         | :white_check_mark: arm64-windows-static-md |

!!! warning ""
    Cross-compiling from Intel-based macOS to ARM is not currently supported; a Mac with Apple Silicon is required.
