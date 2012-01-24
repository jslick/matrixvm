# Matrix VM basic cpu
cmake_minimum_required(VERSION 2.6)

# Build
add_library(charoutputdevice SHARED charoutputdevice.cpp)
target_link_libraries(charoutputdevice ${EXTRA_LIBS})
