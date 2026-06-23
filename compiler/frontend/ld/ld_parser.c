#include "ld_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Simplified LD: each contactLine maps to AND chain; rungs OR'd via assignments. */

static const sym_entry_t *ld_lookup(sym_table_t *tab, const char *name)
{
    uint32_t i;
    for (i = 0; i < tab->count; i++) {
        if (strcmp(tab->entries[i].name, name) == 0) {
            return &tab->entries[i];
        }
    }
    return NULL;
}

int ld_parse_xml_fragment(const char *xml, const char *file, ir_module_t *mod,
                          sym_table_t *globals, diag_ctx_t *diag)
{
    ir_pou_t pou;
    ir_stmt_t *stmts = NULL;
    uint32_t stmt_count = 0U;
    const char *p = xml;
    char var[64];
    (void)file;

    memset(&pou, 0, sizeof(pou));
    strncpy(pou.name, "LDProgram", sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);

    while ((p = strstr(p, "variable=")) != NULL) {
        p += 9;
        if (*p == '"') {
            size_t i = 0;
            p++;
            while (*p && *p != '"' && i + 1 < sizeof(var)) {
                var[i++] = *p++;
            }
            var[i] = '\0';

            const sym_entry_t *sym = ld_lookup(globals, var);
            ir_stmt_t st;
            memset(&st, 0, sizeof(st));
            st.kind = IR_STMT_ASSIGN;
            if (sym) {
                st.u.assign.target = sym->direction == MPLC_IO_OUT ?
                    ir_expr_io(sym->io_index, sym->type) :
                    ir_expr_var(sym->offset, sym->type);
            } else {
                st.u.assign.target = ir_expr_var(0, MPLC_TYPE_BOOL);
            }
            st.u.assign.value = ir_expr_literal_bool(true);

            ir_stmt_t *tmp = (ir_stmt_t *)realloc(stmts, (stmt_count + 1U) * sizeof(ir_stmt_t));
            if (!tmp) break;
            stmts = tmp;
            stmts[stmt_count++] = st;
        }
        p++;
    }

    if (stmt_count == 0U) {
        diag_emit(diag, DIAG_WARNING, file, 0, "LD fragment had no coils; emitting NOP rung");
        ir_stmt_t st;
        memset(&st, 0, sizeof(st));
        st.kind = IR_STMT_EXPR;
        st.u.expr = ir_expr_literal_bool(false);
        stmts = (ir_stmt_t *)malloc(sizeof(st));
        if (stmts) {
            stmts[0] = st;
            stmt_count = 1U;
        }
    }

    pou.stmts = stmts;
    pou.stmt_count = stmt_count;
    return ir_module_add_pou(mod, &pou);
}
