#ifndef MPLC_VM_H
#define MPLC_VM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_vm mplc_vm_t;

typedef struct {
    uint8_t       *global_data;
    size_t         global_size;
    uint8_t       *fb_arena;
    size_t         fb_arena_size;
    const uint8_t *code_base;
    size_t         code_size;
    const mplc_io_entry_t *io_map;
    uint32_t       io_count;
    const mplc_fb_instance_t *fb_instances;
    uint32_t       fb_count;
    uint32_t       max_stack_depth;
} mplc_vm_config_t;

typedef struct {
    uint64_t instructions_executed;
    uint32_t call_depth;
} mplc_vm_stats_t;

int  mplc_vm_create(mplc_vm_t **out_vm, const mplc_vm_config_t *cfg);
void mplc_vm_destroy(mplc_vm_t *vm);

int  mplc_vm_run_pou(mplc_vm_t *vm, uint16_t pou_id, uint32_t code_offset, uint32_t code_size);
void mplc_vm_reset(mplc_vm_t *vm);
void mplc_vm_get_stats(mplc_vm_t *vm, mplc_vm_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_VM_H */
