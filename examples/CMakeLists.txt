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

add_subdirectory(calc)
add_subdirectory(showcase)
add_subdirectory(splash-screen)
add_subdirectory(dashboard)

if (glfw3_FOUND)
    add_subdirectory(own-window)
    add_subdirectory(own-window-gui)
endif ()

if (BRISK_WEBGPU)
    add_subdirectory(webgpu-demo)
endif ()

add_subdirectory(counter)
add_subdirectory(temperature_converter)
add_subdirectory(flight_booker)
add_subdirectory(timer)
add_subdirectory(crud)
