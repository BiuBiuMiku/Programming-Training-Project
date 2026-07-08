cmake_minimum_required(VERSION 3.20)

project(TrainTicketBackEnd VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ssr)

add_executable(TrainTicketBackEnd
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/Logger.cpp
    ${SRC_DIR}/ApiService/ApiService.cpp
    ${SRC_DIR}/TicketService/TicketService.cpp
)

target_include_directories(TrainTicketBackEnd PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/ApiService
    ${SRC_DIR}/TicketService
)

target_compile_definitions(TrainTicketBackEnd PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if (MSVC)
    target_compile_options(TrainTicketBackEnd PRIVATE /W4 /permissive-)
else()
    target_compile_options(TrainTicketBackEnd PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_link_libraries(TrainTicketBackEnd PRIVATE ws2_32)
endif()
