if (NOT _VCPKG_OSX_TOOLCHAIN_OVERRIDE)
    set(_VCPKG_OSX_TOOLCHAIN_OVERRIDE 1)
    
    if (Z_VCPKG_ROOT_DIR)
        include(${Z_VCPKG_ROOT_DIR}/scripts/toolchains/osx.cmake)
    else()
        include(${_VCPKG_ROOT_DIR}/scripts/toolchains/osx.cmake)
    endif()

    string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " -g0 -DNDEBUG -O3 ")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT " -g0 -DNDEBUG -O3 ")
    string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " -gline-tables-only -O1 ")
    string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " -gline-tables-only -O1 ")

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

endif ()
