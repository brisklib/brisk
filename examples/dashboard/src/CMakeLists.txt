add_executable(
    dashboard
    dashboard.cpp
    MainComponent.hpp
    MainComponent.cpp
    AboutComponent.hpp
    AboutComponent.cpp
    SettingsComponent.hpp
    SettingsComponent.cpp
    Model.hpp
    Model.cpp
    View.hpp
    View.cpp
    ViewModel.hpp
    ViewModel.cpp)

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

brisk_setup_executable(dashboard)
