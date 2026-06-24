/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Default simulated motion HAL. Platform targets may provide alternate
 * implementations of the same mplc_motion_hal_* symbols.
 */

#include "mplc_motion_hal.h"
#include <string.h>

typedef struct {
    bool    enabled;
    bool    error;
    int32_t error_id;
    int32_t actual_position;
    int32_t target_position;
    int32_t command_velocity;
    int32_t target_velocity;
    bool    stop_locked;
} mplc_motion_hal_axis_t;

static mplc_motion_hal_axis_t g_hal_axes[MPLC_MOTION_HAL_MAX_AXES];
static uint32_t               g_hal_axis_count;

static bool hal_axis_valid(mplc_axis_ref_t axis)
{
    return axis != MPLC_AXIS_INVALID && axis < g_hal_axis_count;
}

int mplc_motion_hal_init(uint32_t axis_count)
{
    if (axis_count == 0U || axis_count > MPLC_MOTION_HAL_MAX_AXES) {
        return -1;
    }
    memset(g_hal_axes, 0, sizeof(g_hal_axes));
    g_hal_axis_count = axis_count;
    return 0;
}

void mplc_motion_hal_shutdown(void)
{
    g_hal_axis_count = 0U;
}

void mplc_motion_hal_enable(mplc_axis_ref_t axis, bool enable)
{
    if (!hal_axis_valid(axis)) {
        return;
    }
    g_hal_axes[axis].enabled = enable;
}

void mplc_motion_hal_reset(mplc_axis_ref_t axis)
{
    if (!hal_axis_valid(axis)) {
        return;
    }
    g_hal_axes[axis].error = false;
    g_hal_axes[axis].error_id = 0;
}

void mplc_motion_hal_read_actual_position(mplc_axis_ref_t axis, int32_t *position)
{
    if (!position || !hal_axis_valid(axis)) {
        return;
    }
    *position = g_hal_axes[axis].actual_position;
}

void mplc_motion_hal_write_target_position(mplc_axis_ref_t axis, int32_t position)
{
    if (!hal_axis_valid(axis)) {
        return;
    }
    g_hal_axes[axis].target_position = position;
}

void mplc_motion_hal_write_command_velocity(mplc_axis_ref_t axis, int32_t velocity)
{
    if (!hal_axis_valid(axis)) {
        return;
    }
    g_hal_axes[axis].command_velocity = velocity;
}

bool mplc_motion_hal_read_power_status(mplc_axis_ref_t axis)
{
    if (!hal_axis_valid(axis)) {
        return false;
    }
    return g_hal_axes[axis].enabled;
}

bool mplc_motion_hal_read_error(mplc_axis_ref_t axis, int32_t *error_id)
{
    if (!hal_axis_valid(axis)) {
        return false;
    }
    if (error_id) {
        *error_id = g_hal_axes[axis].error_id;
    }
    return g_hal_axes[axis].error;
}

int mplc_motion_hal_start_absolute(mplc_axis_ref_t axis, const mplc_motion_move_t *move)
{
    if (!hal_axis_valid(axis) || !move) {
        return -1;
    }
    g_hal_axes[axis].target_position = move->target_position;
    g_hal_axes[axis].target_velocity = move->velocity;
    g_hal_axes[axis].command_velocity = move->velocity;
    return 0;
}

int mplc_motion_hal_start_velocity(mplc_axis_ref_t axis, const mplc_motion_move_t *move)
{
    if (!hal_axis_valid(axis) || !move) {
        return -1;
    }
    g_hal_axes[axis].target_velocity = move->velocity;
    g_hal_axes[axis].command_velocity = move->velocity;
    return 0;
}

int mplc_motion_hal_get_status(mplc_axis_ref_t axis, mplc_motion_hal_status_t *status)
{
    mplc_motion_hal_axis_t *ax;

    if (!hal_axis_valid(axis) || !status) {
        return -1;
    }
    ax = &g_hal_axes[axis];
    status->actual_position = ax->actual_position;
    status->command_velocity = ax->command_velocity;
    status->error = ax->error;
    status->error_id = ax->error_id;
    status->in_velocity = ax->command_velocity == ax->target_velocity &&
                          ax->target_velocity != 0;
    return 0;
}

int mplc_motion_hal_stop(mplc_axis_ref_t axis, int32_t deceleration, bool lock_axis)
{
    (void)deceleration;
    if (!hal_axis_valid(axis)) {
        return -1;
    }
    g_hal_axes[axis].stop_locked = lock_axis;
    return 0;
}

int mplc_motion_hal_halt(mplc_axis_ref_t axis, int32_t deceleration)
{
    (void)deceleration;
    if (!hal_axis_valid(axis)) {
        return -1;
    }
    g_hal_axes[axis].stop_locked = false;
    return 0;
}

void mplc_motion_hal_cycle(uint32_t dt_us)
{
    mplc_axis_ref_t axis;
    int64_t delta;

    if (dt_us == 0U) {
        return;
    }

    for (axis = 0; axis < g_hal_axis_count; axis++) {
        mplc_motion_hal_axis_t *ax = &g_hal_axes[axis];
        if (!ax->enabled) {
            continue;
        }
        delta = (int64_t)ax->command_velocity * (int64_t)dt_us / 1000000LL;
        ax->actual_position += (int32_t)delta;
    }
}
