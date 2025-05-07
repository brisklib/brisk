get_property(
    _BRISK_CORE
    TARGET brisk-core
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_CORE}" STREQUAL "")
    set(_BRISK_CORE brisk-core)
endif ()

# >libuv
find_package(libuv CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE}
                      $<BUILD_INTERFACE:$<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>>)
# /libuv

# >concurrentqueue
find_package(unofficial-concurrentqueue CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} unofficial::concurrentqueue::concurrentqueue)
# /concurrentqueue

# >stb
find_package(Stb REQUIRED)
target_include_directories(${_BRISK_CORE} ${_DEP_PRIVATE} ${Stb_INCLUDE_DIR})
# /stb

# >fmt
find_package(fmt CONFIG REQUIRED NO_SYSTEM_ENVIRONMENT_PATH)
target_link_libraries(${_BRISK_CORE} ${_DEP_PUBLIC} fmt::fmt)
# /fmt

# >spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PUBLIC} spdlog::spdlog)
# /spdlog

# >brotli
find_package(unofficial-brotli CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} unofficial::brotli::brotlidec unofficial::brotli::brotlienc)
# /brotli

target_compile_definitions(${_BRISK_CORE} ${_DEP_PUBLIC} BRISK_HAVE_BROTLI=1)

# >rapidjson
find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} rapidjson)
# /rapidjson

# >lz4
find_package(lz4 CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} lz4::lz4)
# /lz4

# >msgpack-cxx
find_package(msgpack-cxx CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} msgpack-cxx)
# /msgpack-cxx

# >utf8proc
find_package(unofficial-utf8proc CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PUBLIC} utf8proc)
# /utf8proc

# >zlib
find_package(ZLIB REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} ZLIB::ZLIB)
# /zlib

# >tl-expected
find_package(tl-expected CONFIG REQUIRED)
target_link_libraries(${_BRISK_CORE} ${_DEP_PRIVATE} tl::expected)
# /tl-expected
