# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.21)

project(MPLCStudio
    VERSION 0.1.1
    DESCRIPTION "Qt IDE for MPLC Structured Text and NetBurner deployment"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET mplc_compiler)
    set(MPLC_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../" CACHE INTERNAL "MPLC repository root")
    enable_language(C)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)

    add_subdirectory(${MPLC_REPO_ROOT}/runtime ${CMAKE_BINARY_DIR}/mplc_runtime)
    add_subdirectory(${MPLC_REPO_ROOT}/platforms/generic ${CMAKE_BINARY_DIR}/mplc_hal_generic)
    add_subdirectory(${MPLC_REPO_ROOT}/compiler ${CMAKE_BINARY_DIR}/mplc_compiler)
endif()

add_subdirectory(app)
