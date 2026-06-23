#include "semantic.h"
#include <stdlib.h>
#include <string.h>

static uint32_t type_size(mplc_type_t t)
{
    switch (t) {
    case MPLC_TYPE_BOOL: return 1U;
    case MPLC_TYPE_DINT:
    case MPLC_TYPE_INT: return 4U;
    case MPLC_TYPE_REAL:
    case MPLC_TYPE_LREAL: return 8U;
    default: return 4U;
    }
}

void sym_table_init(sym_table_t *tab)
{
    memset(tab, 0, sizeof(*tab));
}

void sym_table_free(sym_table_t *tab)
{
    free(tab->entries);
    memset(tab, 0, sizeof(*tab));
}

int sym_table_add(sym_table_t *tab, const char *name, mplc_type_t type,
                  mplc_io_direction_t dir, uint16_t io_index)
{
    sym_entry_t *e;
    if (!tab || !name) return -1;
    e = (sym_entry_t *)realloc(tab->entries, (tab->count + 1U) * sizeof(*e));
    if (!e) return -2;
    tab->entries = e;
    e = &tab->entries[tab->count];
    memset(e, 0, sizeof(*e));
    strncpy(e->name, name, sizeof(e->name) - 1U);
    e->type = type;
    e->direction = dir;
    e->io_index = io_index;
    e->io_slot = (dir == MPLC_IO_MEM) ? 0U : (uint16_t)tab->count;
    e->offset = tab->data_size;
    tab->data_size += type_size(type);
    tab->count++;
    return 0;
}

static int analyze_expr(ir_expr_t *expr, diag_ctx_t *diag, const char *file)
{
    if (!expr) return 0;
    if (expr->kind == IR_EXPR_BINOP) {
        if (analyze_expr(expr->left, diag, file) != 0) return -1;
        if (analyze_expr(expr->right, diag, file) != 0) return -1;
    } else if (expr->kind == IR_EXPR_UNOP) {
        if (analyze_expr(expr->left, diag, file) != 0) return -1;
    } else if (expr->kind == IR_EXPR_CALL_NATIVE_FB) {
        int i;
        for (i = 0; i < expr->u.native_fb.param_count; i++) {
            if (analyze_expr(expr->u.native_fb.params[i], diag, file) != 0) return -1;
        }
    }
    return 0;
}

static int analyze_stmt(const ir_stmt_t *stmt, diag_ctx_t *diag, const char *file)
{
    uint32_t i;
    if (!stmt) return 0;
    switch (stmt->kind) {
    case IR_STMT_ASSIGN:
        if (analyze_expr(stmt->u.assign.value, diag, file) != 0) return -1;
        break;
    case IR_STMT_IF:
        if (analyze_expr(stmt->u.if_stmt.cond, diag, file) != 0) return -1;
        for (i = 0; i < stmt->u.if_stmt.then_count; i++) {
            if (analyze_stmt(&stmt->u.if_stmt.then_stmts[i], diag, file) != 0) return -1;
        }
        for (i = 0; i < stmt->u.if_stmt.else_count; i++) {
            if (analyze_stmt(&stmt->u.if_stmt.else_stmts[i], diag, file) != 0) return -1;
        }
        break;
    case IR_STMT_WHILE:
        if (analyze_expr(stmt->u.while_stmt.cond, diag, file) != 0) return -1;
        for (i = 0; i < stmt->u.while_stmt.body_count; i++) {
            if (analyze_stmt(&stmt->u.while_stmt.body[i], diag, file) != 0) return -1;
        }
        break;
    case IR_STMT_EXPR:
        if (analyze_expr(stmt->u.expr, diag, file) != 0) return -1;
        break;
    default:
        break;
    }
    return 0;
}

int semantic_analyze(ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag)
{
    uint32_t p, s;
    if (!mod || !globals || !diag) return -1;

    for (p = 0; p < mod->pou_count; p++) {
        ir_pou_t *pou = &mod->pous[p];
        for (s = 0; s < pou->stmt_count; s++) {
            if (analyze_stmt(&pou->stmts[s], diag, pou->name) != 0) {
                diag_emit(diag, DIAG_ERROR, pou->name, 0, "Semantic error in POU %s", pou->name);
            }
        }
    }

    return diag->error_count > 0U ? -1 : 0;
}
