/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc_hal.h"
#include "mplc_hal_baremetal.h"
#include <string.h>

extern const uint8_t g_embedded_mplc[];
extern const uint32_t g_embedded_mplc_size;

int main(void)
{
    mplc_platform_config_t hal_cfg = { .default_cycle_us = 1000U };
    mplc_runtime_config_t rt_cfg;
    mplc_runtime_t *rt = NULL;

    mplc_hal_init(&hal_cfg);

    memset(&rt_cfg, 0, sizeof(rt_cfg));
    rt_cfg.data_size = 4096U;
    rt_cfg.fb_arena_size = 4096U;

    if (mplc_runtime_create(&rt, &rt_cfg) == 0 &&
        g_embedded_mplc_size > 0U &&
        mplc_runtime_load_package(rt, g_embedded_mplc, g_embedded_mplc_size) == 0) {
        while (1) {
            mplc_hal_baremetal_tick_us(1000U);
            (void)mplc_runtime_run_cycle(rt);
        }
    }

    while (1) {
    }
    return 0;
}
