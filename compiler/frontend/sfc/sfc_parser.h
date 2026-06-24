/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_SFC_PARSER_H
#define MPLC_SFC_PARSER_H

#include "ir.h"
#include "diag.h"
#include "semantic/semantic.h"

int sfc_parse_xml_fragment(const char *xml, const char *file, ir_module_t *mod,
                           sym_table_t *globals, diag_ctx_t *diag);

#endif
