/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_interp.h"
#include <stdio.h>

static int test_diagonal_line(void)
{
    mplc_ms_linear_interp_t interp;
    int32_t sx;
    int32_t sy;
    int32_t total_x = 0;
    int32_t total_y = 0;
    uint32_t guard;

    mplc_ms_linear_interp_init(&interp);
    if (mplc_ms_linear_interp_begin(&interp, 0, 0, 100, 100, 5000U, 5000U, 5000U, 0U, 0U) != 0) {
        return 1;
    }

    for (guard = 0U; guard < 200000U; guard++) {
        bool cont = mplc_ms_linear_interp_next(&interp, &sx, &sy);
        total_x += sx;
        total_y += sy;
        if (!cont) {
            break;
        }
    }

    if (total_x != 100 || total_y != 100) {
        printf("expected 100/100 got %d/%d\n", (int)total_x, (int)total_y);
        return 2;
    }
    if (!mplc_ms_linear_interp_complete(&interp)) {
        return 3;
    }
    return 0;
}

int main(void)
{
    int rc = test_diagonal_line();
    if (rc != 0) {
        printf("test_linear_interp failed: %d\n", rc);
        return rc;
    }
    printf("test_linear_interp ok\n");
    return 0;
}
