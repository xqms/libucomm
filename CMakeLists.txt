
cmake_minimum_required(VERSION 2.8)

set(LIBUCOMM_PARSE_PY ${CMAKE_HOME_DIRECTORY}/parse.py)
include(cmake/libucomm.cmake)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_HOME_DIRECTORY}/include)

add_subdirectory(examples)

set(ENABLE_TESTS ON CACHE BOOL "Enable tests")
if(ENABLE_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
