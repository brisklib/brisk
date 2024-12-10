set(ROOT ${CMAKE_CURRENT_LIST_DIR})

set(URL https://download.brisklib.com/brisk-deps/{TRIPLET}-{DEP_HASH}.tar.xz)

if (DEFINED ENV{VCPKG_TARGET_TRIPLET})
    set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
else ()
    if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
        set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET})
    else ()
        message(FATAL_ERROR "Set VCPKG_TARGET_TRIPLET or VCPKG_DEFAULT_TRIPLET environment variable")
    endif ()
endif ()

include(${ROOT}/dep-hash.cmake)

# Define the URL and destination file for download
string(REPLACE "{DEP_HASH}" "${DEP_HASH}" URL "${URL}")
string(REPLACE "{TRIPLET}" "${VCPKG_TARGET_TRIPLET}" URL "${URL}")
set(DEST_FILE ${ROOT}/Brisk-Dependencies-${DEP_HASH}-${VCPKG_TARGET_TRIPLET}.tar.xz)

if (NOT EXISTS ${DEST_FILE})

    message(STATUS "Downloading ${URL}")
    # Download the file
    file(
        DOWNLOAD ${URL} ${DEST_FILE}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS)
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_STATUS_CODE)
    if (NOT DOWNLOAD_STATUS_CODE EQUAL 0)
        if (DEFINED DEP_BUILD AND NOT DEP_BUILD)
            message(FATAL_ERROR "Download failed (Status ${DOWNLOAD_STATUS_CODE}), exit")
        endif ()
        message(WARNING "Download failed, dependencies will be built using vcpkg (Status ${DOWNLOAD_STATUS_CODE})")

        execute_process(
            COMMAND vcpkg install --x-install-root ${ROOT}/vcpkg_installed --x-feature=icu ${EXTRA_VCPKG_ARGS}
            WORKING_DIRECTORY ${ROOT}
            RESULT_VARIABLE RESULT)
        if (NOT RESULT EQUAL 0)
            message(FATAL_ERROR "vcpkg install failed with exit code ${RESULT}")
        endif ()
    endif ()

    file(REMOVE_RECURSE ${ROOT}/vcpkg_exported)

else ()

    message(STATUS "File ${DEST_FILE} already exists. Remove it to force re-download")

endif ()

if (NOT EXISTS ${ROOT}/vcpkg_exported)

    message(STATUS "Extracting archive")

    # Create the vcpkg_exported directory
    file(MAKE_DIRECTORY ${ROOT}/vcpkg_exported)

    # Extract the archive
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf ${DEST_FILE}
        WORKING_DIRECTORY ${ROOT}/vcpkg_exported
        RESULT_VARIABLE RESULT)
    if (NOT RESULT EQUAL 0)
        message(FATAL_ERROR "cmake -E tar failed with exit code ${RESULT}")
    endif ()

    file(GLOB VCPKG_INSTALLED_CONTENTS ${ROOT}/vcpkg_installed/*)
    if (NOT VCPKG_INSTALLED_CONTENTS STREQUAL "")
        message(
            FATAL_ERROR
                "${ROOT}/vcpkg_installed is not empty. Remove it and run the script again to install dependencies")
    endif ()

    file(REMOVE_RECURSE ${ROOT}/vcpkg_installed)

    file(RENAME ${ROOT}/vcpkg_exported/installed ${ROOT}/vcpkg_installed)

else ()

    message(STATUS "Directory ${ROOT}/vcpkg_exported already exists. Remove it to force extraction from the archive")

endif ()

message(STATUS "All operations finished")
