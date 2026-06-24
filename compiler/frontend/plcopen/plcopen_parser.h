/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_PLCOPEN_H
#define MPLC_PLCOPEN_H

#include "ir.h"
#include "diag.h"
#include "semantic/semantic.h"

int plcopen_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag);

#endif
