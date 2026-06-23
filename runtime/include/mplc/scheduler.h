#ifndef MPLC_SCHEDULER_H
#define MPLC_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_scheduler mplc_scheduler_t;

typedef struct {
    const mplc_task_desc_t *tasks;
    uint32_t                task_count;
    const mplc_pou_desc_t  *pous;
    uint32_t                pou_count;
    uint32_t                default_cycle_us;
} mplc_scheduler_config_t;

typedef int (*mplc_cycle_fn)(void *ctx, uint16_t program_id);
typedef void (*mplc_io_hook_fn)(void *ctx);

int  mplc_scheduler_create(mplc_scheduler_t **out_sched, const mplc_scheduler_config_t *cfg);
void mplc_scheduler_destroy(mplc_scheduler_t *sched);
void mplc_scheduler_set_io_hooks(mplc_scheduler_t *sched, mplc_io_hook_fn read_fn,
                                 mplc_io_hook_fn write_fn);
void mplc_scheduler_set_cycle_fn(mplc_scheduler_t *sched, mplc_cycle_fn fn, void *ctx);

int  mplc_scheduler_run_once(mplc_scheduler_t *sched);
int  mplc_scheduler_run(mplc_scheduler_t *sched, mplc_cycle_fn fn, void *ctx, uint32_t max_cycles);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_SCHEDULER_H */
