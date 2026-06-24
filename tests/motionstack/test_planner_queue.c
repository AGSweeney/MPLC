/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack.h"
#include "port_sim.h"
#include <stdio.h>

static int run_line(mplc_motionstack_t *stack, mplc_ms_port_sim_ctx_t *sim,
                    int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    mplc_ms_segment_t seg;
    uint32_t guard;

    mplc_ms_port_sim_reset(sim);
    memset(&seg, 0, sizeof(seg));
    seg.type = MPLC_MS_SEGMENT_LINEAR;
    seg.start_x = x0;
    seg.start_y = y0;
    seg.end_x = x1;
    seg.end_y = y1;
    seg.nominal_speed = 5000U;

    if (mplc_motionstack_queue_linear(stack, &seg) != 0) {
        return 1;
    }

    for (guard = 0U; guard < 200000U && mplc_motionstack_is_busy(stack); guard++) {
        if (mplc_motionstack_tick(stack) != 0) {
            return 2;
        }
    }

    if (mplc_motionstack_is_busy(stack)) {
        return 3;
    }
    if (mplc_ms_port_sim_axis_steps(sim, MPLC_MOTIONSTACK_AXIS_X) != (x1 - x0)) {
        return 4;
    }
    if (mplc_ms_port_sim_axis_steps(sim, MPLC_MOTIONSTACK_AXIS_Y) != (y1 - y0)) {
        return 5;
    }
    return 0;
}

static int test_queue_and_defer(void)
{
    mplc_motionstack_t stack;
    mplc_motionstack_config_t cfg;
    mplc_ms_port_sim_ctx_t sim;
    mplc_ms_segment_t seg;
    int rc;

    memset(&cfg, 0, sizeof(cfg));
    cfg.sample_rate_hz = 5000U;
    cfg.vel_max = 5000U;
    cfg.accel_max = 5000U;

    mplc_ms_port_sim_init(&sim, 100U);
    if (mplc_motionstack_init(&stack, &cfg) != 0) {
        return 10;
    }
    mplc_motionstack_set_port(&stack, mplc_ms_port_sim_get(), &sim);

    rc = run_line(&stack, &sim, 0, 0, 50, 0);
    if (rc != 0) {
        mplc_motionstack_shutdown(&stack);
        return rc + 100;
    }

    mplc_ms_planner_set_defer_start(&stack.planner, true);
    mplc_ms_port_sim_reset(&sim);
    memset(&seg, 0, sizeof(seg));
    seg.type = MPLC_MS_SEGMENT_LINEAR;
    seg.end_x = 10;
    seg.end_y = 10;
    seg.nominal_speed = 5000U;
    if (mplc_motionstack_queue_linear(&stack, &seg) != 0) {
        mplc_motionstack_shutdown(&stack);
        return 120;
    }
    if (!mplc_motionstack_is_busy(&stack)) {
        mplc_motionstack_shutdown(&stack);
        return 121;
    }
    if (mplc_ms_port_sim_axis_steps(&sim, 0) != 0) {
        mplc_motionstack_shutdown(&stack);
        return 122;
    }
    if (mplc_motionstack_start_deferred(&stack) != 0) {
        mplc_motionstack_shutdown(&stack);
        return 123;
    }
    while (mplc_motionstack_is_busy(&stack)) {
        (void)mplc_motionstack_tick(&stack);
    }

    mplc_motionstack_shutdown(&stack);
    return 0;
}

int main(void)
{
    int rc = test_queue_and_defer();
    if (rc != 0) {
        printf("test_planner_queue failed: %d\n", rc);
        return rc;
    }
    printf("test_planner_queue ok\n");
    return 0;
}
