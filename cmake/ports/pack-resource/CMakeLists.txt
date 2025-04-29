cmake_minimum_required(VERSION 3.22)

project(pack_resource)

if (WIN32)
    set(PLATFORM_SPECIFIC_CPP "_Windows.cpp")
elseif (APPLE)
    set(PLATFORM_SPECIFIC_CPP "_Darwin.mm")
else ()
    set(PLATFORM_SPECIFIC_CPP "_Linux.cpp")
endif ()

add_executable(
    pack_resource
    ${BRISK_ROOT}/src/core/pack_resource.cpp
    ${BRISK_ROOT}/src/core/App.cpp
    ${BRISK_ROOT}/src/core/Binding.cpp
    ${BRISK_ROOT}/src/core/Bytes.cpp
    ${BRISK_ROOT}/src/core/Compression_Brotli.cpp
    ${BRISK_ROOT}/src/core/Compression_GZip.cpp
    ${BRISK_ROOT}/src/core/Compression_LZ4.cpp
    ${BRISK_ROOT}/src/core/Compression.cpp
    ${BRISK_ROOT}/src/core/Encoding.cpp
    ${BRISK_ROOT}/src/core/Io.cpp
    ${BRISK_ROOT}/src/core/Io${PLATFORM_SPECIFIC_CPP}
    ${BRISK_ROOT}/src/core/Json.cpp
    ${BRISK_ROOT}/src/core/Log.cpp
    ${BRISK_ROOT}/src/core/Stream.cpp
    ${BRISK_ROOT}/src/core/Settings.cpp
    ${BRISK_ROOT}/src/core/System.cpp
    ${BRISK_ROOT}/src/core/System${PLATFORM_SPECIFIC_CPP}
    ${BRISK_ROOT}/src/core/Text.cpp
    ${BRISK_ROOT}/src/core/Time.cpp
    ${BRISK_ROOT}/src/core/Threading.cpp
    ${BRISK_ROOT}/src/core/Threading${PLATFORM_SPECIFIC_CPP}
    ${BRISK_ROOT}/src/core/Throw.cpp
    ${BRISK_ROOT}/src/core/Uv.cpp
    ${BRISK_ROOT}/src/core/Version.cpp)

target_include_directories(pack_resource PUBLIC ${BRISK_ROOT}/include)

target_compile_definitions(pack_resource PUBLIC BRISK_HAVE_BROTLI=1)

if (WIN32)
    target_compile_definitions(pack_resource PUBLIC NOMINMAX=1)
endif ()

find_package(fmt CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC fmt::fmt)

find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC rapidjson)

find_package(msgpack-cxx CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC msgpack-cxx)

find_package(libuv CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC $<BUILD_INTERFACE:$<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>>)

find_package(lz4 CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC lz4::lz4)

find_package(unofficial-brotli CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC unofficial::brotli::brotlidec unofficial::brotli::brotlienc)

find_package(unofficial-utf8proc CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC utf8proc)

find_package(ZLIB REQUIRED)
target_link_libraries(pack_resource PUBLIC ZLIB::ZLIB)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(pack_resource PUBLIC spdlog::spdlog)

if (APPLE)
    target_link_libraries(pack_resource PUBLIC "-framework Foundation")
    target_compile_options(pack_resource PUBLIC $<BUILD_INTERFACE:-fobjc-arc>)
endif ()

install(TARGETS pack_resource EXPORT PackResourceTargets)
