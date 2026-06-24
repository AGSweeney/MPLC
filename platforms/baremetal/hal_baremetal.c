/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_hal.h"
#include <string.h>
#include <stdint.h>

/* RP2040-oriented bare-metal stub: timer driven by cycle counter.
 * Integrate with actual hardware timer/GPIO in board-specific layer. */

static mplc_platform_config_t g_cfg;
static mplc_hal_stats_t g_stats;
static uint64_t g_tick_us;
static uint8_t g_retain[8192];

static volatile uint32_t g_gpio_in;
static volatile uint32_t g_gpio_out;

int mplc_hal_init(const mplc_platform_config_t *cfg)
{
    if (cfg) {
        g_cfg = *cfg;
    }
    g_tick_us = 0U;
    g_gpio_in = 0U;
    g_gpio_out = 0U;
    memset(g_retain, 0, sizeof(g_retain));
    memset(&g_stats, 0, sizeof(g_stats));
    return 0;
}

void mplc_hal_shutdown(void) {}

void mplc_hal_digital_read(uint16_t channel, bool *value)
{
    if (!value) {
        return;
    }
    *value = (g_gpio_in >> channel) & 1U;
}

void mplc_hal_digital_write(uint16_t channel, bool value)
{
    if (value) {
        g_gpio_out |= (1UL << channel);
    } else {
        g_gpio_out &= ~(1UL << channel);
    }
}

void mplc_hal_analog_read(uint16_t channel, float *value)
{
    (void)channel;
    if (value) {
        *value = 0.0f;
    }
}

void mplc_hal_analog_write(uint16_t channel, float value)
{
    (void)channel;
    (void)value;
}

uint64_t mplc_hal_time_us(void)
{
    return g_tick_us;
}

void mplc_hal_sleep_until(uint64_t deadline_us)
{
    while (g_tick_us < deadline_us) {
        g_tick_us += 1000ULL;
    }
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

/* Called from board startup or SysTick ISR */
void mplc_hal_baremetal_tick_us(uint64_t delta)
{
    g_tick_us += delta;
}

uint32_t mplc_hal_baremetal_get_gpio_out(void)
{
    return g_gpio_out;
}

void mplc_hal_baremetal_set_gpio_in(uint32_t value)
{
    g_gpio_in = value;
}
