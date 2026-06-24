# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

function(mplc_add_motionstack_test target src)
    add_executable(${target} ${src})
    target_link_libraries(${target} PRIVATE mplc_motionstack)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/MotionStack/include
        ${CMAKE_SOURCE_DIR}/MotionStack/test
        ${CMAKE_SOURCE_DIR}/schema
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

mplc_add_motionstack_test(test_ms_endian test_endian_wire.c)
mplc_add_motionstack_test(test_ms_q15 test_q15.c)
mplc_add_motionstack_test(test_ms_linear test_linear_interp.c)
mplc_add_motionstack_test(test_ms_planner test_planner_queue.c)
mplc_add_motionstack_test(test_ms_junction_planner test_junction_planner.c)
mplc_add_motionstack_test(test_ms_junction_trajectory test_junction_trajectory.c)
mplc_add_motionstack_test(test_ms_linear_accel test_linear_accel.c)
mplc_add_motionstack_test(test_ms_arc test_arc_interp.c)

add_executable(test_ms_endian_be test_endian_wire.c)
target_link_libraries(test_ms_endian_be PRIVATE mplc_motionstack)
target_compile_definitions(test_ms_endian_be PRIVATE MPLC_BIG_ENDIAN)
target_include_directories(test_ms_endian_be PRIVATE
    ${CMAKE_SOURCE_DIR}/MotionStack/include
    ${CMAKE_SOURCE_DIR}/MotionStack/test
    ${CMAKE_SOURCE_DIR}/schema
)
add_test(NAME test_ms_endian_be COMMAND test_ms_endian_be)
