/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "codegen.h"
#include <stdlib.h>
#include <string.h>

void codegen_buf_init(codegen_buf_t *buf)
{
    memset(buf, 0, sizeof(*buf));
}

void codegen_buf_free(codegen_buf_t *buf)
{
    free(buf->bytes);
    memset(buf, 0, sizeof(*buf));
}

static int ensure_cap(codegen_buf_t *buf, uint32_t need)
{
    if (buf->size + need <= buf->capacity) {
        return 0;
    }
    uint32_t cap = buf->capacity ? buf->capacity * 2U : 256U;
    while (cap < buf->size + need) {
        cap *= 2U;
    }
    uint8_t *nb = (uint8_t *)realloc(buf->bytes, cap);
    if (!nb) return -1;
    buf->bytes = nb;
    buf->capacity = cap;
    return 0;
}

void codegen_emit_u8(codegen_buf_t *buf, uint8_t v)
{
    if (ensure_cap(buf, 1U) != 0) return;
    buf->bytes[buf->size++] = v;
}

void codegen_emit_u16(codegen_buf_t *buf, uint16_t v)
{
    codegen_emit_u8(buf, (uint8_t)(v & 0xFFU));
    codegen_emit_u8(buf, (uint8_t)((v >> 8) & 0xFFU));
}

void codegen_emit_u32(codegen_buf_t *buf, uint32_t v)
{
    codegen_emit_u8(buf, (uint8_t)(v & 0xFFU));
    codegen_emit_u8(buf, (uint8_t)((v >> 8) & 0xFFU));
    codegen_emit_u8(buf, (uint8_t)((v >> 16) & 0xFFU));
    codegen_emit_u8(buf, (uint8_t)((v >> 24) & 0xFFU));
}

void codegen_emit_i32(codegen_buf_t *buf, int32_t v)
{
    codegen_emit_u32(buf, (uint32_t)v);
}

static void track_push(codegen_buf_t *buf)
{
    buf->stack_depth++;
    if (buf->stack_depth > buf->max_stack) {
        buf->max_stack = buf->stack_depth;
    }
}

static void track_pop(codegen_buf_t *buf, uint32_t n)
{
    if (buf->stack_depth >= n) {
        buf->stack_depth -= n;
    } else {
        buf->stack_depth = 0;
    }
}

static void gen_expr(codegen_buf_t *buf, const ir_expr_t *e)
{
    if (!e) return;

    switch (e->kind) {
    case IR_EXPR_LITERAL:
        if (e->type == MPLC_TYPE_BOOL) {
            codegen_emit_u8(buf, MPLC_OP_PUSH_BOOL);
            codegen_emit_u8(buf, e->u.lit_bool ? 1U : 0U);
        } else {
            codegen_emit_u8(buf, MPLC_OP_PUSH_I32);
            codegen_emit_i32(buf, e->u.lit_i32);
        }
        track_push(buf);
        break;

    case IR_EXPR_VAR:
        codegen_emit_u8(buf, MPLC_OP_LOAD_VAR);
        codegen_emit_u8(buf, (uint8_t)e->type);
        codegen_emit_u32(buf, e->u.var_offset);
        track_push(buf);
        break;

    case IR_EXPR_IO:
        codegen_emit_u8(buf, MPLC_OP_LOAD_IO);
        codegen_emit_u16(buf, e->u.io_index);
        track_push(buf);
        break;
    case IR_EXPR_BINOP:
        gen_expr(buf, e->left);
        gen_expr(buf, e->right);
        switch (e->u.binop) {
        case IR_BIN_ADD: codegen_emit_u8(buf, MPLC_OP_ADD_I32); break;
        case IR_BIN_SUB: codegen_emit_u8(buf, MPLC_OP_SUB_I32); break;
        case IR_BIN_MUL: codegen_emit_u8(buf, MPLC_OP_MUL_I32); break;
        case IR_BIN_DIV: codegen_emit_u8(buf, MPLC_OP_DIV_I32); break;
        case IR_BIN_MOD: codegen_emit_u8(buf, MPLC_OP_MOD_I32); break;
        case IR_BIN_AND: codegen_emit_u8(buf, MPLC_OP_AND); break;
        case IR_BIN_OR:  codegen_emit_u8(buf, MPLC_OP_OR); break;
        case IR_BIN_XOR: codegen_emit_u8(buf, MPLC_OP_XOR); break;
        case IR_BIN_EQ:  codegen_emit_u8(buf, MPLC_OP_EQ_I32); break;
        case IR_BIN_NE:  codegen_emit_u8(buf, MPLC_OP_NE_I32); break;
        case IR_BIN_LT:  codegen_emit_u8(buf, MPLC_OP_LT_I32); break;
        case IR_BIN_LE:  codegen_emit_u8(buf, MPLC_OP_LE_I32); break;
        case IR_BIN_GT:  codegen_emit_u8(buf, MPLC_OP_GT_I32); break;
        case IR_BIN_GE:  codegen_emit_u8(buf, MPLC_OP_GE_I32); break;
        default: break;
        }
        track_pop(buf, 1U);
        break;

    case IR_EXPR_UNOP:
        gen_expr(buf, e->left);
        if (e->u.unop == IR_UN_NOT) {
            codegen_emit_u8(buf, MPLC_OP_NOT);
        } else if (e->u.unop == IR_UN_NEG) {
            codegen_emit_u8(buf, MPLC_OP_PUSH_I32);
            codegen_emit_i32(buf, 0);
            track_push(buf);
            codegen_emit_u8(buf, MPLC_OP_SUB_I32);
            track_pop(buf, 1U);
        }
        break;

    case IR_EXPR_CALL_NATIVE_FB: {
        int i;
        for (i = 0; i < e->u.native_fb.param_count; i++) {
            gen_expr(buf, e->u.native_fb.params[i]);
        }
        codegen_emit_u8(buf, MPLC_OP_CALL_NATIVE_FB);
        codegen_emit_u16(buf, (uint16_t)e->u.native_fb.fb_type);
        codegen_emit_i32(buf, e->u.native_fb.instance_offset);
        codegen_emit_i32(buf, e->u.native_fb.param_count);
        track_pop(buf, (uint32_t)e->u.native_fb.param_count);
        break;
    }
    default:
        break;
    }
}

static void gen_stmt(codegen_buf_t *buf, const ir_stmt_t *s)
{
    if (!s) return;
    switch (s->kind) {
    case IR_STMT_ASSIGN:
        gen_expr(buf, s->u.assign.value);
        if (s->u.assign.target->kind == IR_EXPR_VAR) {
            codegen_emit_u8(buf, MPLC_OP_STORE_VAR);
            codegen_emit_u8(buf, (uint8_t)s->u.assign.target->type);
            codegen_emit_u32(buf, s->u.assign.target->u.var_offset);
        } else if (s->u.assign.target->kind == IR_EXPR_IO) {
            codegen_emit_u8(buf, MPLC_OP_STORE_IO);
            codegen_emit_u16(buf, s->u.assign.target->u.io_index);
        }
        track_pop(buf, 1U);
        break;
    case IR_STMT_EXPR:
        gen_expr(buf, s->u.expr);
        track_pop(buf, 1U);
        break;
    case IR_STMT_IF: {
        uint32_t else_jmp = 0U;
        uint32_t end_jmp = 0U;
        gen_expr(buf, s->u.if_stmt.cond);
        codegen_emit_u8(buf, MPLC_OP_JMP_IFNOT);
        uint32_t jmp_pos = buf->size;
        codegen_emit_u32(buf, 0U);
        track_pop(buf, 1U);
        uint32_t i;
        for (i = 0; i < s->u.if_stmt.then_count; i++) {
            gen_stmt(buf, &s->u.if_stmt.then_stmts[i]);
        }
        if (s->u.if_stmt.else_count > 0U) {
            codegen_emit_u8(buf, MPLC_OP_JMP);
            end_jmp = buf->size;
            codegen_emit_u32(buf, 0U);
            else_jmp = buf->size;
            memcpy(buf->bytes + jmp_pos, &else_jmp, sizeof(else_jmp));
            for (i = 0; i < s->u.if_stmt.else_count; i++) {
                gen_stmt(buf, &s->u.if_stmt.else_stmts[i]);
            }
            uint32_t end_pos = buf->size;
            memcpy(buf->bytes + end_jmp, &end_pos, sizeof(end_pos));
        } else {
            else_jmp = buf->size;
            memcpy(buf->bytes + jmp_pos, &else_jmp, sizeof(else_jmp));
        }
        break;
    }
    case IR_STMT_WHILE: {
        uint32_t loop_start = buf->size;
        gen_expr(buf, s->u.while_stmt.cond);
        codegen_emit_u8(buf, MPLC_OP_JMP_IFNOT);
        uint32_t exit_jmp = buf->size;
        codegen_emit_u32(buf, 0U);
        track_pop(buf, 1U);
        uint32_t i;
        for (i = 0; i < s->u.while_stmt.body_count; i++) {
            gen_stmt(buf, &s->u.while_stmt.body[i]);
        }
        codegen_emit_u8(buf, MPLC_OP_JMP);
        codegen_emit_u32(buf, loop_start);
        uint32_t exit_pos = buf->size;
        memcpy(buf->bytes + exit_jmp, &exit_pos, sizeof(exit_pos));
        break;
    }
    case IR_STMT_REPEAT: {
        const uint32_t loop_start = buf->size;
        uint32_t i;
        for (i = 0; i < s->u.repeat_stmt.body_count; i++) {
            gen_stmt(buf, &s->u.repeat_stmt.body[i]);
        }
        gen_expr(buf, s->u.repeat_stmt.until_cond);
        codegen_emit_u8(buf, MPLC_OP_JMP_IF);
        const uint32_t exit_pos = buf->size;
        codegen_emit_u32(buf, 0U);
        track_pop(buf, 1U);
        codegen_emit_u8(buf, MPLC_OP_JMP);
        codegen_emit_u32(buf, loop_start);
        const uint32_t end_pos = buf->size;
        memcpy(buf->bytes + exit_pos, &end_pos, sizeof(end_pos));
        break;
    }
    default:
        break;
    }
}

int codegen_module_from_ir(const ir_module_t *mod, codegen_module_t *out)
{
    uint32_t p, s;
    if (!mod || !out) return -1;
    memset(out, 0, sizeof(*out));

    out->pous = (codegen_pou_t *)calloc(mod->pou_count, sizeof(*out->pous));
    if (!out->pous) return -2;
    out->pou_count = mod->pou_count;

    for (p = 0; p < mod->pou_count; p++) {
        const ir_pou_t *ip = &mod->pous[p];
        codegen_pou_t *cp = &out->pous[p];
        cp->pou_id = ip->pou_id;
        codegen_buf_init(&cp->code);
        for (s = 0; s < ip->stmt_count; s++) {
            gen_stmt(&cp->code, &ip->stmts[s]);
        }
        codegen_emit_u8(&cp->code, MPLC_OP_RET);
    }
    return 0;
}

void codegen_module_free(codegen_module_t *mod)
{
    uint32_t i;
    if (!mod) return;
    for (i = 0; i < mod->pou_count; i++) {
        codegen_buf_free(&mod->pous[i].code);
    }
    free(mod->pous);
    memset(mod, 0, sizeof(*mod));
}
