/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_H
#define MPLC_MOTIONSTACK_H

#include "mplc_motionstack_types.h"
#include "mplc_motionstack_endian.h"
#include "mplc_motionstack_q15.h"
#include "mplc_motionstack_units.h"
#include "mplc_motionstack_profile.h"
#include "mplc_motionstack_interp.h"
#include "mplc_motionstack_planner.h"
#include "mplc_motionstack_port.h"
#include "mplc_motionstack_drive.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t sample_rate_hz;
    uint32_t vel_max;
    uint32_t accel_max;
    mplc_ms_mechanics_t mechanics[MPLC_MOTIONSTACK_AXIS_COUNT];
    mplc_ms_drive_config_t drive_cfg[MPLC_MOTIONSTACK_AXIS_COUNT];
} mplc_motionstack_config_t;

typedef struct {
    mplc_motionstack_config_t     cfg;
    mplc_ms_planner_t             planner;
    mplc_ms_profile_t             axis_profile[MPLC_MOTIONSTACK_AXIS_COUNT];
    mplc_ms_drive_ops_t           drive_ops[MPLC_MOTIONSTACK_AXIS_COUNT];
    uint8_t                       drive_ctx[MPLC_MOTIONSTACK_AXIS_COUNT][64];
    mplc_ms_encoder_state_t       encoder[MPLC_MOTIONSTACK_AXIS_COUNT];
    bool                          initialized;
} mplc_motionstack_t;

int  mplc_motionstack_init(mplc_motionstack_t *stack, const mplc_motionstack_config_t *cfg);
void mplc_motionstack_shutdown(mplc_motionstack_t *stack);
void mplc_motionstack_set_port(mplc_motionstack_t *stack,
                               const mplc_motionstack_port_t *port, void *ctx);
int  mplc_motionstack_tick(mplc_motionstack_t *stack);
int  mplc_motionstack_queue_linear(mplc_motionstack_t *stack, const mplc_ms_segment_t *seg);
int  mplc_motionstack_queue_arc(mplc_motionstack_t *stack, const mplc_ms_segment_t *seg);
int  mplc_motionstack_start_deferred(mplc_motionstack_t *stack);
bool mplc_motionstack_is_busy(const mplc_motionstack_t *stack);

extern const mplc_ms_drive_ops_t mplc_ms_drive_generic_stepper_ops;
extern const mplc_ms_drive_ops_t mplc_ms_drive_clearpath_ops;

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_H */
