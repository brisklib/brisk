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
macro (brisk_metadata)
    cmake_parse_arguments("APP" "" "VENDOR;NAME;DESCRIPTION;HOMEPAGE;COPYRIGHT;VERSION;ICON;APPLE_BUNDLE_ID" "" ${ARGN})

    if ("${APP_VERSION}" MATCHES "^([0-9]+)\.([0-9]+)(\.([0-9]+)(\.([0-9]+))?)?(.*)$")
        set(APP_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(APP_VERSION_MINOR ${CMAKE_MATCH_2})
        set(APP_VERSION_RELEASE ${CMAKE_MATCH_4})
        set(APP_VERSION_PATCH ${CMAKE_MATCH_6})
        set(APP_VERSION_SUFFIX ${CMAKE_MATCH_7})
    else ()
        message(FATAL_ERROR "\"${APP_VERSION}\" doesn't look like valid version string")
    endif ()
endmacro ()
