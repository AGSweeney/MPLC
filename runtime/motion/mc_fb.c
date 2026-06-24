/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/motion.h"
#include "mplc/vm.h"
#include <string.h>

typedef struct {
    bool    enable_prev;
    bool    execute_prev;
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
    bool    execute_prev;
    bool    done;
    bool    busy;
    bool    active;
    bool    command_aborted;
    bool    error;
    int32_t error_id;
} mplc_fb_mc_motion_t;

typedef struct {
    bool    enable_prev;
    int32_t position;
    bool    valid;
    bool    error;
    int32_t error_id;
} mplc_fb_mc_read_actual_position_t;

typedef struct {
    bool                 enable_prev;
    mplc_axis_status_t   status;
    bool                 valid;
    bool                 error;
    int32_t              error_id;
} mplc_fb_mc_read_status_t;

static bool rising_edge(bool signal, bool *prev)
{
    bool edge = signal && !*prev;
    *prev = signal;
    return edge;
}

static void mc_power_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_power_t));
}

static void mc_power_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_power_t *fb = (mplc_fb_mc_power_t *)inst;
    bool enable = params[0] != 0;
    uint16_t axis = (uint16_t)params[1];
    (void)vm;

    mplc_motion_power(axis, enable, &fb->status, &fb->valid, &fb->error, &fb->error_id);
}

static void mc_reset_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_reset_t));
}

static void mc_reset_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_reset_t *fb = (mplc_fb_mc_reset_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    (void)vm;

    mplc_motion_reset(axis, execute, &fb->done, &fb->busy, &fb->error, &fb->error_id);
}

static void mc_home_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_home_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    int32_t position = params[2];
    (void)vm;

    mplc_motion_home(axis, execute, position, &fb->done, &fb->busy, &fb->active,
                     &fb->command_aborted, &fb->error, &fb->error_id);
}

static void mc_move_absolute_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_move_absolute_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    int32_t position = params[2];
    int32_t velocity = params[3];
    int32_t acceleration = params[4];
    int32_t deceleration = params[5];
    (void)vm;

    mplc_motion_move_absolute(axis, execute, position, velocity, acceleration, deceleration,
                              &fb->done, &fb->busy, &fb->active, &fb->command_aborted,
                              &fb->error, &fb->error_id);
}

static void mc_move_relative_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_move_relative_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    int32_t distance = params[2];
    int32_t velocity = params[3];
    int32_t acceleration = params[4];
    int32_t deceleration = params[5];
    (void)vm;

    mplc_motion_move_relative(axis, execute, distance, velocity, acceleration, deceleration,
                              &fb->done, &fb->busy, &fb->active, &fb->command_aborted,
                              &fb->error, &fb->error_id);
}

static void mc_move_velocity_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_move_velocity_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = params[0] != 0;
    uint16_t axis = (uint16_t)params[1];
    int32_t velocity = params[2];
    int32_t acceleration = params[3];
    int32_t deceleration = params[4];
    (void)vm;

    mplc_motion_move_velocity(axis, execute, velocity, acceleration, deceleration,
                              &fb->done, &fb->busy, &fb->active, &fb->command_aborted,
                              &fb->error, &fb->error_id);
}

static void mc_stop_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_stop_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    int32_t deceleration = params[2];
    (void)vm;

    mplc_motion_stop(axis, execute, deceleration, &fb->done, &fb->busy, &fb->active,
                     &fb->command_aborted, &fb->error, &fb->error_id);
}

static void mc_halt_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_motion_t));
}

static void mc_halt_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_motion_t *fb = (mplc_fb_mc_motion_t *)inst;
    bool execute = rising_edge(params[0] != 0, &fb->execute_prev);
    uint16_t axis = (uint16_t)params[1];
    int32_t deceleration = params[2];
    (void)vm;

    mplc_motion_halt(axis, execute, deceleration, &fb->done, &fb->busy, &fb->active,
                     &fb->command_aborted, &fb->error, &fb->error_id);
}

static void mc_read_actual_position_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_read_actual_position_t));
}

static void mc_read_actual_position_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_read_actual_position_t *fb = (mplc_fb_mc_read_actual_position_t *)inst;
    bool enable = params[0] != 0;
    uint16_t axis = (uint16_t)params[1];
    (void)vm;

    mplc_motion_read_actual_position(axis, enable, &fb->position, &fb->valid,
                                     &fb->error, &fb->error_id);
}

static void mc_read_status_init(void *inst)
{
    memset(inst, 0, sizeof(mplc_fb_mc_read_status_t));
}

static void mc_read_status_cycle(mplc_vm_t *vm, void *inst, const int32_t *params)
{
    mplc_fb_mc_read_status_t *fb = (mplc_fb_mc_read_status_t *)inst;
    bool enable = params[0] != 0;
    uint16_t axis = (uint16_t)params[1];
    (void)vm;

    mplc_motion_read_status(axis, enable, &fb->status, &fb->valid, &fb->error, &fb->error_id);
}

static const mplc_fb_vtable_t g_mc_vtables[] = {
    { MPLC_FB_MC_POWER,               sizeof(mplc_fb_mc_power_t),               mc_power_init,               mc_power_cycle               },
    { MPLC_FB_MC_RESET,               sizeof(mplc_fb_mc_reset_t),               mc_reset_init,               mc_reset_cycle               },
    { MPLC_FB_MC_HOME,                sizeof(mplc_fb_mc_motion_t),              mc_home_init,                mc_home_cycle                },
    { MPLC_FB_MC_MOVE_ABSOLUTE,       sizeof(mplc_fb_mc_motion_t),              mc_move_absolute_init,       mc_move_absolute_cycle       },
    { MPLC_FB_MC_MOVE_RELATIVE,       sizeof(mplc_fb_mc_motion_t),              mc_move_relative_init,       mc_move_relative_cycle       },
    { MPLC_FB_MC_MOVE_VELOCITY,       sizeof(mplc_fb_mc_motion_t),              mc_move_velocity_init,       mc_move_velocity_cycle       },
    { MPLC_FB_MC_STOP,                sizeof(mplc_fb_mc_motion_t),              mc_stop_init,                mc_stop_cycle                },
    { MPLC_FB_MC_HALT,                sizeof(mplc_fb_mc_motion_t),              mc_halt_init,                mc_halt_cycle                },
    { MPLC_FB_MC_READ_ACTUAL_POSITION, sizeof(mplc_fb_mc_read_actual_position_t), mc_read_actual_position_init, mc_read_actual_position_cycle },
    { MPLC_FB_MC_READ_STATUS,         sizeof(mplc_fb_mc_read_status_t),         mc_read_status_init,         mc_read_status_cycle         },
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
