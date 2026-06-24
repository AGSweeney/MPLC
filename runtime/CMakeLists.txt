# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

if(NOT DEFINED MPLC_REPO_ROOT)
    set(MPLC_REPO_ROOT ${CMAKE_SOURCE_DIR})
endif()

add_library(mplc_runtime STATIC
    core/loader.c
    core/vm.c
    core/scheduler.c
    core/runtime.c
    io/io.c
    stdlib/stdlib.c
    motion/motion.c
    motion/mc_fb.c
    motion/motion_hal_default.c
)

target_include_directories(mplc_runtime PUBLIC
    ${MPLC_REPO_ROOT}/runtime/include
    ${MPLC_REPO_ROOT}/schema
    ${MPLC_REPO_ROOT}/hal/include
)

if(MSVC)
    target_compile_options(mplc_runtime PRIVATE /W4)
else()
    target_compile_options(mplc_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MPLC_NO_FPU)
    target_compile_definitions(mplc_runtime PUBLIC MPLC_NO_FPU=1)
endif()
