cmake_minimum_required(VERSION 3.7)
project(prj1_ds)

include_directories(include)

set(CMAKE_CXX_STANDARD 11)
add_executable(prj1_ds main.cpp messages.h parse.h parse.cpp isis.h isis.cpp messages.cpp)