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

add_library(
    brisk-core STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/core/BasicTypes.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Json.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Bytes.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Compression.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Cryptography.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Hash.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/CRCcompile.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Encoding.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Expected.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Generation.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/IO.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Utilities.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Fixed.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Log.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Optional.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Serialization.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Settings.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Text.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Threading.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Time.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/Math.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Initialization.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/core/internal/Functional.hpp
    ${PROJECT_SOURCE_DIR}/src/core/Json.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Bytes.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Binding.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_Brotli.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_GZip.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Compression_LZ4.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Cryptography.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Hash.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Encoding.cpp
    ${PROJECT_SOURCE_DIR}/src/core/IO.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Log.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Settings.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Text.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Localization.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Threading.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Stream.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Time.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Initialization.cpp
    ${PROJECT_SOURCE_DIR}/src/core/uv.hpp
    ${PROJECT_SOURCE_DIR}/src/core/uv.cpp
    ${PROJECT_SOURCE_DIR}/src/core/App.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Version.cpp
    ${PROJECT_SOURCE_DIR}/src/core/cityhash/city.cc
    ${PROJECT_SOURCE_DIR}/src/core/cityhash/city.h
    ${PROJECT_SOURCE_DIR}/src/core/llvm/SmallVector.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Throw.cpp
    ${PROJECT_SOURCE_DIR}/src/core/System.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Threading${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/System${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/IO${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/DynamicLibrary${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/Cryptography${PLATFORM_SPECIFIC_CPP}
    ${PROJECT_SOURCE_DIR}/src/core/hash/md5.c
    ${PROJECT_SOURCE_DIR}/src/core/hash/sha1.c
    ${PROJECT_SOURCE_DIR}/src/core/hash/sha256.c
    ${PROJECT_SOURCE_DIR}/src/core/hash/sha512.c
    ${PROJECT_SOURCE_DIR}/src/core/hash/sha3.c)

set_property(TARGET brisk-core PROPERTY EXPORT_NAME Core)
add_library(Brisk::Core ALIAS brisk-core)

if (WIN32)
    target_sources(brisk-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/COMInit.cpp)
endif ()
if (APPLE)
    target_sources(brisk-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/AutoreleasePool.mm)
endif ()

target_include_directories(brisk-core PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                             $<INSTALL_INTERFACE:include>)

if (WIN32)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:_CRT_SECURE_NO_WARNINGS=1>)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:UNICODE>)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:WINVER=0x0A00>)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:WIN32=1>)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:_WIN32_WINNT=0xA00>)
    target_compile_definitions(brisk-core PUBLIC $<BUILD_INTERFACE:NOMINMAX>)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Werror=unused-result>)
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Werror=return-type>)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Wno-attributes>)
endif ()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Werror=microsoft-template>)
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Werror=infinite-recursion>)
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-Werror=literal-conversion>)
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-fdiagnostics-absolute-paths>)
endif ()

set(_DEP_PUBLIC PUBLIC)
set(_DEP_PRIVATE PRIVATE)

include(Deps.cmake)

unset(_DEP_PUBLIC)
unset(_DEP_PRIVATE)

if (WIN32)
    target_link_options(brisk-core PUBLIC $<BUILD_INTERFACE:-NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/types.natvis>
                        $<INSTALL_INTERFACE:-NATVIS:$<INSTALL_PREFIX>/${CMAKE_INSTALL_LIBDIR}/types.natvis>)

    install(FILES types.natvis DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif ()

target_compile_definitions(brisk-core PUBLIC BRISK_CFG_$<CONFIG>=1)

# System libraries and frameworks
if (WIN32)
    target_link_libraries(brisk-core PUBLIC ole32 uuid)
endif ()

if (APPLE)
    target_link_libraries(brisk-core PUBLIC "-framework Foundation")
    target_compile_options(brisk-core PUBLIC $<BUILD_INTERFACE:-fobjc-arc>)
endif ()

set_target_properties(brisk-core PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(brisk-core PRIVATE -wd4003)
endif ()
