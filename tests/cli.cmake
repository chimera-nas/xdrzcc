# SPDX-FileCopyrightText: 2026 Ben Jarvis
# SPDX-License-Identifier: LGPL-2.1-only

# Exercise clustered options, --, and output paths containing spaces. No shell.
set(output "${WORK}/cli output")
file(MAKE_DIRECTORY "${output}")
execute_process(COMMAND "${XDRZCC}" -h RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "-h failed: ${rc}")
endif()
execute_process(COMMAND "${XDRZCC}" -z RESULT_VARIABLE rc)
if(NOT rc EQUAL 1)
    message(FATAL_ERROR "unknown option returned ${rc}, expected 1")
endif()
execute_process(COMMAND "${XDRZCC}" -re -- "${SOURCE}/uint32.x"
    "${output}/message.c" "${output}/message.h" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0 OR NOT EXISTS "${output}/message.c" OR NOT EXISTS "${output}/message.h")
    message(FATAL_ERROR "generation with options and spaces failed: ${rc}")
endif()
