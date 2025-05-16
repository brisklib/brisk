cmake_minimum_required(VERSION 3.22)

if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "VCPKG_TARGET_TRIPLET is not defined, checking environment...")

    if (DEFINED ENV{VCPKG_TARGET_TRIPLET})
        set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
        message(STATUS "Using VCPKG_TARGET_TRIPLET environment variable (${VCPKG_TARGET_TRIPLET})")
    else ()
        if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
            set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET})
            message(STATUS "Using VCPKG_DEFAULT_TRIPLET environment variable (${VCPKG_TARGET_TRIPLET})")
        endif ()
    endif ()
endif ()

if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    if (WIN32)
        set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
    elseif (APPLE)
        set(VCPKG_TARGET_TRIPLET uni-osx)
    else ()
        set(VCPKG_TARGET_TRIPLET x64-linux)
    endif ()
    message(STATUS "VCPKG_TARGET_TRIPLET is not set. The auto-detected triplet is ${VCPKG_TARGET_TRIPLET}")
endif ()

if (NOT DEFINED VCPKG_HOST_TRIPLET)
    set(VCPKG_HOST_TRIPLET ${VCPKG_HOST_TRIPLET})
endif ()

set(SUPPORTED_TRIPLETS
    "x64-linux"
    "x64-osx"
    "arm64-osx"
    "uni-osx"
    "x64-windows-static"
    "x86-windows-static"
    "x64-windows-static-md"
    "x86-windows-static-md"
    "arm64-windows-static"
    "arm64-windows-static-md")

# Check if the specified triplet is in the list of supported triplets
if (NOT VCPKG_TARGET_TRIPLET IN_LIST SUPPORTED_TRIPLETS)
    message(
        FATAL_ERROR
            "The specified VCPKG_TARGET_TRIPLET ${VCPKG_TARGET_TRIPLET} is not supported."
            "Supported triplets are: ${SUPPORTED_TRIPLETS}"
            "Use VCPKG_TARGET_TRIPLET environment variable to set supported triplet")
endif ()
