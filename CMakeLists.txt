cmake_minimum_required(VERSION 3.16)

project(VideoProtocolProject VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ищем все нужные пакеты один раз здесь
find_package(Qt6 REQUIRED COMPONENTS Core Network Multimedia Quick)

qt_standard_project_setup(REQUIRES 6.8)

# 1. Сначала собираем общую библиотеку (протокол)
add_subdirectory(Common)

# 2. Собираем SENDER (он использует файлы в текущей папке и линкует Protocol)
qt_add_executable(appVideoSender
    main.cpp
    VideoSender.hpp
    VideoSender.cpp
)

# Оставляем консоль для отладки
set_target_properties(appVideoSender PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

target_link_libraries(appVideoSender
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Multimedia
        Protocol
)

# 3. Подключаем подпапку ресивера
add_subdirectory(VideoReceiver)
