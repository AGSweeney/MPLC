/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_planner.h"
#include <stdio.h>

static int test_l_path_junction_speeds(void)
{
    mplc_ms_planner_t planner;
    mplc_ms_segment_t seg_a;
    mplc_ms_segment_t seg_b;

    mplc_ms_planner_init(&planner, 5000U);
    mplc_ms_planner_set_limits(&planner, 5000U, 5000U);
    mplc_ms_planner_set_junction_deviation(&planner, 1U);

    memset(&seg_a, 0, sizeof(seg_a));
    seg_a.type = MPLC_MS_SEGMENT_LINEAR;
    seg_a.start_x = 0;
    seg_a.start_y = 0;
    seg_a.end_x = 100;
    seg_a.end_y = 0;
    seg_a.nominal_speed = 5000U;

    memset(&seg_b, 0, sizeof(seg_b));
    seg_b.type = MPLC_MS_SEGMENT_LINEAR;
    seg_b.start_x = 100;
    seg_b.start_y = 0;
    seg_b.end_x = 100;
    seg_b.end_y = 80;
    seg_b.nominal_speed = 5000U;

    if (mplc_ms_planner_queue_linear(&planner, &seg_a) != 0 ||
        mplc_ms_planner_queue_linear(&planner, &seg_b) != 0) {
        return 1;
    }

    if (planner.queue[planner.head].entry_speed != 0U) {
        return 2;
    }
    if (planner.queue[planner.head].exit_speed >= 5000U) {
        return 3;
    }

    {
        uint8_t second = (uint8_t)((planner.head + 1U) % MPLC_MOTIONSTACK_QUEUE_DEPTH);
        if (planner.queue[second].entry_speed != planner.queue[planner.head].exit_speed) {
            return 4;
        }
        if (planner.queue[second].entry_speed >= 5000U) {
            return 5;
        }
    }
    return 0;
}

static int test_sharp_corner_slower_than_straight(void)
{
    mplc_ms_planner_t planner90;
    mplc_ms_planner_t planner_straight;
    mplc_ms_segment_t a;
    mplc_ms_segment_t b90;
    mplc_ms_segment_t b_straight;
    uint32_t speed90;
    uint32_t speed_straight;

    mplc_ms_planner_init(&planner90, 5000U);
    mplc_ms_planner_init(&planner_straight, 5000U);
    mplc_ms_planner_set_limits(&planner90, 5000U, 5000U);
    mplc_ms_planner_set_limits(&planner_straight, 5000U, 5000U);

    memset(&a, 0, sizeof(a));
    a.type = MPLC_MS_SEGMENT_LINEAR;
    a.start_x = 0;
    a.start_y = 0;
    a.end_x = 500;
    a.end_y = 0;
    a.nominal_speed = 5000U;

    b90 = a;
    b90.start_x = 500;
    b90.start_y = 0;
    b90.end_x = 500;
    b90.end_y = 100;

    b_straight = a;
    b_straight.start_x = 500;
    b_straight.start_y = 0;
    b_straight.end_x = 1000;
    b_straight.end_y = 0;

    if (mplc_ms_planner_queue_linear(&planner90, &a) != 0 ||
        mplc_ms_planner_queue_linear(&planner90, &b90) != 0 ||
        mplc_ms_planner_queue_linear(&planner_straight, &a) != 0 ||
        mplc_ms_planner_queue_linear(&planner_straight, &b_straight) != 0) {
        return 10;
    }

    speed90 = planner90.queue[planner90.head].exit_speed;
    speed_straight = planner_straight.queue[planner_straight.head].exit_speed;
    if (speed90 == 0U) {
        return 11;
    }
    if (speed_straight <= speed90) {
        printf("straight exit %u should exceed 90 corner exit %u\n",
               (unsigned)speed_straight, (unsigned)speed90);
        return 12;
    }
    return 0;
}

int main(void)
{
    int rc = test_l_path_junction_speeds();
    if (rc != 0) {
        printf("test_junction_planner L-path failed: %d\n", rc);
        return rc;
    }
    rc = test_sharp_corner_slower_than_straight();
    if (rc != 0) {
        printf("test_junction_planner sharp corner failed: %d\n", rc);
        return rc;
    }
    printf("test_junction_planner ok\n");
    return 0;
}
