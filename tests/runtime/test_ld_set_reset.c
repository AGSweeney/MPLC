/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc_hal.h"
#include "../platforms/generic/mplc_hal_generic_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int run_cycle(mplc_runtime_t *rt)
{
    return mplc_runtime_run_cycle(rt);
}

static bool led1(void)
{
    return mplc_hal_generic_get_digital_out(0U);
}

int main(int argc, char **argv)
{
    mplc_runtime_t *rt = NULL;
    mplc_runtime_config_t cfg;
    uint8_t *pkg = NULL;
    size_t pkg_size = 0U;
    const char *path = (argc > 1) ? argv[1]
                                  : "D:/MPLC/build_test/sample_ld_set_reset.mplc";

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
    if (run_cycle(rt) != 0 || !led1()) {
        fprintf(stderr, "set failed: expected Led1 ON when Dip1=1 Dip2=0\n");
        return 3;
    }

    mplc_hal_generic_set_digital_in(0U, false);
    mplc_hal_generic_set_digital_in(1U, false);
    if (run_cycle(rt) != 0 || !led1()) {
        fprintf(stderr, "latch failed: expected Led1 still ON when both dips OFF\n");
        return 4;
    }

    mplc_hal_generic_set_digital_in(0U, false);
    mplc_hal_generic_set_digital_in(1U, true);
    if (run_cycle(rt) != 0 || led1()) {
        fprintf(stderr, "reset failed: expected Led1 OFF when Dip2=1 Dip1=0\n");
        return 5;
    }

    mplc_hal_generic_set_digital_in(0U, false);
    mplc_hal_generic_set_digital_in(1U, false);
    if (run_cycle(rt) != 0 || led1()) {
        fprintf(stderr, "reset latch failed: expected Led1 still OFF when both dips OFF\n");
        return 6;
    }

    printf("set_reset test: OK\n");
    mplc_runtime_destroy(rt);
    free(pkg);
    return 0;
}
