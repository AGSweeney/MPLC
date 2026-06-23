#ifndef MPLC_HAL_H
#define MPLC_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t default_cycle_us;
    uint32_t io_channel_count;
    const char *retain_path;
} mplc_platform_config_t;

typedef struct {
    uint64_t cycle_count;
    uint64_t overrun_count;
    uint64_t last_cycle_us;
    uint64_t max_cycle_us;
} mplc_hal_stats_t;

int  mplc_hal_init(const mplc_platform_config_t *cfg);
void mplc_hal_shutdown(void);

void mplc_hal_digital_read(uint16_t channel, bool *value);
void mplc_hal_digital_write(uint16_t channel, bool value);
void mplc_hal_analog_read(uint16_t channel, float *value);
void mplc_hal_analog_write(uint16_t channel, float value);

uint64_t mplc_hal_time_us(void);
void mplc_hal_sleep_until(uint64_t deadline_us);

int  mplc_hal_retain_read(uint32_t offset, void *buffer, size_t length);
int  mplc_hal_retain_write(uint32_t offset, const void *buffer, size_t length);

void mplc_hal_get_stats(mplc_hal_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_HAL_H */
