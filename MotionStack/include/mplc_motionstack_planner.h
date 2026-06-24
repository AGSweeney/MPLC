/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_PLANNER_H
#define MPLC_MOTIONSTACK_PLANNER_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_motionstack_types.h"
#include "mplc_motionstack_interp.h"
#include "mplc_motionstack_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MS_DEFAULT_JUNCTION_DEVIATION_STEPS 1U

typedef struct {
    mplc_ms_segment_t       queue[MPLC_MOTIONSTACK_QUEUE_DEPTH];
    uint8_t                   head;
    uint8_t                   tail;
    uint8_t                   count;
    bool                      defer_start;
    bool                      driving;
    bool                      use_grbl_planner;
    bool                      stop_at_queue_end;
    uint16_t                  sample_rate_hz;
    uint32_t                  vel_max;
    uint32_t                  accel_max;
    uint32_t                  junction_deviation_steps;
    uint32_t                  junction_dv_max;
    int32_t                   pos_x;
    int32_t                   pos_y;
    mplc_ms_linear_interp_t   linear;
    mplc_ms_arc_interp_t      arc;
    mplc_ms_segment_type_t    active_type;
    const mplc_motionstack_port_t *port;
    void                     *port_ctx;
} mplc_ms_planner_t;

void mplc_ms_planner_init(mplc_ms_planner_t *planner, uint16_t sample_rate_hz);
void mplc_ms_planner_set_port(mplc_ms_planner_t *planner,
                              const mplc_motionstack_port_t *port, void *ctx);
void mplc_ms_planner_set_limits(mplc_ms_planner_t *planner,
                                uint32_t vel_max, uint32_t accel_max);
void mplc_ms_planner_set_grbl_mode(mplc_ms_planner_t *planner, bool enabled);
void mplc_ms_planner_set_junction_deviation(mplc_ms_planner_t *planner,
                                            uint32_t deviation_steps);
void mplc_ms_planner_set_junction_dv_max(mplc_ms_planner_t *planner, uint32_t dv_max);
void mplc_ms_planner_set_stop_at_queue_end(mplc_ms_planner_t *planner, bool stop);

int  mplc_ms_planner_queue_linear(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg);
int  mplc_ms_planner_queue_arc(mplc_ms_planner_t *planner, const mplc_ms_segment_t *seg);
void mplc_ms_planner_set_defer_start(mplc_ms_planner_t *planner, bool defer);
int  mplc_ms_planner_start_deferred(mplc_ms_planner_t *planner);
void mplc_ms_planner_recalculate(mplc_ms_planner_t *planner);
int  mplc_ms_planner_tick(mplc_ms_planner_t *planner);
bool mplc_ms_planner_is_busy(const mplc_ms_planner_t *planner);
uint8_t mplc_ms_planner_queue_count(const mplc_ms_planner_t *planner);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_PLANNER_H */
