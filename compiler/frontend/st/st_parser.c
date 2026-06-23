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

static ir_expr_t *parse_or(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_and(ctx);
    while (match_kw(ctx, "OR")) {
        left = ir_expr_binop(IR_BIN_OR, left, parse_and(ctx), MPLC_TYPE_BOOL);
    }
    return left;
}

static ir_expr_t *parse_cmp(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_or(ctx);
    if (match_kw(ctx, "=")) return ir_expr_binop(IR_BIN_EQ, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<>")) return ir_expr_binop(IR_BIN_NE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<")) return ir_expr_binop(IR_BIN_LT, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, "<=")) return ir_expr_binop(IR_BIN_LE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, ">")) return ir_expr_binop(IR_BIN_GT, left, parse_or(ctx), MPLC_TYPE_BOOL);
    if (match_kw(ctx, ">=")) return ir_expr_binop(IR_BIN_GE, left, parse_or(ctx), MPLC_TYPE_BOOL);
    return left;
}

static ir_expr_t *parse_expr(st_ctx_t *ctx)
{
    ir_expr_t *left = parse_cmp(ctx);
    if (match_kw(ctx, "+")) left = ir_expr_binop(IR_BIN_ADD, left, parse_cmp(ctx), MPLC_TYPE_DINT);
    else if (match_kw(ctx, "-")) left = ir_expr_binop(IR_BIN_SUB, left, parse_cmp(ctx), MPLC_TYPE_DINT);
    else if (match_kw(ctx, "*")) left = ir_expr_binop(IR_BIN_MUL, left, parse_cmp(ctx), MPLC_TYPE_DINT);
    else if (match_kw(ctx, "/")) left = ir_expr_binop(IR_BIN_DIV, left, parse_cmp(ctx), MPLC_TYPE_DINT);
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

static int parse_stmt(st_ctx_t *ctx, ir_stmt_t **stmts, uint32_t *count);

static int parse_stmt(st_ctx_t *ctx, ir_stmt_t **stmts, uint32_t *count)
{
    char ident[64];
    ir_stmt_t st;
    bool have_stmt = false;

    memset(&st, 0, sizeof(st));

    if (match_kw(ctx, "IF")) {
        ir_if_t bif;
        uint32_t i;
        memset(&bif, 0, sizeof(bif));
        bif.cond = parse_expr(ctx);
        match_kw(ctx, "THEN");
        while (*ctx->src && strncmp(ctx->src, "END_IF", 6) != 0 &&
               strncmp(ctx->src, "ELSE", 4) != 0) {
            if (parse_stmt(ctx, &bif.then_stmts, &bif.then_count) != 0) {
                return -1;
            }
        }
        if (match_kw(ctx, "ELSE")) {
            while (*ctx->src && strncmp(ctx->src, "END_IF", 6) != 0) {
                if (parse_stmt(ctx, &bif.else_stmts, &bif.else_count) != 0) {
                    return -1;
                }
            }
        }
        match_kw(ctx, "END_IF");
        st.kind = IR_STMT_IF;
        st.u.if_stmt = bif;
        have_stmt = true;
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
        if (parse_stmt(ctx, &pou.stmts, &pou.stmt_count) != 0) {
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
