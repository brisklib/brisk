set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

set(VCPKG_POLICY_ALLOW_EMPTY_FOLDERS enabled)

file(REAL_PATH "../../.." BRISK_ROOT BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src/bin2c-unversioned")

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS 
        -DBRISK_ROOT=${BRISK_ROOT}
        -DCMAKE_CXX_STANDARD=20
)

vcpkg_cmake_install()

vcpkg_copy_tools(TOOL_NAMES bin2c AUTO_CLEAN)

vcpkg_install_copyright(FILE_LIST "${BRISK_ROOT}/LICENSE.txt")
