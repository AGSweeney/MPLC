/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_HAL_NETBURNER_H
#define MPLC_HAL_NETBURNER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int  mplc_netburner_plc_init(void);
void mplc_netburner_plc_shutdown(void);
int  mplc_netburner_plc_start_task(void);
void mplc_netburner_plc_stop_task(void);

int  mplc_netburner_load_embedded_package(void);
int  mplc_netburner_load_package_file(const char *path);
int  mplc_netburner_run_one_cycle(void);

void mplc_netburner_get_cycle_stats(uint64_t *cycles, uint64_t *overruns);

void mplc_hal_netburner_debug_snapshot(uint8_t *dip_mask, uint8_t *led_shadow);

struct mplc_runtime;
struct mplc_runtime *mplc_netburner_get_runtime(void);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_HAL_NETBURNER_H */
