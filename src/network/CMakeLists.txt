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

add_library(
    brisk-network STATIC
    ${PROJECT_SOURCE_DIR}/include/brisk/network/UserAgent.hpp ${PROJECT_SOURCE_DIR}/include/brisk/network/Fetch.hpp
    ${PROJECT_SOURCE_DIR}/src/network/UserAgent.cpp ${PROJECT_SOURCE_DIR}/src/network/Fetch.cpp)

set_property(TARGET brisk-network PROPERTY EXPORT_NAME Network)
add_library(Brisk::Network ALIAS brisk-network)

target_link_libraries(brisk-network PUBLIC brisk-core)

set(_DEP_PUBLIC PUBLIC)
set(_DEP_PRIVATE PRIVATE)

include(Deps.cmake)

unset(_DEP_PUBLIC)
unset(_DEP_PRIVATE)

set_target_properties(brisk-network PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}")
