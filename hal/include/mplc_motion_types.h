/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Shared motion types used by the runtime engine and motion HAL.
 */

#ifndef MPLC_MOTION_TYPES_H
#define MPLC_MOTION_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t mplc_axis_ref_t;
typedef uint16_t mplc_axis_group_ref_t;

#define MPLC_AXIS_INVALID ((mplc_axis_ref_t)UINT16_MAX)

#ifndef MPLC_CONFIGURED_AXIS_COUNT
#define MPLC_CONFIGURED_AXIS_COUNT 2U
#endif

#define MPLC_MOTION_MAX_AXES 16U

/*
 * Fixed-point scales (independent dimensions):
 *   Position:     engineering units × MPLC_MOTION_POSITION_SCALE
 *   Velocity:     engineering units/second × MPLC_MOTION_VELOCITY_SCALE
 *   Acceleration: engineering units/second² × MPLC_MOTION_ACCELERATION_SCALE
 */
#define MPLC_MOTION_POSITION_SCALE      1000
#define MPLC_MOTION_VELOCITY_SCALE      1000
#define MPLC_MOTION_ACCELERATION_SCALE  1000

typedef uint32_t mplc_motion_command_id_t;

#define MPLC_MOTION_COMMAND_NONE ((mplc_motion_command_id_t)0U)

typedef struct {
    int32_t counts_per_unit_num;
    int32_t counts_per_unit_den;
    int32_t max_velocity;
    int32_t max_acceleration;
} mplc_axis_config_t;

typedef struct {
    int32_t target_position;
    int32_t velocity;
    int32_t acceleration;
    int32_t deceleration;
} mplc_motion_move_t;

typedef struct {
    int32_t position;
} mplc_home_request_t;

typedef struct {
    int32_t position;
    int32_t velocity;
    int32_t acceleration;
    int32_t deceleration;
} mplc_move_absolute_request_t;

typedef struct {
    int32_t distance;
    int32_t velocity;
    int32_t acceleration;
    int32_t deceleration;
} mplc_move_relative_request_t;

typedef struct {
    int32_t velocity;
    int32_t acceleration;
    int32_t deceleration;
} mplc_move_velocity_request_t;

typedef struct {
    int32_t deceleration;
} mplc_stop_request_t;

typedef mplc_stop_request_t mplc_halt_request_t;

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTION_TYPES_H */
