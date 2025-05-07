cmake_minimum_required(VERSION 3.22)

project(icowriter)

if (WIN32)
    set(PLATFORM_SPECIFIC_CPP "_Windows.cpp")
elseif (APPLE)
    set(PLATFORM_SPECIFIC_CPP "_Darwin.mm")
else ()
    set(PLATFORM_SPECIFIC_CPP "_Linux.cpp")
endif ()

add_executable(
    icowriter
    ${BRISK_ROOT}/src/window/icowriter.cpp
    ${BRISK_ROOT}/src/core/App.cpp
    ${BRISK_ROOT}/src/core/Binding.cpp
    ${BRISK_ROOT}/src/core/Bytes.cpp
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
    ${BRISK_ROOT}/src/core/Version.cpp
    ${BRISK_ROOT}/src/core/llvm/SmallVector.cpp
    ${BRISK_ROOT}/src/graphics/ImageFormats_png.cpp
    ${BRISK_ROOT}/src/graphics/ImageTransform.cpp)

target_include_directories(icowriter PUBLIC ${BRISK_ROOT}/include)

target_compile_definitions(icowriter PUBLIC BRISK_HAVE_BROTLI=1)

if (WIN32)
    target_compile_definitions(icowriter PUBLIC NOMINMAX=1)
endif ()

find_package(fmt CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC fmt::fmt)

find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC rapidjson)

find_package(msgpack-cxx CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC msgpack-cxx)

find_package(libuv CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC $<BUILD_INTERFACE:$<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>>)

find_package(lz4 CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC lz4::lz4)

find_package(unofficial-brotli CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC unofficial::brotli::brotlidec unofficial::brotli::brotlienc)

find_package(unofficial-utf8proc CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC utf8proc)

find_package(ZLIB REQUIRED)
target_link_libraries(icowriter PUBLIC ZLIB::ZLIB)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC spdlog::spdlog)

find_package(tl-expected CONFIG REQUIRED)
target_link_libraries(icowriter PUBLIC tl::expected)

find_package(Stb REQUIRED)
target_include_directories(icowriter PUBLIC ${Stb_INCLUDE_DIR})

find_package(PNG REQUIRED)
target_link_libraries(icowriter PUBLIC PNG::PNG)

install(TARGETS icowriter)
