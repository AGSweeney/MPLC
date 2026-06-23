#include "mplc_hal.h"
#include "hal_netburner_config.h"
#include <string.h>

#ifdef MPLC_NETBURNER_NNDK
/*
 * NNDK 3.x headers — available inside an NBEclipse MOD5441X project.
 * Include paths come from the NetBurner tools installation.
 */
#include <predef.h>
#include <stdio.h>
#include <ucos_ii.h>
#else
#include <stdio.h>
#endif

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
    { 0, 0, 0, 0 }, { 1, 0, 0, 0 }, { 2, 0, 0, 0 }, { 3, 0, 0, 0 },
    { 4, 0, 0, 0 }, { 5, 0, 0, 0 }, { 6, 0, 0, 0 }, { 7, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
};

static const mplc_nb_pin_map_t g_digital_out_map[MPLC_NB_MAX_DIGITAL_OUT] = {
    { 0, 0, 0, 0 }, { 1, 0, 0, 0 }, { 2, 0, 0, 0 }, { 3, 0, 0, 0 },
    { 4, 0, 0, 0 }, { 5, 0, 0, 0 }, { 6, 0, 0, 0 }, { 7, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
    { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
};

#ifdef MPLC_NETBURNER_NNDK
static bool nb_read_digital_pin(int16_t pin)
{
    if (pin < 0) {
        return false;
    }
    /* Replace with NNDK GPIO read, e.g. GpioRead(pin) or carrier-specific API */
    return false;
}

static void nb_write_digital_pin(int16_t pin, bool value)
{
    if (pin < 0) {
        return;
    }
    (void)value;
    /* Replace with NNDK GPIO write */
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
    /* OSTimeGet() returns ticks; scale using OS_TICKS_PER_SEC from NNDK */
    return ((uint64_t)OSTimeGet() * 1000000ULL) / 1000ULL;
}

static void nb_sleep_until(uint64_t deadline_us)
{
    uint64_t now = nb_time_us();
    if (deadline_us > now) {
        uint64_t delta_us = deadline_us - now;
        uint32_t ticks = (uint32_t)((delta_us * 1000ULL) / 1000000ULL);
        if (ticks == 0U) {
            ticks = 1U;
        }
        OSTimeDly(ticks);
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
    *value = nb_read_digital_pin(g_digital_in_map[channel].nb_pin);
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
    nb_write_digital_pin(g_digital_out_map[channel].nb_pin, value);
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
