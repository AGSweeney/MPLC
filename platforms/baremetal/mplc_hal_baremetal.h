/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_HAL_BAREMETAL_H
#define MPLC_HAL_BAREMETAL_H

#include <stdint.h>

void mplc_hal_baremetal_tick_us(uint64_t delta);
uint32_t mplc_hal_baremetal_get_gpio_out(void);
void mplc_hal_baremetal_set_gpio_in(uint32_t value);

#endif
