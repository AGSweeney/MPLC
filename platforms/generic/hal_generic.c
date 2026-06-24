/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_hal.h"
#include <string.h>
#include <stdlib.h>

#define GENERIC_MAX_IO 256U

static mplc_platform_config_t g_cfg;
static bool g_digital_in[GENERIC_MAX_IO];
static bool g_digital_out[GENERIC_MAX_IO];
static float g_analog_in[GENERIC_MAX_IO];
static float g_analog_out[GENERIC_MAX_IO];
static uint8_t g_retain[4096];
static mplc_hal_stats_t g_stats;
static uint64_t g_time_us;

int mplc_hal_init(const mplc_platform_config_t *cfg)
{
    if (cfg) {
        g_cfg = *cfg;
    }
    memset(g_digital_in, 0, sizeof(g_digital_in));
    memset(g_digital_out, 0, sizeof(g_digital_out));
    memset(g_retain, 0, sizeof(g_retain));
    memset(&g_stats, 0, sizeof(g_stats));
    g_time_us = 0U;
    return 0;
}

void mplc_hal_shutdown(void) {}

void mplc_hal_digital_read(uint16_t channel, bool *value)
{
    if (!value || channel >= GENERIC_MAX_IO) {
        return;
    }
    *value = g_digital_in[channel];
}

void mplc_hal_digital_write(uint16_t channel, bool value)
{
    if (channel >= GENERIC_MAX_IO) {
        return;
    }
    g_digital_out[channel] = value;
}

void mplc_hal_analog_read(uint16_t channel, float *value)
{
    if (!value || channel >= GENERIC_MAX_IO) {
        return;
    }
    *value = g_analog_in[channel];
}

void mplc_hal_analog_write(uint16_t channel, float value)
{
    if (channel >= GENERIC_MAX_IO) {
        return;
    }
    g_analog_out[channel] = value;
}

uint64_t mplc_hal_time_us(void)
{
    return g_time_us;
}

void mplc_hal_sleep_until(uint64_t deadline_us)
{
    g_time_us = deadline_us;
}

int mplc_hal_retain_read(uint32_t offset, void *buffer, size_t length)
{
    if (!buffer || offset + length > sizeof(g_retain)) {
        return -1;
    }
    memcpy(buffer, g_retain + offset, length);
    return 0;
}

int mplc_hal_retain_write(uint32_t offset, const void *buffer, size_t length)
{
    if (!buffer || offset + length > sizeof(g_retain)) {
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

/* Test helpers */
void mplc_hal_generic_set_digital_in(uint16_t channel, bool value)
{
    if (channel < GENERIC_MAX_IO) {
        g_digital_in[channel] = value;
    }
}

bool mplc_hal_generic_get_digital_out(uint16_t channel)
{
    return channel < GENERIC_MAX_IO ? g_digital_out[channel] : false;
}

void mplc_hal_generic_advance_time_us(uint64_t delta)
{
    g_time_us += delta;
}
