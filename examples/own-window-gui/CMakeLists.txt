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

project(own-window-gui)

find_package(glfw3 CONFIG)

if (NOT _BRISK_INCLUDE_DIR)
    find_package(Brisk CONFIG REQUIRED)
endif ()

add_executable(own-window-gui own-window-gui.cpp ../own-window/WindowGlfw.cpp)
if (APPLE)
    set_source_files_properties(
        ../own-window/WindowGlfw.cpp
        PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

target_link_libraries(own-window-gui PRIVATE brisk-widgets glfw)

brisk_target_remove_resources(own-window-gui fonts/default/bold.ttf fonts/default/light.ttf)

brisk_setup_executable(own-window-gui)

# This disables Brisk implementation of main function (wWinMain on Windows)
target_compile_definitions(own-window-gui PRIVATE BRISK_NO_MAIN=1)
