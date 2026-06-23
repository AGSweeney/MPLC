#include "diag.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void diag_init(diag_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
}

void diag_free(diag_ctx_t *ctx)
{
    free(ctx->msgs);
    memset(ctx, 0, sizeof(*ctx));
}

void diag_emit(diag_ctx_t *ctx, diag_level_t level, const char *file, uint32_t line,
               const char *fmt, ...)
{
    diag_msg_t *m;
    va_list ap;

    if (ctx->count >= ctx->capacity) {
        uint32_t cap = ctx->capacity ? ctx->capacity * 2U : 16U;
        diag_msg_t *nm = (diag_msg_t *)realloc(ctx->msgs, cap * sizeof(*nm));
        if (!nm) return;
        ctx->msgs = nm;
        ctx->capacity = cap;
    }

    m = &ctx->msgs[ctx->count++];
    m->level = level;
    m->line = line;
    if (file) {
        strncpy(m->file, file, sizeof(m->file) - 1U);
    } else {
        m->file[0] = '\0';
    }

    va_start(ap, fmt);
    vsnprintf(m->message, sizeof(m->message), fmt, ap);
    va_end(ap);

    if (level == DIAG_ERROR) {
        ctx->error_count++;
    }
}
