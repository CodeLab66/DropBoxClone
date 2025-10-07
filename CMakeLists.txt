cmake_minimum_required(VERSION 3.29)
project(DropBoxClone C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

# Commenting out the redundant initial add_executable block
# add_executable(DropBoxClone
#         config.c
#         config.h
#         queue.c
#         queue.h
#         user.c
#         user.h
#         task.c
#         task.h
#         utils.c
#         utils.h
#         client_handler.c
#         client_handler.h
#         worker.c
#         worker.h
#         server.c
#         server.h
#         client.c
#         client.h
# )

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

set(CLIENT_SOURCE client.c)

add_executable(DropBoxClone ${SERVER_SOURCES})
add_executable(client ${CLIENT_SOURCE})

target_compile_options(DropBoxClone PRIVATE -Wall)
target_compile_options(client PRIVATE -Wall)

target_link_libraries(DropBoxClone PRIVATE -pthread -lws2_32)
target_link_libraries(client PRIVATE -lws2_32)

if(WIN32)
    target_compile_definitions(DropBoxClone PRIVATE _WIN32_WINNT=0x0601)
    target_compile_definitions(client PRIVATE _WIN32_WINNT=0x0601)
endif()