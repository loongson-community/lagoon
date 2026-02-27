cmake_minimum_required(VERSION 3.10)
project(lagoon LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LAGOON_SOURCES
    lagoon.c
)

set(LAGOON_HEADERS
    include/lagoon.h
)

add_library(lagoon ${LAGOON_SOURCES})
target_include_directories(lagoon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()
