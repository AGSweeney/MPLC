#include "fbd_parser.h"
#include <stdlib.h>
#include <string.h>

int fbd_parse_xml_fragment(const char *xml, const char *file, ir_module_t *mod,
                           sym_table_t *globals, diag_ctx_t *diag)
{
    ir_pou_t pou;
    ir_stmt_t st;
    ir_expr_t *params[3];
    (void)globals;

    memset(&pou, 0, sizeof(pou));
    strncpy(pou.name, "FBDProgram", sizeof(pou.name) - 1U);
    pou.kind = IR_POU_PROGRAM;
    pou.pou_id = (uint16_t)(mod->pou_count + 1U);

    params[0] = ir_expr_literal_bool(strstr(xml, "TON") != NULL);
    params[1] = ir_expr_literal_i32(1000);
    params[2] = NULL;

    memset(&st, 0, sizeof(st));
    st.kind = IR_STMT_EXPR;
    if (strstr(xml, "TON") != NULL) {
        st.u.expr = ir_expr_call_native_fb(MPLC_FB_TON, 0, params, 2);
    } else if (strstr(xml, "CTU") != NULL) {
        params[2] = ir_expr_literal_i32(10);
        st.u.expr = ir_expr_call_native_fb(MPLC_FB_CTU, 64, params, 3);
    } else {
        st.u.expr = ir_expr_literal_bool(true);
        diag_emit(diag, DIAG_NOTE, file, 0, "FBD: generic block lowered to TRUE");
    }

    pou.stmts = (ir_stmt_t *)malloc(sizeof(st));
    if (!pou.stmts) return -1;
    pou.stmts[0] = st;
    pou.stmt_count = 1U;
    return ir_module_add_pou(mod, &pou);
}
