# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(MPLC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MPLC_BUILD_COMPILER "Build host compiler toolchain" ON)
option(MPLC_BUILD_RUNTIME  "Build portable runtime library" ON)
option(MPLC_BUILD_TESTS    "Build unit tests" ON)
option(MPLC_BUILD_STUDIO   "Build Qt MPLC Studio desktop app" OFF)
option(MPLC_NO_FPU           "Use integer-only paths where possible" OFF)

set(MPLC_REPO_ROOT ${CMAKE_SOURCE_DIR} CACHE INTERNAL "MPLC repository root")

include_directories(
    ${MPLC_REPO_ROOT}/schema
    ${MPLC_REPO_ROOT}/hal/include
    ${MPLC_REPO_ROOT}/runtime/include
)

if(MPLC_BUILD_RUNTIME)
    add_subdirectory(runtime)
    add_subdirectory(platforms/generic)
    if(UNIX AND NOT APPLE)
        add_subdirectory(platforms/linux)
    endif()
    add_subdirectory(platforms/baremetal)
    add_subdirectory(platforms/netburner)
endif()

add_subdirectory(MotionStack)

if(MPLC_BUILD_COMPILER)
    add_subdirectory(compiler)
endif()

if(MPLC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MPLC_BUILD_STUDIO)
    add_subdirectory(tools/mplc-studio)
endif()
