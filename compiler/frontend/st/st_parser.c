/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "st_parser.h"
#include "../../semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *src;
    const char *file;
    uint32_t    line;
    diag_ctx_t *diag;
    sym_table_t *globals;
} st_ctx_t;

static void skip_ws(st_ctx_t *ctx)
{
    while (*ctx->src) {
        if (*ctx->src == '\n') {
            ctx->line++;
            ctx->src++;
        } else if (isspace((unsigned char)*ctx->src)) {
            ctx->src++;
        } else if (*ctx->src == '/' && ctx->src[1] == '/') {
            while (*ctx->src && *ctx->src != '\n') ctx->src++;
        } else if (*ctx->src == '#') {
            while (*ctx->src && *ctx->src != '\n') ctx->src++;
        } else if (*ctx->src == '(' && ctx->src[1] == '*') {
            ctx->src += 2;
            while (*ctx->src && !(*ctx->src == '*' && ctx->src[1] == ')')) {
                if (*ctx->src == '\n') ctx->line++;
                ctx->src++;
            }
            if (*ctx->src) ctx->src += 2;
        } else {
            break;
        }
    }
}

static bool match_kw(st_ctx_t *ctx, const char *kw)
{
    size_t n = strlen(kw);
    if (strncmp(ctx->src, kw, n) == 0 && !isalnum((unsigned char)ctx->src[n]) &&
        ctx->src[n] != '_') {
        ctx->src += n;
        skip_ws(ctx);
        return true;
    }
    return false;
}

static bool read_ident(st_ctx_t *ctx, char *out, size_t out_sz)
{
    size_t i = 0;
    if (!isalpha((unsigned char)*ctx->src) && *ctx->src != '_') return false;
    while (isalnum((unsigned char)*ctx->src) || *ctx->src == '_') {
        if (i + 1 < out_sz) out[i++] = *ctx->src;
        ctx->src++;
    }
    out[i] = '\0';
    skip_ws(ctx);
    return i > 0;
}

static const sym_entry_t *lookup_sym(st_ctx_t *ctx, const char *name)
{
    uint32_t i;
    for (i = 0; i < ctx->globals->count; i++) {
        if (strcmp(ctx->globals->entries[i].name, name) == 0) {
            return &ctx->globals->entries[i];
        }
    }
    return NULL;
}

static int parse_stmt(st_ctx_t *ctx, ir_stmt_t **stmts, uint32_t *count);
static ir_expr_t *parse_expr(st_ctx_t *ctx);
static int append_stmt(ir_stmt_t **stmts, uint32_t *count, const ir_stmt_t *st);
static int parse_var_block(st_ctx_t *ctx);

static bool kw_ahead(st_ctx_t *ctx, const char *kw)
{
    size_t n = strlen(kw);
    return strncmp(ctx->src, kw, n) == 0 &&
           !isalnum((unsigned char)ctx->src[n]) && ctx->src[n] != '_';
}

static bool at_if_tail(st_ctx_t *ctx)
{
    skip_ws(ctx);
    return kw_ahead(ctx, "ELSIF") || kw_ahead(ctx, "ELSE") || kw_ahead(ctx, "END_IF");
}

static ir_expr_t *sym_to_expr(const sym_entry_t *sym)
{
    if (!sym) {
        return ir_expr_literal_i32(0);
    }
    if (sym->direction == MPLC_IO_IN || sym->direction == MPLC_IO_OUT) {
        return ir_expr_io(sym->io_slot, sym->type);
    }
    return ir_expr_var(sym->offset, sym->type);
}

static int parse_if_else_tail(st_ctx_t *ctx, ir_stmt_t **else_stmts, uint32_t *else_count)
{
    if (match_kw(ctx, "ELSIF")) {
        ir_if_t nested;
        ir_stmt_t wrapper;
        memset(&nested, 0, sizeof(nested));
        nested.cond = parse_expr(ctx);
        if (!match_kw(ctx, "THEN")) {
            return -1;
        }
        while (!at_if_tail(ctx) && *ctx->src) {
            if (parse_stmt(ctx, &nested.then_stmts, &nested.then_count) != 0) {
                return -1;
            }
        }
        if (parse_if_else_tail(ctx, &nested.else_stmts, &nested.else_count) != 0) {
            return -1;
        }
        memset(&wrapper, 0, sizeof(wrapper));
        wrapper.kind = IR_STMT_IF;
        wrapper.u.if_stmt = nested;
        return append_stmt(else_stmts, else_count, &wrapper);
    }
    if (match_kw(ctx, "ELSE")) {
        while (!kw_ahead(ctx, "END_IF") && *ctx->src) {
            if (parse_stmt(ctx, else_stmts, else_count) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

static ir_expr_t *parse_primary(st_ctx_t *ctx);
static ir_expr_t *parse_expr(st_ctx_t *ctx);

static ir_expr_t *parse_primary(st_ctx_t *ctx)
{
    char ident[64];
    if (*ctx->src == '(') {
        ctx->src++;
        skip_ws(ctx);
        ir_expr_t *e = parse_expr(ctx);
        if (*ctx->src == ')') ctx->src++;
        skip_ws(ctx);
        return e;
    }
    if (isdigit((unsigned char)*ctx->src)) {
        long v = strtol(ctx->src, (char **)&ctx->src, 10);
        skip_ws(ctx);
        return ir_expr_literal_i32((int32_t)v);
    }
    if (match_kw(ctx, "TRUE")) return ir_expr_literal_bool(true);
    if (match_kw(ctx, "FALSE")) return ir_expr_literal_bool(false);
    if (read_ident(ctx, ident, sizeof(ident))) {
        const sym_entry_t *sym = lookup_sym(ctx, ident);
        if (!sym) {
            diag_emit(ctx->diag, DIAG_ERROR, ctx->file, ctx->line, "Unknown variable: %s", ident);
            return ir_expr_literal_i32(0);
        }
        if (sym->direction == MPLC_IO_IN || sym->direction == MPLC_IO_OUT) {
            return ir_expr_io(sym->io_slot, sym->type);
        }
        return ir_expr_var(sym->offset, sym->type);
    }
    return ir_expr_literal_i32(0);
}

static ir_expr_t *parse_unary(st_ctx_t *ctx)
{
    if (*ctx->src == '-') {
        ctx->src++;
        skip_ws(ctx);
        return ir_expr_binop(IR_BIN_SUB, ir_expr_literal_i32(0), parse_unary(ctx), MPLC_TYPE_DINT);
    }
    if (match_kw(ctx, "NOT")) {
        return ir_expr_unop(IR_UN_NOT, parse_unary(ctx), MPLC_TYPE_BOOL);
    }
    return parse_primary(ctx);
}

static ir_expr_t *parse_and(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_unary(ctx);
    while (match_kw(ctx, "AND")) {
        left = ir_expr_binop(IR_BIN_AND, left, parse_unary(ctx), MPLC_TYPE_BOOL);
    }
    return left;
}

static ir_expr_t *parse_xor(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_and(ctx);
    while (match_kw(ctx, "XOR")) {
        left = ir_expr_binop(IR_BIN_XOR, left, parse_and(ctx), MPLC_TYPE_BOOL);
    }
    return left;
}

static ir_expr_t *parse_or(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_xor(ctx);
    while (match_kw(ctx, "OR")) {
        left = ir_expr_binop(IR_BIN_OR, left, parse_xor(ctx), MPLC_TYPE_BOOL);
    }
    return left;
}

static ir_expr_t *parse_cmp(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_or(ctx);
    if (match_kw(ctx, "=")) return ir_expr_binop(IR_BIN_EQ, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<>")) return ir_expr_binop(IR_BIN_NE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<=")) return ir_expr_binop(IR_BIN_LE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, ">=")) return ir_expr_binop(IR_BIN_GE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<")) return ir_expr_binop(IR_BIN_LT, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, ">")) return ir_expr_binop(IR_BIN_GT, left, parse_or(ctx), MPLC_TYPE_BOOL);
    return left;
}

static ir_expr_t *parse_mul(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_cmp(ctx);
    for (;;) {
        if (match_kw(ctx, "*")) {
            left = ir_expr_binop(IR_BIN_MUL, left, parse_cmp(ctx), MPLC_TYPE_DINT);
        } else if (match_kw(ctx, "/")) {
            left = ir_expr_binop(IR_BIN_DIV, left, parse_cmp(ctx), MPLC_TYPE_DINT);
        } else if (match_kw(ctx, "MOD")) {
            left = ir_expr_binop(IR_BIN_MOD, left, parse_cmp(ctx), MPLC_TYPE_DINT);
        } else {
            break;
        }
    }
    return left;
}

static ir_expr_t *parse_expr(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_mul(ctx);
    for (;;) {
        if (match_kw(ctx, "+")) {
            left = ir_expr_binop(IR_BIN_ADD, left, parse_mul(ctx), MPLC_TYPE_DINT);
        } else if (match_kw(ctx, "-")) {
            left = ir_expr_binop(IR_BIN_SUB, left, parse_mul(ctx), MPLC_TYPE_DINT);
        } else {
            break;
        }
    }
    return left;
}

static int append_stmt(ir_stmt_t **stmts, uint32_t *count, const ir_stmt_t *st)
{
    ir_stmt_t *tmp = (ir_stmt_t *)realloc(*stmts, (*count + 1U) * sizeof(ir_stmt_t));
    if (!tmp) {
        return -1;
    }
    *stmts = tmp;
    (*stmts)[*count] = *st;
    (*count)++;
    return 0;
}

static int parse_stmt(st_ctx_t *ctx, ir_stmt_t **stmts, uint32_t *count)
{
    char ident[64];
    ir_stmt_t st;
    bool have_stmt = false;

    memset(&st, 0, sizeof(st));
    skip_ws(ctx);
    if (*ctx->src == ';') {
        ctx->src++;
        skip_ws(ctx);
        return 0;
    }

    if (match_kw(ctx, "IF")) {
        ir_if_t bif;
        memset(&bif, 0, sizeof(bif));
        bif.cond = parse_expr(ctx);
        if (!match_kw(ctx, "THEN")) {
            return -1;
        }
        while (!at_if_tail(ctx) && *ctx->src) {
            if (parse_stmt(ctx, &bif.then_stmts, &bif.then_count) != 0) {
                return -1;
            }
        }
        if (parse_if_else_tail(ctx, &bif.else_stmts, &bif.else_count) != 0) {
            return -1;
        }
        if (!match_kw(ctx, "END_IF")) {
            return -1;
        }
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
        st.kind = IR_STMT_IF;
        st.u.if_stmt = bif;
        have_stmt = true;
    } else if (match_kw(ctx, "WHILE")) {
        ir_while_t wstmt;
        memset(&wstmt, 0, sizeof(wstmt));
        wstmt.cond = parse_expr(ctx);
        if (!match_kw(ctx, "DO")) {
            return -1;
        }
        while (!kw_ahead(ctx, "END_WHILE") && *ctx->src) {
            if (parse_stmt(ctx, &wstmt.body, &wstmt.body_count) != 0) {
                return -1;
            }
        }
        if (!match_kw(ctx, "END_WHILE")) {
            return -1;
        }
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
        st.kind = IR_STMT_WHILE;
        st.u.while_stmt = wstmt;
        have_stmt = true;
    } else if (match_kw(ctx, "REPEAT")) {
        ir_repeat_t rstmt;
        memset(&rstmt, 0, sizeof(rstmt));
        while (!kw_ahead(ctx, "UNTIL") && *ctx->src) {
            if (parse_stmt(ctx, &rstmt.body, &rstmt.body_count) != 0) {
                return -1;
            }
        }
        if (!match_kw(ctx, "UNTIL")) {
            return -1;
        }
        rstmt.until_cond = parse_expr(ctx);
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
        if (!match_kw(ctx, "END_REPEAT")) {
            return -1;
        }
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
        st.kind = IR_STMT_REPEAT;
        st.u.repeat_stmt = rstmt;
        have_stmt = true;
    } else if (match_kw(ctx, "FOR")) {
        char loop_ident[64];
        const sym_entry_t *sym;
        ir_stmt_t init;
        ir_stmt_t incr;
        ir_stmt_t while_st;
        ir_while_t wstmt;
        ir_expr_t *step;
        ir_expr_t *cond;

        if (!read_ident(ctx, loop_ident, sizeof(loop_ident))) {
            diag_emit(ctx->diag, DIAG_ERROR, ctx->file, ctx->line, "FOR requires a variable");
            return -1;
        }
        sym = lookup_sym(ctx, loop_ident);
        if (!sym) {
            diag_emit(ctx->diag, DIAG_ERROR, ctx->file, ctx->line, "Unknown variable: %s", loop_ident);
            return -1;
        }
        if (!match_kw(ctx, ":=")) {
            return -1;
        }
        memset(&init, 0, sizeof(init));
        init.kind = IR_STMT_ASSIGN;
        init.u.assign.target = sym_to_expr(sym);
        init.u.assign.value = parse_expr(ctx);
        if (append_stmt(stmts, count, &init) != 0) {
            return -1;
        }
        if (!match_kw(ctx, "TO")) {
            return -1;
        }
        ir_expr_t *end_expr = parse_expr(ctx);
        step = ir_expr_literal_i32(1);
        if (match_kw(ctx, "BY")) {
            step = parse_expr(ctx);
        }
        if (!match_kw(ctx, "DO")) {
            return -1;
        }
        memset(&wstmt, 0, sizeof(wstmt));
        if (step->kind == IR_EXPR_LITERAL && step->u.lit_i32 < 0) {
            cond = ir_expr_binop(IR_BIN_GE, sym_to_expr(sym), end_expr, MPLC_TYPE_BOOL);
        } else {
            cond = ir_expr_binop(IR_BIN_LE, sym_to_expr(sym), end_expr, MPLC_TYPE_BOOL);
        }
        wstmt.cond = cond;
        while (!kw_ahead(ctx, "END_FOR") && *ctx->src) {
            if (parse_stmt(ctx, &wstmt.body, &wstmt.body_count) != 0) {
                return -1;
            }
        }
        memset(&incr, 0, sizeof(incr));
        incr.kind = IR_STMT_ASSIGN;
        incr.u.assign.target = sym_to_expr(sym);
        incr.u.assign.value = ir_expr_binop(IR_BIN_ADD, sym_to_expr(sym), step, MPLC_TYPE_DINT);
        if (append_stmt(&wstmt.body, &wstmt.body_count, &incr) != 0) {
            return -1;
        }
        if (!match_kw(ctx, "END_FOR")) {
            return -1;
        }
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
        memset(&while_st, 0, sizeof(while_st));
        while_st.kind = IR_STMT_WHILE;
        while_st.u.while_stmt = wstmt;
        return append_stmt(stmts, count, &while_st);
    } else if (read_ident(ctx, ident, sizeof(ident))) {
        if (match_kw(ctx, ":=")) {
            const sym_entry_t *sym = lookup_sym(ctx, ident);
            ir_expr_t *target;
            if (sym && sym->direction == MPLC_IO_OUT) {
                target = ir_expr_io(sym->io_slot, sym->type);
            } else if (sym && sym->direction == MPLC_IO_IN) {
                target = ir_expr_io(sym->io_slot, sym->type);
            } else if (sym) {
                target = ir_expr_var(sym->offset, sym->type);
            } else {
                diag_emit(ctx->diag, DIAG_ERROR, ctx->file, ctx->line, "Unknown: %s", ident);
                target = ir_expr_var(0, MPLC_TYPE_DINT);
            }
            st.kind = IR_STMT_ASSIGN;
            st.u.assign.target = target;
            st.u.assign.value = parse_expr(ctx);
            have_stmt = true;
        }
        if (*ctx->src == ';') {
            ctx->src++;
        }
        skip_ws(ctx);
    } else {
        return 0;
    }

    if (have_stmt) {
        return append_stmt(stmts, count, &st);
    }
    return 0;
}

static int parse_program(st_ctx_t *ctx, ir_module_t *mod)
{
    ir_pou_t pou;
    char name[64];
    memset(&pou, 0, sizeof(pou));

    if (!match_kw(ctx, "PROGRAM")) {
        return -1;
    }
    if (!read_ident(ctx, name, sizeof(name))) {
        return -2;
    }
    strncpy(pou.name, name, sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);

    while (*ctx->src && strncmp(ctx->src, "END_PROGRAM", 11) != 0) {
        skip_ws(ctx);
        if (kw_ahead(ctx, "VAR")) {
            if (parse_var_block(ctx) != 0) {
                return -3;
            }
        } else if (parse_stmt(ctx, &pou.stmts, &pou.stmt_count) != 0) {
            return -3;
        }
    }
    match_kw(ctx, "END_PROGRAM");

    return ir_module_add_pou(mod, &pou);
}

static int parse_var_block(st_ctx_t *ctx)
{
    char ident[64];
    if (!match_kw(ctx, "VAR")) return 0;
    while (*ctx->src && !match_kw(ctx, "END_VAR")) {
        if (read_ident(ctx, ident, sizeof(ident))) {
            mplc_type_t ty = MPLC_TYPE_BOOL;
            mplc_io_direction_t dir = MPLC_IO_MEM;
            uint16_t io_idx = 0U;
            if (match_kw(ctx, "AT")) {
                if (*ctx->src == '%') {
                    ctx->src++;
                    char area = *ctx->src++;
                    if (area == 'I') dir = MPLC_IO_IN;
                    else if (area == 'Q') dir = MPLC_IO_OUT;
                    while (isalpha((unsigned char)*ctx->src)) ctx->src++;
                    io_idx = (uint16_t)strtoul(ctx->src, (char **)&ctx->src, 10);
                    if (*ctx->src == '.') {
                        uint16_t bit;
                        ctx->src++;
                        bit = (uint16_t)strtoul(ctx->src, (char **)&ctx->src, 10);
                        /* %IXn.0 / %QXn.0 = MPLC channel n; non-zero bit uses IEC layout */
                        if (bit != 0U) {
                            io_idx = (uint16_t)(io_idx * 8U + bit);
                        }
                    }
                }
                skip_ws(ctx);
            }
            if (match_kw(ctx, ":")) {
                if (match_kw(ctx, "BOOL")) ty = MPLC_TYPE_BOOL;
                else if (match_kw(ctx, "DINT") || match_kw(ctx, "INT")) ty = MPLC_TYPE_DINT;
                else if (match_kw(ctx, "REAL")) ty = MPLC_TYPE_REAL;
            }
            if (*ctx->src == ';') ctx->src++;
            skip_ws(ctx);
            sym_table_add(ctx->globals, ident, ty, dir, io_idx);
        } else {
            ctx->src++;
        }
    }
    return 0;
}

int st_parse_source(const char *source, const char *filename, ir_module_t *mod,
                    sym_table_t *globals, diag_ctx_t *diag)
{
    st_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.src = source;
    ctx.file = filename ? filename : "<st>";
    ctx.line = 1U;
    ctx.diag = diag;
    ctx.globals = globals;

    skip_ws(&ctx);
    parse_var_block(&ctx);
    skip_ws(&ctx);
    if (parse_program(&ctx, mod) != 0) {
        diag_emit(diag, DIAG_ERROR, filename, 0, "Failed to parse ST program");
        return -1;
    }
    return diag->error_count > 0U ? -1 : 0;
}

int st_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag)
{
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;
    int rc;
    if (!f) {
        diag_emit(diag, DIAG_ERROR, path, 0, "Cannot open file");
        return -1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    rewind(f);
    buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    rc = st_parse_source(buf, path, mod, globals, diag);
    free(buf);
    return rc;
}
