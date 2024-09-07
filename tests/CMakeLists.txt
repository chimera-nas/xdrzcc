macro(unit_test_xdrzcc name xdr_file c_file)

    set(XDR_C ${CMAKE_CURRENT_BINARY_DIR}/${name}_xdr.c)
    set(XDR_H ${CMAKE_CURRENT_BINARY_DIR}/${name}_xdr.h)
    set(XDR_X ${CMAKE_CURRENT_SOURCE_DIR}/${xdr_file})

    add_custom_command(
        OUTPUT ${XDR_C} ${XDR_H}
        COMMAND ${XDRZCC} ${XDR_X} ${XDR_C} ${XDR_H}
        DEPENDS ${xdr_file}
        COMMENT "Compiling ${xdr_file}"
    )

    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    add_executable(${name} ${c_file} ${XDR_C})

    add_dependencies(${name} xdrzcc)
       
    add_test(NAME ${name} COMMAND ${name}) 

endmacro()

unit_test_xdrzcc(uint32 uint32.x uint32.c)
