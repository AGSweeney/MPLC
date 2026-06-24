/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_CODEGEN_H
#define MPLC_CODEGEN_H

#include "ir.h"
#include "mplc_abi.h"
#include <stdint.h>

typedef struct {
    uint8_t *bytes;
    uint32_t size;
    uint32_t capacity;
    uint32_t max_stack;
    uint32_t stack_depth;
} codegen_buf_t;

typedef struct {
    codegen_buf_t code;
    uint16_t      pou_id;
} codegen_pou_t;

typedef struct {
    codegen_pou_t *pous;
    uint32_t       pou_count;
} codegen_module_t;

void codegen_buf_init(codegen_buf_t *buf);
void codegen_buf_free(codegen_buf_t *buf);
void codegen_emit_u8(codegen_buf_t *buf, uint8_t v);
void codegen_emit_u16(codegen_buf_t *buf, uint16_t v);
void codegen_emit_u32(codegen_buf_t *buf, uint32_t v);
void codegen_emit_i32(codegen_buf_t *buf, int32_t v);

int codegen_module_from_ir(const ir_module_t *mod, codegen_module_t *out);
void codegen_module_free(codegen_module_t *mod);

#endif
