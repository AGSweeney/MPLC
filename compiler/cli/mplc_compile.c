/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_compile.h"

#include "frontend/st/st_parser.h"
#include "frontend/il/il_parser.h"
#include "frontend/plcopen/plcopen_parser.h"
#include "semantic/semantic.h"
#include "codegen/codegen.h"
#include "linker/linker.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define str_eq_nocase _stricmp
#else
#include <strings.h>
#define str_eq_nocase strcasecmp
#endif

int mplc_compile_file(const char *input, const char *output, diag_ctx_t *diag)
{
    ir_module_t mod;
    sym_table_t globals;
    codegen_module_t cg;
    linker_output_t pkg;
    int rc = 0;
    const char *ext = strrchr(input, '.');

    ir_module_init(&mod);
    sym_table_init(&globals);
    memset(&cg, 0, sizeof(cg));
    memset(&pkg, 0, sizeof(pkg));

    if (ext && str_eq_nocase(ext, ".st") == 0) {
        rc = st_parse_file(input, &mod, &globals, diag);
    } else if (ext && str_eq_nocase(ext, ".il") == 0) {
        rc = il_parse_file(input, &mod, &globals, diag);
    } else if (ext && str_eq_nocase(ext, ".xml") == 0) {
        rc = plcopen_parse_file(input, &mod, &globals, diag);
    } else {
        rc = st_parse_file(input, &mod, &globals, diag);
    }

    if (rc == 0) {
        rc = semantic_analyze(&mod, &globals, diag);
    }
    if (rc == 0) {
        rc = codegen_module_from_ir(&mod, &cg);
    }
    if (rc == 0) {
        rc = linker_build_package(&mod, &cg, &globals, &pkg);
    }
    if (rc == 0) {
        FILE *f = fopen(output, "wb");
        if (!f) {
            diag_emit(diag, DIAG_ERROR, output, 0, "Cannot write output");
            rc = -1;
        } else {
            fwrite(pkg.data, 1, pkg.size, f);
            fclose(f);
        }
    }

    linker_output_free(&pkg);
    codegen_module_free(&cg);
    sym_table_free(&globals);
    ir_module_free(&mod);
    return rc;
}
