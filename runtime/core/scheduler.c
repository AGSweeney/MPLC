#include "mplc/scheduler.h"
#include "mplc_hal.h"
#include <stdlib.h>

struct mplc_scheduler {
    mplc_scheduler_config_t cfg;
    mplc_cycle_fn           cycle_fn;
    void                   *cycle_ctx;
    mplc_io_hook_fn         io_read;
    mplc_io_hook_fn         io_write;
    uint64_t                cycle_count;
};

int mplc_scheduler_create(mplc_scheduler_t **out_sched, const mplc_scheduler_config_t *cfg)
{
    mplc_scheduler_t *sched;
    if (!out_sched || !cfg) {
        return -1;
    }
    sched = (mplc_scheduler_t *)calloc(1, sizeof(*sched));
    if (!sched) {
        return -2;
    }
    sched->cfg = *cfg;
    *out_sched = sched;
    return 0;
}

void mplc_scheduler_destroy(mplc_scheduler_t *sched)
{
    free(sched);
}

void mplc_scheduler_set_io_hooks(mplc_scheduler_t *sched, mplc_io_hook_fn read_fn,
                                 mplc_io_hook_fn write_fn)
{
    if (!sched) {
        return;
    }
    sched->io_read = read_fn;
    sched->io_write = write_fn;
}

void mplc_scheduler_set_cycle_fn(mplc_scheduler_t *sched, mplc_cycle_fn fn, void *ctx)
{
    if (!sched) {
        return;
    }
    sched->cycle_fn = fn;
    sched->cycle_ctx = ctx;
}

int mplc_scheduler_run_once(mplc_scheduler_t *sched)
{
    uint32_t i;
    uint64_t cycle_start;
    uint64_t deadline;
    uint32_t cycle_us;

    if (!sched || !sched->cycle_fn) {
        return -1;
    }

    cycle_us = sched->cfg.default_cycle_us ? sched->cfg.default_cycle_us : 10000U;
    cycle_start = mplc_hal_time_us();
    deadline = cycle_start + cycle_us;

    if (sched->io_read) {
        sched->io_read(sched->cycle_ctx);
    }

    if (sched->cfg.task_count == 0U) {
        if (sched->cycle_fn(sched->cycle_ctx, 0U) != 0) {
            return -2;
        }
    } else {
        for (i = 0; i < sched->cfg.task_count; i++) {
            if (sched->cycle_fn(sched->cycle_ctx, sched->cfg.tasks[i].program_id) != 0) {
                return -2;
            }
        }
    }

    if (sched->io_write) {
        sched->io_write(sched->cycle_ctx);
    }

    sched->cycle_count++;
    mplc_hal_sleep_until(deadline);
    return 0;
}

int mplc_scheduler_run(mplc_scheduler_t *sched, mplc_cycle_fn fn, void *ctx, uint32_t max_cycles)
{
    uint32_t i;
    if (!sched) {
        return -1;
    }
    sched->cycle_fn = fn;
    sched->cycle_ctx = ctx;

    if (max_cycles == 0U) {
        while (1) {
            if (mplc_scheduler_run_once(sched) != 0) {
                return -2;
            }
        }
    }
    for (i = 0; i < max_cycles; i++) {
        if (mplc_scheduler_run_once(sched) != 0) {
            return -2;
        }
    }
    return 0;
}
