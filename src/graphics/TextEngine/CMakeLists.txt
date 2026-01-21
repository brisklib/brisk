#
# Brisk TextEngine
#
# This target is deliberately isolated from brisk-graphics while the new text
# engine is being migrated.  No Brisk production target links it yet.
#

set(BRISK_TEXT_ENGINE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT EXISTS "${BRISK_TEXT_ENGINE_SOURCE_DIR}/Layout.cpp")
    message(FATAL_ERROR "Brisk TextEngine sources are missing from ${BRISK_TEXT_ENGINE_SOURCE_DIR}")
endif ()

include(FetchContent)

# TextEngine directly uses both FreeType and HarfBuzz. Find them here rather
# than relying on brisk-graphics to discover them later; this target must be
# independently buildable while it remains isolated from the graphics target.
find_package(freetype CONFIG REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(unofficial-utf8proc CONFIG REQUIRED)

# The standalone project owns these two small Unicode dependencies.  Reuse
# Brisk's already configured FreeType, HarfBuzz and utf8proc targets below.
if (NOT TARGET unibreak AND NOT TARGET libunibreak)
    FetchContent_Declare(
        brisk_text_engine_libunibreak
        GIT_REPOSITORY https://github.com/adah1972/libunibreak.git
        GIT_TAG libunibreak_7_0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(brisk_text_engine_libunibreak)
endif ()

if (NOT TARGET sheenbidi AND NOT TARGET SheenBidi)
    FetchContent_Declare(
        brisk_text_engine_sheenbidi
        GIT_REPOSITORY https://github.com/Tehreer/SheenBidi.git
        GIT_TAG v3.0.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(brisk_text_engine_sheenbidi)
endif ()

if (NOT TARGET unibreak AND NOT TARGET libunibreak)
    add_library(brisk-text-engine-unibreak STATIC
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/unibreakbase.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/unibreakdef.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/linebreak.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/linebreakdata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/linebreakdef.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/wordbreak.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/wordbreakdata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/graphemebreak.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/graphemebreakdata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/emojidata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/emojidef.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/indicconjunctbreakdata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/eastasianwidthdata.c
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src/eastasianwidthdef.c)
    target_include_directories(brisk-text-engine-unibreak PUBLIC
        ${brisk_text_engine_libunibreak_SOURCE_DIR}/src)
    set(_BRISK_TEXT_ENGINE_UNIBREAK brisk-text-engine-unibreak)
elseif (TARGET unibreak)
    set(_BRISK_TEXT_ENGINE_UNIBREAK unibreak)
else ()
    set(_BRISK_TEXT_ENGINE_UNIBREAK libunibreak)
endif ()

if (TARGET sheenbidi)
    set(_BRISK_TEXT_ENGINE_SHEENBIDI sheenbidi)
elseif (TARGET SheenBidi)
    set(_BRISK_TEXT_ENGINE_SHEENBIDI SheenBidi)
else ()
    add_library(brisk-text-engine-sheenbidi STATIC
        ${brisk_text_engine_sheenbidi_SOURCE_DIR}/Source/SheenBidi.c)
    target_compile_definitions(brisk-text-engine-sheenbidi PRIVATE SB_CONFIG_UNITY)
    target_include_directories(brisk-text-engine-sheenbidi PUBLIC
        ${brisk_text_engine_sheenbidi_SOURCE_DIR}/Headers)
    set(_BRISK_TEXT_ENGINE_SHEENBIDI brisk-text-engine-sheenbidi)
endif ()

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
        $<BUILD_INTERFACE:freetype>
        $<BUILD_INTERFACE:harfbuzz>
        $<BUILD_INTERFACE:utf8proc>
        $<BUILD_INTERFACE:${_BRISK_TEXT_ENGINE_UNIBREAK}>
        $<BUILD_INTERFACE:${_BRISK_TEXT_ENGINE_SHEENBIDI}>
)

target_compile_features(brisk-text-engine PRIVATE cxx_std_20)
target_compile_definitions(brisk-text-engine PRIVATE TL_BUILDING_LIBRARY=1)

unset(_BRISK_TEXT_ENGINE_UNIBREAK)
unset(_BRISK_TEXT_ENGINE_SHEENBIDI)
