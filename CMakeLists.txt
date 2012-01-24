# Matrix VM root
cmake_minimum_required(VERSION 2.6)
project(matrixvm)

set(CMAKE_CXX_FLAGS "-g3 -Wall --std=c++0x")
include_directories(${PROJECT_SOURCE_DIR})

# Look for dl library
include(CheckLibraryExists)
check_library_exists(dl dlopen "" HAVE_DL)
if (HAVE_DL)
    set (EXTRA_LIBS ${EXTRA_LIBS} dl)
endif (HAVE_DL)

# build
add_executable( matrixvm
                matrixvm.cpp
                machine/motherboard.cpp
                machine/dladapter.cpp)
target_link_libraries(matrixvm
                      ${EXTRA_LIBS})

add_subdirectory(basiccpu)
add_subdirectory(dev)
