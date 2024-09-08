add_definitions(-UNDEBUG)

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

add_test(NAME xdrzcc_no_args COMMAND ${XDRZCC})
set_tests_properties(xdrzcc_no_args PROPERTIES WILL_FAIL TRUE)

add_test(NAME xdrzcc_one_args COMMAND ${XDRZCC} foo)
set_tests_properties(xdrzcc_one_args PROPERTIES WILL_FAIL TRUE)

add_test(NAME xdrzcc_two_args COMMAND ${XDRZCC} foo bar)
set_tests_properties(xdrzcc_two_args PROPERTIES WILL_FAIL TRUE)

add_test(NAME xdrzcc_missing_source COMMAND ${XDRZCC} no such thing)
set_tests_properties(xdrzcc_missing_source PROPERTIES WILL_FAIL TRUE)

add_test(NAME xdrzcc_bad_output_source COMMAND ${XDRZCC} uint32.x nosuchdir/out.c nosuchdir/out.h)
set_tests_properties(xdrzcc_bad_output_source PROPERTIES WILL_FAIL TRUE)

add_test(NAME xdrzcc_bad_output_header COMMAND ${XDRZCC} uint32.x out.c nosuchdir/out.h)
set_tests_properties(xdrzcc_bad_output_header PROPERTIES WILL_FAIL TRUE)

unit_test_xdrzcc(uint32 uint32.x uint32.c)
unit_test_xdrzcc(uint32_array uint32_array.x uint32_array.c)
unit_test_xdrzcc(uint32_vector_one uint32_vector_one.x uint32_vector_one.c)
unit_test_xdrzcc(uint32_vector uint32_vector.x uint32_vector.c)
#unit_test_xdrzcc(rfc7863 rfc7863.x rfc7863.c)
