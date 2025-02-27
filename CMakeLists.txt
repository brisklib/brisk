#
# Brisk
#
# Cross-platform application framework
# --------------------------------------------------------------
#
# Copyright (C) 2024 Brisk Developers
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

if (NOT BRISK_ROOT)
    set(BRISK_ROOT
        ${CMAKE_CURRENT_LIST_DIR}
        CACHE PATH "" FORCE)
endif ()

include(cmake/init.cmake)

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/include/brisk/core/Version.hpp BRISK_VERSION
     REGEX "#define BRISK_VERSION_(MINOR|MAJOR|PATCH)")
string(REGEX MATCHALL "[0-9]+" BRISK_VERSION_MATCH ${BRISK_VERSION})
string(REPLACE ";" "." BRISK_VERSION "${BRISK_VERSION_MATCH}")

project(
    brisk
    VERSION ${BRISK_VERSION}
    LANGUAGES CXX C)

get_directory_property(BRISK_PARENT_DIR PARENT_DIRECTORY)
if (NOT BRISK_PARENT_DIR)
    set(BRISK_STANDALONE TRUE)
endif ()

if (DEBUG_CMAKE)
    get_cmake_property(VARS VARIABLES)
    list(SORT VARS)
    foreach (VAR ${VARS})
        message(STATUS "${VAR}=${${VAR}}")
    endforeach ()
endif ()

option(BRISK_LTO ON "Use Link Time Optimization in Release builds")
option(BRISK_BROTLI "Enable Brotli compression" ON)
if (NOT WIN32)
    set(ENABLE_WEBGPU ON)
else ()
    set(ENABLE_WEBGPU OFF)
endif ()
option(BRISK_WEBGPU "Enable WebGPU" ${ENABLE_WEBGPU})
option(BRISK_ICU "Link to ICU by default for full Unicode support" ON)
option(BRISK_GONOTO "Include GoNoto font" OFF)
option(BRISK_LOG_TO_STDERR "Write log output to stderr (Windows-specific)" OFF)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(TESTS_AND_EXAMPLES ON)
else ()
    set(TESTS_AND_EXAMPLES OFF)
endif ()
option(BRISK_TESTS "Enable Brisk tests building" ${TESTS_AND_EXAMPLES})
option(BRISK_EXAMPLES "Enable Brisk examples building" ${TESTS_AND_EXAMPLES})

if (BRISK_LTO AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options($<$<CONFIG:Release>:-flto=thin>)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(-utf-8)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32 AND BRISK_GENERATE_MAP)
    add_link_options(/Map)
endif ()

if (BRISK_SAN)
    add_compile_options(-fsanitize=${BRISK_SAN})
    add_link_options(-fsanitize=${BRISK_SAN})
endif ()

set(DEPS_DIR
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
    CACHE STRING "DEPS_DIR")

if (WIN32)
    set(PLATFORM_SPECIFIC_CPP "_Windows.cpp")
elseif (APPLE)
    set(PLATFORM_SPECIFIC_CPP "_Darwin.mm")
else ()
    set(PLATFORM_SPECIFIC_CPP "_Linux.cpp")
endif ()

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/licenses.txt" "")

include(GNUInstallDirs)

include(cmake/add_autotests.cmake)
include(cmake/scripts/compile_resources.cmake)
include(cmake/scripts/setup_executable.cmake)
include(cmake/scripts/metadata.cmake)

if (BRISK_LOG_TO_STDERR)
    add_compile_definitions(BRISK_LOG_TO_STDERR=1)
endif ()

include(cmake/add_license.cmake)

file(GLOB_RECURSE LICENSE_LIST ${DEPS_DIR}/share/copyright) # CONFIGURE_DEPENDS

foreach (license_path ${LICENSE_LIST})
    get_filename_component(PKG_PATH ${license_path} DIRECTORY)
    get_filename_component(PKG_NAME ${PKG_PATH} NAME)
    add_license(${PKG_NAME} ${license_path})
endforeach ()

set(BRISK_RESOURCES_DIR ${PROJECT_SOURCE_DIR}/resources)
set(FONTS_DIR ${BRISK_RESOURCES_DIR}/fonts)
set(IMAGES_DIR ${BRISK_RESOURCES_DIR}/images)

if (NOT BRISK_DBG_SUFFIX)
    set(BRISK_DBG_SUFFIX
        "/debug"
        CACHE STRING "")
endif ()

add_subdirectory(src)

add_library(brisk-executable INTERFACE)
set_property(TARGET brisk-executable PROPERTY EXPORT_NAME Executable)
add_library(Brisk::Executable ALIAS brisk-executable)
if (WIN32)
    target_sources(
        brisk-executable
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>>/include/brisk/window/internal/Windows.manifest
    )
endif ()

if (BRISK_TESTS)

    set(TESTS_GRAPHICS_LIBS)
    set(TESTS_GRAPHICS_DEFS)
    find_package(glfw3 CONFIG)
    if (glfw3_FOUND)
        message(STATUS "GLFW3 is found. It will be used for interactive tests")
        list(APPEND TESTS_GRAPHICS_LIBS glfw)
        list(APPEND TESTS_GRAPHICS_DEFS HAVE_GLFW3)
    else ()
        message(STATUS "GLFW3 is not found, skipping interactive tests")
    endif ()

    file(GLOB TESTS_CORE ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*_test.cpp)
    file(GLOB TESTS_NETWORK ${CMAKE_CURRENT_SOURCE_DIR}/src/network/*_test.cpp)
    file(GLOB TESTS_GRAPHICS ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/*_test.cpp)
    file(GLOB TESTS_WINDOW ${CMAKE_CURRENT_SOURCE_DIR}/src/window/*_test.cpp)
    file(GLOB TESTS_GUI ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/*_test.cpp)
    file(GLOB TESTS_WIDGETS ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/*_test.cpp)

    enable_testing()

    add_autotests(core SOURCES ${TESTS_CORE} LIBRARIES brisk-core)
    add_autotests(network SOURCES ${TESTS_NETWORK} LIBRARIES brisk-network)
    add_autotests(
        graphics
        SOURCES
        ${TESTS_GRAPHICS}
        LIBRARIES
        brisk-graphics
        brisk-i18n-icu
        ${TESTS_GRAPHICS_LIBS}
        DEFINITIONS
        ${TESTS_GRAPHICS_DEFS})
    add_autotests(window SOURCES ${TESTS_WINDOW} LIBRARIES brisk-window brisk-i18n-icu)
    add_autotests(gui SOURCES ${TESTS_GUI} LIBRARIES brisk-gui brisk-i18n-icu)
    add_autotests(widgets SOURCES ${TESTS_WIDGETS} LIBRARIES brisk-widgets brisk-i18n-icu)

endif ()

if (BRISK_EXAMPLES)

    add_subdirectory(examples)

endif ()

install(DIRECTORY include/brisk DESTINATION include)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/BriskConfigVersion.cmake
    VERSION ${BRISK_VERSION}
    COMPATIBILITY SameMinorVersion)

set(BRISK_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/brisk"
    CACHE STRING "Path to Brisk CMake files")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BriskConfigVersion.cmake DESTINATION ${BRISK_INSTALL_CMAKEDIR})

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/BriskConfig.cmake INSTALL_DESTINATION "lib/cmake/brisk")

set(EXTRA_INSTALL_TARGETS)

if (TARGET brisk-renderer-d3d11)
    list(APPEND EXTRA_INSTALL_TARGETS brisk-renderer-d3d11)
endif ()
if (TARGET brisk-renderer-webgpu)
    list(APPEND EXTRA_INSTALL_TARGETS brisk-renderer-webgpu)
endif ()

install(
    TARGETS
        brisk-core
        brisk-graphics
        brisk-network
        brisk-window
        brisk-gui
        brisk-widgets
        brisk-executable
        brisk-i18n
        brisk-i18n-icu
        ${EXTRA_INSTALL_TARGETS}
    EXPORT BriskTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}$<$<CONFIG:Debug>:${BRISK_DBG_SUFFIX}>
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}$<$<CONFIG:Debug>:${BRISK_DBG_SUFFIX}>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}$<$<CONFIG:Debug>:${BRISK_DBG_SUFFIX}>)

install(DIRECTORY cmake/scripts/ DESTINATION "${BRISK_INSTALL_CMAKEDIR}/scripts")

install(
    FILES src/core/Deps.cmake
    RENAME Core.cmake
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}/deps")
install(
    FILES src/graphics/Deps.cmake
    RENAME Graphics.cmake
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}/deps")
install(
    FILES src/network/Deps.cmake
    RENAME Network.cmake
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}/deps")
install(
    FILES src/window/Deps.cmake
    RENAME Window.cmake
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}/deps")
install(
    FILES src/gui/Deps.cmake
    RENAME GUI.cmake
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}/deps")

install(DIRECTORY resources/ DESTINATION "resources")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BriskConfig.cmake DESTINATION "${BRISK_INSTALL_CMAKEDIR}")

install(
    EXPORT BriskTargets
    NAMESPACE Brisk::
    DESTINATION "${BRISK_INSTALL_CMAKEDIR}"
    CONFIGURATIONS Debug Release)

set(DEP_HASH_SILENT ON)
include(dep-hash.cmake)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Package-README.md.in ${CMAKE_CURRENT_BINARY_DIR}/Package-README.md)

write_file(${CMAKE_CURRENT_BINARY_DIR}/DEP_HASH ${DEP_HASH})
write_file(${CMAKE_CURRENT_BINARY_DIR}/VCPKG_TARGET_TRIPLET ${VCPKG_TARGET_TRIPLET})

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/Package-README.md
    RENAME README.md
    DESTINATION .)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DEP_HASH ${CMAKE_CURRENT_BINARY_DIR}/VCPKG_TARGET_TRIPLET
        DESTINATION ${BRISK_INSTALL_CMAKEDIR})
