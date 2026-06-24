/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * MPLC starter application for NetBurner MOD54415 (MOD5441X platform).
 *
 * Initializes NNDK, starts the MPLC cyclic scan task, then idles.
 * Serial console output appears on the module debug UART.
 */

#include <init.h>
#include <nbrtos.h>
#include <startnet.h>
#include <system.h>
#include <stdio.h>

#include "mplc_hal_netburner.h"
#include "hal_netburner_config.h"
#include "fs_main.h"

const char *AppName = "MPLC_MOD54415";

void UserMain(void *pd)
{
    (void)pd;

    init();
    EnableSystemDiagnostics();
    fs_main();
    StartHttp();
    WaitForActiveNetwork(TICKS_PER_SECOND * 5);

    printf(
        "Application: %s\r\n"
        "NNDK Revision: %s\r\n"
        "MPLC MOD54415 runtime starting...\r\n",
        AppName, GetReleaseTag());

    if (mplc_netburner_plc_start_task() != 0) {
        printf("MPLC: failed to start PLC task\r\n");
    } else {
        printf("MPLC: PLC task started (priority %u)\r\n", (unsigned)MPLC_NB_TASK_PRIORITY);
        printf("MPLC: upload endpoint http://<device_ip>/mplc_update.html\r\n");
    }

    while (1) {
        OSTimeDly(TICKS_PER_SECOND);
    }
}
