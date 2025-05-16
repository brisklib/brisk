# Choosing Architecture (Triplets)

Dependency management in Brisk is based on Vcpkg, and [Vcpkg triplets](https://learn.microsoft.com/en-us/vcpkg/concepts/triplets) are used to select the target architecture and environment.

If you're unsure which triplet to choose, these are default used by Brisk:

* **Windows**: `x64-windows-static-md` (x86\_64)
* **Linux**: `x64-linux` (x86\_64)
* **macOS (prebuilt dependencies)**: `uni-osx` (arm64/x86\_64 universal binaries)
* **macOS (custom build)**: `arm64-osx` for ARM-based Macs (or `x64-osx` for Intel-based Macs)

!!! note "uni-osx"
    The `uni-osx` triplet is supported only with prebuilt binaries and dependencies because Vcpkg does not support building universal binaries for macOS.
    This triplet is a synthetic one used by Brisk to represent macOS universal binaries that combine `x64-osx` and `arm64-osx` binaries.
    To build dependencies, use one of the Vcpkg-supported triplets: `x64-osx` or `arm64-osx`.  
    Alternatively, use prebuilt dependencies if you need to work with universal binaries.

All these triplets use static linking for dependencies and dynamic linking for the C runtime (CRT).


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

|                           | x86                                      | x64                                      | arm64                                      | uni (arm64, x64)           |
|---------------------------|------------------------------------------|------------------------------------------|--------------------------------------------|----------------------------|
| Linux                     | —                                        | :white_check_mark: x64-linux             | :construction:                             | —                          |
| macOS                     | —                                        | :white_check_mark: x64-osx               | :white_check_mark: arm64-osx               | :white_check_mark: uni-osx |
| Windows (static runtime)  | :white_check_mark: x86-windows-static    | :white_check_mark: x64-windows-static    | :white_check_mark: arm64-windows-static    | —                          |
| Windows (dynamic runtime) | :white_check_mark: x86-windows-static-md | :white_check_mark: x64-windows-static-md | :white_check_mark: arm64-windows-static-md | —                          |
