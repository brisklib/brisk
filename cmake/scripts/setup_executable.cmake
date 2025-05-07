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
include(${CMAKE_CURRENT_LIST_DIR}/${CMAKE_SYSTEM_NAME}/setup_executable.cmake)

function (brisk_setup_executable TARGET)

    if (NOT APP_VERSION_MAJOR)
        set(APP_VERSION_MAJOR 0)
    endif ()
    if (NOT APP_VERSION_MINOR)
        set(APP_VERSION_MINOR 0)
    endif ()
    if (NOT APP_VERSION_RELEASE)
        set(APP_VERSION_RELEASE 0)
    endif ()
    if (NOT APP_VERSION_PATCH)
        set(APP_VERSION_PATCH 0)
    endif ()

    set(APP_METADATA_FILE ${_BRISK_INCLUDE_DIR}/brisk/application/main/Metadata.Defines.hpp.in)

    configure_file(${APP_METADATA_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_meta/Metadata.Defines.hpp @ONLY)
    target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_meta)

    set_target_properties(
        ${TARGET}
        PROPERTIES
            CXX_STANDARD 20
            CMAKE_CXX_STANDARD_REQUIRED ON
            CMAKE_CXX_EXTENSIONS OFF)

    setup_executable_platform(${TARGET})

    brisk_bundle_resources(${TARGET})
endfunction ()
