/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ir.h"
#include "mplc/stdlib.h"
#include <stdlib.h>
#include <string.h>

static ir_expr_t *alloc_expr(void)
{
    ir_expr_t *e = (ir_expr_t *)calloc(1, sizeof(*e));
    return e;
}

ir_expr_t *ir_expr_literal_bool(bool v)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_LITERAL;
    e->type = MPLC_TYPE_BOOL;
    e->u.lit_bool = v;
    return e;
}

ir_expr_t *ir_expr_literal_i32(int32_t v)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_LITERAL;
    e->type = MPLC_TYPE_DINT;
    e->u.lit_i32 = v;
    return e;
}

ir_expr_t *ir_expr_var(uint32_t offset, mplc_type_t type)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_VAR;
    e->type = type;
    e->u.var_offset = offset;
    return e;
}

ir_expr_t *ir_expr_io(uint16_t index, mplc_type_t type)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_IO;
    e->type = type;
    e->u.io_index = index;
    return e;
}

ir_expr_t *ir_expr_binop(ir_binop_t op, ir_expr_t *l, ir_expr_t *r, mplc_type_t type)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_BINOP;
    e->type = type;
    e->left = l;
    e->right = r;
    e->u.binop = op;
    return e;
}

ir_expr_t *ir_expr_unop(ir_unop_t op, ir_expr_t *expr, mplc_type_t type)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_UNOP;
    e->type = type;
    e->left = expr;
    e->u.unop = op;
    return e;
}

ir_expr_t *ir_expr_call_native_fb(mplc_native_fb_t fb, int32_t inst_off, ir_expr_t **params, int n)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_CALL_NATIVE_FB;
    e->type = MPLC_TYPE_BOOL;
    e->u.native_fb.fb_type = fb;
    e->u.native_fb.instance_offset = inst_off;
    e->u.native_fb.param_count = n;
    e->u.native_fb.params = params;
    return e;
}

ir_expr_t *ir_expr_fb_output(mplc_native_fb_t fb, int32_t inst_off, uint8_t output_index)
{
    ir_expr_t *e = alloc_expr();
    if (!e) return NULL;
    e->kind = IR_EXPR_FB_OUTPUT;
    e->type = MPLC_TYPE_BOOL;
    e->u.fb_output.fb_type = fb;
    e->u.fb_output.instance_offset = inst_off;
    e->u.fb_output.output_index = output_index;
    return e;
}

void ir_module_init(ir_module_t *mod)
{
    memset(mod, 0, sizeof(*mod));
    mod->default_cycle_us = 10000U;
}

void ir_module_free(ir_module_t *mod)
{
    free(mod->pous);
    free(mod->globals);
    free(mod->fb_instances);
    memset(mod, 0, sizeof(*mod));
}

int ir_module_add_pou(ir_module_t *mod, const ir_pou_t *pou)
{
    ir_pou_t *np;
    if (!mod || !pou) return -1;
    np = (ir_pou_t *)realloc(mod->pous, (mod->pou_count + 1U) * sizeof(*np));
    if (!np) return -2;
    mod->pous = np;
    mod->pous[mod->pou_count++] = *pou;
    return 0;
}

int ir_module_add_global(ir_module_t *mod, const ir_var_t *var)
{
    ir_var_t *nv;
    if (!mod || !var) return -1;
    nv = (ir_var_t *)realloc(mod->globals, (mod->global_count + 1U) * sizeof(*nv));
    if (!nv) return -2;
    mod->globals = nv;
    mod->globals[mod->global_count++] = *var;
    return 0;
}

int ir_module_add_fb_instance(ir_module_t *mod, mplc_native_fb_t fb_type,
                              int32_t instance_offset, uint16_t instance_id)
{
    ir_fb_instance_t *ni;
    if (!mod) return -1;
    ni = (ir_fb_instance_t *)realloc(mod->fb_instances,
                                     (mod->fb_instance_count + 1U) * sizeof(*ni));
    if (!ni) return -2;
    mod->fb_instances = ni;
    ni = &mod->fb_instances[mod->fb_instance_count];
    ni->fb_type = fb_type;
    ni->instance_offset = instance_offset;
    ni->instance_id = instance_id;
    mod->fb_instance_count++;
    if ((uint32_t)instance_offset + mplc_stdlib_instance_size(fb_type) > mod->fb_arena_size) {
        mod->fb_arena_size = (uint32_t)instance_offset + mplc_stdlib_instance_size(fb_type);
    }
    return 0;
}
