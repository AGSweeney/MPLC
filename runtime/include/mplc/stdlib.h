/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_STDLIB_H
#define MPLC_STDLIB_H

#include <stdint.h>
#include <stdbool.h>
#include "mplc_abi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mplc_vm mplc_vm_t;

typedef void (*mplc_fb_init_fn)(void *inst);
typedef void (*mplc_fb_cycle_fn)(mplc_vm_t *vm, void *inst, const int32_t *params);

typedef struct {
    mplc_native_fb_t   type;
    uint32_t           instance_size;
    mplc_fb_init_fn    init;
    mplc_fb_cycle_fn   cycle;
} mplc_fb_vtable_t;

const mplc_fb_vtable_t *mplc_stdlib_get_vtable(mplc_native_fb_t type);
uint32_t mplc_stdlib_instance_size(mplc_native_fb_t type);

/* Unified lookup: stdlib FBs first, then motion MC FBs. */
const mplc_fb_vtable_t *mplc_fb_get_vtable(mplc_native_fb_t type);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_STDLIB_H */
