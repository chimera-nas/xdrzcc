# SPDX-FileCopyrightText: 2024 - 2025 Ben Jarvis
#
# SPDX-License-Identifier: LGPL-2.1-only

cmake_minimum_required(VERSION 3.18)
project(xdrzcc LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# Find required Flex and Bison packages
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

find_path(UTHASH_INCLUDE_DIR uthash.h)
find_path(UTLIST_INCLUDE_DIR utlist.h)
if(NOT UTHASH_INCLUDE_DIR OR NOT UTLIST_INCLUDE_DIR)
    message(FATAL_ERROR "uthash headers are required (set CMAKE_PREFIX_PATH or UTHASH_INCLUDE_DIR/UTLIST_INCLUDE_DIR)")
endif()
include_directories(SYSTEM ${UTHASH_INCLUDE_DIR} ${UTLIST_INCLUDE_DIR})

if(MSVC)
    add_compile_options(/W3 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer")
    add_definitions(-fsanitize=address -fno-omit-frame-pointer
                    -fno-optimize-sibling-calls -fstack-protector-all)
    add_link_options(-fsanitize=address)
endif()

if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-O3)
endif()

if(NOT MSVC)
    add_compile_options(-g -Wall -Werror)
endif()

add_subdirectory(src)

if (NOT DISABLE_TESTS)
    add_subdirectory(tests)
endif()

if (NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
    set(XDRZCC ${XDRZCC} PARENT_SCOPE)
endif()

# Install man page
install(FILES man/xdrzcc.1 DESTINATION share/man/man1)

