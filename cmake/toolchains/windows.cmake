if (NOT _VCPKG_WINDOWS_TOOLCHAIN_OVERRIDE)
    set(_VCPKG_WINDOWS_TOOLCHAIN_OVERRIDE 1)

    # Recent vcpkg versions derive the root from .vcpkg-root and expose it as
    # Z_VCPKG_ROOT_DIR.  VCPKG_ROOT is not guaranteed to remain in the
    # environment while a chainload toolchain is being evaluated.
    if (DEFINED Z_VCPKG_ROOT_DIR AND NOT "${Z_VCPKG_ROOT_DIR}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${Z_VCPKG_ROOT_DIR}")
    elseif (DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _BRISK_VCPKG_ROOT_DIR)
    elseif (DEFINED VCPKG_ROOT AND NOT "${VCPKG_ROOT}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${VCPKG_ROOT}")
    elseif (DEFINED _VCPKG_ROOT_DIR AND NOT "${_VCPKG_ROOT_DIR}" STREQUAL "")
        set(_BRISK_VCPKG_ROOT_DIR "${_VCPKG_ROOT_DIR}")
    endif ()

    if (NOT EXISTS "${_BRISK_VCPKG_ROOT_DIR}/scripts/toolchains/windows.cmake")
        message(FATAL_ERROR "Could not locate the vcpkg Windows toolchain. Set VCPKG_ROOT or use a vcpkg toolchain that defines Z_VCPKG_ROOT_DIR.")
    endif ()

    include("${_BRISK_VCPKG_ROOT_DIR}/scripts/toolchains/windows.cmake")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Zc:inline")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:inline")

    string(REPLACE "/Z7" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Z7" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    if (DEFINED ENV{LLVM_DIR})
    
        string(REPLACE "/Od" "/O1" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Od" "/O1" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        string(REPLACE "/Ob0" "/Ob1" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        string(REPLACE "/MP" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        string(REPLACE "/MP" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        string(REPLACE "/Z7" "-gline-tables-only" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Z7" "-gline-tables-only" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
        endif ()
        
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=arm64-pc-windows-msvc")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=arm64-pc-windows-msvc")
        endif ()

        file(TO_CMAKE_PATH $ENV{LLVM_DIR} LLVM_DIR_FIXED)
        set(CMAKE_C_COMPILER
            "${LLVM_DIR_FIXED}/bin/clang-cl.exe"
            CACHE PATH "" FORCE)
        set(CMAKE_CXX_COMPILER
            "${LLVM_DIR_FIXED}/bin/clang-cl.exe"
            CACHE PATH "" FORCE)
        set(CMAKE_LINKER
            "${LLVM_DIR_FIXED}/bin/lld-link.exe"
            CACHE PATH "" FORCE)
    endif ()

    # vcpkg's Windows toolchain adds /c65001 to CMAKE_RC_FLAGS.  That option
    # is accepted by Microsoft's resource compiler, but CMake's resource
    # dependency scanner invokes clang-cl with the same flags and clang-cl
    # treats /c65001 as an input file.  Keep the charset option for MSVC and
    # omit it for clang-cl builds.
    if (DEFINED ENV{LLVM_DIR} OR CMAKE_C_COMPILER MATCHES "clang-cl" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
        set(CMAKE_RC_FLAGS "/DWIN32"
            CACHE STRING "Flags used by the Windows resource compiler" FORCE)
    endif ()

    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS}"
        CACHE STRING "" FORCE)

    set(CMAKE_C_FLAGS_RELEASE
        "${CMAKE_C_FLAGS_RELEASE}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE}"
        CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG}"
        CACHE STRING "" FORCE)

    unset(_BRISK_VCPKG_ROOT_DIR)
endif ()
