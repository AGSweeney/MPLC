#include "sfc_parser.h"
#include <stdlib.h>
#include <string.h>

int sfc_parse_xml_fragment(const char *xml, const char *file, ir_module_t *mod,
                           sym_table_t *globals, diag_ctx_t *diag)
{
    ir_pou_t pou;
    ir_stmt_t st;
    (void)globals;

    memset(&pou, 0, sizeof(pou));
    strncpy(pou.name, "SFCProgram", sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);

    memset(&st, 0, sizeof(st));
    st.kind = IR_STMT_IF;
    st.u.if_stmt.cond = ir_expr_literal_bool(strstr(xml, "step") != NULL ||
                                             strstr(xml, "Step") != NULL);
    st.u.if_stmt.then_stmts = (ir_stmt_t *)malloc(sizeof(ir_stmt_t));
    if (!st.u.if_stmt.then_stmts) return -1;
    st.u.if_stmt.then_count = 1U;
    st.u.if_stmt.then_stmts[0].kind = IR_STMT_ASSIGN;
    st.u.if_stmt.then_stmts[0].u.assign.target = ir_expr_var(0, MPLC_TYPE_BOOL);
    st.u.if_stmt.then_stmts[0].u.assign.value = ir_expr_literal_bool(true);

    pou.stmts = (ir_stmt_t *)malloc(sizeof(st));
    if (!pou.stmts) {
        free(st.u.if_stmt.then_stmts);
        return -1;
    }
    pou.stmts[0] = st;
    pou.stmt_count = 1U;

    diag_emit(diag, DIAG_NOTE, file, 0, "SFC chart lowered to conditional step action stub");
    return ir_module_add_pou(mod, &pou);
}
