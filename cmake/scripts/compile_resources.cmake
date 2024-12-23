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
set(_RESOURCES_DIR
    ${CMAKE_BINARY_DIR}/resources
    CACHE PATH "")
set(_RESOURCES_DATA_DIR
    ${CMAKE_BINARY_DIR}/resources-data
    CACHE PATH "")

file(MAKE_DIRECTORY ${_RESOURCES_DIR}/resources ${_RESOURCES_DATA_DIR})

find_program(
    BIN2C_TOOL
    NAMES bin2c
    REQUIRED)

function (brisk_target_link_resource TARGET MODE NAME)
    cmake_parse_arguments("EMBED" "BROTLI;GZIP;LZ4" "INPUT" "" ${ARGN})

    set(FLAGS)
    if (EMBED_BROTLI)
        list(APPEND FLAGS "--br")
    elseif (EMBED_GZIP)
        list(APPEND FLAGS "--gz")
    elseif (EMBED_LZ4)
        list(APPEND FLAGS "--lz4")
    endif ()

    set(DATA ${_RESOURCES_DATA_DIR}/${NAME}.c)
    set(HDR ${_RESOURCES_DIR}/resources/${NAME}.hpp)

    string(MAKE_C_IDENTIFIER ${NAME} CNAME)

    add_custom_command(
        OUTPUT ${DATA} ${HDR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${BIN2C_TOOL} --id ${CNAME} ${FLAGS} ${DATA} ${HDR} ${EMBED_INPUT}
        DEPENDS ${EMBED_INPUT}
        VERBATIM)

    set_source_files_properties(${DATA} ${HDR} PROPERTIES GENERATED TRUE)

    target_sources(${TARGET} PRIVATE ${DATA} ${HDR})
    target_include_directories(${TARGET} ${MODE} $<BUILD_INTERFACE:${_RESOURCES_DIR}>)
    target_compile_definitions(${TARGET} PRIVATE BRISK_RESOURCE_${CNAME}=1)
endfunction ()
