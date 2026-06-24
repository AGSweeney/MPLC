# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

if(UNIX AND NOT APPLE)
    add_executable(mplc_linux main_linux.c hal_linux.c)
    target_link_libraries(mplc_linux PRIVATE mplc_runtime)
    target_include_directories(mplc_linux PRIVATE ${CMAKE_SOURCE_DIR}/hal/include)
endif()
