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
cmake_minimum_required(VERSION 3.16)

project(showcase)

if (NOT _BRISK_INCLUDE_DIR)
    find_package(Brisk CONFIG REQUIRED)
endif ()

add_executable(
    showcase
    src/showcase.cpp
    src/ShowcaseComponent.hpp
    src/ShowcaseComponent.cpp
    src/Buttons.hpp
    src/Buttons.cpp
    src/Editors.hpp
    src/Editors.cpp
    src/Visual.hpp
    src/Visual.cpp
    src/Layout.hpp
    src/Layout.cpp
    src/Dialogs.hpp
    src/Dialogs.cpp
    src/Dropdowns.hpp
    src/Dropdowns.cpp
    src/Typography.hpp
    src/Typography.cpp
    src/Messenger.hpp
    src/Messenger.cpp
    src/Binding.hpp
    src/Binding.cpp)

target_link_libraries(showcase PRIVATE Brisk::Widgets Brisk::Executable)

brisk_metadata(
    VENDOR "Brisk"
    NAME "Showcase"
    DESCRIPTION "Brisk library feature demo"
    VERSION "0.1.2.3-suffix"
    COPYRIGHT "© 2024 Brisk"
    ICON ${PROJECT_SOURCE_DIR}/resources/icon.png
    APPLE_BUNDLE "com.brisklib.showcase")

brisk_target_link_resource(
    showcase PRIVATE cat.svg
    INPUT "${PROJECT_SOURCE_DIR}/resources/cat.svg"
    BROTLI)
brisk_target_link_resource(showcase PRIVATE hot-air-balloons.jpg
                           INPUT "${PROJECT_SOURCE_DIR}/resources/hot-air-balloons.jpg")
brisk_target_link_resource(showcase PRIVATE wp1.webp INPUT "${PROJECT_SOURCE_DIR}/resources/wp1.webp")
brisk_target_link_resource(
    showcase PRIVATE countries.json
    INPUT "${PROJECT_SOURCE_DIR}/resources/countries.json"
    BROTLI)

brisk_setup_executable(showcase)
