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
    brisk-renderer-webgpu STATIC
    WebGPURenderer.hpp
    RenderDevice.hpp
    RenderEncoder.hpp
    ImageRenderTarget.hpp
    WindowRenderTarget.hpp
    ImageBackend.hpp
    WebGPURenderer.cpp
    RenderDevice.cpp
    RenderEncoder.cpp
    ImageRenderTarget.cpp
    WindowRenderTarget.cpp
    ImageBackend.cpp
    WebGPU.cpp
    RenderDevice${PLATFORM_SPECIFIC_CPP}
    WindowRenderTarget${PLATFORM_SPECIFIC_CPP})

target_link_libraries(brisk-renderer-webgpu PUBLIC brisk-core)

if (WIN32)
    target_link_libraries(brisk-renderer-webgpu PUBLIC d3d11 dxgi)
endif ()
if (APPLE)
    target_link_libraries(brisk-renderer-webgpu PUBLIC "-framework IOSurface")
endif ()
