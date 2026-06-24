/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * PLCopen Motion Control Part 1 inspired motion engine.
 *
 * MC_* function blocks (mc_fb.c) sit above this layer: they detect Execute
 * edges, submit commands, store command IDs, and map command status to
 * Busy/Done/Aborted/Error outputs.
 *
 * MC_Stop: controlled deceleration while Execute remains TRUE; the axis stays
 * locked against new motion commands until Execute goes FALSE.
 *
 * MC_Halt: controlled deceleration that may be superseded by another valid
 * motion command; the previous command is reported as CommandAborted.
 */

#ifndef MPLC_MOTION_H
#define MPLC_MOTION_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_abi.h"
#include "mplc/stdlib.h"
#include "mplc_motion_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MPLC_AXIS_STATE_UNKNOWN           = 0,
    MPLC_AXIS_STATE_DISABLED          = 1,
    MPLC_AXIS_STATE_STANDSTILL        = 2,
    MPLC_AXIS_STATE_HOMING            = 3,
    MPLC_AXIS_STATE_STOPPING          = 4,
    MPLC_AXIS_STATE_DISCRETE_MOTION   = 5,
    MPLC_AXIS_STATE_CONTINUOUS_MOTION = 6,
    MPLC_AXIS_STATE_ERRORSTOP         = 7
} mplc_axis_state_t;

typedef enum {
    MPLC_MOTION_CMD_IDLE    = 0,
    MPLC_MOTION_CMD_BUSY    = 1,
    MPLC_MOTION_CMD_DONE    = 2,
    MPLC_MOTION_CMD_ABORTED = 3,
    MPLC_MOTION_CMD_ERROR   = 4
} mplc_motion_command_state_t;

typedef struct {
    mplc_axis_state_t axis_state;
    bool              power_enabled;
    bool              homed;
    int32_t           actual_position;
    int32_t           command_position;
    int32_t           command_velocity;
    bool              error;
    int32_t           error_id;
    bool              busy;
    bool              active;
    bool              stop_locked;
} mplc_axis_status_t;

typedef struct {
    mplc_motion_command_id_t    command_id;
    mplc_motion_command_state_t state;
    mplc_native_fb_t            command_type;
    bool                        busy;
    bool                        active;
    bool                        done;
    bool                        command_aborted;
    bool                        in_velocity;
    bool                        error;
    int32_t                     error_id;
} mplc_motion_command_status_t;

typedef struct {
    bool    status;
    bool    valid;
    bool    error;
    int32_t error_id;
} mplc_power_status_t;

typedef struct {
    int32_t position;
    bool    valid;
    bool    error;
    int32_t error_id;
} mplc_read_position_result_t;

int  mplc_motion_init(uint32_t axis_count);
void mplc_motion_shutdown(void);
void mplc_motion_cycle(uint32_t dt_us);

int  mplc_motion_configure_axis(mplc_axis_ref_t axis, const mplc_axis_config_t *config);

int  mplc_motion_power(mplc_axis_ref_t axis, bool enable, mplc_power_status_t *status);
int  mplc_motion_reset_axis(mplc_axis_ref_t axis);

mplc_motion_command_id_t mplc_motion_start_home(
    mplc_axis_ref_t axis,
    const mplc_home_request_t *request,
    mplc_native_fb_t source_fb);

mplc_motion_command_id_t mplc_motion_start_absolute(
    mplc_axis_ref_t axis,
    const mplc_move_absolute_request_t *request,
    mplc_native_fb_t source_fb);

mplc_motion_command_id_t mplc_motion_start_relative(
    mplc_axis_ref_t axis,
    const mplc_move_relative_request_t *request,
    mplc_native_fb_t source_fb);

mplc_motion_command_id_t mplc_motion_start_velocity(
    mplc_axis_ref_t axis,
    const mplc_move_velocity_request_t *request,
    mplc_native_fb_t source_fb);

mplc_motion_command_id_t mplc_motion_start_stop(
    mplc_axis_ref_t axis,
    const mplc_stop_request_t *request,
    mplc_native_fb_t source_fb);

mplc_motion_command_id_t mplc_motion_start_halt(
    mplc_axis_ref_t axis,
    const mplc_halt_request_t *request,
    mplc_native_fb_t source_fb);

int mplc_motion_release_stop(
    mplc_axis_ref_t axis,
    mplc_motion_command_id_t owner_command_id);

int mplc_motion_command_ack(mplc_motion_command_id_t command_id);

int mplc_motion_command_status(
    mplc_motion_command_id_t command_id,
    mplc_motion_command_status_t *status);

int mplc_motion_read_actual_position(
    mplc_axis_ref_t axis,
    bool enable,
    mplc_read_position_result_t *result);

int mplc_motion_read_axis_status(
    mplc_axis_ref_t axis,
    bool enable,
    mplc_axis_status_t *status);

const mplc_fb_vtable_t *mplc_motion_get_vtable(mplc_native_fb_t type);
uint32_t mplc_motion_instance_size(mplc_native_fb_t type);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTION_H */
