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
} mplc_motion_hal_axis_t;

static mplc_motion_hal_axis_t g_hal_axes[MPLC_MOTION_HAL_MAX_AXES];
static uint32_t               g_hal_axis_count;

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

void mplc_motion_hal_enable(uint16_t axis, bool enable)
{
    if (axis >= g_hal_axis_count) {
        return;
    }
    g_hal_axes[axis].enabled = enable;
}

void mplc_motion_hal_reset(uint16_t axis)
{
    if (axis >= g_hal_axis_count) {
        return;
    }
    g_hal_axes[axis].error = false;
    g_hal_axes[axis].error_id = 0;
}

void mplc_motion_hal_read_actual_position(uint16_t axis, int32_t *position)
{
    if (!position || axis >= g_hal_axis_count) {
        return;
    }
    *position = g_hal_axes[axis].actual_position;
}

void mplc_motion_hal_write_target_position(uint16_t axis, int32_t position)
{
    if (axis >= g_hal_axis_count) {
        return;
    }
    g_hal_axes[axis].target_position = position;
}

void mplc_motion_hal_write_command_velocity(uint16_t axis, int32_t velocity)
{
    if (axis >= g_hal_axis_count) {
        return;
    }
    g_hal_axes[axis].command_velocity = velocity;
}

bool mplc_motion_hal_read_power_status(uint16_t axis)
{
    if (axis >= g_hal_axis_count) {
        return false;
    }
    return g_hal_axes[axis].enabled;
}

bool mplc_motion_hal_read_error(uint16_t axis, int32_t *error_id)
{
    if (axis >= g_hal_axis_count) {
        return false;
    }
    if (error_id) {
        *error_id = g_hal_axes[axis].error_id;
    }
    return g_hal_axes[axis].error;
}

void mplc_motion_hal_cycle(uint32_t dt_us)
{
    uint16_t axis;
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
