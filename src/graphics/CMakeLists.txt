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
    brisk-graphics STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Renderer.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/ImageFormats.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/ImageTransform.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Color.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Matrix.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/RawCanvas.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Canvas.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/SVG.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/RenderState.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Fonts.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Path.hpp
    ${PROJECT_SOURCE_DIR}/include/brisk/graphics/Offscreen.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Gradients.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/SVG.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_png.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_jpeg.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_bmp.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats_webp.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageFormats.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/ImageTransform.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/HtmlParser.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/RawCanvas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Canvas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/RenderState.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/OpenType.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/FlatAllocator.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Atlas.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Atlas.hpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/SingleHeaderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Image.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Path.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Renderer.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/Offscreen.cpp
    #
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vbezier.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vdasher.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vmatrix.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vpath.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vpathmesure.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vraster.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vrect.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/vrle.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_math.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_raster.cpp
    ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype/v_ft_stroker.cpp)

set_property(TARGET brisk-graphics PROPERTY EXPORT_NAME Graphics)
add_library(Brisk::Graphics ALIAS brisk-graphics)

target_link_libraries(brisk-graphics PUBLIC brisk-core)

target_include_directories(brisk-graphics PRIVATE ${PROJECT_SOURCE_DIR}/src/graphics/vector)
target_include_directories(brisk-graphics PRIVATE ${PROJECT_SOURCE_DIR}/src/graphics/vector/freetype)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(brisk-graphics PRIVATE -Wno-deprecated-volatile)
endif ()
target_compile_definitions(brisk-graphics PRIVATE V_NAMESPACE=Brisk)

if (WIN32)
    add_subdirectory(D3D11Renderer)
    target_link_libraries(brisk-graphics PRIVATE brisk-renderer-d3d11)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (MSVC)
        set_source_files_properties(${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp PROPERTIES COMPILE_FLAGS /fp:precise)
    else ()
        set_source_files_properties(${PROJECT_SOURCE_DIR}/src/graphics/Fonts.cpp PROPERTIES COMPILE_FLAGS
                                                                                            -fno-fast-math)
    endif ()
endif ()

target_compile_definitions(brisk-graphics PRIVATE V_NAMESPACE=Brisk)

add_library(brisk-i18n STATIC ${PROJECT_SOURCE_DIR}/src/graphics/I18n_fallback.cpp)
target_link_libraries(brisk-i18n PUBLIC brisk-core)
add_library(Brisk::I18n ALIAS brisk-i18n)
set_property(TARGET brisk-i18n PROPERTY EXPORT_NAME I18n)

set(ICU_DT icudt74l.dat)
file(SIZE "${PROJECT_SOURCE_DIR}/resources/icu/${ICU_DT}" ICUDT_SIZE)

add_library(brisk-i18n-icu STATIC ${PROJECT_SOURCE_DIR}/src/graphics/I18n.cpp)
add_library(Brisk::I18n-ICU ALIAS brisk-i18n-icu)
set_property(TARGET brisk-i18n-icu PROPERTY EXPORT_NAME I18n-ICU)
target_link_libraries(brisk-i18n-icu PUBLIC brisk-core)
target_compile_definitions(brisk-i18n-icu PRIVATE ICUDT_SIZE=${ICUDT_SIZE})
brisk_target_link_resource(
    brisk-i18n-icu PRIVATE icudt
    INPUT "${PROJECT_SOURCE_DIR}/resources/icu/${ICU_DT}"
    BROTLI)

if (BRISK_ICU)
    target_link_libraries(brisk-graphics INTERFACE $<BUILD_INTERFACE:brisk-i18n-icu>)
else()
    target_link_libraries(brisk-graphics INTERFACE $<BUILD_INTERFACE:brisk-i18n>)
endif ()

if (BRISK_WEBGPU)
    add_subdirectory(WebGPURenderer)
endif ()

set(_DEP_PUBLIC PUBLIC)
set(_DEP_PRIVATE PRIVATE)

include(Deps.cmake)

unset(_DEP_PUBLIC)
unset(_DEP_PRIVATE)

if (BRISK_WEBGPU)
    if (TARGET Dawn)
        target_link_libraries(brisk-graphics PRIVATE brisk-renderer-webgpu)
        target_compile_definitions(brisk-graphics PRIVATE BRISK_WEBGPU=1)
    else ()
        message(FATAL_ERROR "BRISK_WEBGPU is ON but Dawn target is missing")
    endif ()
endif ()

set_target_properties(brisk-graphics PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")

if (APPLE)
    target_link_libraries(brisk-graphics PUBLIC "-framework Metal" "-framework IOKit" "-framework QuartzCore"
                                                "-framework CoreGraphics" "-framework CoreVideo")
endif ()
