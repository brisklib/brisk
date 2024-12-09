if (NOT _VCPKG_WINDOWS_TOOLCHAIN_OVERRIDE)
    set(_VCPKG_WINDOWS_TOOLCHAIN_OVERRIDE 1)

    include($ENV{VCPKG_ROOT}/scripts/toolchains/windows.cmake)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Zc:inline")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Zc:inline")

    string(REPLACE "/Z7" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Z7" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    string(REPLACE "/Od" "/O1" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Od" "/O1" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    
    string(REPLACE "/Ob0" "/Ob1" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Ob0" "/Ob1" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

    if (DEFINED ENV{LLVM_DIR})

        string(REPLACE "/MP" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        string(REPLACE "/MP" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

        string(REPLACE "/Z7" "-gline-tables-only" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Z7" "-gline-tables-only" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

        if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
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

endif ()
