# Prebuilt Binaries

The recommended way to use the Brisk library is to download prebuilt package.

The Brisk library depends on several third-party libraries (such as libpng, zlib, etc.). Even if you're using prebuilt Brisk binaries, these libraries must still be linked to build the final executable.

Below are instructions for acquiring the necessary binaries to build Brisk applications.

## Acquiring Complete Brisk Binaries

You can download the Brisk library binaries along with all required dependencies:

#### For Brisk Releases

In the *assets* section of the release page on GitHub:

https://github.com/brisklib/brisk/releases/latest

You should download

1. `Brisk-Prebuilt-<VERSION>-<TRIPLET>.tar.xz` that contains the Brisk binary package, and
2. `Brisk-Dependencies-<HASH>-<TRIPLET>.tar.xz` that contains the Brisk dependencies.

For guidance on choosing the right triplet, see [Choosing architecture (triplets)](triplets.md).

#### For Automated (Nightly) Builds

GitHub Actions artifacts include archives with header files and static libraries:

https://github.com/brisklib/brisk/actions/workflows/test.yml?query=branch%3Amain+

These artifacts that include only the triplet (e.g. `x64-linux`) in their names include both the Brisk package and the Brisk dependencies.

### CMake Variables for Building Applications with Prebuilt Brisk and Dependencies

| CMake variable                      | Value                                                                                          |
|-------------------------------------|------------------------------------------------------------------------------------------------|
| `CMAKE_PREFIX_PATH`                 | `<brisk-prebuilt>/lib/cmake`                                                                   |
| <nobr>`CMAKE_TOOLCHAIN_FILE`</nobr> | `<brisk-dependencies>/scripts/buildsystems/vcpkg.cmake`                                        |
| `VCPKG_INSTALLED_DIR`               | `<brisk-dependencies>/installed`                                                               |
| `VCPKG_TARGET_TRIPLET`              | Must match downloaded package’s triplet                                                        |
| `VCPKG_HOST_TRIPLET`                | Equal to `VCPKG_TARGET_TRIPLET` or `x64-windows-static-md` (Cross-compiling to ARM on Windows) |

Replace `<brisk-prebuilt>` with the path where the `Brisk-Prebuilt-<VERSION>-<TRIPLET>.tar.xz` archive was extracted.
Also, replace `<brisk-dependencies>` with the path where the `Brisk-Dependencies-<VERSION>-<TRIPLET>.tar.xz` archive was extracted.

## Acquiring Only Brisk Dependencies

Alternatively, you can choose to fetch prebuilt dependencies but build the Brisk libraries from source.

This approach is useful if you plan to modify Brisk's source code.

To download only the prebuilt dependencies, use the following command. Set the `VCPKG_DEFAULT_TRIPLET` or `VCPKG_TARGET_TRIPLET` environment variable to match your system's architecture (see [List of Available Triplets](triplets.md#list-of-supported-triplets)).

```bash
cd brisk/repository
cmake -P acquire-deps.cmake
```

After executing this command, the `vcpkg_exported` and `vcpkg_installed` directories will be created.

The script hashes files affecting dependencies (such as library versions and toolchain settings), generates a combined hash, and downloads the corresponding prebuilt binaries from our build server. If specific binaries are not available on our servers, the script will attempt to build them using vcpkg, unless you specify `-DDEP_BUILD=OFF`:

```bash
# Don't try to build, just download
cmake -DDEP_BUILD=OFF -P acquire-deps.cmake
```

!!! note "Note"
    If you're using prebuilt dependencies, make sure to set the `VCPKG_MANIFEST_MODE` CMake variable to `OFF` when building the Brisk libraries to prevent Vcpkg from automatically rebuilding all dependencies.

### CMake Variables for Building Brisk with Prebuilt Dependencies

| CMake variable                      | Value                                                                                          |
|-------------------------------------|------------------------------------------------------------------------------------------------|
| <nobr>`CMAKE_TOOLCHAIN_FILE`</nobr> | `<brisk-repository>/vcpkg_exported/scripts/buildsystems/vcpkg.cmake`                           |
| `VCPKG_INSTALLED_DIR`               | `<brisk-repository>/vcpkg_installed`                                                           |
| `VCPKG_TARGET_TRIPLET`              | Must match downloaded package’s triplet                                                        |
| `VCPKG_HOST_TRIPLET`                | Equal to `VCPKG_TARGET_TRIPLET` or `x64-windows-static-md` (Cross-compiling to ARM on Windows) |

### CMake Variables for Building Applications with Prebuilt Dependencies

| CMake variable                      | Value                                                                                          |
|-------------------------------------|------------------------------------------------------------------------------------------------|
| `CMAKE_PREFIX_PATH`                 | `<brisk-install-path>/lib/cmake`                                                               |
| <nobr>`CMAKE_TOOLCHAIN_FILE`</nobr> | `<brisk-repository>/vcpkg_exported/scripts/buildsystems/vcpkg.cmake`                           |
| `VCPKG_INSTALLED_DIR`               | `<brisk-repository>/vcpkg_installed`                                                           |
| `VCPKG_TARGET_TRIPLET`              | Must match downloaded package’s triplet                                                        |
| `VCPKG_HOST_TRIPLET`                | Equal to `VCPKG_TARGET_TRIPLET` or `x64-windows-static-md` (Cross-compiling to ARM on Windows) |

Replace `<brisk-repository>` with the path to the Brisk repository where `acquire-deps.cmake` was called.
Also, replace `<brisk-install-path>` with the path where Brisk was installed after building (usually `<brisk-repository>/dist`).
