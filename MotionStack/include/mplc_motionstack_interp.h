/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_INTERP_H
#define MPLC_MOTIONSTACK_INTERP_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_motionstack_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t  start_x;
    int32_t  start_y;
    int32_t  end_x;
    int32_t  end_y;
    int32_t  current_x;
    int32_t  current_y;
    int32_t  last_x;
    int32_t  last_y;
    int64_t  current_x_qx;
    int64_t  current_y_qx;
    int32_t  step_inc_x_qx;
    int32_t  step_inc_y_qx;
    int32_t  dir_x_q15;
    int32_t  dir_y_q15;
    uint32_t total_steps;
    uint32_t steps_remaining;
    uint32_t velocity_max;
    uint32_t accel_max;
    uint32_t entry_speed;
    uint32_t exit_speed;
    uint16_t sample_rate_hz;
    bool     active;
} mplc_ms_linear_interp_t;

typedef struct {
    bool     active;
    int32_t  center_x;
    int32_t  center_y;
    int32_t  radius;
    int32_t  start_angle_qx;
    int32_t  end_angle_qx;
    int32_t  current_angle_qx;
    int32_t  angle_span_qx;
    int32_t  angle_inc_qx;
    int32_t  last_x;
    int32_t  last_y;
    int32_t  end_x;
    int32_t  end_y;
    bool     clockwise;
    uint32_t total_steps;
    uint32_t steps_remaining;
    uint32_t velocity_max;
    uint32_t accel_max;
    uint32_t entry_speed;
    uint32_t exit_speed;
    uint16_t sample_rate_hz;
} mplc_ms_arc_interp_t;

void mplc_ms_linear_interp_init(mplc_ms_linear_interp_t *interp);
int  mplc_ms_linear_interp_begin(mplc_ms_linear_interp_t *interp,
                                 int32_t start_x, int32_t start_y,
                                 int32_t end_x, int32_t end_y,
                                 uint32_t velocity_max, uint32_t accel_max,
                                 uint16_t sample_rate_hz,
                                 uint32_t entry_speed, uint32_t exit_speed);
bool mplc_ms_linear_interp_next(mplc_ms_linear_interp_t *interp,
                                int32_t *steps_x, int32_t *steps_y);
bool mplc_ms_linear_interp_complete(const mplc_ms_linear_interp_t *interp);
void mplc_ms_linear_interp_set_exit_speed(mplc_ms_linear_interp_t *interp, uint32_t exit_speed);

void mplc_ms_arc_interp_init(mplc_ms_arc_interp_t *interp);
int  mplc_ms_arc_interp_begin(mplc_ms_arc_interp_t *interp,
                              int32_t center_x, int32_t center_y, int32_t radius,
                              int32_t start_x, int32_t start_y,
                              int32_t end_x, int32_t end_y,
                              bool clockwise, uint32_t velocity_max,
                              uint32_t accel_max, uint16_t sample_rate_hz,
                              uint32_t entry_speed, uint32_t exit_speed);
bool mplc_ms_arc_interp_next(mplc_ms_arc_interp_t *interp,
                             int32_t *steps_x, int32_t *steps_y);
bool mplc_ms_arc_interp_complete(const mplc_ms_arc_interp_t *interp);
void mplc_ms_arc_interp_set_exit_speed(mplc_ms_arc_interp_t *interp, uint32_t exit_speed);

uint32_t mplc_ms_interp_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
uint32_t mplc_ms_u64_sqrt(uint64_t v);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_INTERP_H */
