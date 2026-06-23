#ifndef MPLC_ST_PARSER_H
#define MPLC_ST_PARSER_H

#include "ir.h"
#include "diag.h"
#include "semantic/semantic.h"

int st_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag);
int st_parse_source(const char *source, const char *filename, ir_module_t *mod,
                    sym_table_t *globals, diag_ctx_t *diag);

#endif
