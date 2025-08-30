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
add_library(
    brisk-renderer-d3d11 STATIC
    Renderer.hpp
    RenderDevice.hpp
    RenderEncoder.hpp
    ImageRenderTarget.hpp
    WindowRenderTarget.hpp
    ImageBackend.hpp
    Common.hpp
    Renderer.cpp
    RenderDevice.cpp
    RenderEncoder.cpp
    ImageRenderTarget.cpp
    WindowRenderTarget.cpp
    ImageBackend.cpp
    Common.cpp)

foreach (SHADER_TYPE IN ITEMS "fragment" "vertex")
    set(FXC_MODE_fragment ps_5_0)
    set(FXC_MODE_vertex vs_5_0)

    find_program(FXC fxc.exe)

    if (NOT FXC)
        set(FXC fxc.exe) # Fallback for Visual Studio generator
    endif ()

    add_custom_command(
        OUTPUT ${D3D11_SHADER_DIR}/${SHADER_TYPE}.fxc
        COMMAND
            ${FXC} -nologo -T ${FXC_MODE_${SHADER_TYPE}}
            # -WX
            -Gis -Zpr
            -O3 -E ${SHADER_TYPE}Main -Fo ${D3D11_SHADER_DIR}/${SHADER_TYPE}.fxc
            -Fc ${D3D11_SHADER_DIR}/${SHADER_TYPE}.fxc.asm
            ${CMAKE_CURRENT_LIST_DIR}/${SHADER_TYPE}.hlsl
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${SHADER_TYPE}.hlsl
        VERBATIM)

    set_source_files_properties(${D3D11_SHADER_DIR}/${SHADER_TYPE}.fxc PROPERTIES GENERATED TRUE)

endforeach ()

target_link_libraries(brisk-renderer-d3d11 PUBLIC brisk-core)

target_link_libraries(brisk-renderer-d3d11 PUBLIC d3d11 dxgi)

install(FILES ${D3D11_SHADER_DIR}/fragment.fxc ${D3D11_SHADER_DIR}/vertex.fxc DESTINATION "resources/shaders")

target_sources(brisk-renderer-d3d11 PRIVATE ${D3D11_SHADER_DIR}/fragment.fxc ${D3D11_SHADER_DIR}/vertex.fxc)

set_source_files_properties(${D3D11_SHADER_DIR}/fragment.fxc ${D3D11_SHADER_DIR}/vertex.fxc PROPERTIES HEADER_FILE_ONLY
                                                                                                       TRUE)
