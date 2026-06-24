/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_interp.h"
#include <stdio.h>

static int test_accel_cruise_decel(void)
{
    mplc_ms_linear_interp_t interp;
    int32_t sx;
    int32_t sy;
    uint32_t tick;
    uint32_t max_speed = 0U;
    uint32_t min_nonzero = 0xFFFFFFFFU;
    uint32_t early_sum = 0U;
    uint32_t late_sum = 0U;
    uint32_t mid_sum = 0U;
    uint32_t total_x = 0;

    mplc_ms_linear_interp_init(&interp);
    if (mplc_ms_linear_interp_begin(&interp, 0, 0, 500, 0, 5000U, 5000U, 5000U, 0U, 0U) != 0) {
        return 1;
    }

    for (tick = 0U; tick < 500000U; tick++) {
        uint32_t speed;
        bool cont = mplc_ms_linear_interp_next(&interp, &sx, &sy);
        speed = (uint32_t)(sx < 0 ? -sx : sx);
        total_x += (uint32_t)sx;
        if (speed > 0U) {
            if (speed > max_speed) {
                max_speed = speed;
            }
            if (speed < min_nonzero) {
                min_nonzero = speed;
            }
            if (tick < 200U) {
                early_sum += speed;
            } else if (total_x < 250U) {
                mid_sum += speed;
            } else if (total_x > 400U) {
                late_sum += speed;
            }
        }
        if (!cont) {
            break;
        }
    }

    if (total_x != 500U) {
        printf("total_x=%u\n", (unsigned)total_x);
        return 2;
    }
    if (max_speed == 0U) {
        return 3;
    }
    if (early_sum == 0U || mid_sum == 0U || late_sum == 0U) {
        return 4;
    }
    if (mid_sum <= early_sum) {
        printf("cruise should exceed early ramp\n");
        return 5;
    }
    if (late_sum >= mid_sum) {
        printf("decel tail should be slower than cruise\n");
        return 6;
    }
    return 0;
}

int main(void)
{
    int rc = test_accel_cruise_decel();
    if (rc != 0) {
        printf("test_linear_accel failed: %d\n", rc);
        return rc;
    }
    printf("test_linear_accel ok\n");
    return 0;
}
