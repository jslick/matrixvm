# Matrix VM basic cpu
cmake_minimum_required(VERSION 2.6)

# Build
add_library(basiccpu SHARED basiccpu.cpp)
target_link_libraries(basiccpu ${EXTRA_LIBS})
