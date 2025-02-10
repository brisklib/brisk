get_property(
    _BRISK_GUI
    TARGET brisk-gui
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_GUI}" STREQUAL "")
    set(_BRISK_GUI brisk-gui)
endif ()

brisk_target_resources(
    ${_BRISK_GUI}
    fonts/default/bold.ttf="${BRISK_RESOURCES_DIR}/fonts/Lato-Black.ttf"|BROTLI
    fonts/default/regular.ttf="${BRISK_RESOURCES_DIR}/fonts/Lato-Medium.ttf"|BROTLI
    fonts/default/light.ttf="${BRISK_RESOURCES_DIR}/fonts/Lato-Light.ttf"|BROTLI
    fonts/mono/regular.ttf="${BRISK_RESOURCES_DIR}/fonts/SourceCodePro-Medium.ttf"|BROTLI
    fonts/icons.ttf="${BRISK_RESOURCES_DIR}/fonts/Lucide.ttf"|BROTLI
    fonts/emoji.ttf="${BRISK_RESOURCES_DIR}/fonts/NotoColorEmoji-SVG.otf"|BROTLI)
brisk_target_resources(${_BRISK_GUI} images/brisk-white.png="${BRISK_RESOURCES_DIR}/images/brisk-white.png"
                       images/brisk.svg="${BRISK_RESOURCES_DIR}/images/brisk.svg"|BROTLI)
