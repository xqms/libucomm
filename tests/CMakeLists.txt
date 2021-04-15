
include_directories(${CMAKE_CURRENT_BINARY_DIR})

libucomm_wrap_msg(SIMPLE_MSG simple.msg)
add_executable(libucomm_tests
    main.cpp
    simple.cpp
    simple_cobs.cpp
    bufferio.cpp
    ${SIMPLE_MSG}
)
target_compile_options(libucomm_tests PRIVATE
    "-fsanitize=undefined"
)
target_link_options(libucomm_tests PRIVATE
    "-fsanitize=undefined"
)
add_test(libucomm_tests libucomm_tests)
