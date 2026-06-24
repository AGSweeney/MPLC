# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(MOTIONSTACK_SOURCES
    src/motionstack_endian.c
    src/motionstack_q15.c
    src/motionstack_units.c
    src/motionstack_profile.c
    src/motionstack_linear.c
    src/motionstack_arc.c
    src/motionstack_planner.c
    src/motionstack_core.c
    src/drive_generic_stepper.c
    src/drive_clearpath.c
    test/port_sim.c
)

add_library(mplc_motionstack STATIC ${MOTIONSTACK_SOURCES})
target_include_directories(mplc_motionstack
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test
            ${CMAKE_SOURCE_DIR}/schema
)
