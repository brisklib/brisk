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
set(_RESOURCES_DATA_DIR
    ${CMAKE_BINARY_DIR}/resources
    CACHE PATH "")

file(MAKE_DIRECTORY ${_RESOURCES_DATA_DIR})

set(_SCRIPTS_DIR ${CMAKE_CURRENT_LIST_DIR})

find_program(PACK_RESOURCE_TOOL NAMES pack_resource REQUIRED)

define_property(
    TARGET
    PROPERTY "BRISK_RESOURCES"
    INHERITED)

function (strip_build_interface input_string output_var)
    string(REGEX REPLACE "\\$<BUILD_INTERFACE:([^>]*)>" "\\1" stripped_string "${input_string}")
    set(${output_var}
        "${stripped_string}"
        PARENT_SCOPE)
endfunction ()

function (get_linked_libraries_recursive result target)
    list(APPEND visited_targets ${target})
    get_target_property(interface_libs ${target} INTERFACE_LINK_LIBRARIES)
    get_target_property(link_libs ${target} LINK_LIBRARIES)

    strip_build_interface("${interface_libs}" interface_libs)
    strip_build_interface("${link_libs}" link_libs)

    list(APPEND link_libs ${interface_libs})
    set(result_list "")
    foreach (lib ${link_libs})
        if (TARGET ${lib})
            list(FIND visited_targets ${lib} is_visited)
            if (${is_visited} EQUAL -1)
                get_linked_libraries_recursive(recursive_libs ${lib})
                list(APPEND result_list ${recursive_libs} ${lib})
            endif ()
        endif ()
    endforeach ()
    set(visited_targets
        ${visited_targets}
        PARENT_SCOPE)
    set(${result}
        ${result_list}
        PARENT_SCOPE)
endfunction ()

function (brisk_target_resources target)
    set(absolute_path_list)
    foreach (entry ${ARGN})
        string(REGEX MATCH "^([^=]*)=([^\|]*)(\|.*)?$" match "${entry}")
        set(key "${CMAKE_MATCH_1}")
        set(value "${CMAKE_MATCH_2}")
        set(filters "${CMAKE_MATCH_3}")
        string(REPLACE "\"" "" value "${value}")
        get_filename_component(full_path ${value} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
        list(APPEND absolute_path_list ${key}=${full_path}${filters})
    endforeach ()
    set_property(
        TARGET ${target}
        APPEND
        PROPERTY BRISK_RESOURCES "${absolute_path_list}")
endfunction ()

function (brisk_target_resource target key)
    cmake_parse_arguments("R" "BROTLI;GZIP;LZ4" "INPUT" "" ${ARGN})
    set(filters "")
    if (R_GZIP)
        set(filters "|GZIP")
    elseif (R_LZ4)
        set(filters "|LZ4")
    elseif (R_BROTLI)
        set(filters "|BROTLI")
    endif ()
    brisk_target_resources(${target} ${key}=${R_INPUT}${filters})
endfunction ()

function (brisk_target_link_resource target mode key)
    cmake_parse_arguments("R" "BROTLI;GZIP;LZ4" "INPUT" "" ${ARGN})
    set(filters "")
    if (R_GZIP)
        set(filters "|GZIP")
    elseif (R_LZ4)
        set(filters "|LZ4")
    elseif (R_BROTLI)
        set(filters "|BROTLI")
    endif ()
    brisk_target_resources(${target} ${key}=${R_INPUT}${filters})
endfunction ()

if (CMAKE_GENERATOR MATCHES "^Visual Studio")
    set(GEN_VS
        TRUE
        CACHE INTERNAL "")
endif ()

function (brisk_bundle_resources target)
    get_linked_libraries_recursive(dependent_targets ${target})

    list(APPEND dependent_targets ${target})

    set(resource_list "")
    foreach (dep ${dependent_targets})
        get_target_property(resources ${dep} BRISK_RESOURCES)
        list(APPEND resource_list ${resources})
    endforeach ()

    set(resource_keys "")
    foreach (entry ${resource_list})
        string(REGEX MATCH "^([^=]*)=(.*)$" match "${entry}")
        set(key "${CMAKE_MATCH_1}")
        set(value "${CMAKE_MATCH_2}")
        set("resource_map__${key}" ${value})
        list(APPEND resource_keys ${key})
    endforeach ()
    list(REMOVE_DUPLICATES resource_keys)
    list(SORT resource_keys)

    string(MAKE_C_IDENTIFIER ${target} target_id)

    if (GEN_VS)
        set(resource_dir ${_RESOURCES_DATA_DIR}/${target_id})
        file(MAKE_DIRECTORY "${_RESOURCES_DATA_DIR}/${target_id}")
    else ()
        set(resource_dir ${_RESOURCES_DATA_DIR})
    endif ()

    set(resource_entries_c "${resource_dir}/entries__${target_id}.c")

    file(
        WRITE ${resource_entries_c}
        [=[
#include <brisk/core/internal/Resources.h>
]=])
    set(entries "")

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        set(IS_MSVC TRUE)
    endif ()

    foreach (key ${resource_keys})
        string(REGEX MATCH "^([^\\|]*)(\\|(.*))?$" match "${resource_map__${key}}")
        set(full_path "${CMAKE_MATCH_1}")
        file(TO_CMAKE_PATH "${full_path}" cm_full_path)
        set(filters "${CMAKE_MATCH_3}")
        string(TOUPPER "${filters}" filters)
        string(MAKE_C_IDENTIFIER "${key}" ckey)

        string(SHA256 hash ${cm_full_path})
        string(SUBSTRING "${hash}" 0 8 hash)

        set(full_key ${ckey}_${hash})
        set(resource_key_c "${resource_dir}/${full_key}${filters}.c")
        get_filename_component(dir_path "${resource_key_c}" DIRECTORY)
        file(MAKE_DIRECTORY "${dir_path}")

        set(rsrc_flags "ResourceCompression_None")
        if ((NOT "${filters}" STREQUAL "") OR IS_MSVC)
            set(flags "")
            if ("${filters}" STREQUAL "BROTLI")
                set(rsrc_flags "ResourceCompression_Brotli")
                set(flags "--br")
            elseif ("${filters}" STREQUAL "LZ4")
                set(rsrc_flags "ResourceCompression_LZ4")
                set(flags "--lz4")
            elseif ("${filters}" STREQUAL "GZIP")
                set(rsrc_flags "ResourceCompression_GZip")
                set(flags "--gz")
            elseif ("${filters}" STREQUAL "ZLIB")
                set(rsrc_flags "ResourceCompression_ZLib")
                set(flags "--zlib")
            else ()
                if (NOT "${filters}" STREQUAL "")
                    message(FATAL_ERROR "Unrocognized option in brisk_target_resource: ${filters}")
                endif ()
            endif ()
            if (IS_MSVC)
                list(APPEND flags "--c" "${full_key}")
            endif ()

            if (IS_MSVC)
                set(output_file ${resource_key_c})
            else ()
                set(output_file ${resource_dir}/${full_key}${filters}.bin)
            endif ()

            set(_BRISK_GENERATED_RESOURCES ${BRISK_GENERATED_RESOURCES})
            if (NOT "${output_file}" IN_LIST _BRISK_GENERATED_RESOURCES)

                list(APPEND _BRISK_GENERATED_RESOURCES "${output_file}")
                set(BRISK_GENERATED_RESOURCES
                    "${_BRISK_GENERATED_RESOURCES}"
                    CACHE INTERNAL "Resources generated by Brisk")

                set(CMD ${PACK_RESOURCE_TOOL} ${flags} ${output_file} ${full_path})

                add_custom_command(
                    OUTPUT ${output_file}
                    COMMAND ${CMAKE_COMMAND} "-DIN=${full_path}" "-DOUT=${output_file}" "-DCMD=${CMD}" -P
                            ${_SCRIPTS_DIR}/pack_resource.cmake
                    # COMMAND ${PACK_RESOURCE_TOOL} ${flags} ${output_file} ${full_path}
                    DEPENDS ${full_path}
                    VERBATIM)
            endif ()

            if (IS_MSVC)
                set_source_files_properties(${resource_key_c} PROPERTIES GENERATED TRUE)
            else ()
                file(WRITE ${resource_key_c}
                     "#include <brisk/core/internal/incbin.h>\n\nINCBIN(${full_key}, \"${output_file}\");\n")
                set_source_files_properties(${resource_key_c} OBJECT_DEPENDS ${output_file})
            endif ()
        else ()
            # no filters and not MSVC
            file(WRITE ${resource_key_c}
                 "#include <brisk/core/internal/incbin.h>\n\nINCBIN(${full_key}, \"${full_path}\");\n")
            set_source_files_properties(${resource_key_c} OBJECT_DEPENDS ${full_path})
        endif ()

        set(entries "${entries}    {\"${key}\", rsrc__${full_key}_data, &rsrc__${full_key}_size, ${rsrc_flags}}, \n")
        file(APPEND ${resource_entries_c} "INCBIN_EXTERN(${full_key});\n")
        target_sources(${target} PRIVATE ${resource_key_c})
    endforeach ()

    list(LENGTH resource_keys resource_count)

    if ("${entries}" STREQUAL "")
        set(entries "    {0}\n")
    endif ()

    file(APPEND ${resource_entries_c} "const struct ResourceEntry resourceEntries[] = {\n${entries}};\n")
    file(APPEND ${resource_entries_c} "const uint32_t resourceEntriesSize = ${resource_count};\n")
    target_sources(${target} PRIVATE ${resource_entries_c})
endfunction ()

set(BRISK_EMPTY_FILE ${CMAKE_CURRENT_BINARY_DIR}/empty-file)

file(TOUCH "${BRISK_EMPTY_FILE}")

function (brisk_target_remove_resources target)
    set(resources "${ARGN}")
    list(TRANSFORM resources APPEND "=${BRISK_EMPTY_FILE}")
    brisk_target_resources(${target} ${resources})
endfunction ()
