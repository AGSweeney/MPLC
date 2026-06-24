/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_COMPILE_H
#define MPLC_COMPILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "diag.h"

/* Compile input source to output .mplc package. Returns 0 on success. */
int mplc_compile_file(const char *input, const char *output, diag_ctx_t *diag);

#ifdef __cplusplus
}
#endif

#endif
