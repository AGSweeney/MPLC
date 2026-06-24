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

static int test_competing_move_commands(void)
{
    mplc_power_status_t power;
    mplc_move_absolute_request_t move1;
    mplc_move_absolute_request_t move2;
    mplc_motion_command_status_t status1;
    mplc_motion_command_status_t status2;
    mplc_motion_command_id_t cmd1;
    mplc_motion_command_id_t cmd2;
    uint32_t cycle;
    const uint32_t dt_us = 10000U;

    if (mplc_motion_init(1U) != 0) {
        return 40;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 41;
    }

    move1.position = 100000;
    move1.velocity = 1000;
    move1.acceleration = 1000;
    move1.deceleration = 1000;
    cmd1 = mplc_motion_start_absolute(0, &move1, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (cmd1 == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 42;
    }

    move2.position = 2000;
    move2.velocity = 100000;
    move2.acceleration = 1000;
    move2.deceleration = 1000;
    cmd2 = mplc_motion_start_absolute(0, &move2, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (cmd2 == MPLC_MOTION_COMMAND_NONE || cmd2 == cmd1) {
        mplc_motion_shutdown();
        return 43;
    }

    if (mplc_motion_command_status(cmd1, &status1) != 0 ||
        !status1.command_aborted || status1.done) {
        mplc_motion_shutdown();
        return 44;
    }
    if (mplc_motion_command_status(cmd2, &status2) != 0 || !status2.busy) {
        mplc_motion_shutdown();
        return 45;
    }

    for (cycle = 0; cycle < 1000U; cycle++) {
        mplc_motion_cycle(dt_us);
        mplc_motion_command_status(cmd2, &status2);
        if (status2.done) {
            break;
        }
    }

    if (mplc_motion_command_status(cmd1, &status1) != 0 ||
        !status1.command_aborted || status1.done) {
        mplc_motion_shutdown();
        return 46;
    }
    if (!status2.done) {
        mplc_motion_shutdown();
        return 47;
    }

    mplc_motion_shutdown();
    return 0;
}

static int test_invalid_move_does_not_abort_active(void)
{
    mplc_power_status_t power;
    mplc_axis_config_t config;
    mplc_move_absolute_request_t valid_move;
    mplc_move_absolute_request_t invalid_move;
    mplc_motion_command_status_t active_status;
    mplc_motion_command_id_t cmd1;
    mplc_motion_command_id_t cmd2;

    if (mplc_motion_init(1U) != 0) {
        return 50;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 51;
    }

    config.counts_per_unit_num = 1;
    config.counts_per_unit_den = 1;
    config.max_velocity = 5000;
    config.max_acceleration = 5000;
    if (mplc_motion_configure_axis(0, &config) != 0) {
        mplc_motion_shutdown();
        return 52;
    }

    valid_move.position = 5000;
    valid_move.velocity = 1000;
    valid_move.acceleration = 1000;
    valid_move.deceleration = 1000;
    cmd1 = mplc_motion_start_absolute(0, &valid_move, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (cmd1 == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 53;
    }

    invalid_move.position = 6000;
    invalid_move.velocity = 10000;
    invalid_move.acceleration = 1000;
    invalid_move.deceleration = 1000;
    cmd2 = mplc_motion_start_absolute(0, &invalid_move, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (cmd2 != MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 54;
    }

    if (mplc_motion_command_status(cmd1, &active_status) != 0 ||
        active_status.command_aborted || !active_status.busy) {
        mplc_motion_shutdown();
        return 55;
    }

    mplc_motion_shutdown();
    return 0;
}

static int test_stop_release_owner_only(void)
{
    mplc_power_status_t power;
    mplc_stop_request_t stop1;
    mplc_stop_request_t stop2;
    mplc_motion_command_id_t stop_cmd1;
    mplc_motion_command_id_t stop_cmd2;
    mplc_axis_status_t axis_status;

    if (mplc_motion_init(1U) != 0) {
        return 60;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 61;
    }

    stop1.deceleration = 1000;
    stop_cmd1 = mplc_motion_start_stop(0, &stop1, MPLC_FB_MC_STOP);
    if (stop_cmd1 == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 62;
    }

    if (mplc_motion_release_stop(0, stop_cmd1 + 100U) == 0) {
        mplc_motion_shutdown();
        return 63;
    }

    stop2.deceleration = 1000;
    stop_cmd2 = mplc_motion_start_stop(0, &stop2, MPLC_FB_MC_STOP);
    if (stop_cmd2 == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 64;
    }
    if (mplc_motion_release_stop(0, stop_cmd1) == 0) {
        mplc_motion_shutdown();
        return 65;
    }

    if (mplc_motion_release_stop(0, stop_cmd2) != 0) {
        mplc_motion_shutdown();
        return 66;
    }
    if (mplc_motion_read_axis_status(0, true, &axis_status) != 0 || axis_status.stop_locked) {
        mplc_motion_shutdown();
        return 67;
    }

    mplc_motion_shutdown();
    return 0;
}

static int test_velocity_reports_in_velocity_not_done(void)
{
    mplc_power_status_t power;
    mplc_move_velocity_request_t move;
    mplc_motion_command_status_t status;
    mplc_motion_command_id_t cmd;
    uint32_t cycle;
    const uint32_t dt_us = 10000U;

    if (mplc_motion_init(1U) != 0) {
        return 70;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 71;
    }

    move.velocity = 5000;
    move.acceleration = 1000;
    move.deceleration = 1000;
    cmd = mplc_motion_start_velocity(0, &move, MPLC_FB_MC_MOVE_VELOCITY);
    if (cmd == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 72;
    }

    for (cycle = 0; cycle < 10U; cycle++) {
        mplc_motion_cycle(dt_us);
    }

    if (mplc_motion_command_status(cmd, &status) != 0 ||
        !status.in_velocity || status.done) {
        mplc_motion_shutdown();
        return 73;
    }

    mplc_motion_shutdown();
    return 0;
}

static int test_terminal_status_retained_until_ack(void)
{
    mplc_power_status_t power;
    mplc_move_absolute_request_t move;
    mplc_motion_command_status_t status;
    mplc_motion_command_id_t cmd;
    uint32_t cycle;
    const uint32_t dt_us = 10000U;

    if (mplc_motion_init(1U) != 0) {
        return 80;
    }
    if (mplc_motion_power(0, true, &power) != 0) {
        mplc_motion_shutdown();
        return 81;
    }

    move.position = 1000;
    move.velocity = 100000;
    move.acceleration = 1000;
    move.deceleration = 1000;
    cmd = mplc_motion_start_absolute(0, &move, MPLC_FB_MC_MOVE_ABSOLUTE);
    if (cmd == MPLC_MOTION_COMMAND_NONE) {
        mplc_motion_shutdown();
        return 82;
    }

    for (cycle = 0; cycle < 1000U; cycle++) {
        mplc_motion_cycle(dt_us);
        if (mplc_motion_command_status(cmd, &status) == 0 && status.done) {
            break;
        }
    }
    if (!status.done) {
        mplc_motion_shutdown();
        return 83;
    }

    if (mplc_motion_command_status(cmd, &status) != 0 || !status.done) {
        mplc_motion_shutdown();
        return 84;
    }

    if (mplc_motion_command_ack(cmd) != 0) {
        mplc_motion_shutdown();
        return 85;
    }

    if (mplc_motion_command_status(cmd, &status) != 0 ||
        status.state != MPLC_MOTION_CMD_IDLE) {
        mplc_motion_shutdown();
        return 86;
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

    rc = test_competing_move_commands();
    if (rc != 0) {
        fprintf(stderr, "test_competing_move_commands failed: %d\n", rc);
        return rc;
    }

    rc = test_invalid_move_does_not_abort_active();
    if (rc != 0) {
        fprintf(stderr, "test_invalid_move_does_not_abort_active failed: %d\n", rc);
        return rc;
    }

    rc = test_stop_release_owner_only();
    if (rc != 0) {
        fprintf(stderr, "test_stop_release_owner_only failed: %d\n", rc);
        return rc;
    }

    rc = test_velocity_reports_in_velocity_not_done();
    if (rc != 0) {
        fprintf(stderr, "test_velocity_reports_in_velocity_not_done failed: %d\n", rc);
        return rc;
    }

    rc = test_terminal_status_retained_until_ack();
    if (rc != 0) {
        fprintf(stderr, "test_terminal_status_retained_until_ack failed: %d\n", rc);
        return rc;
    }

    printf("test_motion: OK\n");
    return 0;
}
