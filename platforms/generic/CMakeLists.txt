# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

if(NOT DEFINED MPLC_REPO_ROOT)
    set(MPLC_REPO_ROOT ${CMAKE_SOURCE_DIR})
endif()

add_library(mplc_hal_generic STATIC hal_generic.c)
target_include_directories(mplc_hal_generic PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${MPLC_REPO_ROOT}/hal/include
)
