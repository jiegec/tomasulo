cmake_minimum_required(VERSION 3.12)
project(tomasulo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address ${CMAKE_CXX_FLAGS_DEBUG}")
add_executable(tomasulo tomasulo.cpp parser.cpp)
add_executable(simulator simulator.cpp parser.cpp)
