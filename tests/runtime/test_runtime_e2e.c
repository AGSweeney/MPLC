/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc/loader.h"
#include "mplc_hal.h"
#include "mplc_hal_generic_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal hand-built package: Output := Input */
static int build_sample_package(uint8_t **out, size_t *out_size)
{
    mplc_pkg_header_t hdr;
    mplc_section_header_t sections[7];
    mplc_pou_desc_t pou;
    mplc_task_desc_t task;
    mplc_io_entry_t io[2];
    uint8_t code[] = {
        MPLC_OP_LOAD_IO, 0, 0,
        MPLC_OP_STORE_IO, 1, 0,
        MPLC_OP_RET
    };
    uint8_t data[8] = {0};
    uint32_t off;
    size_t cap = 512U;
    uint8_t *buf = (uint8_t *)malloc(cap);
    size_t sz = 0U;

    if (!buf) {
        return -1;
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = MPLC_MAGIC;
    hdr.abi_version = MPLC_ABI_VERSION;
    hdr.section_count = 7U;
    hdr.default_cycle_us = 1000U;
    hdr.data_size = sizeof(data);

    memset(&pou, 0, sizeof(pou));
    pou.pou_id = 1U;
    pou.kind = MPLC_POU_PROGRAM;
    pou.code_offset = 0U;
    pou.code_size = (uint32_t)sizeof(code);

    memset(&task, 0, sizeof(task));
    task.program_id = 1U;
    task.period_us = 1000U;

    io[0].index = 0U;
    io[0].type = MPLC_TYPE_BOOL;
    io[0].direction = MPLC_IO_IN;
    io[0].data_offset = 0U;
    io[1].index = 0U;
    io[1].type = MPLC_TYPE_BOOL;
    io[1].direction = MPLC_IO_OUT;
    io[1].data_offset = 1U;

    off = (uint32_t)(sizeof(hdr) + 7U * sizeof(sections[0]));
    sections[0].type = MPLC_SECTION_CODE;
    sections[0].offset = off;
    sections[0].size = (uint32_t)sizeof(code);
    off += sections[0].size;
    sections[1].type = MPLC_SECTION_DATA;
    sections[1].offset = off;
    sections[1].size = (uint32_t)sizeof(data);
    off += sections[1].size;
    sections[2].type = MPLC_SECTION_SYMTAB;
    sections[2].offset = off;
    sections[2].size = (uint32_t)sizeof(pou);
    off += sections[2].size;
    sections[3].type = MPLC_SECTION_IOMAP;
    sections[3].offset = off;
    sections[3].size = (uint32_t)sizeof(io);
    off += sections[3].size;
    sections[4].type = MPLC_SECTION_FBINST;
    sections[4].offset = off;
    sections[4].size = 0U;
    sections[5].type = MPLC_SECTION_TASKS;
    sections[5].offset = off;
    sections[5].size = (uint32_t)sizeof(task);
    off += sections[5].size;
    sections[6].type = MPLC_SECTION_SFC;
    sections[6].offset = off;
    sections[6].size = 0U;

#define APPEND(src, n) do { \
        if (sz + (n) > cap) return -1; \
        memcpy(buf + sz, (src), (n)); \
        sz += (n); \
    } while (0)

    APPEND(&hdr, sizeof(hdr));
    APPEND(sections, sizeof(sections));
    APPEND(code, sizeof(code));
    APPEND(data, sizeof(data));
    APPEND(&pou, sizeof(pou));
    APPEND(io, sizeof(io));
    APPEND(&task, sizeof(task));

#undef APPEND

    *out = buf;
    *out_size = sz;
    return 0;
}

int main(void)
{
    mplc_runtime_t *rt = NULL;
    mplc_runtime_config_t cfg;
    uint8_t *pkg = NULL;
    size_t pkg_size = 0U;

    if (build_sample_package(&pkg, &pkg_size) != 0) {
        return 1;
    }

    mplc_hal_init(NULL);
    mplc_hal_generic_set_digital_in(0, true);

    memset(&cfg, 0, sizeof(cfg));
    cfg.data_size = 4096U;
    cfg.fb_arena_size = 4096U;

    if (mplc_runtime_create(&rt, &cfg) != 0 ||
        mplc_runtime_load_package(rt, pkg, pkg_size) != 0 ||
        mplc_runtime_run_cycle(rt) != 0) {
        free(pkg);
        return 2;
    }

    if (!mplc_runtime_get_bool(rt, 1U)) {
        fprintf(stderr, "Expected output true\n");
        free(pkg);
        return 3;
    }

    mplc_runtime_destroy(rt);
    free(pkg);
    printf("test_runtime_e2e OK\n");
    return 0;
}
