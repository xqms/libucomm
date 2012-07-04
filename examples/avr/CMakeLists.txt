
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_CXX_FLAGS "-O2 -fno-exceptions -mmcu=atmega32")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

libucomm_wrap_msg(TEST_MSGS ../test.msg)
add_executable(avr avr.cpp ${TEST_MSGS})
