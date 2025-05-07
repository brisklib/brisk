set(ROOT ${CMAKE_CURRENT_LIST_DIR})

if (WIN32)
    set(TEMP_DIR $ENV{TEMP})
else ()
    set(TEMP_DIR $ENV{TMPDIR})
endif ()

if (NOT DEFINED ENV{AWS_SECRET_ACCESS_KEY})
    message(FATAL_ERROR "AWS_SECRET_ACCESS_KEY is not set. This script is for internal use only")
endif ()

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

# Check if the archive exists on S3
execute_process(COMMAND aws s3 ls s3://gh-bin/brisk-deps/${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz
                RESULT_VARIABLE RESULT)

if (RESULT EQUAL 0) # aws s3 ls was successfull, file exists
    message("Archive does exist on S3, skipping build")
else ()
    message("Archive does not exist on S3, trying to build")

    # Install vcpkg dependencies with advanced-tests feature
    execute_process(COMMAND vcpkg install --x-feature=advanced-tests WORKING_DIRECTORY ${ROOT} COMMAND_ECHO STDOUT
                                                                            COMMAND_ERROR_IS_FATAL ANY)

    # Export the package to a .tar.xz archive
    execute_process(COMMAND vcpkg export --raw --output-dir=${TEMP_DIR} --output=${VCPKG_TARGET_TRIPLET}-${DEP_HASH}
                            COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY)

    execute_process(
        COMMAND cmake -E tar cJf ${TEMP_DIR}/${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz .
        WORKING_DIRECTORY ${TEMP_DIR}/${VCPKG_TARGET_TRIPLET}-${DEP_HASH} COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL
                          ANY)

    # Upload the archive to S3
    execute_process(COMMAND aws s3 cp --acl public-read ${TEMP_DIR}/${VCPKG_TARGET_TRIPLET}-${DEP_HASH}.tar.xz
                            s3://gh-bin/brisk-deps/ COMMAND_ECHO STDOUT COMMAND_ERROR_IS_FATAL ANY)
endif ()
