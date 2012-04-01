# Matrix VM root
cmake_minimum_required(VERSION 2.6)
project(matrixvm)

set(CMAKE_CXX_FLAGS "-g3 -Wall --std=c++0x ${CMAKE_CXX_FLAGS}")
find_library(BOOST_THREAD NAMES "boost_thread" "boost_thread-mt" REQUIRED)
include_directories(${PROJECT_SOURCE_DIR})

include(CheckLibraryExists)

# Look for dl library
check_library_exists(dl dlopen "" HAVE_DL)
if (HAVE_DL)
    set (EXTRA_LIBS ${EXTRA_LIBS} dl)
endif (HAVE_DL)

# Look for X11 library
check_library_exists(X11 XCreateSimpleWindow "" HAVE_X11)
if (HAVE_X11)
    set (EXTRA_LIBS ${EXTRA_LIBS} X11)
endif (HAVE_X11)

# build
add_executable(matrixvm
        matrixvm.cpp
        dev/interruptcontroller.cpp
        dev/basicinterruptcontroller.cpp
        dev/timerdevice.cpp
        dev/x11displaymanager.cpp
        dev/nulldisplaymanager.cpp
        machine/dladapter.cpp
        machine/motherboard.cpp
        )
target_link_libraries(matrixvm ${BOOST_THREAD} ${EXTRA_LIBS})

add_subdirectory(basiccpu)
add_subdirectory(dev)
