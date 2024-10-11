cmake_minimum_required(VERSION 3.22.1)
project(lab1 C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

file(GLOB SOURCES "src/*.c")
add_executable(metrics ${SOURCES} src/main.c)

target_link_libraries(metrics prom promhttp pthread)
