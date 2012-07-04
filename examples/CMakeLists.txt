
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(BUILD_AVR_EXAMPLE OFF CACHE BOOL "Build AVR example")

libucomm_wrap_msg(TEST_MSGS test.msg)
add_executable(test test.cpp ${TEST_MSGS})

if(BUILD_AVR_EXAMPLE)
	add_subdirectory(avr)
endif()
