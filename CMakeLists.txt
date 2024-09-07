cmake_minimum_required(VERSION 3.10)
project(xdrzcc LANGUAGES C)

# Find required Flex and Bison packages
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

include_directories(3rdparty)

add_subdirectory(src)
