# This script computes the hash of files that control dependency building, enabling version management and automatic
# rebuilding of dependencies.

# GLOB_RECURSE returns sorted list of files
file(GLOB_RECURSE FILES "${CMAKE_CURRENT_LIST_DIR}/cmake/ports/*" "${CMAKE_CURRENT_LIST_DIR}/cmake/toolchains/*"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/triplets/*")

list(APPEND FILES "${CMAKE_CURRENT_LIST_DIR}/vcpkg.json")

set(HASHES)

if (NOT DEFINED DEP_HASH_SILENT)
    message("-- Hashing files")
endif ()

foreach (FILE IN LISTS FILES)
    file(READ ${FILE} CONTENT)
    string(SHA256 HASH "${CONTENT}")
    file(RELATIVE_PATH REL_FILE ${CMAKE_CURRENT_LIST_DIR} ${FILE})
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
