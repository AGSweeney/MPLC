/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_hal.h"
#include "hal_netburner_config.h"
#include <string.h>

#ifdef MPLC_NETBURNER_NNDK
#include <predef.h>
#include <stdio.h>
#include <nbrtos.h>
#include <constants.h>
#include <bsp_devboard.h>
#include "simple_ad.h"
#else
#include <stdio.h>
#endif

extern "C" {

static mplc_platform_config_t g_cfg;
static mplc_hal_stats_t g_stats;
static uint64_t g_time_us;

#ifdef MPLC_NETBURNER_NNDK
static uint8_t g_retain[MPLC_NB_RETAIN_SIZE];
#else
static uint8_t g_retain[MPLC_NB_RETAIN_SIZE];
static bool g_digital_in[MPLC_NB_MAX_DIGITAL_IN];
static bool g_digital_out[MPLC_NB_MAX_DIGITAL_OUT];
static float g_analog_in[MPLC_NB_MAX_ANALOG_IN];
static float g_analog_out[MPLC_NB_MAX_ANALOG_OUT];
#endif

static const mplc_nb_pin_map_t g_digital_in_map[MPLC_NB_MAX_DIGITAL_IN] = {
    { -1, 1, 7, 0 }, { -1, 1, 6, 0 }, { -1, 1, 5, 0 }, { -1, 1, 3, 0 },
    { -1, 1, 4, 0 }, { -1, 1, 1, 0 }, { -1, 1, 0, 0 }, { -1, 1, 2, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
};

static const mplc_nb_pin_map_t g_digital_out_map[MPLC_NB_MAX_DIGITAL_OUT] = {
    { 15, 0, 0, 0 }, { 16, 0, 0, 0 }, { 31, 0, 0, 0 }, { 23, 0, 0, 0 },
    { 37, 0, 0, 0 }, { 19, 0, 0, 0 }, { 20, 0, 0, 0 }, { 24, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
};

#ifdef MPLC_NETBURNER_NNDK
/*
 * MOD-DEV-70 DIP switches via Mod5441xFactoryApp ReadSwitch() / SimpleAD.
 */
static bool g_led_gpio_init = false;
static uint8_t g_do_shadow = 0U;
static uint8_t g_dip_cache = 0U;
static tick_t g_dip_cache_tick = 0;

#if MPLC_NB_IO_DEBUG
static uint8_t g_dip_log_last_mask = 0xFFU;
static uint8_t g_led_log_last_shadow = 0xFFU;
static int8_t g_led_write_last[MPLC_NB_MAX_DIGITAL_OUT];
static bool g_io_debug_ready = false;
#endif

static bool nb_dip_mask_bit_is_on(uint8_t mask, uint8_t channel)
{
    bool bit_set = (mask & (uint8_t)(1U << channel)) != 0U;
#if MPLC_NB_DIP_BIT_SET_IS_ON
    return bit_set;
#else
    return !bit_set;
#endif
}

static void nb_log_dips_if_changed(void)
{
#if MPLC_NB_IO_DEBUG
    /* DIP input path is confirmed; keep serial focused on output path debug. */
    g_dip_log_last_mask = g_dip_cache;
#endif
}

static void nb_log_led_shadow_if_changed(uint8_t do_bits)
{
#if MPLC_NB_IO_DEBUG
    if (g_io_debug_ready && g_led_log_last_shadow == do_bits) {
        return;
    }
    iprintf("MPLC LED shadow -> 0x%02X\r\n", do_bits);
    g_led_log_last_shadow = do_bits;
#endif
}

static void nb_init_mod_dev70_leds(void)
{
    if (!g_led_gpio_init) {
        /* Same init sequence as LEAP LeapPort / MOD5441xFactoryApp */
        LED1.function(PinGpioOutputFn);
        LED2.function(PinGpioOutputFn);
        LED3.function(PinGpioOutputFn);
        LED4.function(PinGpioOutputFn);
        LED5.function(PinGpioOutputFn);
        LED6.function(PinGpioOutputFn);
        LED7.function(PinGpioOutputFn);
        LED8.function(PinGpioOutputFn);
        LED1 = 1;
        LED2 = 1;
        LED3 = 1;
        LED4 = 1;
        LED5 = 1;
        LED6 = 1;
        LED7 = 1;
        LED8 = 1;
        g_led_gpio_init = true;
    }
}

/*
 * Apply 8-bit output image to MOD-DEV-70 LEDs (active-low, bit0 = LED1).
 * Matches LEAP LeapPort leap_board_apply_outputs().
 */
static void nb_apply_led_shadow(uint8_t do_bits)
{
    nb_init_mod_dev70_leds();
    LED1 = (do_bits & 0x01U) ? 0 : 1;
    LED2 = (do_bits & 0x02U) ? 0 : 1;
    LED3 = (do_bits & 0x04U) ? 0 : 1;
    LED4 = (do_bits & 0x08U) ? 0 : 1;
    LED5 = (do_bits & 0x10U) ? 0 : 1;
    LED6 = (do_bits & 0x20U) ? 0 : 1;
    LED7 = (do_bits & 0x40U) ? 0 : 1;
    LED8 = (do_bits & 0x80U) ? 0 : 1;
    nb_log_led_shadow_if_changed(do_bits);
}

static void nb_refresh_mod_dev70_dips(void)
{
    g_dip_cache = ReadSwitch();
    g_dip_cache_tick = TimeTick;
    nb_log_dips_if_changed();
}

static bool nb_read_mod_dev70_dip(uint8_t channel)
{
    if (channel >= MPLC_NB_MOD_DEV70_DIP_COUNT) {
        return false;
    }

#if !MPLC_NB_ENABLE_BOARD_DIP
    /* LEAP-style: no ADC DIP reads; mirror digital outputs for bench tests */
    return (g_do_shadow & (uint8_t)(1U << channel)) != 0U;
#else
    /*
     * Refresh on channel 0 every scan so DIP updates do not depend on RTOS tick cadence.
     * Remaining channels reuse the same sampled mask for consistency within a scan.
     */
    if (channel == 0U || g_dip_cache_tick != TimeTick) {
        nb_refresh_mod_dev70_dips();
    }

    return nb_dip_mask_bit_is_on(g_dip_cache, channel);
#endif
}

static void nb_write_mod_dev70_led(uint8_t channel, bool on)
{
    uint8_t bit;

    if (channel >= MPLC_NB_MOD_DEV70_LED_COUNT) {
        return;
    }

    bit = (uint8_t)(1U << channel);
    if (on) {
        g_do_shadow |= bit;
    } else {
        g_do_shadow &= (uint8_t)~bit;
    }
    nb_apply_led_shadow(g_do_shadow);
}

static float nb_read_adc(uint8_t channel)
{
    (void)channel;
    /* Replace with ReadADC(channel) or equivalent MOD5441X ADC API */
    return 0.0f;
}

static void nb_write_dac(uint8_t channel, float value)
{
    (void)channel;
    (void)value;
    /* Replace with WriteDAC(channel, value) or equivalent */
}

static uint64_t nb_time_us(void)
{
    return ((uint64_t)TimeTick * 1000000ULL) / (uint64_t)TICKS_PER_SECOND;
}

static void nb_sleep_until(uint64_t deadline_us)
{
    uint64_t now = nb_time_us();
    if (deadline_us > now) {
        uint64_t delta_us = deadline_us - now;
        uint64_t ticks = (delta_us * (uint64_t)TICKS_PER_SECOND + 999999ULL) / 1000000ULL;
        if (ticks == 0U) {
            ticks = 1U;
        }
        OSTimeDly((uint32_t)ticks);
    }
}
#endif

int mplc_hal_init(const mplc_platform_config_t *cfg)
{
    if (cfg) {
        g_cfg = *cfg;
    } else {
        g_cfg.default_cycle_us = MPLC_NB_DEFAULT_CYCLE_US;
        g_cfg.io_channel_count = MPLC_NB_MAX_DIGITAL_IN;
        g_cfg.retain_path = NULL;
    }

    memset(&g_stats, 0, sizeof(g_stats));
    g_time_us = 0U;

#ifndef MPLC_NETBURNER_NNDK
    memset(g_digital_in, 0, sizeof(g_digital_in));
    memset(g_digital_out, 0, sizeof(g_digital_out));
    memset(g_analog_in, 0, sizeof(g_analog_in));
    memset(g_analog_out, 0, sizeof(g_analog_out));
#endif

    memset(g_retain, 0, sizeof(g_retain));

#ifdef MPLC_NETBURNER_NNDK
    nb_init_mod_dev70_leds();
#if MPLC_NB_IO_DEBUG
    memset(g_led_write_last, -1, sizeof(g_led_write_last));
    iprintf("MPLC IO: MOD-DEV-70 on-change debug logging enabled\r\n");
    iprintf("MPLC LED self-test: all ON (watch carrier LEDs)\r\n");
    nb_apply_led_shadow(0xFFU);
    OSTimeDly(TICKS_PER_SECOND / 4U);
    g_do_shadow = 0U;
    nb_apply_led_shadow(0U);
    iprintf("MPLC LED self-test: all OFF\r\n");
#if MPLC_NB_ENABLE_BOARD_DIP
    nb_refresh_mod_dev70_dips();
#endif
    g_io_debug_ready = true;
#endif
#endif

    return 0;
}

void mplc_hal_shutdown(void)
{
}

void mplc_hal_digital_read(uint16_t channel, bool *value)
{
    if (!value || channel >= MPLC_NB_MAX_DIGITAL_IN) {
        return;
    }

#ifdef MPLC_NETBURNER_NNDK
    if (channel < MPLC_NB_MOD_DEV70_DIP_COUNT) {
        *value = nb_read_mod_dev70_dip((uint8_t)channel);
    } else {
        *value = false;
    }
#else
    *value = g_digital_in[channel];
#endif
}

void mplc_hal_digital_write(uint16_t channel, bool value)
{
    if (channel >= MPLC_NB_MAX_DIGITAL_OUT) {
        return;
    }

#ifdef MPLC_NETBURNER_NNDK
#if MPLC_NB_IO_DEBUG
    if (g_led_write_last[channel] != (int8_t)(value ? 1 : 0)) {
        iprintf("MPLC OUT write ch=%u val=%u\r\n",
                (unsigned)channel, (unsigned)(value ? 1U : 0U));
        g_led_write_last[channel] = (int8_t)(value ? 1 : 0);
    }
#endif
    if (channel < MPLC_NB_MOD_DEV70_LED_COUNT) {
        nb_write_mod_dev70_led((uint8_t)channel, value);
    } else if (g_digital_out_map[channel].nb_pin >= 0) {
        /* extra GPIO outputs — extend here if needed */
    } else {
#if MPLC_NB_IO_DEBUG
        iprintf("MPLC OUT ch=%u has no mapped GPIO\r\n", (unsigned)channel);
#endif
    }
#else
    g_digital_out[channel] = value;
#endif
}

void mplc_hal_analog_read(uint16_t channel, float *value)
{
    if (!value || channel >= MPLC_NB_MAX_ANALOG_IN) {
        return;
    }

#ifdef MPLC_NETBURNER_NNDK
    *value = nb_read_adc((uint8_t)channel);
#else
    *value = g_analog_in[channel];
#endif
}

void mplc_hal_analog_write(uint16_t channel, float value)
{
    if (channel >= MPLC_NB_MAX_ANALOG_OUT) {
        return;
    }

#ifdef MPLC_NETBURNER_NNDK
    nb_write_dac((uint8_t)channel, value);
#else
    g_analog_out[channel] = value;
#endif
}

uint64_t mplc_hal_time_us(void)
{
#ifdef MPLC_NETBURNER_NNDK
    return nb_time_us();
#else
    return g_time_us;
#endif
}

void mplc_hal_sleep_until(uint64_t deadline_us)
{
#ifdef MPLC_NETBURNER_NNDK
    nb_sleep_until(deadline_us);
#else
    if (deadline_us > g_time_us) {
        g_time_us = deadline_us;
    }
#endif
}

int mplc_hal_retain_read(uint32_t offset, void *buffer, size_t length)
{
    if (!buffer || offset + length > sizeof(g_retain)) {
        return -1;
    }

#ifdef MPLC_NETBURNER_NNDK
    /*
     * Production: read from User Params flash at MPLC_NB_RETAIN_BASE + offset.
     * See MOD5441X Platform Reference memory map.
     */
#endif

    memcpy(buffer, g_retain + offset, length);
    return 0;
}

int mplc_hal_retain_write(uint32_t offset, const void *buffer, size_t length)
{
    if (!buffer || offset + length > sizeof(g_retain)) {
        return -1;
    }

    memcpy(g_retain + offset, buffer, length);

#ifdef MPLC_NETBURNER_NNDK
    /* Production: persist to User Params flash sector */
#endif

    return 0;
}

void mplc_hal_get_stats(mplc_hal_stats_t *stats)
{
    if (stats) {
        *stats = g_stats;
    }
}

void mplc_hal_netburner_debug_snapshot(uint8_t *dip_mask, uint8_t *led_shadow)
{
#ifdef MPLC_NETBURNER_NNDK
    if (dip_mask) {
        *dip_mask = g_dip_cache;
    }
    if (led_shadow) {
        *led_shadow = g_do_shadow;
    }
#else
    uint8_t in_mask = 0U;
    uint8_t out_mask = 0U;
    uint8_t i;
    for (i = 0; i < 8U; i++) {
        if (g_digital_in[i]) {
            in_mask |= (uint8_t)(1U << i);
        }
        if (g_digital_out[i]) {
            out_mask |= (uint8_t)(1U << i);
        }
    }
    if (dip_mask) {
        *dip_mask = in_mask;
    }
    if (led_shadow) {
        *led_shadow = out_mask;
    }
#endif
}

#ifndef MPLC_NETBURNER_NNDK
void mplc_hal_netburner_host_set_digital_in(uint16_t channel, bool value)
{
    if (channel < MPLC_NB_MAX_DIGITAL_IN) {
        g_digital_in[channel] = value;
    }
}

bool mplc_hal_netburner_host_get_digital_out(uint16_t channel)
{
    return channel < MPLC_NB_MAX_DIGITAL_OUT ? g_digital_out[channel] : false;
}

void mplc_hal_netburner_host_advance_time_us(uint64_t delta)
{
    g_time_us += delta;
}
#endif

} /* extern "C" */
