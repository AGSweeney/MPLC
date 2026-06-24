/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_HAL_GENERIC_TEST_H
#define MPLC_HAL_GENERIC_TEST_H

#include <stdint.h>
#include <stdbool.h>

void mplc_hal_generic_set_digital_in(uint16_t channel, bool value);
bool mplc_hal_generic_get_digital_out(uint16_t channel);
void mplc_hal_generic_advance_time_us(uint64_t delta);

#endif
