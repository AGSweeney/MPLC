/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_motionstack_q15.h"

int32_t mplc_ms_q15_mul(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * (int64_t)b) >> MPLC_MS_FRACT_BITS);
}

int32_t mplc_ms_q15_div(int32_t num, int32_t den)
{
    if (den == 0) {
        return 0;
    }
    return (int32_t)(((int64_t)num << MPLC_MS_FRACT_BITS) / (int64_t)den);
}

int32_t mplc_ms_isqrt32(uint32_t v)
{
    uint32_t res = 0U;
    uint32_t bit = 1U << 30;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0U) {
        if (v >= res + bit) {
            v -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return (int32_t)res;
}
