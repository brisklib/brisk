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
include_guard(GLOBAL)

function (add_autotests target)
    cmake_parse_arguments(AUTOTESTS "" "" "SOURCES;LIBRARIES;DEFINITIONS;ENVIRONMENT" ${ARGN})

    find_package(Catch2 CONFIG REQUIRED)

    add_executable(${target}_tests ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src/core/AutoTestsMain.cpp
                                   ${AUTOTESTS_SOURCES})
    target_link_libraries(${target}_tests PUBLIC brisk-executable brisk-core Catch2::Catch2 Catch2::Catch2WithMain
                                                 ${AUTOTESTS_LIBRARIES})
    target_compile_definitions(${target}_tests PUBLIC TESTING=1)
    target_compile_definitions(${target}_tests PUBLIC ${AUTOTESTS_DEFINITIONS})
    target_compile_definitions(${target}_tests PUBLIC PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    target_compile_definitions(${target}_tests PUBLIC PROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}")
    if (BRISK_INTERACTIVE_TESTS)
        target_compile_definitions(${target}_tests PUBLIC BRISK_INTERACTIVE_TESTS=1)
    endif ()
    # target_compile_definitions(${target}_tests PUBLIC BRISK_ASSERT_THROWS=1)

    brisk_bundle_resources(${target}_tests)

    add_test(NAME ${target}_autotests COMMAND ${target}_tests)

    if (AUTOTESTS_ENVIRONMENT)
        set_tests_properties(${target}_autotests PROPERTIES ENVIRONMENT ${AUTOTESTS_ENVIRONMENT})
    endif ()
endfunction ()
