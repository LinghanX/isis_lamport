cmake_minimum_required(VERSION 3.7)
project(isis_lamport)

include_directories(include)

set(CMAKE_CXX_STANDARD 11)
add_executable(isis_lamport main.cpp messages.h parse.h parse.cpp isis.h isis.cpp messages.cpp)