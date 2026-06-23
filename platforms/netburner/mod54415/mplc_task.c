/*
 * uC/OS-II PLC cyclic task for NetBurner MOD54415.
 *
 * NNDK build: define MPLC_NETBURNER_NNDK and compile user_main.cpp separately.
 * Host build: MPLC_NETBURNER_HOST_TEST exercises init/cycle without uC/OS.
 */

#include "mplc_hal_netburner.h"
#include "hal_netburner_config.h"
#include <stdio.h>
#include <string.h>

#ifdef MPLC_NETBURNER_NNDK
#include <predef.h>
#include <ucos_ii.h>

static OS_STK g_mplc_task_stack[MPLC_NB_TASK_STACK_SIZE];

static void MplcTask(void *p_arg)
{
    (void)p_arg;

    if (mplc_netburner_plc_init() != 0 ||
        mplc_netburner_load_embedded_package() != 0) {
        printf("MPLC: failed to initialize runtime\r\n");
        while (1) {
            OSTimeDly(100);
        }
    }

    printf("MPLC: runtime started\r\n");

    while (1) {
        if (mplc_netburner_run_one_cycle() != 0) {
            OSTimeDly(1);
        }
    }
}

int mplc_netburner_plc_start_task(void)
{
    INT8U err = OSTaskCreate(
        MplcTask,
        NULL,
        &g_mplc_task_stack[MPLC_NB_TASK_STACK_SIZE - 1],
        MPLC_NB_TASK_PRIORITY);

    return err == OS_NO_ERR ? 0 : -1;
}

void mplc_netburner_plc_stop_task(void)
{
    mplc_netburner_plc_shutdown();
}

#else

int mplc_netburner_plc_start_task(void)
{
    if (mplc_netburner_plc_init() != 0 ||
        mplc_netburner_load_embedded_package() != 0) {
        return -1;
    }
    return 0;
}

void mplc_netburner_plc_stop_task(void)
{
    mplc_netburner_plc_shutdown();
}

int mplc_netburner_plc_run_cycles(uint32_t count)
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
