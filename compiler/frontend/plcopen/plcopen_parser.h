#ifndef MPLC_PLCOPEN_H
#define MPLC_PLCOPEN_H

#include "ir.h"
#include "diag.h"
#include "semantic/semantic.h"

int plcopen_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag);

#endif
