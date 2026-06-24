/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_hal_netburner.h"
#include "mplc_hal_netburner_host.h"
#include "mplc/runtime.h"
#include "mplc_hal.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    if (mplc_netburner_plc_start_task() != 0) {
        printf("NetBurner host test: init failed (no embedded .mplc — expected with stub)\n");
        return 0;
    }

    mplc_hal_netburner_host_set_digital_in(0, true);
    mplc_hal_netburner_host_advance_time_us(10000U);

    if (mplc_netburner_plc_run_cycles(1U) != 0) {
        printf("NetBurner host test: cycle failed\n");
        return 1;
    }

    mplc_netburner_plc_stop_task();
    printf("NetBurner host test OK\n");
    return 0;
}
