# Installing Prerequisites

=== "macOS"

    !!! note "Note"
        Development and deployment are only supported on macOS 11 Big Sur or newer.

    Xcode is required and can be installed through the App Store. 

    Xcode version 14.3 or newer is required, and both Intel and Apple Silicon Macs are supported. Note that command line development tools alone are **not** sufficient.

    Additionally, CMake, Git, and Ninja are required. These can be installed via the [Homebrew package manager](https://brew.sh).

    ```bash
    brew install cmake git ninja
    ```

=== "Linux"

    !!! note "Note"
        GCC 12+ or Clang 16+ is required for development.  
        You may need to add specific repositories to install the latest version of GCC if your default repository has an older version.

    To install GCC, Binutils, and Ninja on Ubuntu, use the following command:

    ```bash
    sudo apt-get install build-essential ninja-build cmake git
    ```


=== "Windows"

    !!! note "Note"
        Development requires Windows 10 or newer and Visual Studio 2022. It is recommended to update to the latest version.

    Download and install Visual Studio 2022 from the official website. The Community version is sufficient.

    [https://visualstudio.microsoft.com/vs/community/](https://visualstudio.microsoft.com/vs/community/){target="_blank"}

    CMake, Git, and Ninja are also required for building on Windows. You can install them using your Windows package manager or manually from their respective official websites.

    Example using Chocolatey:
    ```cmd
    choco install ninja cmake git
    ```
