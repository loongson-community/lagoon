cmake_minimum_required(VERSION 3.10)
project(lagoon VERSION 1.0.0 LANGUAGES C)

set(LAGOON_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(LAGOON_MASTER_PROJECT ON)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(LAGOON_SOURCES
    lagoon.c
)

set(LAGOON_HEADERS include/lagoon.h)

add_library(lagoon ${LAGOON_SOURCES})
if(NOT TARGET lagoon::lagoon)
    add_library(lagoon::lagoon ALIAS lagoon)
endif()
set_target_properties(lagoon PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(lagoon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_definitions(lagoon PRIVATE $<$<CONFIG:Debug>:LA_DEBUG>)

option(BUILD_TESTS "Build tests" OFF)
option(LAGOON_INSTALL "Install Lagoon headers and CMake package files" ON)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lagoonConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lagoonConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lagoon
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/lagoonConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

export(
    TARGETS lagoon
    FILE ${CMAKE_CURRENT_BINARY_DIR}/lagoonTargets.cmake
    NAMESPACE lagoon::
)

if(LAGOON_INSTALL)
    install(
        TARGETS lagoon
        EXPORT lagoonTargets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        FILES ${LAGOON_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        FILES LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/lagoon
    )
    install(
        EXPORT lagoonTargets
        FILE lagoonTargets.cmake
        NAMESPACE lagoon::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lagoon
    )
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/lagoonConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/lagoonConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lagoon
    )
endif()

if(BUILD_TESTS)
    enable_testing()

    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    endif()
endif()

if(LAGOON_MASTER_PROJECT)
    export(PACKAGE lagoon)
endif()
