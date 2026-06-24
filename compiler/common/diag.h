/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_DIAG_H
#define MPLC_DIAG_H

#include <stdint.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE
} diag_level_t;

typedef struct {
    diag_level_t level;
    char         file[256];
    uint32_t     line;
    char         message[512];
} diag_msg_t;

typedef struct {
    diag_msg_t *msgs;
    uint32_t    count;
    uint32_t    capacity;
    uint32_t    error_count;
} diag_ctx_t;

void diag_init(diag_ctx_t *ctx);
void diag_free(diag_ctx_t *ctx);
void diag_emit(diag_ctx_t *ctx, diag_level_t level, const char *file, uint32_t line,
               const char *fmt, ...);

#endif
