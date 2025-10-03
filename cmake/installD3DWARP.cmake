# set(WARP_VERSION "1.0.15")
# set(WARP_PACKAGE_HASH "98c645f20af5b43bf4c9cc5f9158a7091673190e19aedff6e67e2b1f4490dd52")

# set(WARP_VERSION "1.0.14")
# set(WARP_PACKAGE_HASH "e92bec51753869a41fea53a56803de9e8de8c6033becdcd2cfaaedb9ebf82f9f")

# set(WARP_VERSION "1.0.13")
# set(WARP_PACKAGE_HASH "63231c48b0573ba4c078f69cd10a4059a0fee3427107b8219e5e80ab75bd304b")

set(WARP_VERSION "1.0.12")
set(WARP_PACKAGE_HASH "3cbb8d05984fb3057ef9bf27add7cc2f2fd7f4282bc60271e19158f0b5053b79")

set(WARP_ID "microsoft.direct3d.warp")

set(WARP_URL "https://api.nuget.org/v3-flatcontainer/${WARP_ID}/${WARP_VERSION}/${WARP_ID}.${WARP_VERSION}.nupkg")

# Where to store downloaded and extracted files
set(WARP_DOWNLOAD "${CMAKE_BINARY_DIR}/${WARP_ID}.${WARP_VERSION}.nupkg")
set(WARP_EXTRACT_DIR "${CMAKE_BINARY_DIR}/${WARP_ID}.${WARP_VERSION}.extract")

# Download the .nupkg (which is just a ZIP)
file(
    DOWNLOAD "${WARP_URL}" "${WARP_DOWNLOAD}"
    SHOW_PROGRESS
    STATUS dl_status
    EXPECTED_HASH SHA256=${WARP_PACKAGE_HASH})
list(GET dl_status 0 status_code)
if (NOT status_code EQUAL 0)
    message(FATAL_ERROR "Failed to download ${WARP_URL}")
endif ()

# Extract it
file(ARCHIVE_EXTRACT INPUT "${WARP_DOWNLOAD}" DESTINATION "${WARP_EXTRACT_DIR}")

# Copy {x64,win32,arm64}/d3d10warp.dll into CMAKE_BINARY_DIR depending on architecture
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
    set(WARP_ARCH "arm64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WARP_ARCH "x64")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(WARP_ARCH "win32")
else ()
    message(FATAL_ERROR "Unsupported architecture")
endif ()

set(WARP_DLL "${WARP_EXTRACT_DIR}/build/native/bin/${WARP_ARCH}/d3d10warp.dll")

if (EXISTS "${WARP_DLL}")
    message(STATUS "Downloaded d3d10warp.dll successfully")
else ()
    message(FATAL_ERROR "Could not find d3d10warp.dll inside NuGet package!")
endif ()

# Custom command to copy the DLL next to graphics_test

function(place_warp_dll target)
    if (WIN32)
        add_custom_command(
            TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WARP_DLL}" "$<TARGET_FILE_DIR:${target}>/d3d10warp.dll"
                    DEPENDS Direct3D_WARP
            COMMENT "Copying d3d10warp.dll next to ${target}")
    endif()
endfunction()
