/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc/stdlib.h"
#include <stdio.h>

static int test_axis_power_and_move(void)
{
    mplc_power_status_t power;
    mplc_move_absolute_request_t move;
    mplc_motion_command_status_t cmd_status;
    mplc_read_position_result_t position;
    mplc_motion_command_id_t command_id;
    uint32_t cycle;
    const uint32_t dt_us = 10000U;

    if (mplc_motion_init(1U) != 0) {
        return 1;
    }

    if (mplc_motion_power(0, true, &power) != 0 || !power.status || !power.valid || power.error) {
        mplc_motion_shutdown();
        return 2;
    }

    move.position = 5000;
    move.velocity = 100000;
    move.acceleration = 1000;
    move.deceleration = 1000;
    command_id = mplc_motion_start_absolute(0, &move, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 3;
    }

    for (cycle = 0; cycle < 1000U; cycle++) {
        mplc_motion_cycle(dt_us);
        if (mplc_motion_command_status(command_id, &cmd_status) == 0 && cmd_status.done) {
            break;
        }
    }

    if (mplc_motion_read_actual_position(0, true, &position) != 0 ||
        position.error || position.position != 5000) {
        mplc_motion_shutdown();
        return 4;
    }

    mplc_motion_shutdown();
    return 0;
}

static int test_mc_fb_vtables(void)
{
    if (!mplc_fb_get_vtable(MPLC_FB_MC_POWER)) {
        return 10;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_RESET)) {
        return 11;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_HOME)) {
        return 12;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_MOVE_ABSOLUTE)) {
        return 13;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_MOVE_RELATIVE)) {
        return 14;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_MOVE_VELOCITY)) {
        return 15;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_STOP)) {
        return 16;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_HALT)) {
        return 17;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_READ_ACTUAL_POSITION)) {
        return 18;
    }
    if (!mplc_fb_get_vtable(MPLC_FB_MC_READ_STATUS)) {
        return 19;
    }
    if (mplc_motion_instance_size(MPLC_FB_MC_POWER) == 0U) {
        return 20;
    }
    if (mplc_motion_instance_size(MPLC_FB_MC_MOVE_VELOCITY) == 0U) {
        return 21;
    }
    return 0;
}

static int test_stop_locks_axis(void)
{
    mplc_power_status_t power;
    mplc_move_absolute_request_t move;
    mplc_stop_request_t stop;
    mplc_axis_status_t axis_status;
    mplc_motion_command_id_t move_id;

    if (mplc_motion_init(1U) != 0) {
        return 30;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 31;
    }

    move.position = 1000;
    move.velocity = 1000;
    move.acceleration = 1000;
    move.deceleration = 1000;
    move_id = mplc_motion_start_absolute(0, &move, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (move_id == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 32;
    }

    stop.deceleration = 1000;
    if (mplc_motion_start_stop(0, &stop, MPLC_FB_MC_STOP) == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 33;
    }

    move.position = 2000;
    if (mplc_motion_start_absolute(0, &move, MPLC_FB_MC_MOVE_ABSOLUTE) != MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 34;
    }

    if (mplc_motion_read_axis_status(0, true, &axis_status) != 0 || !axis_status.stop_locked) {
        mplc_motion_shutdown();
        return 35;
    }

    mplc_motion_shutdown();
    return 0;
}

int main(void)
{
    int rc;

    rc = test_mc_fb_vtables();
    if (rc != 0) {
        fprintf(stderr, "test_mc_fb_vtables failed: %d\n", rc);
        return rc;
    }

    rc = test_axis_power_and_move();
    if (rc != 0) {
        fprintf(stderr, "test_axis_power_and_move failed: %d\n", rc);
        return rc;
    }

    rc = test_stop_locks_axis();
    if (rc != 0) {
        fprintf(stderr, "test_stop_locks_axis failed: %d\n", rc);
        return rc;
    }

    printf("test_motion: OK\n");
    return 0;
}
