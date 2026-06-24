/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* MPLC MOD54415 project overrides for NetBurner system constants. */

#ifndef MPLC_NB_CONSTANTS_OVERLOAD_H
#define MPLC_NB_CONSTANTS_OVERLOAD_H

/* 100 Hz RTOS tick gives 10 ms OSTimeDly() granularity for PLC scans. */
#ifndef TICKS_PER_SECOND
#define TICKS_PER_SECOND (100)
#endif

#endif /* MPLC_NB_CONSTANTS_OVERLOAD_H */
