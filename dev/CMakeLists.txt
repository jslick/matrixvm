# Matrix VM basic cpu
cmake_minimum_required(VERSION 2.6)

# charoutputdevice
add_library(charoutputdevice SHARED charoutputdevice.cpp)
target_link_libraries(charoutputdevice ${EXTRA_LIBS})

# displaydevice
add_library(displaydevice SHARED displaydevice.cpp)
target_link_libraries(displaydevice ${EXTRA_LIBS})
