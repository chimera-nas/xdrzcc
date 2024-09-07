set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(FLEX_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/xdr.l)
set(FLEX_OUTPUT ${GENERATED_DIR}/lex.yy.c)

set(BISON_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/xdr.y)
set(BISON_OUTPUT ${GENERATED_DIR}/y.tab.c)
set(BISON_HEADER_OUTPUT ${GENERATED_DIR}/y.tab.h)

FLEX_TARGET(lexer ${FLEX_INPUT} ${FLEX_OUTPUT})
BISON_TARGET(parser ${BISON_INPUT} ${BISON_OUTPUT}
             DEFINES_FILE ${BISON_HEADER_OUTPUT})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GENERATED_DIR})

add_flex_bison_dependency(lexer parser)

function(generate_embedded_code input_file output_file var_name)
    file(STRINGS ${input_file} input_content)

    set(output_content "")

    foreach(line IN LISTS input_content)
        string(REPLACE "\\" "\\\\" line "${line}")
        string(REPLACE "\"" "\\\"" line "${line}")
        set(output_content "${output_content}\"${line}\\n\"\n")
    endforeach()

    file(WRITE "${output_file}" "const char* ${var_name} = \n${output_content};\n")
endfunction()

set(BUILTIN_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/xdr_builtin.c)
set(BUILTIN_HEADER ${CMAKE_CURRENT_BINARY_DIR}/xdr_builtin_h.c)

generate_embedded_code(xdr_builtin.c ${BUILTIN_SOURCE} embedded_builtin_c)
generate_embedded_code(xdr_builtin.h ${BUILTIN_HEADER} embedded_builtin_h)

add_executable(xdrzcc
    xdrzcc.c
    ${BUILTIN_SOURCE}
    ${BUILTIN_HEADER}
    ${FLEX_lexer_OUTPUTS}
    ${BISON_parser_OUTPUT_SOURCE}
)

set(XDRZCC ${CMAKE_CURRENT_BINARY_DIR}/xdrzcc PARENT_SCOPE)
