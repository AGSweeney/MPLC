/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Verifies TON delay: output stays off until PT elapses.
 */

#include "mplc/runtime.h"
#include "mplc_hal.h"
#include "../platforms/generic/mplc_hal_generic_test.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef TEST_LD_TON_PKG
#define TEST_LD_TON_PKG "sample_ld_ton.mplc"
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
    const char *path = TEST_LD_TON_PKG;

    if (load_file(path, &pkg, &pkg_size) != 0) {
        fprintf(stderr, "failed to read %s\n", path);
        return 1;
    }

    mplc_hal_init(NULL);
    mplc_hal_generic_set_digital_in(0U, true);

    memset(&cfg, 0, sizeof(cfg));
    cfg.data_size = 8192U;
    cfg.fb_arena_size = 8192U;

    if (mplc_runtime_create(&rt, &cfg) != 0 ||
        mplc_runtime_load_package(rt, pkg, pkg_size) != 0) {
        fprintf(stderr, "runtime load failed\n");
        free(pkg);
        return 2;
    }

    uint64_t t0 = mplc_hal_time_us();

    for (cycle = 0; cycle < 600; cycle++) {
        mplc_hal_generic_advance_time_us(20000U);
        if (mplc_runtime_run_cycle(rt) != 0) {
            fprintf(stderr, "cycle failed\n");
            mplc_runtime_destroy(rt);
            free(pkg);
            return 3;
        }
        if (mplc_hal_generic_get_digital_out(0U)) {
            break;
        }
    }

    const uint64_t elapsed_us = mplc_hal_time_us() - t0;
    if (elapsed_us < 4500000ULL) {
        fprintf(stderr, "TON output turned on too early after %llu us (~%d cycles)\n",
                (unsigned long long)elapsed_us, cycle + 1);
        mplc_runtime_destroy(rt);
        free(pkg);
        return 4;
    }

    if (!mplc_hal_generic_get_digital_out(0U)) {
        fprintf(stderr, "TON output never turned on\n");
        mplc_runtime_destroy(rt);
        free(pkg);
        return 5;
    }

    printf("ld_ton delay test: output delayed until PT elapsed\n");
    mplc_runtime_destroy(rt);
    free(pkg);
    return 0;
}
