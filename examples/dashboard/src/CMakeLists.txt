add_executable(
    dashboard
    dashboard.cpp
    View/MainComponent.hpp
    View/MainComponent.cpp
    View/AboutComponent.hpp
    View/AboutComponent.cpp
    View/SettingsComponent.hpp
    View/SettingsComponent.cpp
    Model/Model.hpp
    Model/Model.cpp
    View/View.hpp
    View/View.cpp
    ViewModel/ViewModel.hpp
    ViewModel/ViewModel.cpp)

brisk_target_link_resource(
    dashboard PRIVATE icon.png
    INPUT "${PROJECT_SOURCE_DIR}/assets/icon.png"
    BROTLI)
brisk_target_link_resource(
    dashboard PRIVATE TitilliumWeb-Regular.ttf
    INPUT "${PROJECT_SOURCE_DIR}/assets/TitilliumWeb-Regular.ttf"
    BROTLI)
brisk_target_link_resource(
    dashboard PRIVATE TitilliumWeb-Bold.ttf
    INPUT "${PROJECT_SOURCE_DIR}/assets/TitilliumWeb-Bold.ttf"
    BROTLI)

target_link_libraries(dashboard PRIVATE Brisk::Widgets Brisk::Executable)
target_include_directories(dashboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

brisk_setup_executable(dashboard)
