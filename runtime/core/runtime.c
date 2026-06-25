/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc/runtime.h"
#include "mplc/loader.h"
#include "mplc/vm.h"
#include "mplc/scheduler.h"
#include "mplc/io.h"
#include "mplc/stdlib.h"
#include "mplc/motion.h"
#include "mplc_hal.h"
#include "mplc_endian.h"
#include <stdlib.h>
#include <string.h>

struct mplc_runtime {
    mplc_loaded_pkg_t   pkg;
    uint8_t            *data_segment;
    size_t              data_segment_size;
    uint8_t            *fb_arena;
    size_t              fb_arena_size;
    mplc_vm_t          *vm;
    mplc_scheduler_t   *scheduler;
    mplc_io_ctx_t       io_ctx;
};

static int runtime_cycle(void *ctx, uint16_t program_id)
{
    mplc_runtime_t *rt = (mplc_runtime_t *)ctx;
    uint32_t i;
    uint32_t dt_us = rt->pkg.header ? MPLC_LE32(rt->pkg.header->default_cycle_us) : 10000U;

    mplc_motion_cycle(dt_us);

    for (i = 0; i < rt->pkg.pou_count; i++) {
        const mplc_pou_desc_t *pou = &rt->pkg.pous[i];
        uint16_t pou_id = MPLC_LE16(pou->pou_id);
        if (pou_id == program_id || (program_id == 0U && pou->kind == MPLC_POU_PROGRAM)) {
            return mplc_vm_run_pou(rt->vm, pou_id,
                                   MPLC_LE32(pou->code_offset),
                                   MPLC_LE32(pou->code_size));
        }
    }
    return 0;
}

static void runtime_io_read(void *ctx)
{
    mplc_runtime_t *rt = (mplc_runtime_t *)ctx;
    mplc_io_read_inputs(&rt->io_ctx);
}

static void runtime_io_write(void *ctx)
{
    mplc_runtime_t *rt = (mplc_runtime_t *)ctx;
    mplc_io_write_outputs(&rt->io_ctx);
}

int mplc_runtime_create(mplc_runtime_t **out_rt, const mplc_runtime_config_t *cfg)
{
    mplc_runtime_t *rt;
    if (!out_rt || !cfg) {
        return -1;
    }
    rt = (mplc_runtime_t *)calloc(1, sizeof(*rt));
    if (!rt) {
        return -2;
    }
    rt->data_segment = (uint8_t *)malloc(cfg->data_size);
    rt->data_segment_size = cfg->data_size;
    rt->fb_arena = (uint8_t *)calloc(1, cfg->fb_arena_size);
    rt->fb_arena_size = cfg->fb_arena_size;
    if (!rt->data_segment || !rt->fb_arena) {
        mplc_runtime_destroy(rt);
        return -3;
    }
    if (cfg->data_segment && cfg->data_size > 0U) {
        memcpy(rt->data_segment, cfg->data_segment, cfg->data_size);
    }
    *out_rt = rt;
    return 0;
}

void mplc_runtime_destroy(mplc_runtime_t *rt)
{
    if (!rt) {
        return;
    }
    mplc_scheduler_destroy(rt->scheduler);
    mplc_vm_destroy(rt->vm);
    free(rt->data_segment);
    free(rt->fb_arena);
    free(rt);
}

int mplc_runtime_load_package(mplc_runtime_t *rt, const uint8_t *data, size_t size)
{
    mplc_vm_config_t vm_cfg;
    mplc_scheduler_config_t sched_cfg;
    uint32_t i;

    if (!rt || !data) {
        return -1;
    }
    if (mplc_loader_parse(data, size, &rt->pkg) != 0) {
        return -2;
    }

    if (rt->pkg.data_size > 0U) {
        size_t copy = rt->pkg.data_size;
        if (rt->data_segment) {
            memset(rt->data_segment, 0, copy);
            memcpy(rt->data_segment, rt->pkg.data_base, copy);
        }
    }

    if (rt->fb_arena && rt->fb_arena_size > 0U) {
        memset(rt->fb_arena, 0, rt->fb_arena_size);
    }

    for (i = 0; i < rt->pkg.fb_count; i++) {
        const mplc_fb_vtable_t *vt = mplc_fb_get_vtable(
            (mplc_native_fb_t)MPLC_LE16(rt->pkg.fb_instances[i].fb_type));
        if (vt && vt->init && rt->fb_arena) {
            void *inst = rt->fb_arena + MPLC_LE32(rt->pkg.fb_instances[i].instance_offset);
            vt->init(inst);
        }
    }

    memset(&vm_cfg, 0, sizeof(vm_cfg));
    vm_cfg.global_data = rt->data_segment;
    vm_cfg.global_size = rt->pkg.data_size;
    vm_cfg.fb_arena = rt->fb_arena;
    vm_cfg.fb_arena_size = rt->pkg.header ? MPLC_LE32(rt->pkg.header->fb_arena_size) : 0U;
    vm_cfg.code_base = rt->pkg.code_base;
    vm_cfg.code_size = rt->pkg.code_size;
    vm_cfg.io_map = rt->pkg.io_map;
    vm_cfg.io_count = rt->pkg.io_count;
    vm_cfg.fb_instances = rt->pkg.fb_instances;
    vm_cfg.fb_count = rt->pkg.fb_count;
    vm_cfg.max_stack_depth = rt->pkg.header ? MPLC_LE32(rt->pkg.header->max_stack_depth)
                                            : MPLC_MAX_STACK_DEPTH;

    if (mplc_vm_create(&rt->vm, &vm_cfg) != 0) {
        return -3;
    }

    mplc_io_init(&rt->io_ctx, rt->pkg.io_map, rt->pkg.io_count, rt->data_segment, rt->pkg.data_size);

    if (mplc_motion_init(0U) != 0) {
        return -5;
    }

    memset(&sched_cfg, 0, sizeof(sched_cfg));
    sched_cfg.tasks = rt->pkg.tasks;
    sched_cfg.task_count = rt->pkg.task_count;
    sched_cfg.pous = rt->pkg.pous;
    sched_cfg.pou_count = rt->pkg.pou_count;
    sched_cfg.default_cycle_us = rt->pkg.header ? MPLC_LE32(rt->pkg.header->default_cycle_us) : 10000U;

    if (mplc_scheduler_create(&rt->scheduler, &sched_cfg) != 0) {
        return -4;
    }
    mplc_scheduler_set_io_hooks(rt->scheduler, runtime_io_read, runtime_io_write);
    mplc_scheduler_set_cycle_fn(rt->scheduler, runtime_cycle, rt);

    return 0;
}

int mplc_runtime_run_cycle(mplc_runtime_t *rt)
{
    if (!rt || !rt->scheduler) {
        return -1;
    }
    return mplc_scheduler_run_once(rt->scheduler);
}

int mplc_runtime_run(mplc_runtime_t *rt, uint32_t max_cycles)
{
    if (!rt || !rt->scheduler) {
        return -1;
    }
    return mplc_scheduler_run(rt->scheduler, runtime_cycle, rt, max_cycles);
}

bool mplc_runtime_get_bool(mplc_runtime_t *rt, uint32_t offset)
{
    if (!rt || !rt->data_segment || offset >= rt->pkg.data_size) {
        return false;
    }
    return rt->data_segment[offset] != 0U;
}

void mplc_runtime_set_bool(mplc_runtime_t *rt, uint32_t offset, bool value)
{
    if (!rt || !rt->data_segment || offset >= rt->pkg.data_size) {
        return;
    }
    rt->data_segment[offset] = value ? 1U : 0U;
}

int32_t mplc_runtime_get_i32(mplc_runtime_t *rt, uint32_t offset)
{
    int32_t v = 0;
    if (!rt || !rt->data_segment || offset + sizeof(int32_t) > rt->pkg.data_size) {
        return 0;
    }
    memcpy(&v, rt->data_segment + offset, sizeof(v));
    return v;
}

void mplc_runtime_set_i32(mplc_runtime_t *rt, uint32_t offset, int32_t value)
{
    if (!rt || !rt->data_segment || offset + sizeof(int32_t) > rt->pkg.data_size) {
        return;
    }
    memcpy(rt->data_segment + offset, &value, sizeof(value));
}
