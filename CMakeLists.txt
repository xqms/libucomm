
cmake_minimum_required(VERSION 2.8)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test.h
	DEPENDS ${CMAKE_HOME_DIRECTORY}/test.msg ${CMAKE_HOME_DIRECTORY}/parse.py
	COMMAND python ${CMAKE_HOME_DIRECTORY}/parse.py	${CMAKE_HOME_DIRECTORY}/test.msg > ${CMAKE_CURRENT_BINARY_DIR}/test.h
)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_HOME_DIRECTORY})

add_executable(main
	main.cpp
	${CMAKE_CURRENT_BINARY_DIR}/test.h
)

