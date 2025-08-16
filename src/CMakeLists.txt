# SPDX-FileCopyrightText: 2024 Ben Jarvis
#
# SPDX-License-Identifier: LGPL

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

set(BUILTIN_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/xdr_builtin.c)
set(BUILTIN_HEADER ${CMAKE_CURRENT_BINARY_DIR}/xdr_builtin_h.c)

add_custom_command(
    OUTPUT ${BUILTIN_SOURCE}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded.sh ${CMAKE_CURRENT_SOURCE_DIR}/xdr_builtin.c ${BUILTIN_SOURCE} embedded_builtin_c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xdr_builtin.c
    COMMENT "Generating embedded C source"
)

add_custom_command(
    OUTPUT ${BUILTIN_HEADER}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_embedded.sh ${CMAKE_CURRENT_SOURCE_DIR}/xdr_builtin.h ${BUILTIN_HEADER} embedded_builtin_h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/xdr_builtin.h
    COMMENT "Generating embedded C header"
)

add_custom_target(generate_embedded_files DEPENDS ${BUILTIN_SOURCE} ${BUILTIN_HEADER})
set_source_files_properties(
    ${FLEX_OUTPUT} PROPERTIES COMPILE_OPTIONS -Wno-unused
)

add_executable(xdrzcc
    xdrzcc.c
    ${BUILTIN_SOURCE}
    ${BUILTIN_HEADER}
    ${FLEX_lexer_OUTPUTS}
    ${BISON_parser_OUTPUT_SOURCE}
)

add_dependencies(xdrzcc generate_embedded_files)

set(XDRZCC ${CMAKE_CURRENT_BINARY_DIR}/xdrzcc PARENT_SCOPE)

install(TARGETS xdrzcc RUNTIME DESTINATION bin)
