# SPDX-FileCopyrightText: 2024 Ben Jarvis
#
# SPDX-License-Identifier: LGPL

cmake_minimum_required(VERSION 3.22)
project(xdrzcc LANGUAGES C)

enable_testing()

# Find required Flex and Bison packages
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(3rdparty)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling AddressSanitizer")
    add_definitions(-fsanitize=address -fno-omit-frame-pointer
                    -fno-optimize-sibling-calls -fstack-protector-all)
    add_link_options(-fsanitize=address)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-O3)
endif()

add_definitions(-g -Wall -Werror)

add_subdirectory(src)
add_subdirectory(tests)

if (NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
    set(XDRZCC ${XDRZCC} PARENT_SCOPE)
endif()

