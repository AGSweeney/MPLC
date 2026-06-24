/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_MOTIONSTACK_Q15_H
#define MPLC_MOTIONSTACK_Q15_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MPLC_MS_FRACT_BITS 15
#define MPLC_MS_TWO_PI_QX  32768

int32_t mplc_ms_q15_mul(int32_t a, int32_t b);
int32_t mplc_ms_q15_div(int32_t num, int32_t den);
int32_t mplc_ms_isqrt32(uint32_t v);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_MOTIONSTACK_Q15_H */
