
include_directories(${CMAKE_CURRENT_BINARY_DIR})

libucomm_wrap_msg(SIMPLE_MSG simple.msg)
add_executable(libucomm_tests
    main.cpp
    simple.cpp
    simple_cobs.cpp
    bufferio.cpp
    ${SIMPLE_MSG}
)
add_test(libucomm_tests libucomm_tests)
