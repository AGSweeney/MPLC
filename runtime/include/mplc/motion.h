/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * PLCopen Motion Control Part 1 inspired interface.
 * Positions and velocities use fixed-point int32 values scaled by
 * MPLC_MOTION_POSITION_SCALE (default: 1000 = milli-units).
 */

#ifndef MPLC_MOTION_H
#define MPLC_MOTION_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_abi.h"
#include "mplc/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MOTION_POSITION_SCALE 1000
#define MPLC_MOTION_MAX_AXES       16U

/* PLCopen-inspired axis group state (Table 21). */
typedef enum {
    MPLC_AXIS_GROUP_UNKNOWN          = 0,
    MPLC_AXIS_GROUP_DISABLED         = 1,
    MPLC_AXIS_GROUP_STANDSTILL      = 2,
    MPLC_AXIS_GROUP_HOMING           = 3,
    MPLC_AXIS_GROUP_STOPPING         = 4,
    MPLC_AXIS_GROUP_DISCRETE_MOTION  = 5,
    MPLC_AXIS_GROUP_CONTINUOUS_MOTION = 6,
    MPLC_AXIS_GROUP_ERRORSTOP        = 7
} mplc_axis_group_state_t;

/* PLCopen-inspired axis state detail. */
typedef enum {
    MPLC_AXIS_STATE_UNKNOWN          = 0,
    MPLC_AXIS_STATE_DISABLED         = 1,
    MPLC_AXIS_STATE_STANDSTILL       = 2,
    MPLC_AXIS_STATE_HOMING           = 3,
    MPLC_AXIS_STATE_STOPPING         = 4,
    MPLC_AXIS_STATE_DISCRETE_MOTION  = 5,
    MPLC_AXIS_STATE_CONTINUOUS_MOTION = 6,
    MPLC_AXIS_STATE_ERRORSTOP        = 7
} mplc_axis_state_t;

typedef struct {
    mplc_axis_state_t       axis_state;
    mplc_axis_group_state_t group_state;
    bool                    power_enabled;
    bool                    homed;
    int32_t                 actual_position;
    int32_t                 command_position;
    int32_t                 command_velocity;
    bool                    error;
    int32_t                 error_id;
    bool                    busy;
    bool                    active;
} mplc_axis_status_t;

int  mplc_motion_init(uint32_t axis_count);
void mplc_motion_shutdown(void);
void mplc_motion_cycle(uint32_t dt_us);

int  mplc_motion_power(uint16_t axis, bool enable, bool *status, bool *valid,
                       bool *error, int32_t *error_id);
int  mplc_motion_reset(uint16_t axis, bool execute, bool *done, bool *busy,
                       bool *error, int32_t *error_id);
int  mplc_motion_home(uint16_t axis, bool execute, int32_t position,
                      bool *done, bool *busy, bool *active, bool *command_aborted,
                      bool *error, int32_t *error_id);
int  mplc_motion_move_absolute(uint16_t axis, bool execute, int32_t position,
                               int32_t velocity, int32_t acceleration, int32_t deceleration,
                               bool *done, bool *busy, bool *active, bool *command_aborted,
                               bool *error, int32_t *error_id);
int  mplc_motion_move_relative(uint16_t axis, bool execute, int32_t distance,
                               int32_t velocity, int32_t acceleration, int32_t deceleration,
                               bool *done, bool *busy, bool *active, bool *command_aborted,
                               bool *error, int32_t *error_id);
int  mplc_motion_move_velocity(uint16_t axis, bool execute, int32_t velocity,
                               int32_t acceleration, int32_t deceleration,
                               bool *done, bool *busy, bool *active, bool *command_aborted,
                               bool *error, int32_t *error_id);
int  mplc_motion_stop(uint16_t axis, bool execute, int32_t deceleration,
                      bool *done, bool *busy, bool *active, bool *command_aborted,
                      bool *error, int32_t *error_id);
int  mplc_motion_halt(uint16_t axis, bool execute, int32_t deceleration,
                      bool *done, bool *busy, bool *active, bool *command_aborted,
                      bool *error, int32_t *error_id);
int  mplc_motion_read_actual_position(uint16_t axis, bool enable,
                                      int32_t *position, bool *valid,
                                      bool *error, int32_t *error_id);
int  mplc_motion_read_status(uint16_t axis, bool enable,
                             mplc_axis_status_t *status,
                             bool *valid, bool *error, int32_t *error_id);

const mplc_fb_vtable_t *mplc_motion_get_vtable(mplc_native_fb_t type);
uint32_t mplc_motion_instance_size(mplc_native_fb_t type);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTION_H */
