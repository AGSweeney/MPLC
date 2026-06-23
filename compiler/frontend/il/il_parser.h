#ifndef MPLC_IL_PARSER_H
#define MPLC_IL_PARSER_H

#include "ir.h"
#include "diag.h"
#include "semantic/semantic.h"

int il_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag);

#endif
