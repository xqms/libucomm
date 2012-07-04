
include_directories(${CMAKE_CURRENT_BINARY_DIR})

libucomm_wrap_msg(SIMPLE_MSG simple.msg)
add_executable(simple simple.cpp bufferio.cpp ${SIMPLE_MSG})
add_test(simple simple)
