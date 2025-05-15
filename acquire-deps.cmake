cmake_minimum_required(VERSION 3.22)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})

set(URL https://download.brisklib.com/brisk-deps/{TRIPLET}-{DEP_HASH}.tar.xz)

if (NOT DEFINED VCPKG_TARGET_TRIPLET)
    message(STATUS "VCPKG_TARGET_TRIPLET is not defined, checking environment...")

    if (DEFINED ENV{VCPKG_TARGET_TRIPLET})
        set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
        message(STATUS "Using VCPKG_TARGET_TRIPLET environment variable (${VCPKG_TARGET_TRIPLET})")
    else ()
        if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET})
            set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET})
            message(STATUS "Using VCPKG_DEFAULT_TRIPLET environment variable (${VCPKG_TARGET_TRIPLET})")
        else ()
            message(
                FATAL_ERROR
                    "Set VCPKG_TARGET_TRIPLET cmake variable or VCPKG_TARGET_TRIPLET or VCPKG_DEFAULT_TRIPLET environment variable"
            )
        endif ()
    endif ()

endif ()

include(${ROOT}/cmake/get_triplet.cmake)

include(${ROOT}/cmake/dep-hash.cmake)

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

        find_program(VCPKG_EXECUTABLE vcpkg)
        if (NOT VCPKG_EXECUTABLE)
            message(FATAL_ERROR "vcpkg not found. Please ensure vcpkg is installed and accessible in your PATH.")
        endif ()

        execute_process(COMMAND ${VCPKG_EXECUTABLE} install --x-install-root ${ROOT}/vcpkg_installed --x-feature=icu
                                ${EXTRA_VCPKG_ARGS} WORKING_DIRECTORY ${ROOT} COMMAND_ERROR_IS_FATAL ANY)
    endif ()

    file(REMOVE_RECURSE ${ROOT}/vcpkg_exported)

else ()

    message(STATUS "File ${DEST_FILE} already exists. Remove it to force re-download")

endif ()

if (DEFINED ENV{EXPORTED_DIR})
    set(EXPORTED_DIR $ENV{EXPORTED_DIR})
else ()
    set(EXPORTED_DIR ${ROOT}/vcpkg_exported)
endif ()

if (NOT EXISTS ${EXPORTED_DIR})

    message(STATUS "Extracting archive")

    # Create the vcpkg_exported directory
    file(MAKE_DIRECTORY ${EXPORTED_DIR})

    # Extract the archive
    file(ARCHIVE_EXTRACT INPUT "${DEST_FILE}" DESTINATION "${EXPORTED_DIR}" VERBOSE)

    if (NOT DEFINED INSTALL OR INSTALL)

        file(GLOB VCPKG_INSTALLED_CONTENTS ${ROOT}/vcpkg_installed/*)
        if (NOT VCPKG_INSTALLED_CONTENTS STREQUAL "")
            message(
                FATAL_ERROR "${ROOT}/vcpkg_installed is not empty. Remove it and run the script again to install dependencies")
        endif ()

        file(REMOVE_RECURSE ${ROOT}/vcpkg_installed)

        file(RENAME ${EXPORTED_DIR}/installed ${ROOT}/vcpkg_installed)

    endif ()

else ()

    message(STATUS "Directory ${EXPORTED_DIR} already exists. Remove it to force extraction from the archive")

endif ()

message(STATUS "All operations finished")
