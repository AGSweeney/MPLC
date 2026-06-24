/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * NNDK application entry template for MOD54415 + MPLC.
 *
 * Copy this file into your NBEclipse MOD5441X project and wire Init() to your
 * existing network / web server startup as needed.
 */

#include "mplc_hal_netburner.h"
#include <stdio.h>

#ifdef MPLC_NETBURNER_NNDK
#include <predef.h>
#include <init.h>
#include <ucos_ii.h>

extern "C" void UserMain(void *pd)
{
    (void)pd;

    printf("MOD54415 MPLC starting\r\n");

    if (mplc_netburner_plc_start_task() != 0) {
        printf("MPLC: task create failed\r\n");
    }

    while (1) {
        OSTimeDly(100);
    }
}

#else

/*
 * Host stub — links with mplc_netburner_test for desktop validation.
 */
int main(void)
{
    printf("NetBurner MPLC host stub — use mplc_netburner_test instead\r\n");
    return 0;
}

#endif
