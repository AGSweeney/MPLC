/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc_hal_netburner.h"
#include "mplc_hal.h"
#include "hal_netburner_config.h"
#include "mplc_nb_fs.h"
#include <stdlib.h>
#include <string.h>

extern const uint8_t g_embedded_mplc[];
extern const uint32_t g_embedded_mplc_size;

static mplc_runtime_t *g_rt = NULL;
static uint64_t g_plc_cycles = 0U;
static uint8_t *g_file_package = NULL;
static size_t g_file_package_size = 0U;

int mplc_netburner_plc_init(void)
{
    mplc_platform_config_t hal_cfg;
    mplc_runtime_config_t rt_cfg;

    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.default_cycle_us = MPLC_NB_DEFAULT_CYCLE_US;
    hal_cfg.io_channel_count = MPLC_NB_MAX_DIGITAL_IN;

    if (mplc_hal_init(&hal_cfg) != 0) {
        return -1;
    }

    memset(&rt_cfg, 0, sizeof(rt_cfg));
    rt_cfg.data_size = 8192U;
    rt_cfg.fb_arena_size = 8192U;

    if (mplc_runtime_create(&g_rt, &rt_cfg) != 0) {
        return -2;
    }

    return 0;
}

void mplc_netburner_plc_shutdown(void)
{
    mplc_runtime_destroy(g_rt);
    g_rt = NULL;
    free(g_file_package);
    g_file_package = NULL;
    g_file_package_size = 0U;
    mplc_hal_shutdown();
}

int mplc_netburner_load_embedded_package(void)
{
    free(g_file_package);
    g_file_package = NULL;
    g_file_package_size = 0U;

    if (!g_rt || g_embedded_mplc_size == 0U) {
        return -1;
    }
    return mplc_runtime_load_package(g_rt, g_embedded_mplc, g_embedded_mplc_size);
}

int mplc_netburner_load_package_file(const char *path)
{
    uint8_t *buf;
    size_t size;
    int rc;

    if (!g_rt || !path || path[0] == '\0') {
        return -1;
    }

    rc = mplc_nb_fs_read_all(path, &buf, &size, (long)MPLC_NB_PACKAGE_FILE_MAX_BYTES);
    if (rc != 0) {
        return rc;
    }

    rc = mplc_runtime_load_package(g_rt, buf, size);
    if (rc != 0) {
        free(buf);
        return -10;
    }

    free(g_file_package);
    g_file_package = buf;
    g_file_package_size = size;
    return 0;
}

int mplc_netburner_run_one_cycle(void)
{
    int rc;
    if (!g_rt) {
        return -1;
    }
    rc = mplc_runtime_run_cycle(g_rt);
    if (rc == 0) {
        g_plc_cycles++;
    }
    return rc;
}

void mplc_netburner_get_cycle_stats(uint64_t *cycles, uint64_t *overruns)
{
    mplc_hal_stats_t stats;
    if (cycles) {
        *cycles = g_plc_cycles;
    }
    mplc_hal_get_stats(&stats);
    if (overruns) {
        *overruns = stats.overrun_count;
    }
}

mplc_runtime_t *mplc_netburner_get_runtime(void)
{
    return g_rt;
}
