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
function (setup_executable_platform TARGET)
    set(ICON_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/icon)

    set_target_properties(${TARGET} PROPERTIES MACOSX_BUNDLE TRUE)

    configure_file(${_BRISK_INCLUDE_DIR}/brisk/application/main/MacOSXBundleInfo.plist.in
                   ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.MacOSXBundleInfo.plist @ONLY)

    set_target_properties(${TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                                               ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.MacOSXBundleInfo.plist)

    if (APP_ICON)
        set(OUT_ICONSET ${ICON_OUT_DIR}/${TARGET}.iconset)
        set(OUT_ICON ${ICON_OUT_DIR}/${TARGET}.icns)
        file(MAKE_DIRECTORY ${OUT_ICONSET})
        add_custom_command(
            OUTPUT ${OUT_ICON}
            COMMAND sips -z 16 16 ${APP_ICON} --out ${OUT_ICONSET}/icon_16x16.png
            COMMAND sips -z 32 32 ${APP_ICON} --out ${OUT_ICONSET}/icon_16x16@2x.png
            COMMAND sips -z 32 32 ${APP_ICON} --out ${OUT_ICONSET}/icon_32x32.png
            COMMAND sips -z 64 64 ${APP_ICON} --out ${OUT_ICONSET}/icon_32x32@2x.png
            COMMAND sips -z 128 128 ${APP_ICON} --out ${OUT_ICONSET}/icon_128x128.png
            COMMAND sips -z 256 256 ${APP_ICON} --out ${OUT_ICONSET}/icon_128x128@2x.png
            COMMAND sips -z 256 256 ${APP_ICON} --out ${OUT_ICONSET}/icon_256x256.png
            COMMAND sips -z 512 512 ${APP_ICON} --out ${OUT_ICONSET}/icon_256x256@2x.png
            COMMAND sips -z 512 512 ${APP_ICON} --out ${OUT_ICONSET}/icon_512x512.png
            COMMAND sips -z 1024 1024 ${APP_ICON} --out ${OUT_ICONSET}/icon_512x512@2x.png
            COMMAND sips -z 1024 1024 ${APP_ICON} --out ${OUT_ICONSET}/icon_1024x1024.png
            COMMAND iconutil -c icns --output ${OUT_ICON} ${OUT_ICONSET}
            DEPENDS ${APP_ICON}
            VERBATIM)
        set_source_files_properties(${OUT_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(${TARGET} PRIVATE ${OUT_ICON})

        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.MacOSXBundleInfo.plist OBJECT_DEPENDS
                                    ${OUT_ICON})
    endif ()

    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.MacOSXBundleInfo.plist)
    target_sources(${TARGET} PRIVATE ${_BRISK_INCLUDE_DIR}/brisk/application/main/Main_Darwin.mm)
endfunction ()
