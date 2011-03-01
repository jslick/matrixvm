# Matrix VM machine
cmake_minimum_required(VERSION 2.6)

# Look for dl library
include(CheckLibraryExists)
check_library_exists(dl dlopen "" HAVE_DL)
if (HAVE_DL)
    set (EXTRA_LIBS ${EXTRA_LIBS} dl)
endif (HAVE_DL)

# build
add_executable(matrixvm matrixvm.cpp motherboard.cpp dladapter.cpp)
target_link_libraries(matrixvm boost_filesystem boost_system ${EXTRA_LIBS})
