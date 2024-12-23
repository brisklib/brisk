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

find_program(
    ICOWRITER_TOOL
    NAMES icowriter
    REQUIRED)

function (setup_executable_platform TARGET)

    set(ICON_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/icon)
    if (APP_ICON)
        set(HAS_ICON 1)
    else ()
        set(HAS_ICON 0)
    endif ()

    configure_file(${_BRISK_INCLUDE_DIR}/brisk/application/main/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.app.rc
                   @ONLY)
    if (NOT BRISK_FORCE_CONSOLE)
        set_target_properties(${TARGET} PROPERTIES WIN32_EXECUTABLE TRUE)
        target_compile_definitions(${TARGET} PRIVATE WIN32_GUI=1)
    endif ()

    if (APP_ICON)
        set(OUT_ICON ${ICON_OUT_DIR}/${TARGET}.ico)

        add_custom_command(
            OUTPUT ${OUT_ICON}
            COMMAND ${ICOWRITER_TOOL} ${APP_ICON} ${OUT_ICON}
            DEPENDS ${APP_ICON}
            VERBATIM)

        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.app.rc OBJECT_DEPENDS ${OUT_ICON})
    endif ()

    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.app.rc)
    target_sources(${TARGET} PRIVATE ${_BRISK_INCLUDE_DIR}/brisk/application/main/Main_Windows.cpp)
endfunction ()
