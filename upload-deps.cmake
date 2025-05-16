cmake_minimum_required(VERSION 3.22)

set(ROOT ${CMAKE_CURRENT_LIST_DIR})

if (NOT DEFINED ENV{AWS_SECRET_ACCESS_KEY})
    message(FATAL_ERROR "AWS_SECRET_ACCESS_KEY is not set. This script is for internal use only")
endif ()

include(${ROOT}/cmake/get_triplet.cmake)

include(${ROOT}/cmake/dep-hash.cmake)

if (DEFINED ENV{EXPORTED_DIR})
    set(EXPORTED_DIR $ENV{EXPORTED_DIR})
else ()
    if (WIN32)
        set(EXPORTED_DIR $ENV{TEMP}/${VCPKG_TARGET_TRIPLET}-${DEP_HASH})
    else ()
        set(EXPORTED_DIR $ENV{TMPDIR}/${VCPKG_TARGET_TRIPLET}-${DEP_HASH})
    endif ()
endif ()

# Check if the archive exists on S3
execute_process(COMMAND aws s3 ls s3://gh-bin/brisk-deps/${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz
                RESULT_VARIABLE RESULT)

if (RESULT EQUAL 0) # aws s3 ls was successfull, file exists
    message("Archive ${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz does exist on S3, skipping build")
else ()
    message("Archive ${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz does not exist on S3, trying to build")
    if (NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "uni-osx")

        # Install vcpkg dependencies with advanced-tests feature
        execute_process(COMMAND vcpkg install --x-feature=advanced-tests WORKING_DIRECTORY ${ROOT} COMMAND_ECHO STDOUT
                                                                                           COMMAND_ERROR_IS_FATAL ANY)
        get_filename_component(PARENT_DIR "${EXPORTED_DIR}" DIRECTORY)

        get_filename_component(FOLDER_NAME "${EXPORTED_DIR}" NAME)

        # Export the package to a .tar.xz archive
        execute_process(COMMAND vcpkg export --raw --output-dir=${PARENT_DIR} --output=${FOLDER_NAME} COMMAND_ECHO
                                STDOUT COMMAND_ERROR_IS_FATAL ANY)

    else ()

        file(MAKE_DIRECTORY ${EXPORTED_DIR})

        if (NOT EXISTS "${NATIVE_DIR}/x64-osx")
            message(FATAL_ERROR "Directory ${NATIVE_DIR}/x64-osx does not exist")
        endif ()

        if (NOT EXISTS "${NATIVE_DIR}/arm64-osx")
            message(FATAL_ERROR "Directory ${NATIVE_DIR}/arm64-osx does not exist")
        endif ()

        file(COPY "${NATIVE_DIR}/x64-osx/vcpkg" DESTINATION "${EXPORTED_DIR}")
        file(COPY "${NATIVE_DIR}/x64-osx/.vcpkg-root" DESTINATION "${EXPORTED_DIR}")
        file(COPY "${NATIVE_DIR}/x64-osx/scripts" DESTINATION "${EXPORTED_DIR}")
        file(COPY "${NATIVE_DIR}/x64-osx/installed/vcpkg" DESTINATION "${EXPORTED_DIR}/installed")

        set(PREFER_X64
            include/jconfig.h
            vcpkg.spdx.json
            vcpkg_abi_info.txt
            libpng16-debug.cmake
            libpng16-release.cmake
            zstdTargets-debug.cmake
            zstdTargets-release.cmake
            icu-config)

        execute_process(
            COMMAND
                ${CMAKE_COMMAND} -DDIR_ARM64=${NATIVE_DIR}/arm64-osx/installed/arm64-osx
                -DDIR_X64=${NATIVE_DIR}/x64-osx/installed/x64-osx -DDIR_OUT=${EXPORTED_DIR}/installed/uni-osx
                "-DPREFER_X64=${PREFER_X64}" -P ${ROOT}/cmake/macos-merge-binaries.cmake
            WORKING_DIRECTORY ${ROOT} COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY)
    endif ()

    execute_process(COMMAND cmake -E tar cJf ${ROOT}/Brisk-Dependencies-${DEP_HASH}-${VCPKG_TARGET_TRIPLET}.tar.xz .
                    WORKING_DIRECTORY ${EXPORTED_DIR} COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY)

    # Upload the archive to S3
    execute_process(
        COMMAND
            aws s3 cp --acl public-read ${ROOT}/Brisk-Dependencies-${DEP_HASH}-${VCPKG_TARGET_TRIPLET}.tar.xz
            s3://gh-bin/brisk-deps/${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz COMMAND_ECHO STDOUT
            COMMAND_ERROR_IS_FATAL ANY)
endif ()
