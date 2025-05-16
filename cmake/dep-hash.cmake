cmake_minimum_required(VERSION 3.22)

# This script computes the hash of files that control dependency building, enabling version management and automatic
# rebuilding of dependencies.

get_filename_component(ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)

# GLOB_RECURSE returns sorted list of files
file(GLOB_RECURSE FILES "${ROOT_DIR}/cmake/ports/*" "${ROOT_DIR}/cmake/toolchains/*" "${ROOT_DIR}/cmake/triplets/*")

list(APPEND FILES "${ROOT_DIR}/vcpkg.json")

set(HASHES)

if (NOT DEFINED DEP_HASH_SILENT)
    message("-- Hashing files")
endif ()

foreach (FILE IN LISTS FILES)
    file(READ ${FILE} CONTENT)
    string(SHA256 HASH "${CONTENT}")
    file(RELATIVE_PATH REL_FILE ${ROOT_DIR} ${FILE})
    if (NOT DEFINED DEP_HASH_SILENT)
        message("        ${REL_FILE} -> ${HASH}")
    endif ()
    list(APPEND HASHES "${HASH}")
endforeach ()

if (NOT DEFINED DEP_HASH_SILENT)
    message("-- Combined hash is")
endif ()

string(SHA256 DEP_HASH "${HASHES}")

string(SUBSTRING "${DEP_HASH}" 0 8 DEP_HASH)

if (NOT DEFINED DEP_HASH_SILENT)
    message(STATUS ${DEP_HASH})
endif ()

if (CMAKE_SCRIPT_MODE_FILE)
    message(STATUS ${DEP_HASH})
endif ()
