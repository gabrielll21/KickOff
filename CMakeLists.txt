cmake_minimum_required(VERSION 3.16)
project(KickOff LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

add_executable(KickOff src/main.cpp)

target_link_libraries(KickOff PRIVATE glfw OpenGL::GL)
