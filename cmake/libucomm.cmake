
macro(libucomm_wrap_msg outfile msg)
    get_filename_component(${outfile} ${msg} NAME_WE)
    set(${outfile} ${CMAKE_CURRENT_BINARY_DIR}/${${outfile}}.h)
    add_custom_command(
        OUTPUT ${${outfile}}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${msg}
        COMMAND python3 ${LIBUCOMM_PARSE_PY}
            ${CMAKE_CURRENT_SOURCE_DIR}/${msg}
            > ${${outfile}}
    )
endmacro()
