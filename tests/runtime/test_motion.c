/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc/stdlib.h"
#include <stdio.h>
#include <string.h>

static int test_axis_power_and_move(void)
{
    bool status = false;
    bool valid = false;
    bool error = false;
    bool done = false;
    bool busy = false;
    bool active = false;
    bool command_aborted = false;
    int32_t error_id = 0;
    int32_t position = 0;
    uint32_t cycle;
    const uint32_t dt_us = 10000U;

    if (mplc_motion_init(1U) != 0) {
        return 1;
    }

    if (mplc_motion_power(0, true, &status, &valid, &error, &error_id) != 0 ||
        !status || !valid || error) {
        mplc_motion_shutdown();
        return 2;
    }

    if (mplc_motion_move_absolute(0, true, 5000, 100000, 1000, 1000,
                                  &done, &busy, &active, &command_aborted,
                                  &error, &error_id) != 0 || error) {
        mplc_motion_shutdown();
        return 3;
    }

    for (cycle = 0; cycle < 1000U; cycle++) {
        mplc_motion_cycle(dt_us);
    }

    if (mplc_motion_read_actual_position(0, true, &position, &valid, &error, &error_id) != 0 ||
        error || position != 5000) {
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

    printf("test_motion: OK\n");
    return 0;
}
