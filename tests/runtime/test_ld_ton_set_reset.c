/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * TON-gated set/reset: Dip1 on + Dip2 off for 2s sets Led1; opposite resets.
 */

#include "mplc/runtime.h"
#include "mplc_hal.h"
#include "../platforms/generic/mplc_hal_generic_test.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef TEST_LD_TON_SET_RESET_PKG
#define TEST_LD_TON_SET_RESET_PKG "sample_ld_ton_set_reset.mplc"
#endif

static int load_file(const char *path, uint8_t **buf, size_t *size)
{
    FILE *f = fopen(path, "rb");
    long sz;
    uint8_t *tmp;

    if (!f) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -2;
    }
    sz = ftell(f);
    if (sz <= 0L) {
        fclose(f);
        return -3;
    }
    rewind(f);
    tmp = (uint8_t *)malloc((size_t)sz);
    if (!tmp) {
        fclose(f);
        return -4;
    }
    if (fread(tmp, 1U, (size_t)sz, f) != (size_t)sz) {
        free(tmp);
        fclose(f);
        return -5;
    }
    fclose(f);
    *buf = tmp;
    *size = (size_t)sz;
    return 0;
}

int main(void)
{
    mplc_runtime_t *rt = NULL;
    mplc_runtime_config_t cfg;
    uint8_t *pkg = NULL;
    size_t pkg_size = 0U;
    int cycle;
    const char *path = TEST_LD_TON_SET_RESET_PKG;
    uint64_t t0;

    if (load_file(path, &pkg, &pkg_size) != 0) {
        fprintf(stderr, "failed to read %s\n", path);
        return 1;
    }

    mplc_hal_init(NULL);
    memset(&cfg, 0, sizeof(cfg));
    cfg.data_size = 8192U;
    cfg.fb_arena_size = 8192U;

    if (mplc_runtime_create(&rt, &cfg) != 0 ||
        mplc_runtime_load_package(rt, pkg, pkg_size) != 0) {
        fprintf(stderr, "runtime load failed\n");
        free(pkg);
        return 2;
    }

    mplc_hal_generic_set_digital_in(0U, true);
    mplc_hal_generic_set_digital_in(1U, false);
    t0 = mplc_hal_time_us();

    for (cycle = 0; cycle < 400; cycle++) {
        mplc_hal_generic_advance_time_us(100000U);
        if (mplc_runtime_run_cycle(rt) != 0) {
            fprintf(stderr, "cycle failed during set\n");
            mplc_runtime_destroy(rt);
            free(pkg);
            return 3;
        }
        if (mplc_hal_generic_get_digital_out(0U)) {
            break;
        }
    }

    if (!mplc_hal_generic_get_digital_out(0U)) {
        fprintf(stderr, "Led1 never set after TON delay\n");
        mplc_runtime_destroy(rt);
        free(pkg);
        return 4;
    }

    const uint64_t set_ms = (mplc_hal_time_us() - t0) / 1000ULL;
    if (set_ms < 1500ULL) {
        fprintf(stderr, "Led1 set too early after %llu ms\n", (unsigned long long)set_ms);
        mplc_runtime_destroy(rt);
        free(pkg);
        return 5;
    }

    mplc_hal_generic_set_digital_in(0U, false);
    mplc_hal_generic_set_digital_in(1U, false);
    for (cycle = 0; cycle < 20; cycle++) {
        mplc_hal_generic_advance_time_us(100000U);
        mplc_runtime_run_cycle(rt);
    }
    if (!mplc_hal_generic_get_digital_out(0U)) {
        fprintf(stderr, "Led1 did not latch after set\n");
        mplc_runtime_destroy(rt);
        free(pkg);
        return 6;
    }

    mplc_hal_generic_set_digital_in(0U, false);
    mplc_hal_generic_set_digital_in(1U, true);
    t0 = mplc_hal_time_us();
    for (cycle = 0; cycle < 400; cycle++) {
        mplc_hal_generic_advance_time_us(100000U);
        if (mplc_runtime_run_cycle(rt) != 0) {
            fprintf(stderr, "cycle failed during reset\n");
            mplc_runtime_destroy(rt);
            free(pkg);
            return 7;
        }
        if (!mplc_hal_generic_get_digital_out(0U)) {
            break;
        }
    }

    if (mplc_hal_generic_get_digital_out(0U)) {
        fprintf(stderr, "Led1 never reset after TON delay\n");
        mplc_runtime_destroy(rt);
        free(pkg);
        return 8;
    }

    const uint64_t reset_ms = (mplc_hal_time_us() - t0) / 1000ULL;
    if (reset_ms < 1500ULL) {
        fprintf(stderr, "Led1 reset too early after %llu ms\n", (unsigned long long)reset_ms);
        mplc_runtime_destroy(rt);
        free(pkg);
        return 9;
    }

    printf("ld_ton_set_reset test OK (set ~%llu ms, reset ~%llu ms)\n",
           (unsigned long long)set_ms, (unsigned long long)reset_ms);
    mplc_runtime_destroy(rt);
    free(pkg);
    return 0;
}
