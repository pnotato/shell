cmake_minimum_required(VERSION 3.14)

project(shell C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(shell src/main.c src/history.c)