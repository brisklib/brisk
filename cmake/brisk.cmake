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
include_guard(GLOBAL)

if (CMAKE_VERSION VERSION_LESS "3.22.0")
    message(FATAL_ERROR "At least CMake 3.22 is required for Brisk. Your version is ${CMAKE_VERSION}")
endif ()

get_filename_component(BRISK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(BRISK_ROOT
    ${BRISK_ROOT}
    CACHE PATH "" FORCE)

include(${CMAKE_CURRENT_LIST_DIR}/init.cmake)

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/../include/brisk/core/Version.hpp BRISK_VERSION
     REGEX "#define BRISK_VERSION_(MINOR|MAJOR|PATCH)")
string(REGEX MATCHALL "[0-9]+" BRISK_VERSION_MATCH ${BRISK_VERSION})
string(REPLACE ";" "." BRISK_VERSION "${BRISK_VERSION_MATCH}")

message(STATUS "BRISK ${BRISK_VERSION}, ROOT = ${BRISK_ROOT}")

macro (brisk_setup)
    add_subdirectory(${BRISK_ROOT} brisk-bin)
endmacro ()
