get_property(
    _BRISK_GRAPHICS
    TARGET brisk-graphics
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_GRAPHICS}" STREQUAL "")
    set(_BRISK_GRAPHICS brisk-graphics)
endif ()

get_property(
    _BRISK_I18N_ICU
    TARGET brisk-i18n-icu
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_I18N_ICU}" STREQUAL "")
    set(_BRISK_I18N_ICU brisk-i18n-icu)
endif ()

if (BRISK_WEBGPU)
    get_property(
        _BRISK_RENDERER_WEBGPU
        TARGET brisk-renderer-webgpu
        PROPERTY ALIASED_TARGET)
    if ("${_BRISK_RENDERER_WEBGPU}" STREQUAL "")
        set(_BRISK_RENDERER_WEBGPU brisk-renderer-webgpu)
    endif ()
endif ()

if (WIN32)
    get_property(
        _BRISK_RENDERER_D3D11
        TARGET brisk-renderer-d3d11
        PROPERTY ALIASED_TARGET)
    if ("${_BRISK_RENDERER_D3D11}" STREQUAL "")
        set(_BRISK_RENDERER_D3D11 brisk-renderer-d3d11)
    endif ()
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
find_package(
    ICU
    COMPONENTS uc
    REQUIRED)
target_link_libraries(${_BRISK_I18N_ICU} ${_DEP_PRIVATE} ICU::uc)
# /icu

if (BRISK_WEBGPU)
    find_package(Dawn CONFIG REQUIRED)
    target_link_libraries(${_BRISK_RENDERER_WEBGPU} ${_DEP_PRIVATE} Dawn)

    brisk_target_link_resource(
        ${_BRISK_RENDERER_WEBGPU} PUBLIC "webgpu/webgpu.wgsl"
        INPUT ${BRISK_RESOURCES_DIR}/shaders/webgpu.wgsl
        BROTLI)
endif ()

# >tinyxml2
find_package(tinyxml2 CONFIG REQUIRED)
target_link_libraries(${_BRISK_GRAPHICS} ${_DEP_PRIVATE} tinyxml2::tinyxml2)
# >tinyxml2

brisk_target_link_resource(
    ${_BRISK_I18N_ICU} PRIVATE "internal/icudt.dat"
    INPUT ${BRISK_RESOURCES_DIR}/icu/${ICU_DT}
    BROTLI)

if (WIN32)
    if (_EXPORT_MODE)
        set(SHADER_DIR ${BRISK_RESOURCES_DIR}/shaders)
    else ()
        set(SHADER_DIR ${D3D11_SHADER_DIR})
    endif ()

    brisk_target_link_resource(
        ${_BRISK_RENDERER_D3D11} PUBLIC "d3d11/fragment.fxc"
        INPUT ${SHADER_DIR}/fragment.fxc
        BROTLI)
    brisk_target_link_resource(
        ${_BRISK_RENDERER_D3D11} PUBLIC "d3d11/vertex.fxc"
        INPUT ${SHADER_DIR}/vertex.fxc
        BROTLI)

    if (NOT _EXPORT_MODE)
        target_sources(${_BRISK_RENDERER_D3D11} PRIVATE ${D3D11_SHADER_DIR}/fragment.fxc ${D3D11_SHADER_DIR}/vertex.fxc)
        set_source_files_properties(${D3D11_SHADER_DIR}/fragment.fxc ${D3D11_SHADER_DIR}/vertex.fxc
                                    PROPERTIES HEADER_FILE_ONLY ON)
    endif ()
endif ()
