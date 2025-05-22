# Prebuilt Binaries

The recommended way to use the Brisk library is to download prebuilt package.

The Brisk library depends on several third-party libraries (such as libpng, zlib, etc.). Even if you're using prebuilt Brisk binaries, these libraries must still be linked to build the final executable.

Below are instructions for acquiring the necessary binaries to build Brisk applications.

## Acquiring Complete Brisk Binaries

You can download the Brisk library binaries along with all required dependencies:

#### Brisk Releases

In the *assets* section of the release page on GitHub:

[https://github.com/brisklib/brisk/releases/latest](https://github.com/brisklib/brisk/releases/latest){target="_blank"}

!!! note ""
    === "macOS"
        For macOS, download 

        * `Brisk-Prebuilt-...-uni-osx.tar.xz`, and
        * `Brisk-Dependencies-...-uni-osx.tar.xz`

        This package contains binaries for both Intel- and ARM-based Macs.

    === "Linux"
        For x86_64 Linux, download 

        * `Brisk-Prebuilt-...-x64-linux.tar.xz`, and
        * `Brisk-Dependencies-...-x64-linux.tar.xz`

    === "Windows"
        For x86_64 Windows, download

        * `Brisk-Prebuilt-...-x64-windows-static-md.tar.xz`, and
        * `Brisk-Dependencies-...-x64-windows-static-md.tar.xz`

        To target ARM Windows, download

        * `Brisk-Prebuilt-...-arm64-windows-static-md.tar.xz`, and
        * `Brisk-Dependencies-...-arm64-windows-static-md.tar.xz`

For guidance on choosing the right triplet, see [Choosing architecture (triplets)](../advanced/triplets.md).

[How to extract a `tar.xz` archive](#extracting-tarxz-files).

#### Automated (Nightly) Builds

If you're interested in the latest features, you can find the automated builds in the GitHub Actions artifacts:

[https://github.com/brisklib/brisk/actions/workflows/test.yml?query=branch%3Amain+](https://github.com/brisklib/brisk/actions/workflows/test.yml?query=branch%3Amain+){target="_blank"}

These artifacts that include only the triplet (e.g. `x64-linux`) in their names include both the Brisk package and the Brisk dependencies.

[How to extract a `tar.xz` archive](#extracting-tarxz-files).

## Acquiring Only Brisk Dependencies

Alternatively, you can choose to fetch prebuilt dependencies but build the Brisk libraries from source.

This approach is useful if you plan to modify Brisk's source code.

To download only the prebuilt dependencies, use the following command. Set the `VCPKG_DEFAULT_TRIPLET` or `VCPKG_TARGET_TRIPLET` environment variable to match your system's architecture (see [List of Available Triplets](../advanced/triplets.md#list-of-supported-triplets)).

```bash
cd brisk/repository
cmake -P acquire-deps.cmake
```

??? tip "Download specific triplet"
    ```bash
    cd brisk/repository
    cmake -DVCPKG_TARGET_TRIPLET=<TRIPLET> -P acquire-deps.cmake
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

These CMake variables are required for building __the Brisk libraries__.

| CMake variable                      | Value                                                                                          |
|-------------------------------------|------------------------------------------------------------------------------------------------|
| <nobr>`CMAKE_TOOLCHAIN_FILE`</nobr> | `<brisk-repository>/vcpkg_exported/scripts/buildsystems/vcpkg.cmake`                           |
| `VCPKG_INSTALLED_DIR`               | `<brisk-repository>/vcpkg_installed`                                                           |
| `VCPKG_TARGET_TRIPLET`              | Must match downloaded package’s triplet                                                        |
| `VCPKG_HOST_TRIPLET`                | Equal to `VCPKG_TARGET_TRIPLET` or `x64-windows-static-md` (Cross-compiling to ARM on Windows) |

Replace `<brisk-repository>` with the path to the Brisk repository where `acquire-deps.cmake` was called.

## Extracting .tar.xz files

=== "macOS"
    Open Terminal, navigate to the archive's directory using `cd`, then run
    
    ```
    tar -xf archive.tar.xz
    ```

=== "Linux"
    Open a terminal, navigate to the archive's directory using `cd`, then run 

    ```
    tar -xf archive.tar.xz
    ```

=== "Windows"
    Since Windows 10 version 1803 (build 17063), the `tar` command is included by default. Open Command Prompt or PowerShell, navigate to the archive's directory using `cd`, then run:

    ```powershell
    tar -xf archive.tar.xz
    ```

    Alternatively, you can install 7-Zip or WinRAR, right-click the `.tar.xz` file, and select "Extract" or "Extract Here" from the context menu. You can also use WSL (Windows Subsystem for Linux) and run the same `tar` command in a Linux terminal.
