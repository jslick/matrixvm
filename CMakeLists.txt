# Matrix VM root
cmake_minimum_required(VERSION 2.6)
project(matrixvm)

set(CMAKE_CXX_FLAGS "-g -Wall")
include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(machine)
add_subdirectory(mycpu)
