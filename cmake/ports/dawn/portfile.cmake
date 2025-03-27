vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/dawn
    REF "ac1885d2426eace7038fe48ea59179b360469bcf"
    SHA512 95d222a9b22eaf029e878bca516c8145cb3c81542885ef170664bfc75f8215207e93ddf78dbd5cec9745c62a2187742d37716a0b48a8801b71489f1616d3f5a5
    PATCHES 
        install.patch
)

vcpkg_find_acquire_program(PYTHON3)
get_filename_component(PYTHON3_DIR "${PYTHON3}" DIRECTORY)
vcpkg_add_to_path("${PYTHON3_DIR}")

vcpkg_find_acquire_program(GIT)
get_filename_component(GIT_EXE_PATH "${GIT}" DIRECTORY)
vcpkg_add_to_path("${GIT_EXE_PATH}")

execute_process(COMMAND ${PYTHON3} tools/fetch_dawn_dependencies.py
    WORKING_DIRECTORY ${SOURCE_PATH})

set(DESKTOP_GL OFF)
set(USE_WAYLAND OFF)
if (VCPKG_TARGET_IS_LINUX)
    set(DESKTOP_GL ON)
    set(USE_WAYLAND ON)
endif ()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_SHARED_LIBS=OFF
        -DDAWN_BUILD_MONOLITHIC_LIBRARY=ON
        -DDAWN_BUILD_SAMPLES=OFF
        -DTINT_BUILD_TESTS=OFF
        -DDAWN_USE_GLFW=OFF
        -DDAWN_USE_WINDOWS_UI=OFF
        -DTINT_BUILD_CMD_TOOLS=OFF
        -DDAWN_ENABLE_INSTALL=ON
        -DTINT_ENABLE_INSTALL=OFF
        -DABSL_ENABLE_INSTALL=OFF

        -DDAWN_ENABLE_D3D11=OFF
        -DDAWN_ENABLE_NULL=OFF
        -DDAWN_ENABLE_DESKTOP_GL=${DESKTOP_GL}
        -DDAWN_USE_WAYLAND=${USE_WAYLAND}
        -DDAWN_ENABLE_OPENGLES=OFF
        -DTINT_BUILD_GLSL_VALIDATOR=OFF
        -DTINT_BUILD_SPV_READER=OFF
        -DTINT_BUILD_WGSL_WRITER=ON # must be ON for shader cache
)

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/Dawn")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
