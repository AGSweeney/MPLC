#include "mplc/runtime.h"
#include "mplc_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *read_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long sz;
    uint8_t *buf;
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    sz = ftell(f);
    if (sz <= 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

int main(int argc, char **argv)
{
    mplc_platform_config_t hal_cfg = { .default_cycle_us = 10000U };
    mplc_runtime_config_t rt_cfg;
    mplc_runtime_t *rt = NULL;
    uint8_t *pkg = NULL;
    size_t pkg_size = 0U;
    uint32_t cycles = 0U;
    const char *path;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program.mplc> [cycles]\n", argv[0]);
        return 1;
    }
    path = argv[1];
    if (argc >= 3) {
        cycles = (uint32_t)strtoul(argv[2], NULL, 10);
    }

    pkg = read_file(path, &pkg_size);
    if (!pkg) {
        fprintf(stderr, "Failed to read %s\n", path);
        return 1;
    }

    if (mplc_hal_init(&hal_cfg) != 0) {
        free(pkg);
        return 1;
    }

    memset(&rt_cfg, 0, sizeof(rt_cfg));
    rt_cfg.data_size = 4096U;
    rt_cfg.fb_arena_size = 4096U;
    if (mplc_runtime_create(&rt, &rt_cfg) != 0 ||
        mplc_runtime_load_package(rt, pkg, pkg_size) != 0) {
        fprintf(stderr, "Failed to load MPLC package\n");
        free(pkg);
        mplc_hal_shutdown();
        return 1;
    }

    if (cycles == 0U) {
        (void)mplc_runtime_run(rt, 0U);
    } else {
        (void)mplc_runtime_run(rt, cycles);
    }

    mplc_runtime_destroy(rt);
    mplc_hal_shutdown();
    free(pkg);
    return 0;
}
