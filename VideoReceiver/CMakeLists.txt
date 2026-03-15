cmake_minimum_required(VERSION 3.16)

project(VideoReceiver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appVideoReceiver
    main.cpp
    QmlImageItem.hpp
    QmlImageItem.cpp
    VideoReceiver.hpp
    VideoReceiver.cpp
)

qt_add_qml_module(appVideoReceiver
    URI VideoReceiver
    QML_FILES
        Main.qml
        RESOURCES README.md
)

set_target_properties(appVideoReceiver PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appVideoReceiver
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Protocol
)

include(GNUInstallDirs)
install(TARGETS appVideoReceiver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
