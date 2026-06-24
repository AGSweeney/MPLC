# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

if(NOT DEFINED MPLC_REPO_ROOT)
    set(MPLC_REPO_ROOT ${CMAKE_SOURCE_DIR})
endif()

add_library(mplc_compiler STATIC
    cli/mplc_compile.c
    common/ir.c
    common/diag.c
    semantic/semantic.c
    codegen/codegen.c
    linker/linker.c
    frontend/st/st_parser.c
    frontend/il/il_parser.c
    frontend/plcopen/plcopen_parser.c
    frontend/ld/ld_parser.c
    frontend/fbd/fbd_parser.c
    frontend/sfc/sfc_parser.c
)

target_include_directories(mplc_compiler
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/cli
        ${CMAKE_CURRENT_LIST_DIR}/common
        ${CMAKE_CURRENT_LIST_DIR}/semantic
        ${MPLC_REPO_ROOT}/schema
        ${MPLC_REPO_ROOT}/runtime/include
)

target_link_libraries(mplc_compiler PUBLIC mplc_runtime mplc_hal_generic)

if(MSVC)
    target_compile_options(mplc_compiler PRIVATE /W4)
else()
    target_compile_options(mplc_compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mplc cli/main.c)
target_link_libraries(mplc PRIVATE mplc_compiler)
