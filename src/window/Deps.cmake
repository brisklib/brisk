get_property(
    _BRISK_WINDOW
    TARGET brisk-window
    PROPERTY ALIASED_TARGET)
if ("${_BRISK_WINDOW}" STREQUAL "")
    set(_BRISK_WINDOW brisk-window)
endif ()

if (NOT WIN32 AND NOT APPLE)    
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(${_BRISK_WINDOW} ${_DEP_PUBLIC} glfw)
endif ()
