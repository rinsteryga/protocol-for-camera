cmake_minimum_required(VERSION 3.16)

project(VideoSender VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Multimedia Network)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(Common)

qt_add_executable(appVideoSender
    main.cpp
    VideoSender.hpp
    VideoSender.cpp
)

qt_add_qml_module(appVideoSender
    URI VideoSender
    QML_FILES
        Main.qml
        RESOURCES README.md
        RESOURCES .clang-format
)

set_target_properties(appVideoSender PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appVideoSender
    PRIVATE
        Qt6::Quick
        Qt6::Multimedia
        Qt6::Network
        Protocol
)

include(GNUInstallDirs)
install(TARGETS appVideoSender
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_subdirectory(VideoReceiver)
