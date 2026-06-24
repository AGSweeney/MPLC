/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_hal.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/mman.h>
#include <sched.h>
#endif

static mplc_platform_config_t g_cfg;
static mplc_hal_stats_t g_stats;
static uint8_t *g_retain;
static size_t g_retain_size;

int mplc_hal_init(const mplc_platform_config_t *cfg)
{
    if (cfg) {
        g_cfg = *cfg;
    }
    g_retain_size = 65536U;
    g_retain = (uint8_t *)calloc(1, g_retain_size);
    if (!g_retain) {
        return -1;
    }
    if (cfg && cfg->retain_path) {
        FILE *f = fopen(cfg->retain_path, "rb");
        if (f) {
            (void)fread(g_retain, 1, g_retain_size, f);
            fclose(f);
        }
    }
    memset(&g_stats, 0, sizeof(g_stats));
    return 0;
}

void mplc_hal_shutdown(void)
{
    if (g_cfg.retain_path && g_retain) {
        FILE *f = fopen(g_cfg.retain_path, "wb");
        if (f) {
            (void)fwrite(g_retain, 1, g_retain_size, f);
            fclose(f);
        }
    }
    free(g_retain);
    g_retain = NULL;
}

void mplc_hal_digital_read(uint16_t channel, bool *value)
{
    if (value) {
        *value = false;
    }
    (void)channel;
}

void mplc_hal_digital_write(uint16_t channel, bool value)
{
    (void)channel;
    (void)value;
}

void mplc_hal_analog_read(uint16_t channel, float *value)
{
    if (value) {
        *value = 0.0f;
    }
    (void)channel;
}

void mplc_hal_analog_write(uint16_t channel, float value)
{
    (void)channel;
    (void)value;
}

uint64_t mplc_hal_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

void mplc_hal_sleep_until(uint64_t deadline_us)
{
    while (mplc_hal_time_us() < deadline_us) {
        usleep(100);
    }
}

int mplc_hal_retain_read(uint32_t offset, void *buffer, size_t length)
{
    if (!g_retain || !buffer || offset + length > g_retain_size) {
        return -1;
    }
    memcpy(buffer, g_retain + offset, length);
    return 0;
}

int mplc_hal_retain_write(uint32_t offset, const void *buffer, size_t length)
{
    if (!g_retain || !buffer || offset + length > g_retain_size) {
        return -1;
    }
    memcpy(g_retain + offset, buffer, length);
    return 0;
}

void mplc_hal_get_stats(mplc_hal_stats_t *stats)
{
    if (stats) {
        *stats = g_stats;
    }
}
