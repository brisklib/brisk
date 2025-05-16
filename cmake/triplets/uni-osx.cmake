message(
    FATAL_ERROR
        [[
The uni-osx triplet is only supported with prebuilt binaries and dependencies
because Vcpkg does not support building universal binaries for macOS.
The uni-osx triplet is a synthetic triplet used by Brisk to represent macOS universal binaries that combine x64-osx and arm64-osx binaries.

To build dependencies, please use one of the triplets supported by Vcpkg: x64-osx or arm64-osx.
Alternatively, use prebuilt dependencies if you need to work with universal binaries.
Refer to the documentation: https://docs.brisklib.com/getting_started/triplets/
]])
