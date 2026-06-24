/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_LINKER_H
#define MPLC_LINKER_H

#include "codegen/codegen.h"
#include "ir.h"
#include "semantic/semantic.h"
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *data;
    size_t   size;
} linker_output_t;

int linker_build_package(const ir_module_t *mod, const codegen_module_t *cg,
                         const sym_table_t *globals, linker_output_t *out);
void linker_output_free(linker_output_t *out);

#endif
