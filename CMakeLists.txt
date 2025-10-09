cmake_minimum_required(VERSION 3.20)
project(DropBoxClone C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

set(SERVER_SOURCES
        config.c
        queue.c
        user.c
        task.c
        utils.c
        client_handler.c
        worker.c
        server.c
)

set(CLIENT_SOURCES
        client.c
        config.c
)

# Corrected executable names to match what CLion detected
add_executable(DropBoxClone ${SERVER_SOURCES})
add_executable(client ${CLIENT_SOURCES})

target_compile_options(DropBoxClone PRIVATE -Wall)
target_compile_options(client PRIVATE -Wall)

target_link_libraries(DropBoxClone PRIVATE ws2_32 pthread)
target_link_libraries(client PRIVATE ws2_32)

if(WIN32)
    target_compile_definitions(DropBoxClone PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(client PRIVATE _WIN32_WINNT=0x0601)
endif()