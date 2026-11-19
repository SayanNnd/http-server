cmake_minimum_required(VERSION 4.4)
project(server C)

set(CMAKE_C_STANDARD 23)

add_executable(server main.c)

target_compile_options(server PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
)

target_link_options(server PRIVATE
        -fsanitize=address,undefined
)