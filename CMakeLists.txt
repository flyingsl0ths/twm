cmake_minimum_required(VERSION 3.29.0 FATAL_ERROR)

set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In Source Builds Not Allowed")
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
endif ()

set(NAME "twm")
set(DESCRIPTION "A toy window manager")

project(
        ${NAME}
        VERSION 1.0.0
        DESCRIPTION ${DESCRIPTION}
        LANGUAGES CXX)

add_library(compile-options INTERFACE)

target_compile_options(
        compile-options INTERFACE "-Wextra;-Wall;-Werror;-Wunused-local-typedefs")

target_compile_features(compile-options INTERFACE cxx_std_20)

find_package(X11 REQUIRED)

add_subdirectory(app)
