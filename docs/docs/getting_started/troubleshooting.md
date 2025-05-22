# Troubleshooting

## CMake Emits `Brisk requires MSVC...`

When configuring Brisk or your project, you may encounter the following error:

```
Brisk requires MSVC or an MSVC-compatible compiler (e.g., clang-cl) on Windows
```

This indicates that your environment is not using Microsoft Visual Studio's MSVC compiler or an MSVC-compatible compiler, such as `clang-cl`. On Windows, Brisk requires one of these compilers to build correctly.

**Solution:**

1. Ensure you are using Visual Studio (2022 is required) or a compatible compiler, such as the built-in LLVM toolchain or a standalone Clang installation configured in MSVC mode (`clang-cl`).
2. Run your commands in the **Developer Command Prompt for Visual Studio 2022**, which automatically sets up the necessary environment variables. Alternatively, you can manually configure the environment in a standard console window by running the appropriate setup script.
3. Verify that the paths in the commands below match your Visual Studio installation directory (e.g., adjust for Community, Professional, or Enterprise editions).

=== "Classic Console (cmd.exe)"

    ```batch
    call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
    ```

    Use one of the following arguments to target different architectures:

    - `x64`: For 64-bit Windows applications.
    - `x64_x86`: For 32-bit applications.
    - `x64_arm64`: For ARM64 applications.

=== "PowerShell"

    ```powershell
    & 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64
    ```

    The `-Arch` parameter specifies the target architecture (`amd64`, `x86`, or `arm64`), and `-HostArch` specifies the host architecture. Adjust these based on your build requirements.

**Additional Notes:**
- If the error persists, verify that Visual Studio is installed with the **C++ Desktop Development** workload, which includes MSVC and related tools.
- For Clang, ensure it is correctly configured to use the `clang-cl` frontend (not `clang` or `clang++`).

## CMake Emits `No CMAKE_CXX_COMPILER could be found`

This error indicates that CMake cannot locate a C++ compiler in your environment.

**On Windows:**
Refer to the [previous section](#cmake-emits-brisk-requires-msvc) for instructions on setting up the MSVC environment using the Developer Command Prompt or manual environment configuration.

**On macOS:**
Ensure that Xcode is installed, as it provides the necessary Clang compiler. You can install Xcode from the Mac App Store and verify the command-line tools are available by running:

```bash
xcode-select --install
```

**On Linux:**
Install a compiler toolchain if it is not already present. For example:
- On Debian/Ubuntu (Apt-based distributions), install the `build-essential` package:
  ```bash
  sudo apt update
  sudo apt install build-essential
  ```

After installing the required tools, rerun CMake to confirm the compiler is detected.

## Error `This version of ...\icowriter.exe is not compatible with the version of Windows you're running` During Build

This error typically occurs when cross-compiling for ARM64 on an x86_64 Windows system. Brisk builds utility tools (e.g., `icowriter.exe`) to convert images to `.ico` format or bundle resources. These tools must be compatible with the host system (x86_64 in this case).

**Solution:**
Set the host triplet to ensure the utilities are built for your host architecture:

```bash
-DVCPKG_HOST_TRIPLET=x64-windows-static-md
```

**Additional Notes:**
- Verify that the VCPKG toolchain is correctly configured in your CMake setup.
- If cross-compiling, ensure the target triplet (e.g., `arm64-windows-static-md`) is also specified correctly in your CMake configuration.

## Error `mismatch detected for 'RuntimeLibrary'` During Build

This error occurs when there is a mismatch between the MSVC runtime library used by your project and the one expected by the selected VCPKG triplet. By default, CMake links applications to the dynamic MSVC runtime (`MultiThreadedDLL`). However, if you use a VCPKG triplet that implies static linking (e.g., triplets without the `-md` suffix, such as `x64-windows-static`), you must explicitly configure the runtime library.

**Solution:**
Add the following CMake variable when configuring your project to select the appropriate runtime library:

```bash
-DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded$<$<CONFIG:Debug>:Debug>"
```

This setting ensures:
- `MultiThreaded` is used for Release builds (statically linked runtime).
- `MultiThreadedDebug` is used for Debug builds.

**Additional Notes:**
- If you are using a dynamic runtime triplet (e.g., `x64-windows-static-md`), this variable is typically not needed, as the default dynamic runtime (`MultiThreadedDLL`) aligns with the triplet.
- Always verify that the VCPKG triplet and CMake runtime settings are consistent to avoid linker errors.
