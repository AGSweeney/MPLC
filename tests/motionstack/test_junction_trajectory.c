/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_planner.h"
#include "port_sim.h"
#include <stdio.h>

static uint32_t measure_min_speed_near_corner(mplc_ms_planner_t *planner,
                                              mplc_ms_port_sim_ctx_t *sim,
                                              int32_t corner_x, int32_t corner_y,
                                              uint32_t window)
{
    int32_t prev_x = 0;
    int32_t prev_y = 0;
    uint32_t min_speed = 0xFFFFFFFFU;
    uint32_t guard;
    bool have_prev = false;

    mplc_ms_port_sim_reset(sim);
    for (guard = 0U; guard < 500000U && mplc_ms_planner_is_busy(planner); guard++) {
        int32_t x = mplc_ms_port_sim_axis_steps(sim, 0);
        int32_t y = mplc_ms_port_sim_axis_steps(sim, 1);
        int32_t dx;
        int32_t dy;
        uint32_t speed;

        (void)mplc_ms_planner_tick(planner);

        if (have_prev) {
            dx = x - prev_x;
            dy = y - prev_y;
            speed = (uint32_t)(dx < 0 ? -dx : dx) + (uint32_t)(dy < 0 ? -dy : dy);
            speed *= planner->sample_rate_hz;
            if (x >= corner_x - (int32_t)window && x <= corner_x + (int32_t)window &&
                y >= corner_y - (int32_t)window && y <= corner_y + (int32_t)window) {
                if (speed < min_speed) {
                    min_speed = speed;
                }
            }
        }
        prev_x = x;
        prev_y = y;
        have_prev = true;
    }
    return min_speed;
}

static int test_corner_decel(void)
{
    mplc_ms_planner_t planner;
    mplc_ms_port_sim_ctx_t sim;
    mplc_ms_segment_t seg_a;
    mplc_ms_segment_t seg_b;
    uint32_t min_near_corner;
    uint32_t cruise_min = 0xFFFFFFFFU;
    uint32_t guard;

    mplc_ms_planner_init(&planner, 5000U);
    mplc_ms_planner_set_limits(&planner, 5000U, 5000U);
    mplc_ms_port_sim_init(&sim, 100U);
    mplc_ms_planner_set_port(&planner, mplc_ms_port_sim_get(), &sim);

    memset(&seg_a, 0, sizeof(seg_a));
    seg_a.type = MPLC_MS_SEGMENT_LINEAR;
    seg_a.start_x = 0;
    seg_a.start_y = 0;
    seg_a.end_x = 200;
    seg_a.end_y = 0;
    seg_a.nominal_speed = 5000U;

    memset(&seg_b, 0, sizeof(seg_b));
    seg_b.type = MPLC_MS_SEGMENT_LINEAR;
    seg_b.start_x = 200;
    seg_b.start_y = 0;
    seg_b.end_x = 200;
    seg_b.end_y = 150;
    seg_b.nominal_speed = 5000U;

    if (mplc_ms_planner_queue_linear(&planner, &seg_a) != 0 ||
        mplc_ms_planner_queue_linear(&planner, &seg_b) != 0) {
        return 1;
    }

    for (guard = 0U; guard < 500000U && mplc_ms_planner_is_busy(&planner); guard++) {
        int32_t x = mplc_ms_port_sim_axis_steps(&sim, 0);
        int32_t dx;
        uint32_t speed;
        (void)mplc_ms_planner_tick(&planner);
        if (x > 40 && x < 120) {
            dx = mplc_ms_port_sim_axis_steps(&sim, 0) - x;
            (void)dx;
            speed = 5000U;
            if (speed < cruise_min) {
                cruise_min = speed;
            }
        }
    }

    mplc_ms_planner_init(&planner, 5000U);
    mplc_ms_planner_set_limits(&planner, 5000U, 5000U);
    mplc_ms_planner_set_port(&planner, mplc_ms_port_sim_get(), &sim);
    if (mplc_ms_planner_queue_linear(&planner, &seg_a) != 0 ||
        mplc_ms_planner_queue_linear(&planner, &seg_b) != 0) {
        return 2;
    }

    min_near_corner = measure_min_speed_near_corner(&planner, &sim, 200, 0, 15U);
    if (min_near_corner == 0xFFFFFFFFU) {
        return 3;
    }
    if (min_near_corner >= 4500U) {
        printf("min speed near corner %u expected decel below cruise\n",
               (unsigned)min_near_corner);
        return 4;
    }

    if (mplc_ms_port_sim_axis_steps(&sim, 0) != 200 ||
        mplc_ms_port_sim_axis_steps(&sim, 1) != 150) {
        return 5;
    }
    return 0;
}

int main(void)
{
    int rc = test_corner_decel();
    if (rc != 0) {
        printf("test_junction_trajectory failed: %d\n", rc);
        return rc;
    }
    printf("test_junction_trajectory ok\n");
    return 0;
}
