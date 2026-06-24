/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTION_HAL_H
#define MPLC_MOTION_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MOTION_HAL_MAX_AXES 16U

int  mplc_motion_hal_init(uint32_t axis_count);
void mplc_motion_hal_shutdown(void);

void mplc_motion_hal_enable(uint16_t axis, bool enable);
void mplc_motion_hal_reset(uint16_t axis);

void mplc_motion_hal_read_actual_position(uint16_t axis, int32_t *position);
void mplc_motion_hal_write_target_position(uint16_t axis, int32_t position);
void mplc_motion_hal_write_command_velocity(uint16_t axis, int32_t velocity);

bool mplc_motion_hal_read_power_status(uint16_t axis);
bool mplc_motion_hal_read_error(uint16_t axis, int32_t *error_id);

void mplc_motion_hal_cycle(uint32_t dt_us);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTION_HAL_H */
