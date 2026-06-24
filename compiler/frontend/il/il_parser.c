/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "il_parser.h"
#include "../../semantic/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const sym_entry_t *lookup(sym_table_t *tab, const char *name)
{
    uint32_t i;
    for (i = 0; i < tab->count; i++) {
        if (strcmp(tab->entries[i].name, name) == 0) {
            return &tab->entries[i];
        }
    }
    return NULL;
}

int il_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag)
{
    FILE *f = fopen(path, "rb");
    char line[512];
    ir_pou_t pou;
    ir_stmt_t *stmts = NULL;
    uint32_t stmt_count = 0U;
    uint32_t line_no = 0U;

    if (!f) {
        diag_emit(diag, DIAG_ERROR, path, 0, "Cannot open IL file");
        return -1;
    }

    memset(&pou, 0, sizeof(pou));
    strncpy(pou.name, "ILProgram", sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        char mnem[32], operand[64];
        ir_stmt_t st;
        line_no++;

        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == ';' || *p == '\n') continue;

        if (sscanf(p, " %31s %63s", mnem, operand) < 1) continue;

        memset(&st, 0, sizeof(st));

        if (strcmp(mnem, "LD") == 0 || strcmp(mnem, "LDN") == 0) {
            const sym_entry_t *sym = lookup(globals, operand);
            ir_expr_t *e = sym ? ir_expr_var(sym->offset, sym->type) :
                                 ir_expr_literal_bool(false);
            if (strcmp(mnem, "LDN") == 0) {
                e = ir_expr_unop(IR_UN_NOT, e, MPLC_TYPE_BOOL);
            }
            st.kind = IR_STMT_EXPR;
            st.u.expr = e;
        } else if (strcmp(mnem, "ST") == 0 || strcmp(mnem, "STN") == 0) {
            const sym_entry_t *sym = lookup(globals, operand);
            st.kind = IR_STMT_ASSIGN;
            st.u.assign.target = sym ? ir_expr_var(sym->offset, sym->type) :
                                       ir_expr_var(0, MPLC_TYPE_BOOL);
            st.u.assign.value = ir_expr_literal_bool(true);
            if (strcmp(mnem, "STN") == 0) {
                st.u.assign.value = ir_expr_literal_bool(false);
            }
        } else if (strcmp(mnem, "AND") == 0) {
            st.kind = IR_STMT_EXPR;
            st.u.expr = ir_expr_binop(IR_BIN_AND, ir_expr_literal_bool(true),
                                      ir_expr_literal_bool(true), MPLC_TYPE_BOOL);
        } else if (strcmp(mnem, "OR") == 0) {
            st.kind = IR_STMT_EXPR;
            st.u.expr = ir_expr_binop(IR_BIN_OR, ir_expr_literal_bool(false),
                                      ir_expr_literal_bool(false), MPLC_TYPE_BOOL);
        } else if (strcmp(mnem, "ADD") == 0) {
            st.kind = IR_STMT_EXPR;
            st.u.expr = ir_expr_binop(IR_BIN_ADD, ir_expr_literal_i32(0),
                                      ir_expr_literal_i32(0), MPLC_TYPE_DINT);
        } else {
            continue;
        }

        {
            ir_stmt_t *tmp = (ir_stmt_t *)realloc(stmts, (stmt_count + 1U) * sizeof(ir_stmt_t));
            if (!tmp) break;
            stmts = tmp;
            stmts[stmt_count++] = st;
        }
    }

    fclose(f);
    pou.stmts = stmts;
    pou.stmt_count = stmt_count;
    return ir_module_add_pou(mod, &pou);
}
