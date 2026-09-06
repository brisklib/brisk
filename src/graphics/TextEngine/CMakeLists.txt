#
# Brisk TextEngine
#
# Brisk TextEngine implementation and its external text-layout dependencies.
#

set(BRISK_TEXT_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT EXISTS "${BRISK_TEXT_ENGINE_SOURCE_DIR}/Layout.cpp")
    message(FATAL_ERROR "Brisk TextEngine sources are missing from ${BRISK_TEXT_ENGINE_SOURCE_DIR}")
endif ()

find_package(freetype CONFIG REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
if (NOT TARGET unibreak AND NOT TARGET libunibreak::libunibreak)
    find_package(libunibreak CONFIG REQUIRED)
endif ()
if (NOT TARGET SheenBidi::SheenBidi)
    find_package(SheenBidi CONFIG REQUIRED)
endif ()
find_package(unofficial-utf8proc CONFIG REQUIRED)

add_library(brisk-text-engine STATIC
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/BreakIterator.cpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/FontDatabase.cpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/Layout.cpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/include/text_layout/BreakIterator.hpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/include/text_layout/FontDatabase.hpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/include/text_layout/Layout.hpp
    ${BRISK_TEXT_ENGINE_SOURCE_DIR}/include/text_layout/Types.hpp)

set_property(TARGET brisk-text-engine PROPERTY EXPORT_NAME TextEngine)
add_library(Brisk::TextEngine ALIAS brisk-text-engine)

target_include_directories(brisk-text-engine
    PUBLIC
        $<BUILD_INTERFACE:${BRISK_TEXT_ENGINE_SOURCE_DIR}/include>
    PRIVATE
        ${BRISK_TEXT_ENGINE_SOURCE_DIR}/src)

target_link_libraries(brisk-text-engine
    PUBLIC
        brisk-core
    PRIVATE
        freetype
        harfbuzz
        utf8proc
        libunibreak::libunibreak
        SheenBidi::SheenBidi
)

target_compile_features(brisk-text-engine PRIVATE cxx_std_20)
target_compile_definitions(brisk-text-engine PRIVATE TL_BUILDING_LIBRARY=1)
