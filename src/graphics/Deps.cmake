get_property(
    _BRISK_GRAPHICS
    TARGET brisk-graphics
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_GRAPHICS}" STREQUAL "")
    set(_BRISK_GRAPHICS brisk-graphics)
endif ()

# >PNG
find_package(PNG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} PNG::PNG)
# /PNG

# >WebP
find_package(WebP CONFIG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} WebP::webp WebP::webpdecoder)
# /WebP

# >libjpeg-turbo
find_package(libjpeg-turbo CONFIG REQUIRED)
target_link_libraries(
    ${_BRISK_GRAPHICS} ${_DEP_PRIVATE}
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>)
# /libjpeg-turbo

# >lunasvg
find_package(unofficial-lunasvg CONFIG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} unofficial::lunasvg::lunasvg)
# /lunasvg

# >harfbuzz
find_package(harfbuzz CONFIG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} harfbuzz)
# /harfbuzz

# >freetype
find_package(freetype CONFIG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} freetype)
# /freetype

# >icu
if (BRISK_ICU)
    find_package(ICU COMPONENTS uc)
    if (TARGET ICU::uc)
        target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} ICU::uc)
    endif ()
endif ()
# /icu

if (BRISK_WEBGPU)
    find_package(Dawn CONFIG)
    if (TARGET Brisk::brisk-renderer-webgpu)
        target_link_libraries(Brisk::brisk-renderer-webgpu ${_DEP_PRIVATE} Dawn)
    else ()
        target_link_libraries(brisk-renderer-webgpu ${_DEP_PRIVATE} Dawn)
    endif ()
endif ()
