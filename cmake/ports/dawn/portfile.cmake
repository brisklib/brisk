vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/dawn
    REF "0a552b460b2624b2e43b129e10157feba5220c19"
    SHA512 fc5c17bde6934fa79f2df724c3fd22271efa3f2f18e75aa649de5c070601462722e0cafd265c90375b6763f9de98a56a5045017b307af423b3e6903eb07bf042
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

configure_file("${CMAKE_CURRENT_LIST_DIR}/DawnConfig.cmake.in"
        "${CURRENT_PACKAGES_DIR}/share/${PORT}/DawnConfig.cmake" @ONLY)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
