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
# Brisk is dual-licensed under the GNU General Public License version 2 (GPL-2.0+), and a commercial license. You may
# use, modify, and distribute this software under the terms of the GPL-2.0+ license if you comply with its conditions.
#
# You should have received a copy of the GNU General Public License along with this program. If not, see
# <http://www.gnu.org/licenses/>.
#
# If you do not wish to be bound by the terms of the GNU GPL, you may purchase a commercial license. For commercial
# licensing options, please visit: https://brisklib.com
#
cmake_minimum_required(VERSION 3.22)

project(font-specimen)

if (NOT _BRISK_INCLUDE_DIR)
    find_package(Brisk CONFIG REQUIRED)
endif ()

add_executable(font-specimen font-specimen.cpp)
target_link_libraries(font-specimen PRIVATE brisk-graphics brisk-executable)

set(BRISK_FORCE_CONSOLE ON)
brisk_setup_executable(font-specimen)
