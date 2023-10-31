cmake_minimum_required(VERSION 3.26)
project(PSO_P C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(PSO_P
        client.c
        server.c
        server.c
        client.c
)
