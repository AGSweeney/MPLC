/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc_motion_hal.h"
#include <string.h>

typedef struct {
    mplc_axis_state_t state;
    bool              power_enabled;
    bool              homed;
    int32_t           target_position;
    int32_t           command_velocity;
    int32_t           acceleration;
    int32_t           deceleration;
    bool              error;
    int32_t           error_id;
    bool              busy;
    bool              active;
    bool              stop_pending;
    bool              halt_pending;
    bool              home_pending;
    int32_t           home_position;
} mplc_axis_t;

static mplc_axis_t g_axes[MPLC_MOTION_MAX_AXES];
static uint32_t    g_axis_count;
static bool        g_initialized;

static mplc_axis_group_state_t axis_to_group(mplc_axis_state_t state)
{
    switch (state) {
    case MPLC_AXIS_STATE_DISABLED:          return MPLC_AXIS_GROUP_DISABLED;
    case MPLC_AXIS_STATE_STANDSTILL:        return MPLC_AXIS_GROUP_STANDSTILL;
    case MPLC_AXIS_STATE_HOMING:            return MPLC_AXIS_GROUP_HOMING;
    case MPLC_AXIS_STATE_STOPPING:          return MPLC_AXIS_GROUP_STOPPING;
    case MPLC_AXIS_STATE_DISCRETE_MOTION:   return MPLC_AXIS_GROUP_DISCRETE_MOTION;
    case MPLC_AXIS_STATE_CONTINUOUS_MOTION: return MPLC_AXIS_GROUP_CONTINUOUS_MOTION;
    case MPLC_AXIS_STATE_ERRORSTOP:         return MPLC_AXIS_GROUP_ERRORSTOP;
    default:                                return MPLC_AXIS_GROUP_UNKNOWN;
    }
}

static bool axis_valid(uint16_t axis)
{
    return g_initialized && axis < g_axis_count;
}

static void axis_set_error(mplc_axis_t *ax, int32_t error_id)
{
    ax->error = true;
    ax->error_id = error_id;
    ax->state = MPLC_AXIS_STATE_ERRORSTOP;
    ax->busy = false;
    ax->active = false;
    ax->stop_pending = false;
    ax->halt_pending = false;
    ax->home_pending = false;
}

static void fill_status(uint16_t axis, mplc_axis_status_t *status)
{
    mplc_axis_t *ax = &g_axes[axis];

    memset(status, 0, sizeof(*status));
    status->axis_state = ax->state;
    status->group_state = axis_to_group(ax->state);
    status->power_enabled = ax->power_enabled;
    status->homed = ax->homed;
    status->error = ax->error;
    status->error_id = ax->error_id;
    status->busy = ax->busy;
    status->active = ax->active;
    mplc_motion_hal_read_actual_position(axis, &status->actual_position);
    status->command_position = ax->target_position;
    status->command_velocity = ax->command_velocity;
}

static int axis_require_powered(uint16_t axis, mplc_axis_t *ax)
{
    if (!axis_valid(axis)) {
        return -1;
    }
    if (!ax->power_enabled) {
        axis_set_error(ax, 1);
        return -2;
    }
    if (ax->error) {
        return -3;
    }
    return 0;
}

int mplc_motion_init(uint32_t axis_count)
{
    if (axis_count == 0U || axis_count > MPLC_MOTION_MAX_AXES) {
        return -1;
    }
    if (mplc_motion_hal_init(axis_count) != 0) {
        return -2;
    }
    memset(g_axes, 0, sizeof(g_axes));
    g_axis_count = axis_count;
    g_initialized = true;
    return 0;
}

void mplc_motion_shutdown(void)
{
    if (!g_initialized) {
        return;
    }
    mplc_motion_hal_shutdown();
    g_axis_count = 0U;
    g_initialized = false;
}

void mplc_motion_cycle(uint32_t dt_us)
{
    uint16_t axis;

    if (!g_initialized) {
        return;
    }

    mplc_motion_hal_cycle(dt_us);

    for (axis = 0; axis < g_axis_count; axis++) {
        mplc_axis_t *ax = &g_axes[axis];
        int32_t actual = 0;
        int32_t remaining;

        if (!ax->power_enabled || ax->state == MPLC_AXIS_STATE_DISABLED) {
            continue;
        }

        mplc_motion_hal_read_actual_position(axis, &actual);

        if (ax->state == MPLC_AXIS_STATE_HOMING && ax->home_pending) {
            ax->target_position = ax->home_position;
            ax->command_velocity = 1000;
            ax->state = MPLC_AXIS_STATE_DISCRETE_MOTION;
            ax->home_pending = false;
            ax->busy = true;
            ax->active = true;
        }

        if (ax->state == MPLC_AXIS_STATE_DISCRETE_MOTION) {
            remaining = ax->target_position - actual;
            if (remaining == 0) {
                if (ax->target_position == ax->home_position) {
                    ax->homed = true;
                }
                ax->state = MPLC_AXIS_STATE_STANDSTILL;
                ax->busy = false;
                ax->active = false;
                ax->command_velocity = 0;
                mplc_motion_hal_write_command_velocity(axis, 0);
            } else {
                mplc_motion_hal_write_command_velocity(axis,
                    remaining > 0 ? ax->command_velocity : -ax->command_velocity);
            }
        } else if (ax->state == MPLC_AXIS_STATE_CONTINUOUS_MOTION) {
            mplc_motion_hal_write_command_velocity(axis, ax->command_velocity);
        } else if (ax->state == MPLC_AXIS_STATE_STOPPING || ax->stop_pending || ax->halt_pending) {
            if (ax->command_velocity > 0) {
                ax->command_velocity -= ax->deceleration;
                if (ax->command_velocity < 0) {
                    ax->command_velocity = 0;
                }
            } else if (ax->command_velocity < 0) {
                ax->command_velocity += ax->deceleration;
                if (ax->command_velocity > 0) {
                    ax->command_velocity = 0;
                }
            }
            mplc_motion_hal_write_command_velocity(axis, ax->command_velocity);
            if (ax->command_velocity == 0) {
                ax->state = MPLC_AXIS_STATE_STANDSTILL;
                ax->busy = false;
                ax->active = false;
                ax->stop_pending = false;
                ax->halt_pending = false;
            }
        }
    }
}

int mplc_motion_power(uint16_t axis, bool enable, bool *status, bool *valid,
                      bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];

    if (enable) {
        ax->power_enabled = true;
        mplc_motion_hal_enable(axis, true);
        if (ax->error) {
            ax->state = MPLC_AXIS_STATE_ERRORSTOP;
        } else {
            ax->state = MPLC_AXIS_STATE_STANDSTILL;
        }
    } else {
        ax->power_enabled = false;
        ax->busy = false;
        ax->active = false;
        ax->command_velocity = 0;
        ax->state = MPLC_AXIS_STATE_DISABLED;
        mplc_motion_hal_enable(axis, false);
        mplc_motion_hal_write_command_velocity(axis, 0);
    }

    if (status) {
        *status = mplc_motion_hal_read_power_status(axis);
    }
    if (valid) {
        *valid = true;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return 0;
}

int mplc_motion_reset(uint16_t axis, bool execute, bool *done, bool *busy,
                      bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];

    if (execute && ax->state == MPLC_AXIS_STATE_ERRORSTOP) {
        ax->error = false;
        ax->error_id = 0;
        mplc_motion_hal_reset(axis);
        ax->state = ax->power_enabled ? MPLC_AXIS_STATE_STANDSTILL : MPLC_AXIS_STATE_DISABLED;
    }

    if (done) {
        *done = execute && !ax->error && ax->state != MPLC_AXIS_STATE_ERRORSTOP;
    }
    if (busy) {
        *busy = false;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return 0;
}

int mplc_motion_home(uint16_t axis, bool execute, int32_t position,
                     bool *done, bool *busy, bool *active, bool *command_aborted,
                     bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;
    int rc;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];
    rc = axis_require_powered(axis, ax);
    if (rc != 0) {
        goto out;
    }

    if (execute && ax->state == MPLC_AXIS_STATE_STANDSTILL) {
        ax->home_position = position;
        ax->home_pending = true;
        ax->state = MPLC_AXIS_STATE_HOMING;
        ax->busy = true;
        ax->active = true;
    }

out:
    if (done) {
        *done = execute && ax->state == MPLC_AXIS_STATE_STANDSTILL && ax->homed;
    }
    if (busy) {
        *busy = ax->busy;
    }
    if (active) {
        *active = ax->active;
    }
    if (command_aborted) {
        *command_aborted = false;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return rc;
}

int mplc_motion_move_absolute(uint16_t axis, bool execute, int32_t position,
                              int32_t velocity, int32_t acceleration, int32_t deceleration,
                              bool *done, bool *busy, bool *active, bool *command_aborted,
                              bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;
    int rc;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];
    rc = axis_require_powered(axis, ax);
    if (rc != 0) {
        goto out;
    }

    if (execute && (ax->state == MPLC_AXIS_STATE_STANDSTILL ||
                    ax->state == MPLC_AXIS_STATE_CONTINUOUS_MOTION)) {
        ax->target_position = position;
        ax->command_velocity = velocity > 0 ? velocity : 1000;
        ax->acceleration = acceleration > 0 ? acceleration : 1000;
        ax->deceleration = deceleration > 0 ? deceleration : 1000;
        ax->state = MPLC_AXIS_STATE_DISCRETE_MOTION;
        ax->busy = true;
        ax->active = true;
        mplc_motion_hal_write_target_position(axis, position);
    }

out:
    if (done) {
        *done = execute && ax->state == MPLC_AXIS_STATE_STANDSTILL && !ax->busy;
    }
    if (busy) {
        *busy = ax->busy;
    }
    if (active) {
        *active = ax->active;
    }
    if (command_aborted) {
        *command_aborted = false;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return rc;
}

int mplc_motion_move_relative(uint16_t axis, bool execute, int32_t distance,
                              int32_t velocity, int32_t acceleration, int32_t deceleration,
                              bool *done, bool *busy, bool *active, bool *command_aborted,
                              bool *error, int32_t *error_id)
{
    int32_t actual = 0;

    if (!axis_valid(axis)) {
        return -1;
    }
    mplc_motion_hal_read_actual_position(axis, &actual);
    return mplc_motion_move_absolute(axis, execute, actual + distance, velocity,
                                     acceleration, deceleration, done, busy, active,
                                     command_aborted, error, error_id);
}

int mplc_motion_move_velocity(uint16_t axis, bool execute, int32_t velocity,
                              int32_t acceleration, int32_t deceleration,
                              bool *done, bool *busy, bool *active, bool *command_aborted,
                              bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;
    int rc;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];
    rc = axis_require_powered(axis, ax);
    if (rc != 0) {
        goto out;
    }

    if (execute) {
        ax->command_velocity = velocity;
        ax->acceleration = acceleration > 0 ? acceleration : 1000;
        ax->deceleration = deceleration > 0 ? deceleration : 1000;
        ax->state = MPLC_AXIS_STATE_CONTINUOUS_MOTION;
        ax->busy = true;
        ax->active = true;
    } else if (ax->state == MPLC_AXIS_STATE_CONTINUOUS_MOTION) {
        ax->state = MPLC_AXIS_STATE_STANDSTILL;
        ax->busy = false;
        ax->active = false;
        ax->command_velocity = 0;
        mplc_motion_hal_write_command_velocity(axis, 0);
    }

out:
    if (done) {
        *done = !execute && ax->state == MPLC_AXIS_STATE_STANDSTILL;
    }
    if (busy) {
        *busy = ax->busy;
    }
    if (active) {
        *active = ax->active;
    }
    if (command_aborted) {
        *command_aborted = false;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return rc;
}

int mplc_motion_stop(uint16_t axis, bool execute, int32_t deceleration,
                     bool *done, bool *busy, bool *active, bool *command_aborted,
                     bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;
    int rc = 0;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];

    if (execute && ax->power_enabled && !ax->error) {
        ax->deceleration = deceleration > 0 ? deceleration : 1000;
        ax->stop_pending = true;
        ax->state = MPLC_AXIS_STATE_STOPPING;
        ax->busy = true;
        ax->active = true;
    }

    if (done) {
        *done = execute && ax->state == MPLC_AXIS_STATE_STANDSTILL && !ax->stop_pending;
    }
    if (busy) {
        *busy = ax->busy;
    }
    if (active) {
        *active = ax->active;
    }
    if (command_aborted) {
        *command_aborted = execute && ax->stop_pending;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return rc;
}

int mplc_motion_halt(uint16_t axis, bool execute, int32_t deceleration,
                     bool *done, bool *busy, bool *active, bool *command_aborted,
                     bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;
    int rc = 0;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];

    if (execute && ax->power_enabled && !ax->error) {
        ax->deceleration = deceleration > 0 ? deceleration : 1000;
        ax->halt_pending = true;
        ax->state = MPLC_AXIS_STATE_STOPPING;
        ax->busy = true;
        ax->active = true;
    }

    if (done) {
        *done = execute && ax->state == MPLC_AXIS_STATE_STANDSTILL && !ax->halt_pending;
    }
    if (busy) {
        *busy = ax->busy;
    }
    if (active) {
        *active = ax->active;
    }
    if (command_aborted) {
        *command_aborted = execute && ax->halt_pending;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return rc;
}

int mplc_motion_read_actual_position(uint16_t axis, bool enable,
                                     int32_t *position, bool *valid,
                                     bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;

    if (!axis_valid(axis)) {
        return -1;
    }
    ax = &g_axes[axis];

    if (enable) {
        mplc_motion_hal_read_actual_position(axis, position);
    } else if (position) {
        *position = 0;
    }
    if (valid) {
        *valid = enable && ax->power_enabled;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return 0;
}

int mplc_motion_read_status(uint16_t axis, bool enable,
                            mplc_axis_status_t *status,
                            bool *valid, bool *error, int32_t *error_id)
{
    mplc_axis_t *ax;

    if (!axis_valid(axis) || !status) {
        return -1;
    }
    ax = &g_axes[axis];

    if (enable) {
        fill_status(axis, status);
    } else {
        memset(status, 0, sizeof(*status));
    }
    if (valid) {
        *valid = enable;
    }
    if (error) {
        *error = ax->error;
    }
    if (error_id) {
        *error_id = ax->error_id;
    }
    return 0;
}

const mplc_fb_vtable_t *mplc_fb_get_vtable(mplc_native_fb_t type)
{
    const mplc_fb_vtable_t *vt = mplc_stdlib_get_vtable(type);
    if (vt) {
        return vt;
    }
    return mplc_motion_get_vtable(type);
}
