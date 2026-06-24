/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc_hal.h"
#include "../platforms/generic/mplc_hal_generic_test.h"
#include <stdio.h>
#include <stdlib.h>

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
    uint16_t ch;
    const char *path =
        "D:/MPLC/platforms/netburner/mod54415/nb_project/mod_dev70_dips_to_leds.mplc";

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

    for (ch = 0; ch < 8U; ch++) {
        uint16_t other;
        mplc_hal_generic_set_digital_in(ch, true);
        mplc_hal_generic_advance_time_us(10000U);
        if (mplc_runtime_run_cycle(rt) != 0) {
            fprintf(stderr, "cycle failed for ch %u\n", ch);
            break;
        }
        for (other = 0; other < 8U; other++) {
            bool out = mplc_hal_generic_get_digital_out(other);
            if (out != (other == ch)) {
                fprintf(stderr, "ch %u in: expected only led %u on, led %u=%d\n",
                        ch, ch, other, out ? 1 : 0);
                mplc_runtime_destroy(rt);
                free(pkg);
                return 3;
            }
        }
        mplc_hal_generic_set_digital_in(ch, false);
    }

    printf("dips_to_leds test: all 8 channels OK\n");
    mplc_runtime_destroy(rt);
    free(pkg);
    return 0;
}
