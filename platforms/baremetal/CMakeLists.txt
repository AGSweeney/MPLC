# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_library(mplc_hal_baremetal STATIC hal_baremetal.c)
target_include_directories(mplc_hal_baremetal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(MPLC_BAREMETAL_HOST_TEST "Build bare-metal runner for host smoke tests" ON)

if(MPLC_BAREMETAL_HOST_TEST)
    add_executable(mplc_baremetal_test main_baremetal.c embedded_stub.c)
    target_compile_definitions(mplc_baremetal_test PRIVATE MPLC_BAREMETAL_HOST_TEST=1)
    target_link_libraries(mplc_baremetal_test PRIVATE mplc_runtime mplc_hal_baremetal mplc_hal_generic)
    target_include_directories(mplc_baremetal_test PRIVATE ${CMAKE_SOURCE_DIR}/hal/include)
endif()
