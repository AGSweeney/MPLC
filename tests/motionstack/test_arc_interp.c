/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_interp.h"
#include <stdio.h>

static int run_arc(bool clockwise, int32_t *out_x, int32_t *out_y)
{
    mplc_ms_arc_interp_t interp;
    int32_t sx;
    int32_t sy;
    int32_t total_x = 0;
    int32_t total_y = 0;
    uint32_t guard;

    mplc_ms_arc_interp_init(&interp);
    if (mplc_ms_arc_interp_begin(&interp,
                                 0, 0, 10,
                                 10, 0,
                                 0, 10,
                                 clockwise, 5000U, 5000U, 5000U, 0U, 0U) != 0) {
        return -1;
    }

    for (guard = 0U; guard < 500000U; guard++) {
        bool cont = mplc_ms_arc_interp_next(&interp, &sx, &sy);
        total_x += sx;
        total_y += sy;
        if (!cont) {
            break;
        }
    }

    *out_x = total_x;
    *out_y = total_y;
    if (!mplc_ms_arc_interp_complete(&interp)) {
        return -2;
    }
    return 0;
}

static int test_ccw_quarter_arc(void)
{
    int32_t x;
    int32_t y;
    int rc = run_arc(false, &x, &y);
    if (rc != 0) {
        return 1;
    }
    if (x < -11 || x > -9 || y < 9 || y > 11) {
        printf("ccw end delta got %d/%d expected about -10/10\n", (int)x, (int)y);
        return 2;
    }
    return 0;
}

static int test_cw_quarter_arc(void)
{
    mplc_ms_arc_interp_t interp;
    int32_t sx;
    int32_t sy;
    int32_t total_x = 0;
    int32_t total_y = 0;
    uint32_t guard;

    mplc_ms_arc_interp_init(&interp);
    if (mplc_ms_arc_interp_begin(&interp,
                                 0, 0, 10,
                                 10, 0,
                                 0, -10,
                                 true, 5000U, 5000U, 5000U, 0U, 0U) != 0) {
        return 10;
    }

    for (guard = 0U; guard < 500000U; guard++) {
        bool cont = mplc_ms_arc_interp_next(&interp, &sx, &sy);
        total_x += sx;
        total_y += sy;
        if (!cont) {
            break;
        }
    }

    if (total_x < -11 || total_x > -9 || total_y < -11 || total_y > -9) {
        printf("cw end delta got %d/%d expected about -10/-10\n", (int)total_x, (int)total_y);
        return 11;
    }
    return 0;
}

int main(void)
{
    int rc = test_ccw_quarter_arc();
    if (rc != 0) {
        printf("test_arc_interp ccw failed: %d\n", rc);
        return rc;
    }
    rc = test_cw_quarter_arc();
    if (rc != 0) {
        printf("test_arc_interp cw failed: %d\n", rc);
        return rc;
    }
    printf("test_arc_interp ok\n");
    return 0;
}
