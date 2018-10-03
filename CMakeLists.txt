cmake_minimum_required(VERSION 3.12)
project(isis_lamport)

include_directories(include)

set(CMAKE_CXX_STANDARD 17)
add_executable(isis_lamport main.cpp messages.h parse.h parse.cpp isis.h isis.cpp messages.cpp)