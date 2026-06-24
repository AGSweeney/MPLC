/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "plcopen_parser.h"
#include "../st/st_parser.h"
#include "../ld/ld_parser.h"
#include "../fbd/fbd_parser.h"
#include "../sfc/sfc_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal PLCopen XML scanner — extracts body sections by language tag. */

static char *read_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long sz;
    char *buf;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    if (sz <= 0) { fclose(f); return NULL; }
    rewind(f);
    buf = (char *)malloc((size_t)sz + 1U);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    *out_size = (size_t)sz;
    return buf;
}

static const char *find_tag(const char *xml, const char *tag)
{
    char open[64];
    snprintf(open, sizeof(open), "<%s", tag);
    return strstr(xml, open);
}

int plcopen_parse_file(const char *path, ir_module_t *mod, sym_table_t *globals, diag_ctx_t *diag)
{
    size_t sz = 0U;
    char *xml = read_file(path, &sz);
    const char *p;
    int rc = 0;

    if (!xml) {
        diag_emit(diag, DIAG_ERROR, path, 0, "Cannot read PLCopen XML");
        return -1;
    }

    if ((p = find_tag(xml, "ST")) != NULL) {
        const char *end = strstr(p, "</ST>");
        if (end) {
            size_t len = (size_t)(end - p);
            char *block = (char *)malloc(len + 1U);
            if (block) {
                memcpy(block, p, len);
                block[len] = '\0';
                rc |= st_parse_source(block, path, mod, globals, diag);
                free(block);
            }
        }
    }

    if ((p = find_tag(xml, "LD")) != NULL) {
        rc |= ld_parse_xml_fragment(p, path, mod, globals, diag);
    }

    if ((p = find_tag(xml, "FBD")) != NULL) {
        rc |= fbd_parse_xml_fragment(p, path, mod, globals, diag);
    }

    if ((p = find_tag(xml, "SFC")) != NULL) {
        rc |= sfc_parse_xml_fragment(p, path, mod, globals, diag);
    }

    if (mod->pou_count == 0U) {
        diag_emit(diag, DIAG_ERROR, path, 0, "No supported POU body found in PLCopen XML");
        rc = -1;
    }

    free(xml);
    return rc;
}
