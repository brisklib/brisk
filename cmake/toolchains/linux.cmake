if (NOT _VCPKG_LINUX_TOOLCHAIN_OVERRIDE)
    set(_VCPKG_LINUX_TOOLCHAIN_OVERRIDE 1)

    # Prefer the root discovered by the vcpkg toolchain.  Keep the environment
    # and cache-variable fallbacks for older vcpkg versions and standalone use.
    if (DEFINED Z_VCPKG_ROOT_DIR AND NOT "${Z_VCPKG_ROOT_DIR}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${Z_VCPKG_ROOT_DIR}")
    elseif (DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _BRISK_VCPKG_ROOT_DIR)
    elseif (DEFINED VCPKG_ROOT AND NOT "${VCPKG_ROOT}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${VCPKG_ROOT}")
    elseif (DEFINED _VCPKG_ROOT_DIR AND NOT "${_VCPKG_ROOT_DIR}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${_VCPKG_ROOT_DIR}")
    endif ()

    if (NOT EXISTS "${_BRISK_VCPKG_ROOT_DIR}/scripts/toolchains/linux.cmake")
        message(FATAL_ERROR "Could not locate the vcpkg Linux toolchain. Set VCPKG_ROOT or use a vcpkg toolchain that defines Z_VCPKG_ROOT_DIR.")
    endif ()

    include("${_BRISK_VCPKG_ROOT_DIR}/scripts/toolchains/linux.cmake")

    string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " -g0 -DNDEBUG -O3 ")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT " -g0 -DNDEBUG -O3 ")
    string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " -g1 -O1 ")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " -g1 -O1 ")

    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS_INIT}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS_INIT}"
        CACHE STRING "" FORCE)

    set(CMAKE_C_FLAGS_RELEASE
        "${CMAKE_C_FLAGS_RELEASE_INIT}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE_INIT}"
        CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG_INIT}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG_INIT}"
        CACHE STRING "" FORCE)

    unset(_BRISK_VCPKG_ROOT_DIR)
endif ()
