/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc_motion_hal.h"
#include <string.h>

#define MPLC_MOTION_MAX_COMMANDS 32U

typedef struct {
    mplc_motion_command_id_t    id;
    mplc_axis_ref_t             axis;
    mplc_native_fb_t            type;
    mplc_motion_command_state_t state;
    bool                        active;
    bool                        command_aborted;
    int32_t                     error_id;
} mplc_motion_command_t;

typedef struct {
    mplc_axis_state_t           state;
    mplc_axis_config_t          config;
    bool                        power_enabled;
    bool                        homed;
    int32_t                     target_position;
    int32_t                     command_velocity;
    int32_t                     target_velocity;
    int32_t                     acceleration;
    int32_t                     deceleration;
    bool                        error;
    int32_t                     error_id;
    bool                        stop_locked;
    bool                        home_pending;
    int32_t                     home_position;
    mplc_motion_command_id_t    active_command;
    mplc_native_fb_t            active_command_type;
} mplc_axis_runtime_t;

static mplc_axis_runtime_t     g_axes[MPLC_MOTION_MAX_AXES];
static mplc_motion_command_t     g_commands[MPLC_MOTION_MAX_COMMANDS];
static uint32_t                  g_axis_count;
static mplc_motion_command_id_t  g_next_command_id = 1U;
static bool                      g_initialized;

static bool axis_ref_valid(mplc_axis_ref_t axis)
{
    return g_initialized && axis != MPLC_AXIS_INVALID && axis < g_axis_count;
}

static mplc_axis_runtime_t *axis_runtime(mplc_axis_ref_t axis)
{
    if (!axis_ref_valid(axis)) {
        return NULL;
    }
    return &g_axes[axis];
}

static mplc_motion_command_t *command_by_id(mplc_motion_command_id_t command_id)
{
    uint32_t i;

    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return NULL;
    }
    for (i = 0; i < MPLC_MOTION_MAX_COMMANDS; i++) {
        if (g_commands[i].id == command_id) {
            return &g_commands[i];
        }
    }
    return NULL;
}

static mplc_motion_command_t *command_alloc(void)
{
    uint32_t i;

    for (i = 0; i < MPLC_MOTION_MAX_COMMANDS; i++) {
        if (g_commands[i].id == MPLC_MOTION_COMMAND_NONE) {
            return &g_commands[i];
        }
    }
    return NULL;
}

static int32_t default_velocity(const mplc_axis_runtime_t *ax)
{
    if (ax->config.max_velocity > 0) {
        return ax->config.max_velocity;
    }
    return 1000;
}

static int32_t default_acceleration(const mplc_axis_runtime_t *ax)
{
    if (ax->config.max_acceleration > 0) {
        return ax->config.max_acceleration;
    }
    return 1000;
}

static void axis_set_error(mplc_axis_runtime_t *ax, int32_t error_id)
{
    ax->error = true;
    ax->error_id = error_id;
    ax->state = MPLC_AXIS_STATE_ERRORSTOP;
    ax->home_pending = false;
}

static void abort_command(mplc_motion_command_t *cmd, mplc_axis_runtime_t *ax)
{
    if (!cmd || cmd->state != MPLC_MOTION_CMD_BUSY) {
        return;
    }
    cmd->state = MPLC_MOTION_CMD_ABORTED;
    cmd->active = false;
    cmd->command_aborted = true;
    if (ax && ax->active_command == cmd->id) {
        ax->active_command = MPLC_MOTION_COMMAND_NONE;
        ax->active_command_type = MPLC_FB_CUSTOM;
    }
}

static int axis_require_powered(mplc_axis_ref_t axis, mplc_axis_runtime_t *ax)
{
    if (!ax) {
        return -1;
    }
    if (!ax->power_enabled) {
        axis_set_error(ax, 1);
        return -2;
    }
    if (ax->error) {
        return -3;
    }
    if (ax->stop_locked) {
        return -4;
    }
    return 0;
}

static void fill_axis_status(mplc_axis_ref_t axis, mplc_axis_status_t *status)
{
    mplc_axis_runtime_t *ax = &g_axes[axis];

    memset(status, 0, sizeof(*status));
    status->axis_state = ax->state;
    status->power_enabled = ax->power_enabled;
    status->homed = ax->homed;
    status->error = ax->error;
    status->error_id = ax->error_id;
    status->busy = ax->active_command != MPLC_MOTION_COMMAND_NONE ||
                   ax->state == MPLC_AXIS_STATE_STOPPING ||
                   ax->state == MPLC_AXIS_STATE_HOMING ||
                   ax->state == MPLC_AXIS_STATE_DISCRETE_MOTION ||
                   ax->state == MPLC_AXIS_STATE_CONTINUOUS_MOTION;
    status->active = status->busy;
    status->stop_locked = ax->stop_locked;
    mplc_motion_hal_read_actual_position(axis, &status->actual_position);
    status->command_position = ax->target_position;
    status->command_velocity = ax->command_velocity;
}

static mplc_motion_command_id_t start_command(
    mplc_axis_ref_t axis,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_t *cmd;
    mplc_motion_command_t *prev;
    mplc_motion_command_id_t command_id;

    if (!ax) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (axis_require_powered(axis, ax) != 0) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    prev = command_by_id(ax->active_command);
    if (prev && prev->state == MPLC_MOTION_CMD_BUSY) {
        abort_command(prev, ax);
    }

    cmd = command_alloc();
    if (!cmd) {
        axis_set_error(ax, 10);
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = g_next_command_id++;
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        command_id = g_next_command_id++;
    }

    cmd->id = command_id;
    cmd->axis = axis;
    cmd->type = source_fb;
    cmd->state = MPLC_MOTION_CMD_BUSY;
    cmd->active = true;

    ax->active_command = command_id;
    ax->active_command_type = source_fb;
    return command_id;
}

static void complete_command(mplc_axis_runtime_t *ax, mplc_motion_command_t *cmd)
{
    if (!ax || !cmd || cmd->state != MPLC_MOTION_CMD_BUSY) {
        return;
    }
    cmd->state = MPLC_MOTION_CMD_DONE;
    cmd->active = false;
    if (ax->active_command == cmd->id) {
        ax->active_command = MPLC_MOTION_COMMAND_NONE;
        ax->active_command_type = MPLC_FB_CUSTOM;
    }
}

int mplc_motion_init(uint32_t axis_count)
{
    if (axis_count == 0U) {
        axis_count = MPLC_CONFIGURED_AXIS_COUNT;
    }
    if (axis_count > MPLC_MOTION_MAX_AXES) {
        return -1;
    }
    if (mplc_motion_hal_init(axis_count) != 0) {
        return -2;
    }
    memset(g_axes, 0, sizeof(g_axes));
    memset(g_commands, 0, sizeof(g_commands));
    g_axis_count = axis_count;
    g_next_command_id = 1U;
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

int mplc_motion_configure_axis(mplc_axis_ref_t axis, const mplc_axis_config_t *config)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax || !config) {
        return -1;
    }
    ax->config = *config;
    return 0;
}

void mplc_motion_cycle(uint32_t dt_us)
{
    mplc_axis_ref_t axis;

    if (!g_initialized) {
        return;
    }

    mplc_motion_hal_cycle(dt_us);

    for (axis = 0; axis < g_axis_count; axis++) {
        mplc_axis_runtime_t *ax = &g_axes[axis];
        mplc_motion_command_t *cmd = command_by_id(ax->active_command);
        int32_t actual = 0;
        int32_t remaining;

        if (!ax->power_enabled || ax->state == MPLC_AXIS_STATE_DISABLED) {
            continue;
        }

        mplc_motion_hal_read_actual_position(axis, &actual);

        if (ax->state == MPLC_AXIS_STATE_HOMING && ax->home_pending) {
            ax->target_position = ax->home_position;
            ax->command_velocity = default_velocity(ax);
            ax->state = MPLC_AXIS_STATE_DISCRETE_MOTION;
            ax->home_pending = false;
        }

        if (ax->state == MPLC_AXIS_STATE_DISCRETE_MOTION) {
            remaining = ax->target_position - actual;
            if (remaining == 0) {
                if (ax->target_position == ax->home_position) {
                    ax->homed = true;
                }
                ax->state = MPLC_AXIS_STATE_STANDSTILL;
                ax->command_velocity = 0;
                mplc_motion_hal_write_command_velocity(axis, 0);
                complete_command(ax, cmd);
            } else {
                mplc_motion_hal_write_command_velocity(
                    axis, remaining > 0 ? ax->command_velocity : -ax->command_velocity);
            }
        } else if (ax->state == MPLC_AXIS_STATE_CONTINUOUS_MOTION) {
            mplc_motion_hal_write_command_velocity(axis, ax->command_velocity);
        } else if (ax->state == MPLC_AXIS_STATE_STOPPING) {
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
                mplc_motion_hal_write_command_velocity(axis, 0);
                if (cmd && cmd->type == MPLC_FB_MC_STOP) {
                    complete_command(ax, cmd);
                } else if (cmd && cmd->type == MPLC_FB_MC_HALT) {
                    complete_command(ax, cmd);
                }
            }
        }
    }
}

int mplc_motion_power(mplc_axis_ref_t axis, bool enable, mplc_power_status_t *status)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax) {
        return -1;
    }

    if (enable) {
        ax->power_enabled = true;
        mplc_motion_hal_enable(axis, true);
        ax->state = ax->error ? MPLC_AXIS_STATE_ERRORSTOP : MPLC_AXIS_STATE_STANDSTILL;
    } else {
        mplc_motion_command_t *cmd = command_by_id(ax->active_command);
        ax->power_enabled = false;
        ax->stop_locked = false;
        ax->command_velocity = 0;
        ax->state = MPLC_AXIS_STATE_DISABLED;
        abort_command(cmd, ax);
        mplc_motion_hal_enable(axis, false);
        mplc_motion_hal_write_command_velocity(axis, 0);
    }

    if (status) {
        status->status = mplc_motion_hal_read_power_status(axis);
        status->valid = true;
        status->error = ax->error;
        status->error_id = ax->error_id;
    }
    return 0;
}

int mplc_motion_reset_axis(mplc_axis_ref_t axis)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax) {
        return -1;
    }
    if (ax->state != MPLC_AXIS_STATE_ERRORSTOP) {
        return -2;
    }
    ax->error = false;
    ax->error_id = 0;
    mplc_motion_hal_reset(axis);
    ax->state = ax->power_enabled ? MPLC_AXIS_STATE_STANDSTILL : MPLC_AXIS_STATE_DISABLED;
    return 0;
}

mplc_motion_command_id_t mplc_motion_start_home(
    mplc_axis_ref_t axis,
    const mplc_home_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_id_t command_id;

    if (!ax || !request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (axis_require_powered(axis, ax) != 0) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (ax->state != MPLC_AXIS_STATE_STANDSTILL) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = start_command(axis, source_fb);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    ax->home_position = request->position;
    ax->home_pending = true;
    ax->state = MPLC_AXIS_STATE_HOMING;
    return command_id;
}

mplc_motion_command_id_t mplc_motion_start_absolute(
    mplc_axis_ref_t axis,
    const mplc_move_absolute_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_id_t command_id;
    mplc_motion_move_t move;

    if (!ax || !request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (axis_require_powered(axis, ax) != 0) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (ax->state != MPLC_AXIS_STATE_STANDSTILL &&
        ax->state != MPLC_AXIS_STATE_CONTINUOUS_MOTION) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = start_command(axis, source_fb);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    ax->target_position = request->position;
    ax->command_velocity = request->velocity > 0 ? request->velocity : default_velocity(ax);
    ax->acceleration = request->acceleration > 0 ? request->acceleration : default_acceleration(ax);
    ax->deceleration = request->deceleration > 0 ? request->deceleration : default_acceleration(ax);
    ax->state = MPLC_AXIS_STATE_DISCRETE_MOTION;

    move.target_position = request->position;
    move.velocity = ax->command_velocity;
    move.acceleration = ax->acceleration;
    move.deceleration = ax->deceleration;
    mplc_motion_hal_start_absolute(axis, &move);
    mplc_motion_hal_write_target_position(axis, request->position);
    return command_id;
}

mplc_motion_command_id_t mplc_motion_start_relative(
    mplc_axis_ref_t axis,
    const mplc_move_relative_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_move_absolute_request_t absolute;
    int32_t actual = 0;

    if (!request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    mplc_motion_hal_read_actual_position(axis, &actual);
    absolute.position = actual + request->distance;
    absolute.velocity = request->velocity;
    absolute.acceleration = request->acceleration;
    absolute.deceleration = request->deceleration;
    return mplc_motion_start_absolute(axis, &absolute, source_fb);
}

mplc_motion_command_id_t mplc_motion_start_velocity(
    mplc_axis_ref_t axis,
    const mplc_move_velocity_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_id_t command_id;
    mplc_motion_move_t move;

    if (!ax || !request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (axis_require_powered(axis, ax) != 0) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = start_command(axis, source_fb);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    ax->command_velocity = request->velocity;
    ax->target_velocity = request->velocity;
    ax->acceleration = request->acceleration > 0 ? request->acceleration : default_acceleration(ax);
    ax->deceleration = request->deceleration > 0 ? request->deceleration : default_acceleration(ax);
    ax->state = MPLC_AXIS_STATE_CONTINUOUS_MOTION;

    move.target_position = 0;
    move.velocity = request->velocity;
    move.acceleration = ax->acceleration;
    move.deceleration = ax->deceleration;
    mplc_motion_hal_start_velocity(axis, &move);
    return command_id;
}

mplc_motion_command_id_t mplc_motion_start_stop(
    mplc_axis_ref_t axis,
    const mplc_stop_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_id_t command_id;

    if (!ax || !request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (!ax->power_enabled || ax->error) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = start_command(axis, source_fb);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    ax->deceleration = request->deceleration > 0 ? request->deceleration : default_acceleration(ax);
    ax->stop_locked = true;
    ax->state = MPLC_AXIS_STATE_STOPPING;
    mplc_motion_hal_stop(axis, ax->deceleration, true);
    return command_id;
}

mplc_motion_command_id_t mplc_motion_start_halt(
    mplc_axis_ref_t axis,
    const mplc_halt_request_t *request,
    mplc_native_fb_t source_fb)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);
    mplc_motion_command_id_t command_id;

    if (!ax || !request) {
        return MPLC_MOTION_COMMAND_NONE;
    }
    if (!ax->power_enabled || ax->error) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    command_id = start_command(axis, source_fb);
    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        return MPLC_MOTION_COMMAND_NONE;
    }

    ax->deceleration = request->deceleration > 0 ? request->deceleration : default_acceleration(ax);
    ax->state = MPLC_AXIS_STATE_STOPPING;
    mplc_motion_hal_halt(axis, ax->deceleration);
    return command_id;
}

int mplc_motion_release_stop(mplc_axis_ref_t axis)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax) {
        return -1;
    }
    ax->stop_locked = false;
    mplc_motion_hal_stop(axis, 0, false);
    return 0;
}

int mplc_motion_command_status(
    mplc_motion_command_id_t command_id,
    mplc_motion_command_status_t *status)
{
    mplc_motion_command_t *cmd = command_by_id(command_id);
    mplc_axis_runtime_t *ax;

    if (!status) {
        return -1;
    }
    memset(status, 0, sizeof(*status));
    if (!cmd) {
        status->command_id = command_id;
        status->state = MPLC_MOTION_CMD_IDLE;
        return 0;
    }

    ax = axis_runtime(cmd->axis);
    status->command_id = cmd->id;
    status->state = cmd->state;
    status->command_type = cmd->type;
    status->busy = cmd->state == MPLC_MOTION_CMD_BUSY;
    status->active = cmd->active;
    status->done = cmd->state == MPLC_MOTION_CMD_DONE;
    status->command_aborted = cmd->command_aborted;
    status->error = ax ? ax->error : false;
    status->error_id = ax ? ax->error_id : cmd->error_id;

    if (cmd->type == MPLC_FB_MC_MOVE_VELOCITY && ax) {
        mplc_motion_hal_status_t hal_status;
        if (mplc_motion_hal_get_status(cmd->axis, &hal_status) == 0) {
            status->in_velocity = hal_status.in_velocity;
        }
    }
    return 0;
}

int mplc_motion_read_actual_position(
    mplc_axis_ref_t axis,
    bool enable,
    mplc_read_position_result_t *result)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax || !result) {
        return -1;
    }

    if (enable) {
        mplc_motion_hal_read_actual_position(axis, &result->position);
    } else {
        result->position = 0;
    }
    result->valid = enable && ax->power_enabled;
    result->error = ax->error;
    result->error_id = ax->error_id;
    return 0;
}

int mplc_motion_read_axis_status(
    mplc_axis_ref_t axis,
    bool enable,
    mplc_axis_status_t *status)
{
    mplc_axis_runtime_t *ax = axis_runtime(axis);

    if (!ax || !status) {
        return -1;
    }
    if (enable) {
        fill_axis_status(axis, status);
    } else {
        memset(status, 0, sizeof(*status));
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
