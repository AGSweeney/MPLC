/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc/vm.h"
#include <string.h>

typedef struct {
    bool    enable_prev;
    bool    status;
    bool    valid;
    bool    error;
    int32_t error_id;
} mplc_fb_mc_power_t;

typedef struct {
    bool    execute_prev;
    bool    done;
    bool    busy;
    bool    error;
    int32_t error_id;
} mplc_fb_mc_reset_t;

typedef struct {
    bool                     execute_prev;
    bool                     execute_level;
    mplc_motion_command_id_t command_id;
    bool                     done;
    bool                     busy;
    bool                     active;
    bool                     command_aborted;
    bool                     error;
    int32_t                  error_id;
} mplc_fb_mc_motion_t;

typedef struct {
    bool                     execute_prev;
    bool                     execute_level;
    mplc_motion_command_id_t command_id;
    bool                     in_velocity;
    bool                     busy;
    bool                     active;
    bool                     command_aborted;
    bool                     error;
    int32_t                  error_id;
} mplc_fb_mc_move_velocity_t;

typedef struct {
    bool    enable_prev;
    int32_t position;
    bool    valid;
    bool    error;
    int32_t error_id;
} mplc_fb_mc_read_actual_position_t;

typedef struct {
    bool               enable_prev;
    mplc_axis_status_t status;
    bool               valid;
    bool               error;
    int32_t            error_id;
} mplc_fb_mc_read_status_t;

static bool rising_edge(bool signal, bool *prev)
{
    bool edge = signal && !*prev;
    *prev = signal;
    return edge;
}

static void update_motion_outputs(
    mplc_fb_mc_motion_t *fb,
    mplc_motion_command_id_t command_id)
{
    mplc_motion_command_status_t status;

    if (command_id == MPLC_MOTION_COMMAND_NONE) {
        fb->done = false;
        fb->busy = false;
        fb->active = false;
        fb->command_aborted = false;
        return;
    }

    if (mplc_motion_command_status(command_id, &status) != 0) {
        return;
    }

    fb->done = status.done;
    fb->busy = status.busy;
    fb->active = status.active;
    fb->command_aborted = status.command_aborted;
    fb->error = status.error;
    fb->error_id = status.error_id;
}

static void mc_power_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_power_t));
}

static void mc_power_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_power_t *fb = (mplc_fb_mc_power_t *)inst;
    bool enable = params[0] != 0;
    mplc_axis_ref_t axis = (mplc_axis_ref_t)params[1];
    mplc_power_status_t status;
    (void)vm;

    if (mplc_motion_power(axis, enable, &status) == 0) {
        fb->status = status.status;
        fb->valid = status.valid;
        fb->error = status.error;
        fb->error_id = status.error_id;
    }
}

static void mc_reset_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_reset_t));
}

static void mc_reset_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_reset_t *fb = (mplc_fb_mc_reset_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    mplc_axis_ref_t axis = (mplc_axis_ref_t)params[1];
    mplc_axis_status_t axis_status;
    (void)vm;

    if (execute) {
        if (mplc_motion_reset_axis(axis) == 0) {
            fb->done = true;
        }
    } else {
        fb->done = false;
    }
    fb->busy = false;
    if (mplc_motion_read_axis_status(axis, true, &axis_status) == 0) {
        fb->error = axis_status.error;
        fb->error_id = axis_status.error_id;
    }
}

static void mc_home_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_home_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = params[0] != 0;
    mplc_axis_ref_t axis = (mplc_axis_ref_t)params[1];
    mplc_home_request_t request;
    (void)vm;

    if (rising_edge(execute, &fb->execute_prev)) {
        request.position = params[2];
        fb->command_id = mplc_motion_start_home(axis, &request, MPLC_FB_MC_HOME);
    }
    update_motion_outputs(fb, fb->command_id);
}

static void mc_move_absolute_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_move_absolute_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    mplc_move_absolute_request_t request;
    (void)vm;

    if (rising_edge(params[0] != 0, &fb->execute_prev)) {
        request.position = params[2];
        request.velocity = params[3];
        request.acceleration = params[4];
        request.deceleration = params[5];
        fb->command_id = mplc_motion_start_absolute(
            (mplc_axis_ref_t)params[1], &request, MPLC_FB_MC_MOVE_ABSOLUTE);
    }
    update_motion_outputs(fb, fb->command_id);
}

static void mc_move_relative_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_move_relative_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    mplc_move_relative_request_t request;
    (void)vm;

    if (rising_edge(params[0] != 0, &fb->execute_prev)) {
        request.distance = params[2];
        request.velocity = params[3];
        request.acceleration = params[4];
        request.deceleration = params[5];
        fb->command_id = mplc_motion_start_relative(
            (mplc_axis_ref_t)params[1], &request, MPLC_FB_MC_MOVE_RELATIVE);
    }
    update_motion_outputs(fb, fb->command_id);
}

static void mc_move_velocity_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_move_velocity_t));
}

static void mc_move_velocity_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_move_velocity_t *fb = (mplc_fb_mc_move_velocity_t *)inst;
    bool execute = params[0] != 0;
    mplc_move_velocity_request_t request;
    mplc_motion_command_status_t status;
    (void)vm;

    if (execute && !fb->execute_level) {
        request.velocity = params[2];
        request.acceleration = params[3];
        request.deceleration = params[4];
        fb->command_id = mplc_motion_start_velocity(
            (mplc_axis_ref_t)params[1], &request, MPLC_FB_MC_MOVE_VELOCITY);
    } else if (!execute && fb->execute_level) {
        fb->command_id = MPLC_MOTION_COMMAND_NONE;
    }
    fb->execute_level = execute;

    if (fb->command_id != MPLC_MOTION_COMMAND_NONE &&
        mplc_motion_command_status(fb->command_id, &status) == 0) {
        fb->in_velocity = status.in_velocity;
        fb->busy = status.busy;
        fb->active = status.active;
        fb->command_aborted = status.command_aborted;
        fb->error = status.error;
        fb->error_id = status.error_id;
    } else {
        fb->in_velocity = false;
        fb->busy = false;
        fb->active = false;
        fb->command_aborted = false;
    }
}

static void mc_stop_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_stop_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = params[0] != 0;
    mplc_axis_ref_t axis = (mplc_axis_ref_t)params[1];
    mplc_stop_request_t request;
    (void)vm;

    if (rising_edge(execute, &fb->execute_prev)) {
        request.deceleration = params[2];
        fb->command_id = mplc_motion_start_stop(axis, &request, MPLC_FB_MC_STOP);
    }
    if (!execute && fb->execute_level) {
        mplc_motion_release_stop(axis);
        fb->command_id = MPLC_MOTION_COMMAND_NONE;
    }
    fb->execute_level = execute;
    update_motion_outputs(fb, fb->command_id);
}

static void mc_halt_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_halt_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    mplc_halt_request_t request;
    (void)vm;

    if (rising_edge(params[0] != 0, &fb->execute_prev)) {
        request.deceleration = params[2];
        fb->command_id = mplc_motion_start_halt(
            (mplc_axis_ref_t)params[1], &request, MPLC_FB_MC_HALT);
    }
    update_motion_outputs(fb, fb->command_id);
}

static void mc_read_actual_position_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_read_actual_position_t));
}

static void mc_read_actual_position_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_read_actual_position_t *fb = (mplc_fb_mc_read_actual_position_t *)inst;
    mplc_read_position_result_t result;
    (void)vm;

    if (mplc_motion_read_actual_position(
            (mplc_axis_ref_t)params[1], params[0] != 0, &result) == 0) {
        fb->position = result.position;
        fb->valid = result.valid;
        fb->error = result.error;
        fb->error_id = result.error_id;
    }
}

static void mc_read_status_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_read_status_t));
}

static void mc_read_status_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_read_status_t *fb = (mplc_fb_mc_read_status_t *)inst;
    (void)vm;

    if (mplc_motion_read_axis_status(
            (mplc_axis_ref_t)params[1], params[0] != 0, &fb->status) == 0) {
        fb->valid = params[0] != 0;
        fb->error = fb->status.error;
        fb->error_id = fb->status.error_id;
    }
}

static const mplc_fb_vtable_t g_mc_vtables[] = {
    { MPLC_FB_MC_POWER,                sizeof(mplc_fb_mc_power_t),                mc_power_init,                mc_power_cycle                },
    { MPLC_FB_MC_RESET,                sizeof(mplc_fb_mc_reset_t),                mc_reset_init,                mc_reset_cycle                },
    { MPLC_FB_MC_HOME,                 sizeof(mplc_fb_mc_motion_t),               mc_home_init,                 mc_home_cycle                 },
    { MPLC_FB_MC_MOVE_ABSOLUTE,        sizeof(mplc_fb_mc_motion_t),               mc_move_absolute_init,        mc_move_absolute_cycle        },
    { MPLC_FB_MC_MOVE_RELATIVE,        sizeof(mplc_fb_mc_motion_t),               mc_move_relative_init,        mc_move_relative_cycle        },
    { MPLC_FB_MC_MOVE_VELOCITY,        sizeof(mplc_fb_mc_move_velocity_t),        mc_move_velocity_init,        mc_move_velocity_cycle        },
    { MPLC_FB_MC_STOP,                 sizeof(mplc_fb_mc_motion_t),               mc_stop_init,                 mc_stop_cycle                 },
    { MPLC_FB_MC_HALT,                 sizeof(mplc_fb_mc_motion_t),               mc_halt_init,                 mc_halt_cycle                 },
    { MPLC_FB_MC_READ_ACTUAL_POSITION, sizeof(mplc_fb_mc_read_actual_position_t), mc_read_actual_position_init, mc_read_actual_position_cycle },
    { MPLC_FB_MC_READ_STATUS,          sizeof(mplc_fb_mc_read_status_t),          mc_read_status_init,          mc_read_status_cycle          },
};

const mplc_fb_vtable_t *mplc_motion_get_vtable(mplc_native_fb_t type)
{
    uint32_t i;
    for (i = 0; i < sizeof(g_mc_vtables) / sizeof(g_mc_vtables[0]); i++) {
        if (g_mc_vtables[i].type == type) {
            return &g_mc_vtables[i];
        }
    }
    return NULL;
}

uint32_t mplc_motion_instance_size(mplc_native_fb_t type)
{
    const mplc_fb_vtable_t *vt = mplc_motion_get_vtable(type);
    return vt ? vt->instance_size : 0U;
}
