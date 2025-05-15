#
# Brisk
#
# Cross-platform application framework
# --------------------------------------------------------------
#
# Copyright (C) 2025 Brisk Developers
#
# This file is part of the Brisk library.
#
# Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+), and a commercial license. You may
# use, modify, and distribute this software under the terms of the GPL-2.0+ license if you comply with its conditions.
#
# You should have received a copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
#
# If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial license. For commercial
# licensing options, please visit: https://brisklib.com
#
cmake_minimum_required(VERSION 3.22)

include_guard(GLOBAL)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET
        "11"
        CACHE STRING "" FORCE)
endif ()

list(APPEND CMAKE_MODULE_PATH ${BRISK_ROOT}/cmake/packages)

if (NOT CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake" AND NOT DEFINED HAS_VCPKG)

    if (CMAKE_TOOLCHAIN_FILE)
        message(
            WARNING
                "CMAKE_TOOLCHAIN_FILE is set but does not point to a vcpkg toolchain. "
                "Make sure that vcpkg is installed and can be used by Brisk. "
                "Define HAS_VCPKG to indicate that Vcpkg toolchain is loaded")
    endif ()

    set(VCPKG_ROOT
        ${CMAKE_SOURCE_DIR}/vcpkg
        CACHE PATH "")

    find_program(
        VCPKG_TOOL
        NAMES vcpkg
        PATHS ${VCPKG_ROOT}
        NO_DEFAULT_PATH)

    if (VCPKG_TOOL)
        message(STATUS "Vcpkg executable found at ${VCPKG_TOOL}")
    else ()
        message(STATUS "Downloading vcpkg to use with Brisk...")

        include(FetchContent)
        FetchContent_Declare(
            vcpkg
            GIT_REPOSITORY https://github.com/microsoft/vcpkg/
            GIT_TAG 2024.09.23
            SOURCE_DIR ${VCPKG_ROOT})
        FetchContent_MakeAvailable(vcpkg)
    endif ()

    set(VCPKG_MANIFEST_MODE
        TRUE
        CACHE BOOL "")
    set(VCPKG_MANIFEST_DIR
        ${BRISK_ROOT}
        CACHE PATH "")

    set(ENV{VCPKG_ROOT} ${VCPKG_ROOT})

    set(CMAKE_TOOLCHAIN_FILE
        ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
        CACHE PATH "")
    set(VCPKG_INSTALLED_DIR
        ${CMAKE_SOURCE_DIR}/vcpkg_installed
        CACHE PATH "")

    message(STATUS "Vcpkg is set up successfully")

else ()
    message(STATUS "Vcpkg is already set up via toolchain. Brisk will use it for dependencies")
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/get_triplet.cmake)

set(VCPKG_TARGET_TRIPLET
    ${VCPKG_TARGET_TRIPLET}
    CACHE STRING "" FORCE)
set(VCPKG_HOST_TRIPLET
    ${VCPKG_HOST_TRIPLET}
    CACHE STRING "" FORCE)

set(_BRISK_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../include)

if ("${VCPKG_TARGET_TRIPLET}" MATCHES "windows-static$")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

if ("${VCPKG_TARGET_TRIPLET}" STREQUAL "uni-osx")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
elseif ("${VCPKG_TARGET_TRIPLET}" STREQUAL "arm64-osx")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
elseif ("${VCPKG_TARGET_TRIPLET}" STREQUAL "x64-osx")
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif ()
set(CMAKE_OSX_ARCHITECTURES
    ${CMAKE_OSX_ARCHITECTURES}
    CACHE STRING "" FORCE)

if (CMAKE_CROSSCOMPILING)
    set(VCPKG_USE_HOST_TOOLS
        ON
        CACHE BOOL "Enable host tools for find_program" FORCE)
endif ()
