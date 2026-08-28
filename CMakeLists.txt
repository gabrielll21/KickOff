cmake_minimum_required(VERSION 4.3)
project(KickOff)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(KickOff
        src/main.cpp
        src/main.h)
