# Добавляем исполняемый файл ресивера
qt_add_executable(appVideoReceiver
    main.cpp
    QmlImageItem.hpp
    QmlImageItem.cpp
    VideoReceiver.hpp
    VideoReceiver.cpp
)

# Создаем QML модуль
qt_add_qml_module(appVideoReceiver
    URI VideoReceiver
    VERSION 1.0
    QML_FILES
        Main.qml
)

# Для ресивера на Windows консоль не нужна (WIN32_EXECUTABLE TRUE)
set_target_properties(appVideoReceiver PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(appVideoReceiver
    PRIVATE
        Qt6::Quick
        Qt6::Network
        Protocol
)
