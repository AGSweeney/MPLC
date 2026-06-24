/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * uC/OS-II PLC cyclic task for NetBurner MOD54415.
 *
 * NNDK build: define MPLC_NETBURNER_NNDK and compile with NBEclipse.
 * Host build: MPLC_NETBURNER_HOST_TEST exercises init/cycle without uC/OS.
 */

#include "mplc_hal_netburner.h"
#include "hal_netburner_config.h"
#include "mplc_nb_fs.h"
#include <stdio.h>
#include <string.h>

#ifdef MPLC_NETBURNER_NNDK
#include <predef.h>
#include <nbrtos.h>

#define MPLC_NB_TASK_STACK_WORDS (MPLC_NB_TASK_STACK_SIZE / sizeof(uint32_t))

static uint32_t g_mplc_task_stack[MPLC_NB_TASK_STACK_WORDS];

static void MplcTask(void *p_arg)
{
    int load_rc = -1;
    tick_t last_cycle_err_tick = 0;
    (void)p_arg;

    if (mplc_netburner_plc_init() != 0) {
        printf("MPLC: failed to initialize runtime\r\n");
        while (1) {
            OSTimeDly(100);
        }
    }

#if MPLC_NB_PACKAGE_FILE_BOOT
    {
        long pkg_size = 0L;
        int probe_rc = mplc_nb_fs_probe_file(MPLC_NB_PACKAGE_FILE_PATH, &pkg_size);
        if (probe_rc == 0) {
            printf("MPLC: found package file '%s' (%ld bytes)\r\n",
                   MPLC_NB_PACKAGE_FILE_PATH, pkg_size);
        } else {
            printf("MPLC: package file '%s' not on filesystem (probe %d)\r\n",
                   MPLC_NB_PACKAGE_FILE_PATH, probe_rc);
        }
    }
    load_rc = mplc_netburner_load_package_file(MPLC_NB_PACKAGE_FILE_PATH);
    if (load_rc == 0) {
        printf("MPLC: loaded package file '%s'\r\n", MPLC_NB_PACKAGE_FILE_PATH);
    } else {
        printf("MPLC: package file load failed (%d), using embedded package\r\n", load_rc);
    }
#endif
    if (load_rc != 0 && mplc_netburner_load_embedded_package() != 0) {
        printf("MPLC: failed to load embedded package\r\n");
        while (1) {
            OSTimeDly(100);
        }
    }

    printf("MPLC: runtime started\r\n");

    while (1) {
        int rc = mplc_netburner_run_one_cycle();

        if (rc != 0) {
            if ((tick_t)(TimeTick - last_cycle_err_tick) >= (tick_t)TICKS_PER_SECOND) {
                iprintf("MPLC: cycle error rc=%d\r\n", rc);
                last_cycle_err_tick = TimeTick;
            }
            OSTimeDly(1);
        }
    }
}

extern "C" int mplc_netburner_plc_start_task(void)
{
    uint8_t err = OSTaskCreatewName(
        MplcTask,
        NULL,
        &g_mplc_task_stack[MPLC_NB_TASK_STACK_WORDS],
        g_mplc_task_stack,
        MPLC_NB_TASK_PRIORITY,
        "MplcPLC");

    return err == OS_NO_ERR ? 0 : -1;
}

extern "C" void mplc_netburner_plc_stop_task(void)
{
    mplc_netburner_plc_shutdown();
}

#else

extern "C" int mplc_netburner_plc_start_task(void)
{
    if (mplc_netburner_plc_init() != 0 ||
        mplc_netburner_load_embedded_package() != 0) {
        return -1;
    }
    return 0;
}

extern "C" void mplc_netburner_plc_stop_task(void)
{
    mplc_netburner_plc_shutdown();
}

extern "C" int mplc_netburner_plc_run_cycles(uint32_t count)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        if (mplc_netburner_run_one_cycle() != 0) {
            return -1;
        }
    }
    return 0;
}

#endif
