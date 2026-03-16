# Создаем статическую библиотеку, чтобы не дублировать код в Sender и Receiver
add_library(Protocol STATIC
    PacketData.cpp
    PacketData.hpp
    PacketHeader.hpp
)

target_link_libraries(Protocol
    PUBLIC
        Qt6::Core
)

# Это позволит писать #include "PacketData.hpp" везде
target_include_directories(Protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
