/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Motion HAL contract.
 *
 * Production backends (ClearCore, NetBurner pulse engines) should prefer the
 * command-level entry points (start_absolute, stop, get_status). The
 * scan-level write_* helpers remain for the default simulator only.
 */

#ifndef MPLC_MOTION_HAL_H
#define MPLC_MOTION_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_motion_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MOTION_HAL_MAX_AXES MPLC_MOTION_MAX_AXES

typedef struct {
    int32_t actual_position;
    int32_t command_velocity;
    bool    in_velocity;
    bool    error;
    int32_t error_id;
} mplc_motion_hal_status_t;

int  mplc_motion_hal_init(uint32_t axis_count);
void mplc_motion_hal_shutdown(void);

void mplc_motion_hal_enable(mplc_axis_ref_t axis, bool enable);
void mplc_motion_hal_reset(mplc_axis_ref_t axis);

void mplc_motion_hal_read_actual_position(mplc_axis_ref_t axis, int32_t *position);

bool mplc_motion_hal_read_power_status(mplc_axis_ref_t axis);
bool mplc_motion_hal_read_error(mplc_axis_ref_t axis, int32_t *error_id);

/* Command-level interface (preferred for hardware backends). */
int mplc_motion_hal_start_absolute(mplc_axis_ref_t axis, const mplc_motion_move_t *move);
int mplc_motion_hal_start_velocity(mplc_axis_ref_t axis, const mplc_motion_move_t *move);
int mplc_motion_hal_get_status(mplc_axis_ref_t axis, mplc_motion_hal_status_t *status);
int mplc_motion_hal_stop(mplc_axis_ref_t axis, int32_t deceleration, bool lock_axis);
int mplc_motion_hal_halt(mplc_axis_ref_t axis, int32_t deceleration);

/* Scan-level simulator helpers (default HAL only). */
void mplc_motion_hal_write_target_position(mplc_axis_ref_t axis, int32_t position);
void mplc_motion_hal_write_command_velocity(mplc_axis_ref_t axis, int32_t velocity);

void mplc_motion_hal_cycle(uint32_t dt_us);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTION_HAL_H */
